#include "identity.h"

#include <string.h>

#include "agreements.h"
#include "crypto.h"
#include "formats.h"
#include "mmr.h"
#include "text14.h"

const char *mor_next_name(int n)
{
	switch (n) {
	case MOR_NEXT_END: return "end";
	case MOR_NEXT_PENDING: return "pending";
	case MOR_NEXT_CONTESTED: return "contested";
	default: return "unknown";
	}
}

#define MAX_DEPTH 48
#define MAX_CAND 32
#define MAX_OPS 32

int mor_is_chain_act(const mor_world *w, const mor_act *a)
{
	return mor_world_is_mir(w, a, MOR_IDENTITY) && (a->type == 0 || a->type == 1 || a->type == 16);
}

int64_t mor_chain_act_pos(const mor_world *w, const mor_act *a)
{
	if (!mor_is_chain_act(w, a))
		return -1;
	if (a->type == 0)
		return 0;
	const mor_cbor *p = mor_pl(a, 1);
	return p && p->type == MOR_CBOR_UINT && p->v < INT64_MAX ? (int64_t)p->v : -1;
}

int64_t mor_chain_position_of(const mor_chain *c, const uint8_t id[32])
{
	for (size_t i = 0; c && i < c->npos; i++)
		if (!memcmp(c->pos[i].act->id, id, 32))
			return (int64_t)i;
	return -1;
}

/* The identity of the act that bears it (a chain act's identity). */
static const uint8_t *chain_owner(const mor_act *a)
{
	return a->type == 0 ? a->id : a->signer;
}

/* ---- homes and rules ---- */

typedef struct ops_t {
	uint8_t op[MAX_OPS][32];
	int self[MAX_OPS];
	size_t n;
	int has_self;
} ops_t;

static void op_of_home(const mor_cbor *home, const uint8_t *X, uint8_t out[32], int *self)
{
	const mor_cbor *o = &home->items[0];
	if (mor_cbor_is_null(o) || !memcmp(o->data, X, 32)) {
		memcpy(out, X, 32);
		*self = 1;
	} else {
		memcpy(out, o->data, 32);
		*self = 0;
	}
}

static void ops_of(const mor_cbor *homes, const uint8_t *X, ops_t *o)
{
	memset(o, 0, sizeof *o);
	for (uint64_t i = 0; homes && i < homes->v && o->n < MAX_OPS; i++) {
		uint8_t op[32];
		int self, dup = 0;
		op_of_home(&homes->items[i], X, op, &self);
		for (size_t k = 0; k < o->n; k++)
			if (!memcmp(o->op[k], op, 32))
				dup = 1;
		if (dup)
			continue;
		memcpy(o->op[o->n], op, 32);
		o->self[o->n] = self;
		o->has_self |= self;
		o->n++;
	}
}

typedef struct rule_t {
	int auth;      /* 1: one authoritative operator; 0: a threshold */
	uint8_t op[32];
	uint64_t k;
} rule_t;

static void rule_of(const mor_cbor *homes, const mor_cbor *hr, const uint8_t *X, rule_t *r)
{
	ops_t o;
	ops_of(homes, X, &o);
	memset(r, 0, sizeof *r);
	if (hr && hr->type == MOR_CBOR_ARRAY && hr->v == 2) {
		if (hr->items[0].v == 0 && hr->items[1].v < homes->v) {
			int self;
			r->auth = 1;
			op_of_home(&homes->items[hr->items[1].v], X, r->op, &self);
			return;
		}
		if (hr->items[0].v == 1) {
			r->k = hr->items[1].v;
			return;
		}
	}
	/* The default (rule 2): a single operator; else a self-hosted home is
	 * authoritative; else the majority of distinct operators. */
	if (o.n == 1) {
		r->auth = 1;
		memcpy(r->op, o.op[0], 32);
	} else if (o.has_self) {
		r->auth = 1;
		memcpy(r->op, X, 32);
	} else {
		r->k = o.n / 2 + 1;
	}
}

/* Genesis check 4 and 5's conditions, for a home rule and audit against a set of homes. */
static int rule_fits(const mor_cbor *homes, const mor_cbor *hr, const uint8_t *X)
{
	if (!hr || mor_cbor_is_null(hr))
		return 1;
	ops_t o;
	ops_of(homes, X, &o);
	if (o.n < 2)
		return 0;
	if (hr->items[0].v == 0)
		return hr->items[1].v < homes->v;
	return hr->items[1].v > o.n / 2 && hr->items[1].v <= o.n;
}

static int audit_fits(const mor_cbor *au)
{
	if (!au || mor_cbor_is_null(au))
		return 1;
	return au->items[0].v >= 1 && au->items[0].v <= au->items[1].v;
}

/* ---- checks ---- */

static int key_equals(const mor_sigval *s, const mor_cbor *sk)
{
	if (!sk || sk->type != MOR_CBOR_ARRAY || sk->v != 2)
		return 0;
	const mor_cbor *sc = &sk->items[0], *k = &sk->items[1];
	if (s->kind == MOR_SCHEME_NUM) {
		if (sc->type != MOR_CBOR_UINT || sc->v != s->num)
			return 0;
	} else if (!mor_cbor_is_hash(sc) || memcmp(sc->data, s->hash, 32)) {
		return 0;
	}
	return k->v == s->key_len && !memcmp(k->data, s->key, s->key_len);
}

