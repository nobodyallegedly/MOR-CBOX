#include "agreements.h"

#include <string.h>

#include "formats.h"
#include "identity.h"
#include "text14.h"

#define MAX_LIST 64

const char *mor_backing_name(int b)
{
	switch (b) {
	case MOR_BK_BACKED: return "backed";
	case MOR_BK_BINDS: return "binds";
	case MOR_BK_VOID: return "not backed (void)";
	case MOR_BK_DISPUTED: return "disputed";
	case MOR_BK_NOT_BACKED: return "not backed";
	default: return "unknown";
	}
}

const char *mor_deadline_name(int d)
{
	switch (d) {
	case MOR_DL_MET_ANCHOR: return "met, by an anchor of the act";
	case MOR_DL_MET_KEEPERS: return "met, by the keepers' anchored records";
	case MOR_DL_MISSED: return "missed";
	default: return "undetermined";
	}
}

static int is_agr(const mor_world *w, const mor_act *a, uint64_t type)
{
	return mor_world_is_mir(w, a, MOR_AGREEMENTS) && a->type == type;
}

static const mor_act *terms_of(mor_world *w, const uint8_t *id)
{
	const mor_act *t = mor_world_get(w, id);
	return is_agr(w, t, 0) ? t : NULL;
}

int mor_history_holds(mor_world *w, const mor_act *from, const uint8_t chain[32], const uint8_t id[32])
{
	size_t mark = mor_arena_mark(w->a);
	size_t cap = w->nacts + 1, head = 0, tail = 0;
	const mor_act **q = mor_arena_array(w->a, cap, sizeof *q);
	uint8_t *seen = mor_arena_alloc(w->a, cap);
	int found = 0;
	if (!q || !seen)
		goto out;
	q[tail++] = from;
	while (head < tail && !found) {
		const mor_act *x = q[head++];
		const uint8_t *next[MAX_LIST];
		size_t nn = 0;
		for (uint64_t i = 0; x->prev && i < x->prev->v && nn < MAX_LIST; i++)
			next[nn++] = x->prev->items[i].data;
		for (uint64_t i = 0; x->objects && i < x->objects->v && nn < MAX_LIST; i++) {
			const mor_cbor *o = &x->objects->items[i];
			if (!chain || !memcmp(o->items[0].data, chain, 32))
				next[nn++] = o->items[1].data;
		}
		for (size_t i = 0; i < nn; i++) {
			if (!memcmp(next[i], id, 32)) {
				found = 1;
				break;
			}
			const mor_act *y = mor_world_get(w, next[i]);
			if (!y)
				continue;
			size_t idx = (size_t)(y - w->acts);
			if (seen[idx] || tail >= cap)
				continue;
			seen[idx] = 1;
			q[tail++] = y;
		}
	}
out:
	mor_arena_reset(w->a, mark);
	return found;
}

/* ---- belongs to ---- */

static void add_unique(const uint8_t **out, size_t *n, size_t max, const uint8_t *id)
{
	for (size_t i = 0; i < *n; i++)
		if (!memcmp(out[i], id, 32))
			return;
	if (*n < max)
		out[(*n)++] = id;
}