/* The checks on every act (Identity, "The checks on every act"), and the
 * format of its MIR. Returns VALID, INVALID or UNKNOWN (a scheme by hash). */
static int every_act(const mor_world *w, const mor_act *a)
{
	if (a->open == MOR_OPEN_NO_KEY) {
		/* A private act this verifier cannot open: its outside and signature
		 * still decide invalid; nothing else can be read (Envelopes rule 10). */
		return (!a->sig_shape_ok || mor_act_sig_verify(a) == 0) ? MOR_ST_INVALID : MOR_ST_UNKNOWN;
	}
	if (a->open != MOR_OPEN_OK)
		return MOR_ST_INVALID;
	int unknown = 0;
	int s = mor_act_sig_verify(a);
	if (s == 0)
		return MOR_ST_INVALID;
	if (s < 0)
		unknown = 1;
	int mir = mor_world_mir_of(w, a->spec);
	if (mir != MOR_MIR_NONE && !mor_format_ok(w, a, NULL))
		return MOR_ST_INVALID;
	/* Only Identity, Money and Agreements types may acknowledge (Envelopes rule 18). */
	if (a->acks && !mor_type_may_ack(w, a))
		return MOR_ST_INVALID;
	/* Identity acts are public, save a private link (Envelopes rule 8). */
	if (mir == MOR_IDENTITY && !a->key && !(a->type >= 6 && a->type <= 8))
		return MOR_ST_INVALID;
	/* An Envelopes encryption key is an Identity act in spirit, public too. */
	if (mir == MOR_ENVELOPES && a->type == 4 && !a->key)
		return MOR_ST_INVALID;
	return unknown ? MOR_ST_UNKNOWN : MOR_ST_VALID;
}

static int genesis_checks(const mor_world *w, const mor_act *g)
{
	int st = every_act(w, g);
	if (st == MOR_ST_INVALID)
		return st;
	if (!mor_world_is_mir(w, g, MOR_IDENTITY) || g->type != 0)
		return MOR_ST_INVALID;
	/* 1. signer, binding, prev and objects are absent */
	if (g->signer || g->binding || g->prev || g->objects)
		return MOR_ST_INVALID;
	/* 2. the key in the signature is the signing key declared */
	if (!key_equals(&g->sig, mor_pl(g, 0)))
		return MOR_ST_INVALID;
	/* 3-5: homes, a home rule that fits, an audit that fits */
	if (!rule_fits(mor_pl(g, 2), mor_pl(g, 3), g->id) || !audit_fits(mor_pl(g, 5)))
		return MOR_ST_INVALID;
	return st;
}

/* Rotation checks 1-6 and chain signature checks 1-4, against the state at
 * the position before. */
static int chain_act_checks(const mor_world *w, const mor_act *r, const mor_chain_pos *prev)
{
	int st = every_act(w, r);
	if (st == MOR_ST_INVALID)
		return st;
	if (r->type != 1 && r->type != 16)
		return MOR_ST_INVALID;
	if (!r->signer || r->binding || r->prev)
		return MOR_ST_INVALID;
	if (mor_pl(r, 1)->v < 1)
		return MOR_ST_INVALID;
	if (!mor_hash_eq(mor_pl_hash(r, 0), prev->act->id))
		return MOR_ST_INVALID;
	uint8_t c[32];
	if (!r->sig_shape_ok)
		return MOR_ST_INVALID;
	mor_chain_commit(c, &r->sig);
	if (memcmp(c, prev->commit->items[1].data, 32))
		return MOR_ST_INVALID;
	if (r->type == 1) {
		/* check 4: a homeless rotation declares a new set of homes */
		if (mor_pl(r, 11) && !mor_pl(r, 6))
			return MOR_ST_INVALID;
		/* check 5: a new or a kept rule fits the (new) homes; a new audit fits */
		const mor_cbor *homes = mor_pl(r, 6) ? mor_pl(r, 6) : prev->homes;
		const mor_cbor *hr = mor_pl(r, 7) ? mor_pl(r, 7) : prev->home_rule;
		if (!rule_fits(homes, hr, r->signer) || !audit_fits(mor_pl(r, 10)))
			return MOR_ST_INVALID;
	}
	return st;
}

/* ---- inclusion proofs (RFC 9162, 2.1.3.2) and cosignatures ---- */

static void leaf_hash(uint8_t out[32], const uint8_t id[32])
{
	uint8_t b[33];
	b[0] = 0;
	memcpy(b + 1, id, 32);
	mor_sha256(out, b, sizeof b);
}

static void node_hash(uint8_t out[32], const uint8_t l[32], const uint8_t r[32])
{
	uint8_t b[65];
	b[0] = 1;
	memcpy(b + 1, l, 32);
	memcpy(b + 33, r, 32);
	mor_sha256(out, b, sizeof b);
}

static int inclusion_ok(const mor_incl_proof *p, const uint8_t leaf_id[32], uint64_t size, const uint8_t root[32])
{
	if (p->index >= size)
		return 0;
	uint64_t fn = p->index, sn = size - 1;
	uint8_t r[32];
	leaf_hash(r, leaf_id);
	for (size_t i = 0; i < p->npath; i++) {
		if (sn == 0)
			return 0;
		if ((fn & 1u) || fn == sn) {
			node_hash(r, p->path[i], r);
			if (!(fn & 1u))
				while (!(fn & 1u) && fn != 0) {
					fn >>= 1;
					sn >>= 1;
				}
		} else {
			node_hash(r, r, p->path[i]);
		}
		fn >>= 1;
		sn >>= 1;
	}
	return sn == 0 && !memcmp(r, root, 32);
}

/* A log summary is cosigned for X when at least the required number of
 * valid cosignatures from X's declared auditors name it. */
static int summary_cosigned(mor_world *w, const mor_act *sum, const mor_cbor *audit)
{
	if (!audit || mor_cbor_is_null(audit))
		return 0;
	uint64_t need = audit->items[0].v, got = 0;
	const mor_cbor *auditors = &audit->items[1];
	uint8_t seen[MAX_OPS][32];
	size_t nseen = 0;
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *c = &w->acts[i];
		if (!mor_world_is_mir(w, c, MOR_IDENTITY) || c->type != 10 || !c->signer)
			continue;
		if (!mor_objects_has(c, sum->signer, sum->id) || !mor_act_names_in(auditors, c->signer))
			continue;
		int dup = 0;
		for (size_t k = 0; k < nseen; k++)
			if (!memcmp(seen[k], c->signer, 32))
				dup = 1;
		if (dup || nseen >= MAX_OPS)
			continue;
		/* counts only while its signer's chain keeps it valid */
		if (mor_status_act(w, c, 0) != MOR_ST_VALID)
			continue;
		memcpy(seen[nseen++], c->signer, 32);
		got++;
	}
	return got >= need;
}

/* Is a receipt included under a cosigned log summary of its operator? */
static int receipt_cosigned(mor_world *w, const mor_act *rc, const mor_cbor *audit)
{
	for (size_t i = 0; i < w->nproofs; i++) {
		const mor_incl_proof *p = &w->proofs[i];
		if (p->has_receipt && memcmp(p->receipt, rc->id, 32))
			continue;
		if (p->index != mor_pl(rc, 3)->v)
			continue;
		const mor_act *sum = mor_world_get(w, p->summary);
		if (!sum || !mor_world_is_mir(w, sum, MOR_IDENTITY) || sum->type != 9 || !mor_hash_eq(sum->signer, rc->signer))
			continue;
		if (mor_status_act(w, sum, 0) == MOR_ST_INVALID)
			continue;
		if (!inclusion_ok(p, rc->id, mor_pl(sum, 0)->v, mor_pl(sum, 1)->data))
			continue;
		if (summary_cosigned(w, sum, audit))
			return 1;
	}
	return 0;
}

/* ---- the chain ---- */

static mor_chain *chain_slot(mor_world *w, const uint8_t id[32])
{
	for (size_t i = 0; i < w->nchains; i++)
		if (!memcmp(w->chains[i]->id, id, 32))
			return w->chains[i];
	if (w->nchains >= w->chains_cap)
		return NULL;
	mor_chain *c = mor_arena_alloc(w->a, sizeof *c);
	if (!c)
		return NULL;
	memcpy(c->id, id, 32);
	w->chains[w->nchains++] = c;
	return c;
}

static int push_pos(mor_world *w, mor_chain *c, const mor_chain_pos *p)
{
	if (c->npos >= c->cap) {
		size_t ncap = c->cap ? c->cap * 2 : 8;
		mor_chain_pos *np = mor_arena_array(w->a, ncap, sizeof *np);
		if (!np)
			return -1;
		if (c->npos)
			memcpy(np, c->pos, c->npos * sizeof *np);
		c->pos = np;
		c->cap = ncap;
	}
	c->pos[c->npos++] = *p;
	return 0;
}

static int is_dishonest(const mor_chain *c, const uint8_t op[32], uint64_t n)
{
	for (size_t i = 0; i < c->ndishonest; i++)
		if (!memcmp(c->dishonest[i].op, op, 32) && n > c->dishonest[i].from)
			return 1;
	return 0;
}

/* Has the operator a counting rotation after the chain act its receipt is bound to? */
static int op_rotated_after(mor_world *w, const mor_act *rc)
{
	const mor_chain *oc = mor_chain_get(w, rc->signer);
	const mor_act *b = mor_world_get(w, rc->binding);
	if (!oc || !b)
		return 0;
	int64_t pb = mor_chain_position_of(oc, b->id);
	if (pb < 0)
		return 0;
	for (size_t i = (size_t)pb + 1; i < oc->npos; i++)
		if (oc->pos[i].act->type == 1)
			return 1;
	return 0;
}

typedef struct support_t {
	const mor_act *cand[MAX_CAND];
	int st[MAX_CAND];
	size_t ncand;
	uint8_t by[MAX_CAND][MAX_OPS]; /* by[c][op] = 1 when op supports cand c */
	int conflict;
	int unknown;
} support_t;

/* Gather each operator's support for the candidates at position n (receipt
 * checks 1 to 6, conflicting receipts). */