size_t mor_belongs_to(mor_world *w, const mor_act *a, const uint8_t **out, size_t max)
{
	size_t n = 0;
	if (!a || a->open != MOR_OPEN_OK)
		return 0;
	/* An agreement, or an act of its chain, named in objects. */
	for (uint64_t i = 0; a->objects && i < a->objects->v; i++) {
		const mor_cbor *o = &a->objects->items[i];
		for (int k = 0; k < 2; k++) {
			const uint8_t *x = o->items[k].data;
			if (terms_of(w, x)) {
				add_unique(out, &n, max, x);
				continue;
			}
			const mor_act *e = mor_world_get(w, x);
			for (uint64_t j = 0; e && e->objects && j < e->objects->v; j++) {
				const uint8_t *c = e->objects->items[j].items[0].data;
				if (terms_of(w, c))
					add_unique(out, &n, max, c);
			}
		}
	}
	/* Through the act it answers (Money receipts, claims and obligations; offers). */
	if (mor_world_is_mir(w, a, MOR_MONEY)) {
		const mor_cbor *pur = mor_pl(a, 9);
		if (pur && terms_of(w, pur->items[0].data))
			add_unique(out, &n, max, pur->items[0].data);
		const uint8_t *ful = a->type == 2 ? mor_pl_hash(a, 5) : a->type == 3 ? mor_pl_hash(a, 4) :
				    a->type == 1 ? mor_pl_hash(a, 4) : NULL;
		if (ful) {
			const mor_act *f = mor_world_get(w, ful);
			if (terms_of(w, ful))
				add_unique(out, &n, max, ful);
			else if (f && mor_world_is_mir(w, f, MOR_MONEY) && f->type == 1 && mor_pl_hash(f, 4) &&
				 terms_of(w, mor_pl_hash(f, 4)))
				add_unique(out, &n, max, mor_pl_hash(f, 4));
			else if (f && is_agr(w, f, 6))
				add_unique(out, &n, max, mor_pl_hash(f, 0) ? mor_pl_hash(f, 0) : f->id);
		}
	}
	if (is_agr(w, a, 6))
		add_unique(out, &n, max, mor_pl_hash(a, 0) ? mor_pl_hash(a, 0) : a->id);
	return n;
}

/* ---- keepers ---- */

/* A keeper's record of an act: an Agreements type 2 act of the keeper's
 * operator naming the act and its own signer, valid under Identity. */
static const mor_act *record_of(mor_world *w, const uint8_t op[32], const mor_act *x)
{
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *r = &w->acts[i];
		if (!is_agr(w, r, 2) || !mor_hash_eq(r->signer, op))
			continue;
		const mor_cbor *e = mor_pl(r, 0);
		if (memcmp(e->items[0].data, x->id, 32) || memcmp(e->items[1].data, mor_signer(x), 32))
			continue;
		int st = mor_status_act(w, r, 0);
		/* Rule 42: a record a rule reads to place an act keeps its effect
		 * whatever the keeper's later rotation disowned. */
		if (st == MOR_ST_VALID || st == MOR_ST_VOID || st == MOR_ST_DISPUTED)
			return r;
	}
	return NULL;
}

/* Is record r at or before record s in the keeper's sequence? */
static int at_or_before(mor_world *w, const mor_act *r, const mor_act *s)
{
	const mor_act *cur = s;
	for (size_t steps = 0; cur && steps <= w->nacts; steps++) {
		if (cur == r)
			return 1;
		const uint8_t *p = mor_prev(cur);
		if (!p)
			return 0;
		cur = mor_world_get(w, p);
		if (cur && !mor_hash_eq(cur->signer, s->signer))
			return 0;
	}
	return 0;
}

/* The keepers' rule [0] / [1, k] / [2, ops] met by the keepers flagged. */
static int keepers_rule_met(const mor_cbor *ops, const mor_cbor *rule, const int *ok, int fewer_ok)
{
	uint64_t n = ops->v, cnt = 0;
	for (uint64_t i = 0; i < n; i++)
		cnt += (uint64_t)ok[i];
	(void)fewer_ok;
	if (!rule || rule->items[0].v == 0)
		return cnt == n;
	if (rule->items[0].v == 1)
		return cnt >= rule->items[1].v;
	for (uint64_t j = 0; j < rule->items[1].v; j++) {
		int found = 0;
		for (uint64_t i = 0; i < n; i++)
			if (!memcmp(ops->items[i].data, rule->items[1].items[j].data, 32) && ok[i])
				found = 1;
		if (!found)
			return 0;
	}
	return 1;
}

/* The version in force for the act of an agreement it belongs to: for a deal,
 * the version it names (the latest complete version its history holds on
 * that line is not followed further in MOR 1 vectors; see QUESTIONS.md). */
static const mor_act *version_in_force(mor_world *w, const uint8_t *agreement)
{
	return terms_of(w, agreement);
}

int mor_keepers_rescue(mor_world *w, const mor_act *a, const mor_act *ending)
{
	const uint8_t *agrs[MAX_LIST];
	size_t na = mor_belongs_to(w, a, agrs, MAX_LIST);
	for (size_t i = 0; i < na; i++) {
		const mor_act *t = version_in_force(w, agrs[i]);
		const mor_cbor *kp = t ? mor_pl(t, 3) : NULL;
		if (!kp)
			continue;
		const mor_cbor *ops = &kp->items[0], *rule = &kp->items[1];
		int ok[MAX_LIST] = { 0 };
		for (uint64_t k = 0; k < ops->v && k < MAX_LIST; k++) {
			const mor_act *rx = record_of(w, ops->items[k].data, a);
			const mor_act *re = record_of(w, ops->items[k].data, ending);
			ok[k] = rx && re && rx != re && at_or_before(w, rx, re);
		}
		if (keepers_rule_met(ops, rule, ok, 0))
			return 1;
	}
	return 0;
}

/* ---- agreements: signatures and completeness ---- */

/* Has `party` a valid signature act naming `act`? */
static int signed_by(mor_world *w, const uint8_t act[32], const uint8_t party[32], int *unknown)
{
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *s = &w->acts[i];
		if (!is_agr(w, s, 1) || !mor_hash_eq(s->signer, party) || !mor_hash_eq(mor_pl_hash(s, 0), act))
			continue;
		int st = mor_status_act(w, s, 0);
		if (st == MOR_ST_VALID)
			return 1;
		if (st == MOR_ST_UNKNOWN || st == MOR_ST_PENDING)
			*unknown = 1;
	}
	return 0;
}

static int complete_version(mor_world *w, const mor_act *t, int depth);

int mor_agreement_exists(mor_world *w, const uint8_t terms[32])
{
	const mor_act *t = mor_world_get(w, terms);
	if (!t)
		return MOR_AGR_UNKNOWN;
	if (!is_agr(w, t, 0))
		return MOR_AGR_INVALID;
	return complete_version(w, t, 0);
}

static int complete_version(mor_world *w, const mor_act *t, int depth)
{
	int st = mor_status_act(w, t, 0);
	if (st == MOR_ST_INVALID)
		return MOR_AGR_INVALID;
	if (st == MOR_ST_UNKNOWN)
		return MOR_AGR_UNKNOWN;
	if (depth > 16)
		return MOR_AGR_UNKNOWN;
	int unknown = 0;
	const mor_cbor *parties = mor_pl(t, 0);
	const uint8_t *parent = mor_pl_hash(t, 11);
	if (!parent) {
		/* Founding terms exist once every party has signed them. */
		for (uint64_t i = 0; i < parties->v; i++)
			if (!signed_by(w, t->id, parties->items[i].data, &unknown))
				return unknown ? MOR_AGR_UNKNOWN : MOR_AGR_NOT;
		return MOR_AGR_EXISTS;
	}
	const mor_act *p = mor_world_get(w, parent);
	if (!p)
		return MOR_AGR_UNKNOWN;
	if (!is_agr(w, p, 0))
		return MOR_AGR_INVALID;
	int pe = complete_version(w, p, depth + 1);
	if (pe != MOR_AGR_EXISTS)
		return pe == MOR_AGR_UNKNOWN ? MOR_AGR_UNKNOWN : MOR_AGR_NOT;
	const mor_cbor *mark = mor_pl(t, 4), *pparties = mor_pl(p, 0);
	if (!mor_pl(p, 12)) {
		/* A deal: the mark names the clone rule [1] and the parent's parties (rule 47). */
		if (mark->v != 1 || mark->items[0].items[0].v != 1 || mark->items[0].items[0].items[0].v != 1)
			return MOR_AGR_INVALID;
		const mor_cbor *signers = &mark->items[0].items[1];
		if (signers->v != pparties->v)
			return MOR_AGR_INVALID;
		for (uint64_t i = 0; i < pparties->v; i++)
			if (!mor_act_names_in(signers, pparties->items[i].data))
				return MOR_AGR_INVALID;
	}
	/* Every party the mark names, and every party the clone adds, signed it. */
	for (uint64_t m = 0; m < mark->v; m++) {
		const mor_cbor *signers = &mark->items[m].items[1];
		for (uint64_t i = 0; i < signers->v; i++)
			if (!signed_by(w, t->id, signers->items[i].data, &unknown))
				return unknown ? MOR_AGR_UNKNOWN : MOR_AGR_NOT;
	}
	for (uint64_t i = 0; i < parties->v; i++)
		if (!mor_act_names_in(pparties, parties->items[i].data) &&
		    !signed_by(w, t->id, parties->items[i].data, &unknown))
			return unknown ? MOR_AGR_UNKNOWN : MOR_AGR_NOT;
	return MOR_AGR_EXISTS;
}