static void gather_support(mor_world *w, mor_chain *c, const uint8_t *X, uint64_t n, const ops_t *o,
			   const mor_cbor *audit, support_t *s, int selfmode)
{
	for (size_t k = 0; k < o->n; k++) {
		if (o->self[k] || selfmode) {
			/* A self-hosted home: the rotation served stands in for a receipt; two
			 * different rotations served at one position are a conflict for good. */
			size_t nval = 0;
			for (size_t i = 0; i < s->ncand; i++)
				if (s->st[i] == MOR_ST_VALID)
					nval++;
			if (nval > 1) {
				s->conflict = 1;
				continue;
			}
			for (size_t i = 0; i < s->ncand; i++)
				if (s->st[i] == MOR_ST_VALID)
					s->by[i][k] = 1;
			continue;
		}
		if (is_dishonest(c, o->op[k], n))
			continue;
		/* The receipts of this operator for position n that count. */
		const mor_act *counted[MAX_CAND];
		size_t which[MAX_CAND];
		int cos[MAX_CAND];
		size_t nc = 0;
		for (size_t i = 0; i < w->nacts && nc < MAX_CAND; i++) {
			const mor_act *rc = &w->acts[i];
			if (!mor_world_is_mir(w, rc, MOR_IDENTITY) || rc->type != 2 || rc->open != MOR_OPEN_OK)
				continue;
			if (!mor_hash_eq(rc->signer, o->op[k]) || !mor_hash_eq(mor_pl_hash(rc, 0), X) ||
			    mor_pl(rc, 2)->v != n)
				continue;
			/* 1. it names a real act: a held candidate passing its checks */
			size_t ci = s->ncand;
			for (size_t j = 0; j < s->ncand; j++)
				if (s->st[j] == MOR_ST_VALID && mor_hash_eq(s->cand[j]->id, mor_pl_hash(rc, 1)))
					ci = j;
			if (ci == s->ncand)
				continue;
			/* 2. valid as an everyday act of the operator; 5. void unless cosigned */
			int st = mor_status_act(w, rc, 0);
			int cosigned = (audit && !mor_cbor_is_null(audit)) ? receipt_cosigned(w, rc, audit) : 0;
			if (st == MOR_ST_UNKNOWN) {
				s->unknown = 1;
				continue;
			}
			if (st == MOR_ST_INVALID || st == MOR_ST_PENDING)
				continue;
			if ((st == MOR_ST_VOID || st == MOR_ST_DISPUTED) && !cosigned)
				continue;
			/* 4. under an audit requirement, included under a cosigned summary */
			if (audit && !mor_cbor_is_null(audit) && !cosigned)
				continue;
			counted[nc] = rc;
			which[nc] = ci;
			cos[nc] = cosigned;
			nc++;
		}
		int distinct = 0;
		for (size_t i = 0; i < nc; i++) {
			int seen = 0;
			for (size_t j = 0; j < i; j++)
				if (which[j] == which[i])
					seen = 1;
			distinct += !seen;
		}
		if (distinct > 1) {
			/* A conflict: neither receipt counts, that home counts for nothing here.
			 * Proven dishonest when every receipt survives the home's rotations:
			 * under a cosigned summary, or kept by a rotation of the operator. */
			s->conflict = 1;
			int survive = 1;
			for (size_t i = 0; i < nc; i++)
				if (!cos[i] && !op_rotated_after(w, counted[i]))
					survive = 0;
			if (survive && c->ndishonest < 16) {
				memcpy(c->dishonest[c->ndishonest].op, o->op[k], 32);
				c->dishonest[c->ndishonest].from = n;
				c->ndishonest++;
			}
			continue;
		}
		if (distinct == 1)
			s->by[which[0]][k] = 1;
	}
}

/* Apply a home rule to the support gathered: the index of the candidate
 * that counts, or -1. */
static int apply_rule(const support_t *s, const ops_t *o, const rule_t *r, const uint8_t *X)
{
	(void)X;
	for (size_t i = 0; i < s->ncand; i++) {
		if (s->st[i] != MOR_ST_VALID)
			continue;
		if (r->auth) {
			for (size_t k = 0; k < o->n; k++)
				if (!memcmp(o->op[k], r->op, 32) && s->by[i][k])
					return (int)i;
		} else {
			uint64_t cnt = 0;
			for (size_t k = 0; k < o->n; k++)
				cnt += s->by[i][k];
			if (r->k && cnt >= r->k)
				return (int)i;
		}
	}
	return -1;
}

static const mor_act *find_escape(mor_world *w, const uint8_t *X, const mor_act *h, const mor_chain_pos *prev)
{
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *e = &w->acts[i];
		if (!mor_world_is_mir(w, e, MOR_IDENTITY) || e->type != 14 || !mor_hash_eq(e->signer, X))
			continue;
		if (!mor_objects_has(e, X, h->id))
			continue;
		/* signed by the owner's current signing key, bound to the act that set it */
		if (!mor_hash_eq(e->binding, prev->key_act->id) || !key_equals(&e->sig, prev->signing_key))
			continue;
		if (every_act(w, e) != MOR_ST_VALID || !e->prev)
			continue;
		return e;
	}
	return NULL;
}

static int op_closed(mor_world *w, const uint8_t op[32])
{
	const mor_chain *oc = mor_chain_get(w, op);
	for (size_t i = 1; oc && i < oc->npos; i++)
		if (oc->pos[i].act->type == 1 && mor_cbor_is_true(mor_pl(oc->pos[i].act, 12)))
			return 1;
	return 0;
}

static int op_unreached(const mor_world *w, const uint8_t op[32])
{
	for (size_t i = 0; i < w->nunreached; i++)
		if (!memcmp(w->unreached[i], op, 32))
			return 1;
	return 0;
}

/* The homeless procedure (steps 1 to 5) and the escape with both keys. */
static int homeless_counts(mor_world *w, mor_chain *c, const uint8_t *X, uint64_t n, const mor_act *h,
			   const mor_chain_pos *prev, int *unaudited)
{
	*unaudited = 0;
	const mor_act *esc = find_escape(w, X, h, prev);
	ops_t old;
	ops_of(prev->homes, X, &old);
	if (!esc) {
		/* 3. no valid objection from a home of the old set */
		for (size_t i = 0; i < w->nacts; i++) {
			const mor_act *ob = &w->acts[i];
			if (!mor_world_is_mir(w, ob, MOR_IDENTITY) || ob->type != 12 || !ob->signer)
				continue;
			if (!mor_objects_has(ob, X, h->id))
				continue;
			int from_old = 0;
			for (size_t k = 0; k < old.n; k++)
				if (!old.self[k] && !memcmp(old.op[k], ob->signer, 32))
					from_old = 1;
			if (!from_old || is_dishonest(c, ob->signer, n) || op_closed(w, ob->signer))
				continue;
			if (mor_status_act(w, ob, 0) == MOR_ST_VALID)
				return 0;
		}
		/* 4. enough old homes are gone that the old rule can no longer be met */
		rule_t r;
		rule_of(prev->homes, prev->home_rule, X, &r);
		size_t gone = 0;
		int need_auth_gone = r.auth;
		int auth_gone = 0;
		for (size_t k = 0; k < old.n; k++) {
			int g = 0;
			if (old.self[k])
				g = 0; /* a self-hosted home is never gone */
			else if (op_closed(w, old.op[k]))
				g = 1;
			else if (prev->audit && !mor_cbor_is_null(prev->audit)) {
				uint64_t cnt = 0;
				for (size_t i = 0; i < w->nacts; i++) {
					const mor_act *ab = &w->acts[i];
					if (!mor_world_is_mir(w, ab, MOR_IDENTITY) || ab->type != 13 || !ab->signer)
						continue;
					if (!mor_objects_has(ab, X, h->id) || !mor_hash_eq(mor_pl_hash(ab, 0), old.op[k]))
						continue;
					if (!mor_act_names_in(&prev->audit->items[1], ab->signer))
						continue;
					if (mor_status_act(w, ab, 0) == MOR_ST_VALID)
						cnt++;
				}
				g = cnt >= prev->audit->items[0].v;
			} else if (op_unreached(w, old.op[k])) {
				g = 1;
				*unaudited = 1;
			}
			gone += (size_t)g;
			if (g && need_auth_gone && !memcmp(old.op[k], r.op, 32))
				auth_gone = 1;
		}
		if (r.auth ? !auth_gone : !(gone > old.n - (r.k > old.n ? old.n : r.k)))
			return 0;
	}
	/* 5. receipts from the new homes meet the new home rule */
	const mor_cbor *homes = mor_pl(h, 6);
	const mor_cbor *hr = mor_pl(h, 7) ? (mor_cbor_is_null(mor_pl(h, 7)) ? NULL : mor_pl(h, 7)) : prev->home_rule;
	const mor_cbor *audit = mor_pl(h, 10) ? mor_pl(h, 10) : prev->audit;
	ops_t nw;
	ops_of(homes, X, &nw);
	support_t s;
	memset(&s, 0, sizeof s);
	s.cand[0] = h;
	s.st[0] = MOR_ST_VALID;
	s.ncand = 1;
	gather_support(w, c, X, n, &nw, audit, &s, 0);
	rule_t r;
	rule_of(homes, hr, X, &r);
	return apply_rule(&s, &nw, &r, X) == 0;
}

static void advance(mor_chain_pos *np, const mor_chain_pos *prev, const mor_act *r, int unaudited)
{
	*np = *prev;
	np->act = r;
	np->unaudited = unaudited;
	if (r->type == 16) {
		np->commit = mor_pl(r, 2);
		return;
	}
	np->signing_key = mor_pl(r, 2);
	np->key_act = r;
	np->commit = mor_pl(r, 3);
	if (mor_pl(r, 6))
		np->homes = mor_pl(r, 6);
	if (mor_pl(r, 7))
		np->home_rule = mor_cbor_is_null(mor_pl(r, 7)) ? NULL : mor_pl(r, 7);
	if (mor_pl(r, 10))
		np->audit = mor_cbor_is_null(mor_pl(r, 10)) ? NULL : mor_pl(r, 10);
}