/* The complete clones of a version (versions naming it as parent). */
static size_t complete_children(mor_world *w, const uint8_t v[32], const mor_act **out, size_t max)
{
	size_t n = 0;
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *c = &w->acts[i];
		if (!is_agr(w, c, 0) || !mor_hash_eq(mor_pl_hash(c, 11), v))
			continue;
		if (complete_version(w, c, 0) == MOR_AGR_EXISTS && n < max)
			out[n++] = c;
	}
	return n;
}

int mor_deal_fork(mor_world *w, const uint8_t version[32], mor_fork *out)
{
	memset(out, 0, sizeof *out);
	memcpy(out->reference, version, 32);
	const mor_act *kids[MAX_LIST];
	size_t n = complete_children(w, version, kids, MAX_LIST);
	out->branches = (int)n;
	/* Tangled: three or more complete versions share one version, or a branch
	 * splits again (rule 47, "A tangled fork stays on its reference"). */
	if (n >= 3)
		out->tangled = 1;
	for (size_t i = 0; i < n && n >= 2; i++) {
		const mor_act *cur = kids[i];
		for (size_t steps = 0; steps < 64; steps++) {
			const mor_act *gk[MAX_LIST];
			size_t m = complete_children(w, cur->id, gk, MAX_LIST);
			if (m >= 2) {
				out->tangled = 1;
				break;
			}
			if (m == 0)
				break;
			cur = gk[0];
		}
	}
	return 0;
}

/* ---- a collective's acts: agreement in force, areas, consent ---- */

/* The layer of a task (Development, "The tasks the core defines"). */
static int task_layer(uint64_t task)
{
	if (task >= 1 && task <= 3)
		return 0;
	if (task == 4 || task == 5 || task == 11)
		return 1;
	if (task == 6 || task == 7)
		return 2;
	if (task == 8 || task == 9 || task == 10 || (task >= 12 && task <= 14))
		return 3;
	return -1;
}

/* The layers an act belongs to, under the agreement in force (a bit set). */
static unsigned act_layers(const mor_world *w, const mor_act *x, const mor_act *agr)
{
	switch (mor_world_mir_of(w, x->spec)) {
	case MOR_IDENTITY: return 1u << 0;
	case MOR_ENVELOPES:
	case MOR_TEXT: return 1u << 1;
	case MOR_MONEY: return 1u << 2;
	case MOR_AGREEMENTS: return 1u << 3;
	case MOR_DEVELOPMENT: return 1u << 4;
	default: break;
	}
	unsigned l = 0;
	const mor_cbor *m = mor_pl(agr, 2), *ext = mor_pl(agr, 15);
	for (uint64_t i = 0; m && i < m->v; i++)
		if (!memcmp(m->items[2 * i + 1].data, x->spec, 32)) {
			int tl = task_layer(m->items[2 * i].v);
			if (tl >= 0)
				l |= 1u << tl;
		}
	if (ext && mor_act_names_in(ext, x->spec))
		l |= 1u << 4;
	return l;
}

static int area_reaches(const mor_world *w, const mor_cbor *area, const mor_act *x, unsigned layers)
{
	const mor_cbor *k = mor_cbor_map_get(area, 3);
	(void)w;
	for (uint64_t i = 0; k && i < k->v; i++) {
		const mor_cbor *kd = &k->items[i];
		if (kd->items[0].v == 0 && kd->items[1].v < 8 && ((layers >> kd->items[1].v) & 1u))
			return 1;
		if (kd->items[0].v == 1 && !memcmp(kd->items[1].data, x->spec, 32) && kd->items[2].v == x->type)
			return 1;
	}
	return 0;
}

/* A holder's member key: a signature act of a founding or clone naming a key. */
static const mor_act *member_key_sig(mor_world *w, const mor_act *x)
{
	const mor_act *b = x->binding ? mor_world_get(w, x->binding) : NULL;
	if (!b || !is_agr(w, b, 1) || !mor_pl(b, 1))
		return NULL;
	return b;
}