static void chain_compute(mor_world *w, mor_chain *c, int selfmode)
{
	const uint8_t *X = c->id;
	const mor_act *g = mor_world_get(w, X);
	c->npos = 0;
	c->ndishonest = 0;
	c->next = MOR_NEXT_END;
	if (!g) {
		c->genesis_status = MOR_ST_UNKNOWN;
		c->next = MOR_NEXT_UNKNOWN;
		return;
	}
	c->genesis_status = genesis_checks(w, g);
	if (c->genesis_status != MOR_ST_VALID) {
		c->next = c->genesis_status == MOR_ST_UNKNOWN ? MOR_NEXT_UNKNOWN : MOR_NEXT_END;
		return;
	}
	mor_chain_pos p0 = { g, mor_pl(g, 0), g, mor_pl(g, 1), mor_pl(g, 2), mor_pl(g, 3), mor_pl(g, 5), 0 };
	if (push_pos(w, c, &p0))
		return;
	for (uint64_t n = 1;; n++) {
		const mor_chain_pos *prev = &c->pos[c->npos - 1];
		support_t s;
		memset(&s, 0, sizeof s);
		int any_unknown_cand = 0;
		for (size_t i = 0; i < w->nacts && s.ncand < MAX_CAND; i++) {
			const mor_act *r = &w->acts[i];
			if (!mor_is_chain_act(w, r) || r->type == 0 || !mor_hash_eq(r->signer, X))
				continue;
			if (mor_chain_act_pos(w, r) != (int64_t)n || !mor_hash_eq(mor_pl_hash(r, 0), prev->act->id))
				continue;
			int st = chain_act_checks(w, r, prev);
			if (st == MOR_ST_INVALID)
				continue;
			if (st == MOR_ST_UNKNOWN)
				any_unknown_cand = 1;
			s.cand[s.ncand] = r;
			s.st[s.ncand] = st;
			s.ncand++;
		}
		if (s.ncand == 0) {
			c->next = MOR_NEXT_END;
			return;
		}
		ops_t o;
		ops_of(prev->homes, X, &o);
		/* The audit requirement a rotation's receipts are judged under: the one
		 * it declares, else the one in effect before it. Candidates share a
		 * position; each is judged under its own. */
		const mor_cbor *audit = prev->audit;
		if (s.ncand == 1 && s.cand[0]->type == 1 && mor_pl(s.cand[0], 10))
			audit = mor_cbor_is_null(mor_pl(s.cand[0], 10)) ? NULL : mor_pl(s.cand[0], 10);
		gather_support(w, c, X, n, &o, audit, &s, selfmode);
		rule_t r;
		rule_of(prev->homes, prev->home_rule, X, &r);
		int win = apply_rule(&s, &o, &r, X);
		/* A homeless rotation never counts under the old rule by its own field; the
		 * rule counts what the old homes receipted. */
		if (win >= 0) {
			mor_chain_pos np;
			advance(&np, prev, s.cand[win], 0);
			if (push_pos(w, c, &np))
				return;
			continue;
		}
		/* The homeless procedure, and the escape with both keys. */
		int done = 0;
		for (size_t i = 0; i < s.ncand && !done; i++) {
			const mor_act *h = s.cand[i];
			int unaudited;
			if (s.st[i] != MOR_ST_VALID || h->type != 1 || !mor_cbor_is_true(mor_pl(h, 11)))
				continue;
			if (homeless_counts(w, c, X, n, h, prev, &unaudited)) {
				mor_chain_pos np;
				advance(&np, prev, h, unaudited);
				if (push_pos(w, c, &np))
					return;
				done = 1;
			}
		}
		if (done)
			continue;
		if (s.conflict)
			c->next = MOR_NEXT_CONTESTED;
		else if (s.unknown || any_unknown_cand)
			c->next = MOR_NEXT_UNKNOWN;
		else
			c->next = MOR_NEXT_PENDING;
		return;
	}
}

const mor_chain *mor_chain_get(mor_world *w, const uint8_t id[32])
{
	mor_chain *c = chain_slot(w, id);
	if (!c)
		return NULL;
	if (c->state == 2)
		return c;
	if (c->state == 1 || w->depth > MAX_DEPTH) {
		/* Resolving leads back to an identity being resolved: accept its
		 * chain on its own signatures, as a self-hosted identity's. */
		mor_chain *tmp = mor_arena_alloc(w->a, sizeof *tmp);
		if (!tmp)
			return NULL;
		memcpy(tmp->id, id, 32);
		tmp->state = 2;
		w->depth++;
		chain_compute(w, tmp, 1);
		w->depth--;
		return tmp;
	}
	c->state = 1;
	w->depth++;
	chain_compute(w, c, 0);
	w->depth--;
	c->state = 2;
	return c;
}

int mor_identity_agreement(mor_world *w, const uint8_t id[32], const mor_cbor **value)
{
	const mor_chain *c = mor_chain_get(w, id);
	*value = NULL;
	if (!c || c->genesis_status == MOR_ST_UNKNOWN)
		return -1;
	if (c->genesis_status != MOR_ST_VALID)
		return 0;
	/* Declarations: for each spec and kind, the counting chain act with the
	 * highest position wins; a null value removes the kind. */
	const mor_cbor *found = NULL;
	for (size_t i = 0; i < c->npos; i++) {
		const mor_act *a = c->pos[i].act;
		const mor_cbor *d = a->type == 0 ? mor_pl(a, 4) : a->type == 1 ? mor_pl(a, 8) : NULL;
		for (uint64_t k = 0; d && k < d->v; k++) {
			const mor_cbor *e = &d->items[k];
			if (!memcmp(e->items[0].data, w->mir[MOR_AGREEMENTS], 32) && e->items[1].v == 0)
				found = mor_cbor_is_null(&e->items[2]) ? NULL : &e->items[2];
		}
	}
	*value = found;
	return found != NULL;
}

/* ---- kept ancestry (rules 33 to 35) ---- */

static int rebuild_line_in(mor_world *w, const mor_cbor *tip, const uint8_t *signer, const mor_act *a, int *on,
			   const mor_act **line, size_t cap, size_t *np);

/* Rebuild the line leading to a kept tip and check that it fits: returns 1
 * when rebuilt and fitting (and sets *on when `a` lies on it), 0 when it was
 * rebuilt and does not fit, -1 when an act it needs is not held. */
static int rebuild_line(mor_world *w, const mor_cbor *tip, const uint8_t *signer, const mor_act *a, int *on)
{
	size_t cap = w->nacts + 1, n = 0;
	size_t mark = mor_arena_mark(w->a);
	const mor_act **line = mor_arena_array(w->a, cap, sizeof *line);
	int r = rebuild_line_in(w, tip, signer, a, on, line, cap, &n);
	mor_arena_reset(w->a, mark);
	return r;
}

static int rebuild_line_in(mor_world *w, const mor_cbor *tip, const uint8_t *signer, const mor_act *a, int *on,
			   const mor_act **line, size_t cap, size_t *np)
{
	size_t n = 0;
	const mor_act *cur = mor_world_get(w, tip->items[0].data);
	*on = 0;
	if (!line)
		return -1;
	while (1) {
		if (!cur)
			return -1;
		if (cur->open != MOR_OPEN_OK || !mor_hash_eq(cur->signer, signer) || !cur->prev || n >= cap)
			return 0;
		line[n++] = cur;
		if (cur->prev->v == 0)
			break;
		if (cur->prev->v != 1)
			return 0;
		cur = mor_world_get(w, cur->prev->items[0].data);
	}
	/* positions 1..n from the first act; each carries S(p - 1); the tip's S(n) */
	mor_mmr m;
	mor_mmr_init(&m);
	for (size_t i = 0; i < n; i++) {
		const mor_act *x = line[n - 1 - i];
		uint8_t s[32];
		mor_mmr_root(&m, s);
		if (!x->has_pos || x->pos != i + 1 || !x->summary || memcmp(x->summary, s, 32))
			return 0;
		mor_mmr_add(&m, x->id);
	}
	uint8_t s[32];
	mor_mmr_root(&m, s);
	if (tip->items[1].v != n || memcmp(tip->items[2].data, s, 32))
		return 0;
	for (size_t i = 0; i < n; i++)
		if (line[i] == a)
			*on = 1;
	*np = n;
	return 1;
}

/* An acknowledgement by another identity, by an act of a type that may
 * acknowledge, that counts while its signer's chain keeps it valid. */
static int acknowledged_by_other(mor_world *w, const mor_act *a, const uint8_t *except)
{
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *k = &w->acts[i];
		if (k->open != MOR_OPEN_OK || !mor_acks_name(k, a->id) || !mor_type_may_ack(w, k))
			continue;
		if (!k->signer || mor_hash_eq(k->signer, mor_signer(a)))
			continue;
		if (except && mor_hash_eq(k->signer, except))
			continue;
		int st = mor_status_act(w, k, 0);
		if (st == MOR_ST_VALID || st == MOR_ST_SCOPED)
			return 1;
	}
	return 0;
}

int mor_acknowledged_by_other(mor_world *w, const mor_act *a, const uint8_t *except)
{
	return acknowledged_by_other(w, a, except);
}

/* The judgement of an act signed with a key a rotation replaced. */
static int judged_by(mor_world *w, const mor_act *a, const mor_act *rot, int flags)
{
	const mor_cbor *kept = mor_pl(rot, 4), *dis = mor_pl(rot, 5);
	int found = 0, unbuilt = 0;
	for (uint64_t i = 0; kept && i < kept->v; i++) {
		int on = 0;
		int r = rebuild_line(w, &kept->items[i], rot->signer, a, &on);
		if (r < 0)
			unbuilt = 1;
		else if (r == 1 && on)
			found = 1;
	}
	if (found && !(dis && mor_act_names_in(dis, a->id)))
		return MOR_ST_VALID;
	if (!found && unbuilt)
		return MOR_ST_UNKNOWN;
	if (acknowledged_by_other(w, a, NULL))
		return MOR_ST_DISPUTED;
	if ((flags & MOR_Q_KEEPERS) && mor_keepers_rescue(w, a, rot))
		return MOR_ST_DISPUTED;
	return MOR_ST_VOID;
}

/* The first counting rotation after position b (chain signatures replace no key). */
static const mor_act *next_rotation(const mor_chain *c, int64_t b)
{
	for (size_t i = (size_t)b + 1; i < c->npos; i++)
		if (c->pos[i].act->type == 1)
			return c->pos[i].act;
	return NULL;
}

const mor_act *mor_voiding_rotation(mor_world *w, const mor_act *a)
{
	if (!a->signer || !a->binding)
		return NULL;
	const mor_act *b = mor_world_get(w, a->binding);
	if (!b || !mor_is_chain_act(w, b))
		return NULL;
	const mor_chain *c = mor_chain_get(w, a->signer);
	int64_t pb = c ? mor_chain_position_of(c, b->id) : -1;
	return pb < 0 ? NULL : next_rotation(c, pb);
}