static int sig_key_equals(const mor_sigval *s, const mor_cbor *sk)
{
	if (!sk || sk->v != 2)
		return 0;
	if (s->kind == MOR_SCHEME_NUM ? (sk->items[0].type != MOR_CBOR_UINT || sk->items[0].v != s->num)
				      : (!mor_cbor_is_hash(&sk->items[0]) || memcmp(sk->items[0].data, s->hash, 32)))
		return 0;
	return sk->items[1].v == s->key_len && !memcmp(sk->items[1].data, s->key, s->key_len);
}

/* The agreement a collective's identity chain declares at a chain act (the
 * declaration of kind 0 at or before it). */
static const uint8_t *declared_at(mor_world *w, const uint8_t *coll, const mor_act *link)
{
	const mor_chain *c = mor_chain_get(w, coll);
	int64_t p = c ? mor_chain_position_of(c, link->id) : -1;
	const uint8_t *found = NULL;
	for (int64_t i = 0; i <= p; i++) {
		const mor_act *a = c->pos[i].act;
		const mor_cbor *d = a->type == 0 ? mor_pl(a, 4) : a->type == 1 ? mor_pl(a, 8) : NULL;
		for (uint64_t k = 0; d && k < d->v; k++) {
			const mor_cbor *e = &d->items[k];
			if (memcmp(e->items[0].data, w->mir[MOR_AGREEMENTS], 32) || e->items[1].v != 0)
				continue;
			const mor_cbor *v = &e->items[2];
			if (mor_cbor_is_hash(v))
				found = v->data;
			else if (v->type == MOR_CBOR_ARRAY && v->v >= 1 && mor_cbor_is_hash(&v->items[0]))
				found = v->items[0].data;
			else
				found = NULL;
		}
	}
	return found;
}

/* Is every party of the agreement among the act's recipients, or the act public? */
static int done_sealed(const mor_act *x, const mor_act *agr)
{
	if (x->key)
		return 1;
	const mor_cbor *parties = mor_pl(agr, 0);
	for (uint64_t i = 0; i < parties->v; i++)
		if (!x->to || !mor_act_names_in(x->to, parties->items[i].data))
			return 0;
	return 1;
}

/* Has holder h signed the agreement (or a version it descends from)? */
static int holder_signed(mor_world *w, const mor_act *agr, const uint8_t *h)
{
	int unknown = 0;
	const mor_act *v = agr;
	for (int d = 0; v && d < 32; d++) {
		if (signed_by(w, v->id, h, &unknown))
			return 1;
		const uint8_t *p = mor_pl_hash(v, 11);
		v = p ? terms_of(w, p) : NULL;
	}
	return 0;
}