static int chain_act_status(mor_world *w, const mor_act *a)
{
	if (a->type == 0) {
		const mor_chain *c = mor_chain_get(w, a->id);
		return c ? c->genesis_status : MOR_ST_UNKNOWN;
	}
	if (!a->signer)
		return MOR_ST_INVALID;
	const mor_chain *c = mor_chain_get(w, a->signer);
	if (!c)
		return MOR_ST_UNKNOWN;
	int64_t n = mor_chain_act_pos(w, a);
	if (n < 1)
		return MOR_ST_INVALID;
	if ((size_t)n < c->npos) {
		if (c->pos[n].act == a)
			return MOR_ST_VALID;
		/* Another act counts at its position. Built on the same predecessor and
		 * valid, it lost; built on something that does not count, it is judged by that. */
		if (mor_hash_eq(mor_pl_hash(a, 0), c->pos[n - 1].act->id))
			return chain_act_checks(w, a, &c->pos[n - 1]) == MOR_ST_INVALID ? MOR_ST_INVALID : MOR_ST_VOID;
		const mor_act *p = mor_world_get(w, mor_pl_hash(a, 0));
		return p ? MOR_ST_VOID : MOR_ST_UNKNOWN;
	}
	if ((size_t)n == c->npos) {
		if (!mor_hash_eq(mor_pl_hash(a, 0), c->pos[n - 1].act->id)) {
			const mor_act *p = mor_world_get(w, mor_pl_hash(a, 0));
			return p ? MOR_ST_VOID : MOR_ST_UNKNOWN;
		}
		int st = chain_act_checks(w, a, &c->pos[n - 1]);
		if (st != MOR_ST_VALID)
			return st;
		switch (c->next) {
		case MOR_NEXT_PENDING: return MOR_ST_PENDING;
		case MOR_NEXT_CONTESTED: return MOR_ST_CONTESTED;
		default: return MOR_ST_UNKNOWN;
		}
	}
	/* Beyond the chain's end: built on an act that does not count yet. */
	const mor_act *p = mor_world_get(w, mor_pl_hash(a, 0));
	if (!p)
		return MOR_ST_UNKNOWN;
	if (p == a)
		return MOR_ST_INVALID;
	int ps = mor_status_act(w, p, 0);
	return ps == MOR_ST_PENDING ? MOR_ST_PENDING : ps == MOR_ST_UNKNOWN ? MOR_ST_UNKNOWN : MOR_ST_VOID;
}

static int everyday_status(mor_world *w, const mor_act *a, int flags)
{
	/* everyday check 1 */
	if (!a->signer || !a->binding || !a->prev)
		return MOR_ST_INVALID;
	if (a->prev->v > 1)
		return MOR_ST_INVALID;
	const mor_act *b = mor_world_get(w, a->binding);
	if (b == a)
		return MOR_ST_INVALID;
	if (!b)
		return MOR_ST_UNKNOWN;
	if (mor_world_is_mir(w, b, MOR_IDENTITY)) {
		if (!mor_is_chain_act(w, b) || !mor_hash_eq(chain_owner(b), a->signer))
			return MOR_ST_INVALID;
		if (b->type == 16)
			return MOR_ST_INVALID;
		/* everyday check 2 */
		const mor_cbor *key = b->type == 0 ? mor_pl(b, 0) : mor_pl(b, 2);
		if (!key_equals(&a->sig, key))
			return MOR_ST_INVALID;
		const mor_chain *c = mor_chain_get(w, a->signer);
		if (!c)
			return MOR_ST_UNKNOWN;
		int64_t pb = mor_chain_position_of(c, b->id);
		if (pb < 0) {
			int bs = mor_status_act(w, b, 0);
			if (bs == MOR_ST_PENDING || bs == MOR_ST_UNKNOWN || bs == MOR_ST_CONTESTED)
				return bs == MOR_ST_UNKNOWN ? MOR_ST_UNKNOWN : MOR_ST_PENDING;
			return MOR_ST_INVALID; /* the binding names no act that counts */
		}
		/* everyday check 3: a later counting rotation judges it */
		const mor_act *rot = next_rotation(c, pb);
		if (!rot)
			return MOR_ST_VALID;
		/* An escape endorsement naming that rotation is never judged by it. */
		if (mor_world_is_mir(w, a, MOR_IDENTITY) && a->type == 14 && mor_objects_has(a, a->signer, rot->id))
			return MOR_ST_VALID;
		return judged_by(w, a, rot, flags);
	}
	/* A scoped key: the binding names a held act of a higher MIR; Identity
	 * checks only the signature (done), the installing MIR judges the rest. */
	if (mor_world_is_mir(w, a, MOR_IDENTITY))
		return MOR_ST_INVALID; /* a scoped key never signs an Identity act */
	return MOR_ST_SCOPED;
}

int mor_status_act(mor_world *w, const mor_act *a, int flags)
{
	if (!a)
		return MOR_ST_UNKNOWN;
	if (w->depth > MAX_DEPTH)
		return MOR_ST_UNKNOWN;
	w->depth++;
	int st = every_act(w, a);
	int result;
	if (st == MOR_ST_INVALID || a->open != MOR_OPEN_OK) {
		result = st;
	} else {
		int unknown = st == MOR_ST_UNKNOWN;
		int inner;
		if (mor_is_chain_act(w, a))
			inner = chain_act_status(w, a);
		else
			inner = everyday_status(w, a, flags);
		/* Text rule 14, at a verifier implementing Agreements. */
		if (inner != MOR_ST_INVALID && mor_world_is_mir(w, a, MOR_TEXT)) {
			int t = mor_text14(w, a);
			if (t == MOR_ST_INVALID)
				inner = MOR_ST_INVALID;
			else if (t == MOR_ST_UNKNOWN && inner != MOR_ST_PENDING)
				unknown = 1;
		}
		if (inner == MOR_ST_INVALID)
			result = inner;
		else if (unknown)
			result = MOR_ST_UNKNOWN;
		else
			result = inner;
	}
	w->depth--;
	return result;
}

int mor_status(mor_world *w, const uint8_t id[32], int flags)
{
	const mor_act *a = mor_world_get(w, id);
	if (!a)
		return MOR_ST_UNKNOWN;
	return mor_status_act(w, a, flags);
}