int mor_consent_counts(mor_world *w, const uint8_t id[32])
{
	const mor_act *x = mor_world_get(w, id);
	if (!x)
		return -1;
	int st = mor_status_act(w, x, 0);
	if (st == MOR_ST_INVALID || st == MOR_ST_VOID)
		return 0;
	if (st == MOR_ST_UNKNOWN)
		return -1;
	const uint8_t *coll = x->signer;
	const mor_cbor *decl;
	if (!coll)
		return 0; /* a genesis is no act of a collective's areas */
	int isc = mor_identity_agreement(w, coll, &decl);
	if (isc < 0)
		return -1;
	if (isc == 0)
		return st == MOR_ST_VALID ? 1 : 0; /* not a collective: its own signature */
	/* The key it is signed with, and the agreement in force for it (rule 72). */
	const mor_act *mk = member_key_sig(w, x);
	const uint8_t *agr_id = NULL;
	const uint8_t *holder = NULL;
	if (mk) {
		if (!sig_key_equals(&x->sig, mor_pl(mk, 1)))
			return 0;
		/* a key named by two signatures of different signers is no member's */
		for (size_t i = 0; i < w->nacts; i++) {
			const mor_act *o = &w->acts[i];
			if (is_agr(w, o, 1) && o != mk && mor_pl(o, 1) && !mor_hash_eq(o->signer, mk->signer) &&
			    sig_key_equals(&x->sig, mor_pl(o, 1)))
				return 0;
		}
		agr_id = mor_pl_hash(mk, 0);
		holder = mk->signer;
	} else {
		const mor_act *b = x->binding ? mor_world_get(w, x->binding) : NULL;
		if (!b)
			return -1;
		if (!mor_is_chain_act(w, b))
			return -1; /* a grant key: judged by backing (rule 80) */
		agr_id = declared_at(w, coll, b);
	}
	const mor_act *agr = agr_id ? terms_of(w, agr_id) : NULL;
	if (!agr)
		return -1;
	if (mk && !mor_act_names_in(mor_pl(agr, 0), holder))
		return 0;
	/* Done: sealed to every member, or public, and on the collective's chain,
	 * citing a decision it acts under (rules 57, 58). */
	if (!done_sealed(x, agr))
		return 0;
	int cites = 0, unknown = 0;
	for (uint64_t i = 0; x->objects && i < x->objects->v; i++) {
		const mor_cbor *o = &x->objects->items[i];
		if (memcmp(o->items[0].data, coll, 32))
			continue;
		if (mk && !memcmp(o->items[1].data, agr->id, 32)) {
			cites = 1;
			continue;
		}
		int d = mor_is_decision(w, coll, o->items[1].data);
		if (d < 0)
			unknown = 1;
		else if (d == 1)
			cites = 1;
		else
			return 0; /* naming on the chain an act that is no decision of it */
	}
	if (!cites)
		return unknown ? -1 : 0;
	/* The areas that reach it (rule 60). */
	const mor_cbor *areas = mor_pl(agr, 19);
	unsigned layers = act_layers(w, x, agr);
	int reached = 0;
	for (uint64_t i = 0; areas && i < areas->v; i++) {
		const mor_cbor *area = &areas->items[i];
		if (!area_reaches(w, area, x, layers))
			continue;
		reached = 1;
		if (mor_cbor_map_get(area, 6))
			return -1; /* a limits cMIP this verifier does not hold */
		const mor_cbor *holders = mor_cbor_map_get(area, 1);
		uint64_t need = mor_cbor_map_get(area, 2)->v, got = 0;
		/* When fewer voices remain than the number, all of them meet it (rule 65). */
		if (need > holders->v)
			need = holders->v;
		if (need == 0)
			return 0; /* frozen */
		for (uint64_t h = 0; h < holders->v; h++) {
			const uint8_t *hid = holders->items[h].data;
			if (!holder_signed(w, agr, hid))
				continue;
			int ok = mk && mor_hash_eq(holder, hid);
			for (size_t i2 = 0; i2 < w->nacts && !ok; i2++) {
				const mor_act *s = &w->acts[i2];
				if (!is_agr(w, s, 1) || !mor_hash_eq(mor_pl_hash(s, 0), x->id))
					continue;
				const mor_act *smk = member_key_sig(w, s);
				const uint8_t *by = smk ? smk->signer : s->signer;
				if (!mor_hash_eq(by, hid))
					continue;
				int ss = mor_status_act(w, s, 0);
				if (ss == MOR_ST_VALID || (smk && ss == MOR_ST_SCOPED))
					ok = 1;
			}
			got += (uint64_t)ok;
		}
		if (got < need)
			return 0;
	}
	if (!reached) {
		/* No area reaches it: it counts on the collective's own signature; an act
		 * signed with a member key there counts for nothing (rules 60, 73). An act
		 * of a specification the agreement never adopted counts for nothing where
		 * the agreement has areas. */
		if (mk)
			return 0;
		if (areas && layers == 0 && mor_world_mir_of(w, x->spec) == MOR_MIR_NONE)
			return 0;
		return 1;
	}
	return 1;
}

/* ---- grants (rules 73 to 80) ---- */

static int is_decision_type(const mor_world *w, const mor_act *x)
{
	if (mor_is_chain_act(w, x))
		return 1;
	return mor_world_is_mir(w, x, MOR_AGREEMENTS) && (x->type == 17 || x->type == 9 || x->type == 10);
}

/* Within the grant's reach: its kinds (field 6), or, naming none, every act but a decision. */
static int within_reach(const mor_world *w, const mor_act *g, const mor_act *x)
{
	const mor_cbor *k = mor_pl(g, 6);
	if (is_decision_type(w, x))
		return 0;
	if (!k)
		return 1;
	unsigned layers = 0;
	switch (mor_world_mir_of(w, x->spec)) {
	case MOR_IDENTITY: layers = 1u; break;
	case MOR_ENVELOPES:
	case MOR_TEXT: layers = 2u; break;
	case MOR_MONEY: layers = 4u; break;
	case MOR_AGREEMENTS: layers = 8u; break;
	case MOR_DEVELOPMENT: layers = 16u; break;
	default: break;
	}
	for (uint64_t i = 0; i < k->v; i++) {
		const mor_cbor *kd = &k->items[i];
		if (kd->items[0].v == 0 && kd->items[1].v < 8 && ((layers >> kd->items[1].v) & 1u))
			return 1;
		if (kd->items[0].v == 1 && !memcmp(kd->items[1].data, x->spec, 32) && kd->items[2].v == x->type)
			return 1;
	}
	return 0;
}

/* Did the grantor adopt the act: an act of its own key that counts
 * acknowledging, citing or paying on it (rule 76)? */
static int adopted(mor_world *w, const mor_act *x, const uint8_t *grantor)
{
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *k = &w->acts[i];
		if (k == x || !mor_hash_eq(k->signer, grantor) || k->open != MOR_OPEN_OK)
			continue;
		const mor_act *b = k->binding ? mor_world_get(w, k->binding) : NULL;
		if (!b || !mor_is_chain_act(w, b))
			continue; /* its own key, never a grant key */
		if (is_agr(w, k, 10))
			continue; /* a revocation's history is judged as the ending */
		int names = mor_acks_name(k, x->id) || mor_history_holds(w, k, grantor, x->id) ||
			    (mor_world_is_mir(w, k, MOR_MONEY) && k->type == 2 && mor_hash_eq(mor_pl_hash(k, 5), x->id));
		if (names && mor_status_act(w, k, 0) == MOR_ST_VALID)
			return 1;
	}
	return 0;
}

int mor_backing(mor_world *w, const uint8_t id[32])
{
	const mor_act *x = mor_world_get(w, id);
	if (!x)
		return MOR_BK_UNKNOWN;
	int st = mor_status_act(w, x, 0);
	if (st == MOR_ST_INVALID)
		return MOR_BK_NOT_BACKED;
	if (st == MOR_ST_UNKNOWN)
		return MOR_BK_UNKNOWN;
	const mor_act *g = x->binding ? mor_world_get(w, x->binding) : NULL;
	if (!g)
		return MOR_BK_UNKNOWN;
	if (!is_agr(w, g, 9))
		return MOR_BK_NOT_BACKED;
	const uint8_t *grantor = g->signer;
	/* The act is the grantor's, signed with the grant key, citing the grant on
	 * the grantor's chain. */
	if (!mor_hash_eq(x->signer, grantor) || !sig_key_equals(&x->sig, mor_pl(g, 9)))
		return MOR_BK_NOT_BACKED;
	if (!mor_objects_has(x, grantor, g->id))
		return MOR_BK_NOT_BACKED;
	/* The grant counts: valid, public (a person's grant), accepted by its grantee. */
	int gs = mor_status_act(w, g, 0);
	if (gs == MOR_ST_UNKNOWN || gs == MOR_ST_PENDING)
		return MOR_BK_UNKNOWN;
	if (gs != MOR_ST_VALID || !g->key)
		return MOR_BK_NOT_BACKED;
	int unknown = 0;
	if (!signed_by(w, g->id, mor_pl_hash(g, 0), &unknown))
		return unknown ? MOR_BK_UNKNOWN : MOR_BK_NOT_BACKED;
	if (!within_reach(w, g, x))
		return MOR_BK_NOT_BACKED;
	if (mor_pl(g, 3) || mor_pl(g, 4))
		return MOR_BK_UNKNOWN; /* limits judged by a cMIP this verifier does not hold */
	/* Endings: revocations of the grant by its grantor (rule 76). */
	int any = 0, held_by_all = 1, rescued = 1;
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *v = &w->acts[i];
		if (!is_agr(w, v, 10) || !mor_hash_eq(mor_pl_hash(v, 0), g->id) || !mor_hash_eq(v->signer, grantor))
			continue;
		if (mor_status_act(w, v, 0) != MOR_ST_VALID)
			continue;
		any = 1;
		if (mor_history_holds(w, v, grantor, x->id))
			continue;
		held_by_all = 0;
		/* racing it or after it: void, unless another identity (never the
		 * grantee) acknowledged it, or a keeper recorded it before the revocation */
		if (!mor_acknowledged_by_other(w, x, mor_pl_hash(g, 0)) && !mor_keepers_rescue(w, x, v))
			rescued = 0;
	}
	if (!any)
		return MOR_BK_BACKED;
	if (held_by_all)
		return MOR_BK_BINDS;
	if (adopted(w, x, grantor))
		return MOR_BK_BINDS;
	return rescued ? MOR_BK_DISPUTED : MOR_BK_VOID;
}

/* ---- deadlines (rule 33) ---- */

static int anchor_point(const mor_world *w, const uint8_t id[32], const uint8_t *clock, size_t clen, uint64_t *pt)
{
	int found = 0;
	for (size_t i = 0; i < w->nanchors; i++) {
		const mor_anchor *an = &w->anchors[i];
		if (memcmp(an->act, id, 32) || an->clock_len != clen || memcmp(an->clock, clock, clen))
			continue;
		if (!found || an->point < *pt)
			*pt = an->point;
		found = 1;
	}
	return found;
}

int mor_deadline(mor_world *w, const uint8_t id[32], const uint8_t (*keepers)[32], size_t nk, const mor_cbor *rule,
		 const uint8_t *clock, size_t clen, uint64_t point)
{
	uint64_t pt;
	/* An anchor of the act at or before the point shows it met, whatever any
	 * keeper's sequence lacks. */
	if (anchor_point(w, id, clock, clen, &pt) && pt <= point)
		return MOR_DL_MET_ANCHOR;
	const mor_act *x = mor_world_get(w, id);
	int met[MAX_LIST] = { 0 }, missed[MAX_LIST] = { 0 };
	if (nk > MAX_LIST)
		nk = MAX_LIST;
	for (size_t k = 0; k < nk; k++) {
		const mor_act *rx = x ? record_of(w, keepers[k], x) : NULL;
		for (size_t i = 0; i < w->nacts; i++) {
			const mor_act *r = &w->acts[i];
			if (!is_agr(w, r, 2) || !mor_hash_eq(r->signer, keepers[k]))
				continue;
			if (!anchor_point(w, r->id, clock, clen, &pt))
				continue;
			if (mor_status_act(w, r, 0) == MOR_ST_INVALID)
				continue;
			int holds = rx && at_or_before(w, rx, r);
			if (pt <= point && holds)
				met[k] = 1;
			if (pt > point && !holds)
				missed[k] = 1;
		}
	}
	/* The keepers' rule counts them: met when the keepers showing it met meet
	 * the rule; missed when those showing it missed leave it unmeetable. */
	uint64_t nmet = 0, nmiss = 0;
	for (size_t k = 0; k < nk; k++) {
		nmet += (uint64_t)met[k];
		nmiss += (uint64_t)missed[k];
	}
	uint64_t kind = rule ? rule->items[0].v : 0;
	if (kind == 0) {
		if (nmet == nk && nk)
			return MOR_DL_MET_KEEPERS;
		if (nmiss > 0)
			return MOR_DL_MISSED;
	} else if (kind == 1) {
		uint64_t need = rule->items[1].v;
		if (nmet >= need)
			return MOR_DL_MET_KEEPERS;
		if (nmiss > nk - need)
			return MOR_DL_MISSED;
	} else {
		const mor_cbor *ops = &rule->items[1];
		int all = 1, anymiss = 0;
		for (uint64_t j = 0; j < ops->v; j++) {
			int m = 0, s = 0;
			for (size_t k = 0; k < nk; k++)
				if (!memcmp(keepers[k], ops->items[j].data, 32)) {
					m = met[k];
					s = missed[k];
				}
			all &= m;
			anymiss |= s;
		}
		if (all)
			return MOR_DL_MET_KEEPERS;
		if (anymiss)
			return MOR_DL_MISSED;
	}
	return MOR_DL_UNDETERMINED;
}
