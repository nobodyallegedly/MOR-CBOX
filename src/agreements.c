#include "agreements.h"

#include <string.h>

#include "formats.h"
#include "identity.h"
#include "money.h"
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

/* The steps "belongs to" takes from an act: each objects entry, and each act
 * it answers (a Money record's claim and what it fulfils, an obligation's
 * agreement or offer, an offer's agreement). */
static size_t belong_steps(const mor_world *w, const mor_act *y, const uint8_t **next, size_t max)
{
	size_t n = 0;
	for (uint64_t i = 0; y->objects && i < y->objects->v; i++)
		for (int k = 0; k < 2 && n < max; k++)
			next[n++] = y->objects->items[i].items[k].data;
	if (mor_world_is_mir(w, y, MOR_MONEY)) {
		const mor_cbor *pur = mor_pl(y, 9);
		if (pur && n < max)
			next[n++] = pur->items[0].data;
		const uint8_t *ful = y->type == 2 ? mor_pl_hash(y, 5) : (y->type == 3 || y->type == 1) ? mor_pl_hash(y, 4) : NULL;
		if (ful && n < max)
			next[n++] = ful;
	}
	if (is_agr(w, y, 6) && mor_pl_hash(y, 0) && n < max)
		next[n++] = mor_pl_hash(y, 0);
	return n;
}

size_t mor_belongs_to(mor_world *w, const mor_act *a, const uint8_t **out, size_t max, int *unknown)
{
	size_t n = 0;
	*unknown = 0;
	if (!a || a->open != MOR_OPEN_OK)
		return 0;
	/* Every action and decision of a collective belongs to its agreement. */
	const mor_cbor *decl;
	if (a->signer && !mor_is_chain_act(w, a) && !mor_world_is_mir(w, a, MOR_IDENTITY) &&
	    mor_identity_agreement(w, a->signer, &decl) == 1) {
		const mor_cbor *v = decl && decl->type == MOR_CBOR_ARRAY && decl->v ? &decl->items[0] : decl;
		if (mor_cbor_is_hash(v))
			add_unique(out, &n, max, v->data);
	}
	/* A lone seller's offer is itself the agreement its payments belong to. */
	if (is_agr(w, a, 6) && !mor_pl_hash(a, 0))
		add_unique(out, &n, max, a->id);
	/* From act to act, at no fixed depth, as far as the acts held allow. */
	size_t mark = mor_arena_mark(w->a);
	size_t cap = w->nacts + 1, head = 0, tail = 0;
	const mor_act **q = mor_arena_array(w->a, cap, sizeof *q);
	uint8_t *seen = mor_arena_alloc(w->a, cap);
	if (!q || !seen) {
		*unknown = 1;
		goto out;
	}
	memset(seen, 0, cap);
	q[tail++] = a;
	seen[(size_t)(a - w->acts)] = 1;
	while (head < tail) {
		const mor_act *y = q[head++];
		const uint8_t *next[2 * MAX_LIST];
		size_t nn = belong_steps(w, y, next, 2 * MAX_LIST);
		for (size_t i = 0; i < nn; i++) {
			const mor_act *t = mor_world_get(w, next[i]);
			if (!t) {
				*unknown = 1; /* beyond an act not held, unknown */
				continue;
			}
			if (is_agr(w, t, 0)) {
				add_unique(out, &n, max, t->id);
				continue;
			}
			if (is_agr(w, t, 6) && !mor_pl_hash(t, 0))
				add_unique(out, &n, max, t->id);
			/* an identity's genesis, rotations and Identity acts belong to none */
			if (mor_world_is_mir(w, t, MOR_IDENTITY))
				continue;
			size_t idx = (size_t)(t - w->acts);
			if (seen[idx] || tail >= cap)
				continue;
			seen[idx] = 1;
			q[tail++] = t;
		}
	}
out:
	mor_arena_reset(w->a, mark);
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

static int complete_version(mor_world *w, const mor_act *t, int depth);

/* How many versions lie between t and v on v's line (t a clone of a clone
 * ... of v), or -1 when t is not on it. */
static int line_depth(mor_world *w, const mor_act *t, const uint8_t *v)
{
	for (int d = 0; t && d < 64; d++) {
		if (!memcmp(t->id, v, 32))
			return d;
		const uint8_t *p = mor_pl_hash(t, 11);
		t = p ? terms_of(w, p) : NULL;
	}
	return -1;
}

/* The version in force for an act of an agreement it belongs to ("Belongs
 * to"): for a deal, the latest complete version, on the line of the version
 * the act names, that the act's history holds, or the version it names; for
 * a collective, the agreement its chain declares (named already). */
static const mor_act *version_in_force(mor_world *w, const uint8_t *agreement, const mor_act *a)
{
	const mor_act *named = terms_of(w, agreement);
	if (!named || mor_pl(named, 12))
		return named;
	const mor_act *best = named;
	int bd = 0;
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *c = &w->acts[i];
		if (!is_agr(w, c, 0) || c == named)
			continue;
		int d = line_depth(w, c, agreement);
		if (d <= bd || !mor_history_holds(w, a, NULL, c->id))
			continue;
		if (complete_version(w, c, 0) != MOR_AGR_EXISTS)
			continue;
		best = c;
		bd = d;
	}
	return best;
}

int mor_keepers_rescue(mor_world *w, const mor_act *a, const mor_act *ending)
{
	const uint8_t *agrs[MAX_LIST];
	int unknown = 0;
	size_t na = mor_belongs_to(w, a, agrs, MAX_LIST, &unknown);
	for (size_t i = 0; i < na; i++) {
		const mor_act *t = version_in_force(w, agrs[i], a);
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
	/* Beyond a step to an act not held, whether a keeper speaks for it cannot be told. */
	return unknown ? -1 : 0;
}

/* ---- agreements: signatures and completeness ---- */

#define MAX_NESTING 8

static const mor_act *deal_in_force(mor_world *w, const mor_act *founding);

/* Has `party` a valid signature act naming `act`? A party that is an
 * agreement, named by its line (its founding version), signs where every
 * party of its version in force signs, at most eight layers deep; a deeper
 * or looping one cannot be told (rule 1, "Any agreement may be a party"). */
static int signed_by_d(mor_world *w, const uint8_t act[32], const uint8_t party[32], int *unknown, int depth)
{
	const mor_act *inner = terms_of(w, party);
	if (inner) {
		if (depth >= MAX_NESTING || !memcmp(party, act, 32)) {
			*unknown = 1;
			return 0;
		}
		const mor_act *v = deal_in_force(w, inner);
		if (!v) {
			*unknown = 1;
			return 0;
		}
		const mor_cbor *ps = mor_pl(v, 0);
		for (uint64_t i = 0; i < ps->v; i++)
			if (!signed_by_d(w, act, ps->items[i].data, unknown, depth + 1))
				return 0;
		return 1;
	}
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

static int signed_by(mor_world *w, const uint8_t act[32], const uint8_t party[32], int *unknown)
{
	return signed_by_d(w, act, party, unknown, 0);
}

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

/* A deal's version in force on its line: from its founding, the one
 * complete clone of each version, while there is exactly one. */
static const mor_act *deal_in_force(mor_world *w, const mor_act *founding)
{
	const mor_act *cur = founding;
	for (int d = 0; d < 64; d++) {
		const mor_act *kids[2];
		if (complete_children(w, cur->id, kids, 2) != 1)
			break;
		cur = kids[0];
	}
	return cur;
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

static int done_sealed(const mor_act *x, const mor_act *agr);

/* The agreement in force for a collective's act, from the one its chain
 * declares: an operational clone a record of the collective puts in force
 * is in force at once for every act whose history holds that record
 * (record, type 17; "Made before, made after"). */
static int in_history_of(mor_world *w, const mor_act *const *roots, size_t nroots, const mor_act *r)
{
	for (size_t i = 0; i < nroots; i++)
		if (roots[i] == r || mor_history_holds(w, roots[i], NULL, r->id))
			return 1;
	return 0;
}

static const mor_act *recorded_in_force_at(mor_world *w, const uint8_t *coll, const mor_act *const *roots,
					   size_t nroots, const mor_act *base, int include_roots)
{
	const mor_act *best = base;
	int bd = 0;
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *r = &w->acts[i];
		if (!is_agr(w, r, 17) || !mor_hash_eq(r->signer, coll) || !mor_pl_hash(r, 0))
			continue;
		int is_root = 0;
		for (size_t k = 0; k < nroots; k++)
			is_root |= roots[k] == r;
		if (is_root && !include_roots)
			continue;
		const mor_act *c = terms_of(w, mor_pl_hash(r, 0));
		int d = c ? line_depth(w, c, base->id) : -1;
		if (d <= bd || !done_sealed(r, best) || !in_history_of(w, roots, nroots, r))
			continue;
		if (mor_status_act(w, r, 0) != MOR_ST_VALID)
			continue;
		/* the signature acts it names, valid, naming the clone */
		const mor_cbor *sigs = mor_pl(r, 2);
		int ok = sigs != NULL;
		for (uint64_t k = 0; ok && k < sigs->v; k++) {
			const mor_act *sg = mor_world_get(w, sigs->items[k].data);
			ok = sg && is_agr(w, sg, 1) && mor_hash_eq(mor_pl_hash(sg, 0), c->id) &&
			     mor_status_act(w, sg, 0) == MOR_ST_VALID;
		}
		if (!ok)
			continue;
		best = c;
		bd = d;
	}
	return best;
}

static const mor_act *recorded_in_force(mor_world *w, const uint8_t *coll, const mor_act *x, const mor_act *base)
{
	return recorded_in_force_at(w, coll, &x, 1, base, 0);
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

static int after_ending(mor_world *w, const uint8_t *coll, const mor_act *x);

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
	/* After the line of a complete fork or closing, its keys count for nothing. */
	if (after_ending(w, coll, x))
		return 0;
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
	if (!mk)
		agr = recorded_in_force(w, coll, x, agr);
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
		if (!mor_acknowledged_by_other(w, x, mor_pl_hash(g, 0)) && mor_keepers_rescue(w, x, v) != 1)
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

/* ---- a collective's endings: fork (type 19) and closing (type 20), rule 91 ---- */

/* Has `who` a chain signature (Identity type 16) naming `act` that counts in
 * its identity chain? 1 yes, 0 no, -1 its chain cannot be told. */
static int chain_signed(mor_world *w, const uint8_t *who, const uint8_t act[32])
{
	const mor_chain *c = mor_chain_get(w, who);
	if (!c || c->genesis_status != MOR_ST_VALID)
		return -1;
	for (size_t i = 0; i < c->npos; i++) {
		const mor_act *s = c->pos[i].act;
		if (s->type == 16 && mor_hash_eq(mor_pl_hash(s, 3), act))
			return 1;
	}
	return 0;
}

/* A change rule (terms field 18's form, rule 65) met by the parties flagged:
 * [0] every party, [1, n] n of them (all where fewer remain), [2, ids]
 * those named, [4, rules] each of them. 1, 0, or -1 for a form counted by
 * share, which needs stakes this verifier does not weigh. */
static int change_rule_met(const mor_cbor *rule, const mor_cbor *parties, const int *ok, int depth)
{
	uint64_t n = parties->v, cnt = 0;
	for (uint64_t i = 0; i < n; i++)
		cnt += (uint64_t)ok[i];
	if (!rule || rule->items[0].v == 0)
		return cnt == n;
	switch (rule->items[0].v) {
	case 1: return cnt >= (rule->items[1].v < n ? rule->items[1].v : n);
	case 2:
		for (uint64_t j = 0; j < rule->items[1].v; j++) {
			int found = 0;
			for (uint64_t i = 0; i < n; i++)
				if (ok[i] && !memcmp(parties->items[i].data, rule->items[1].items[j].data, 32))
					found = 1;
			if (!found)
				return 0;
		}
		return 1;
	case 4: {
		int unknown = 0;
		if (depth > 8)
			return -1;
		for (uint64_t j = 0; j < rule->items[1].v; j++) {
			int m = change_rule_met(&rule->items[1].items[j], parties, ok, depth + 1);
			if (m == 0)
				return 0;
			if (m < 0)
				unknown = 1;
		}
		return unknown ? -1 : 1;
	}
	default: return -1;
	}
}

/* Is every act the history from these roots names held? (prev and the
 * predecessors objects name, followed back.) */
static int history_all_held(mor_world *w, const mor_act *const *roots, size_t nroots)
{
	size_t mark = mor_arena_mark(w->a);
	size_t cap = w->nacts + 1, head = 0, tail = 0;
	const mor_act **q = mor_arena_array(w->a, cap, sizeof *q);
	uint8_t *seen = mor_arena_alloc(w->a, cap);
	int all = 1;
	if (!q || !seen) {
		all = 0;
		goto out;
	}
	memset(seen, 0, cap);
	for (size_t i = 0; i < nroots && tail < cap; i++) {
		size_t idx = (size_t)(roots[i] - w->acts);
		if (!seen[idx]) {
			seen[idx] = 1;
			q[tail++] = roots[i];
		}
	}
	while (head < tail && all) {
		const mor_act *x = q[head++];
		const uint8_t *next[MAX_LIST];
		size_t nn = 0;
		for (uint64_t i = 0; x->prev && i < x->prev->v && nn < MAX_LIST; i++)
			next[nn++] = x->prev->items[i].data;
		for (uint64_t i = 0; x->objects && i < x->objects->v && nn < MAX_LIST; i++)
			next[nn++] = x->objects->items[i].items[1].data;
		for (size_t i = 0; i < nn; i++) {
			const mor_act *y = mor_world_get(w, next[i]);
			if (!y) {
				all = 0;
				break;
			}
			size_t idx = (size_t)(y - w->acts);
			if (seen[idx] || tail >= cap)
				continue;
			seen[idx] = 1;
			q[tail++] = y;
		}
	}
out:
	mor_arena_reset(w->a, mark);
	return all;
}

#define MAX_TIPS 16

/* The kept tips of an ending, held: 1, 0 (one does not hold), -1 (cannot tell). */
static int ending_tips(mor_world *w, const mor_act *e, const uint8_t *coll, const mor_act **tips, size_t *n)
{
	const mor_cbor *kt = mor_pl(e, 3);
	*n = 0;
	int unknown = 0;
	for (uint64_t i = 0; i < kt->v; i++) {
		int h = mor_kept_tip_holds(w, &kt->items[i], coll);
		if (h == 0)
			return 0;
		if (h < 0) {
			unknown = 1;
			continue;
		}
		if (*n < MAX_TIPS)
			tips[(*n)++] = mor_world_get(w, kt->items[i].items[0].data);
	}
	return unknown ? -1 : 1;
}

/* Does the obligation bind the collective (done, rules 57, 58)? */
static int binds_collective(const mor_act *ob, const mor_act *agr)
{
	return done_sealed(ob, agr);
}

int mor_ending_complete(mor_world *w, const uint8_t id[32])
{
	const mor_act *e = mor_world_get(w, id);
	if (!e)
		return -1;
	int fork = is_agr(w, e, 19);
	if (!fork && !is_agr(w, e, 20))
		return 0;
	int st = mor_status_act(w, e, 0);
	if (st == MOR_ST_UNKNOWN || st == MOR_ST_PENDING)
		return -1;
	if (st != MOR_ST_VALID)
		return 0;
	const uint8_t *coll = mor_pl_hash(e, 1);
	int unknown = 0;
	/* Its chain act counts in the collective's identity chain. */
	const mor_chain *c = mor_chain_get(w, coll);
	if (!c || c->genesis_status == MOR_ST_UNKNOWN)
		return -1;
	const mor_act *ca = mor_world_get(w, mor_pl_hash(e, 2));
	if (!ca)
		return -1;
	if (mor_chain_position_of(c, ca->id) < 0)
		return 0;
	/* Every kept tip it names holds there. */
	const mor_act *tips[MAX_TIPS];
	size_t ntips;
	int th = ending_tips(w, e, coll, tips, &ntips);
	if (th == 0)
		return 0;
	if (th < 0)
		unknown = 1;
	/* The agreement it names is the one in force at its line. */
	const uint8_t *decl = declared_at(w, coll, ca);
	const mor_act *base = decl ? terms_of(w, decl) : NULL;
	if (!base)
		return -1;
	const mor_act *agr = recorded_in_force_at(w, coll, tips, ntips, base, 1);
	if (!mor_hash_eq(agr->id, mor_pl_hash(e, 0)))
		return 0;
	/* Done: sealed to every member, or public. */
	if (!done_sealed(e, agr))
		return 0;
	/* The members' chain signatures, under the constitutional change rule. */
	const mor_cbor *parties = mor_pl(agr, 0);
	int ok[MAX_LIST] = { 0 };
	if (parties->v > MAX_LIST)
		return -1;
	const mor_cbor *sides = fork ? mor_pl(e, 4) : NULL;
	for (uint64_t i = 0; i < parties->v; i++) {
		const uint8_t *m = parties->items[i].data;
		if (sides) {
			/* listed on exactly one side */
			int on = 0;
			for (uint64_t k = 0; k < sides->v; k++)
				on += mor_act_names_in(&sides->items[k].items[1], m);
			if (on > 1)
				return 0;
			if (on == 0)
				continue;
		}
		int cs = chain_signed(w, m, e->id);
		if (cs < 0)
			unknown = 1;
		else if (cs == 0 && sides)
			return 0; /* every member it lists signs it */
		ok[i] = cs == 1;
	}
	/* Its signer is a member whose signature counts. */
	int signer_ok = 0;
	for (uint64_t i = 0; i < parties->v; i++)
		if (ok[i] && mor_hash_eq(parties->items[i].data, e->signer))
			signer_ok = 1;
	if (!signer_ok && !unknown)
		return 0;
	int met = change_rule_met(mor_pl(agr, 18), parties, ok, 0);
	if (met == 0 && !unknown)
		return 0;
	if (met < 0)
		unknown = 1;
	if (sides) {
		/* Every member it lists is a member; each successor held, its founding
		 * terms having exactly that side's members as parties and naming the
		 * original as forked from. */
		for (uint64_t k = 0; k < sides->v; k++) {
			const mor_cbor *mem = &sides->items[k].items[1];
			for (uint64_t j = 0; j < mem->v; j++)
				if (!mor_act_names_in(parties, mem->items[j].data))
					return 0;
			const mor_cbor *sv;
			int isc = mor_identity_agreement(w, sides->items[k].items[0].data, &sv);
			if (isc < 0) {
				unknown = 1;
				continue;
			}
			const mor_cbor *v = sv && sv->type == MOR_CBOR_ARRAY && sv->v ? &sv->items[0] : sv;
			const mor_act *st2 = isc == 1 && mor_cbor_is_hash(v) ? terms_of(w, v->data) : NULL;
			if (!st2) {
				if (isc == 1 && mor_cbor_is_hash(v))
					unknown = 1;
				else
					return 0;
				continue;
			}
			const mor_cbor *sp = mor_pl(st2, 0);
			if (sp->v != mem->v || !mor_hash_eq(mor_pl_hash(st2, 23), coll))
				return 0;
			for (uint64_t j = 0; j < mem->v; j++)
				if (!mor_act_names_in(sp, mem->items[j].data))
					return 0;
		}
	}
	/* What the history it cites holds: every act of it held, else unknown. */
	if (!history_all_held(w, tips, ntips))
		return -1;
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *ob = &w->acts[i];
		if (!mor_world_is_mir(w, ob, MOR_MONEY) || ob->type != 1 || !mor_hash_eq(ob->signer, coll))
			continue;
		if (!in_history_of(w, tips, ntips, ob) || !binds_collective(ob, agr))
			continue;
		if (fork) {
			/* handed out to a side by field 6 */
			const mor_cbor *f6 = mor_pl(e, 6);
			int handed = 0;
			for (uint64_t k = 0; f6 && k < f6->v; k++)
				handed |= !memcmp(f6->items[k].items[0].data, ob->id, 32);
			if (!handed)
				return 0;
		} else {
			/* owed nothing: paid in full, or released by its creditor */
			int64_t paid = mor_money_paid_toward(w, ob->id);
			if (paid < 0) {
				unknown = 1;
				continue;
			}
			if ((uint64_t)paid < mor_pl(ob, 2)->items[1].v)
				return 0;
		}
	}
	if (!fork) {
		/* holds nothing: no stake of another agreement held names it as a holder */
		for (size_t i = 0; i < w->nacts; i++) {
			const mor_act *t = &w->acts[i];
			const mor_cbor *sk = is_agr(w, t, 0) ? mor_pl(t, 7) : NULL;
			if (!sk || line_depth(w, t, base->id) >= 0)
				continue;
			for (uint64_t k = 0; k < sk->v; k++) {
				const mor_cbor *hs = &sk->items[k].items[1];
				for (uint64_t j = 0; j < hs->v; j++)
					if (mor_cbor_is_hash(&hs->items[j].items[0]) && !memcmp(hs->items[j].items[0].data, coll, 32))
						return 0;
			}
		}
	}
	return unknown ? -1 : 1;
}

/* Does a collective's act lie after the line of a complete ending of it? An
 * act counts as made before when bound to an earlier link of its identity
 * chain than the ending's chain act, or bound to that chain act and in the
 * history the ending cites (rule 91, "The line"). */
static int after_ending(mor_world *w, const uint8_t *coll, const mor_act *x)
{
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *e = &w->acts[i];
		if (!(is_agr(w, e, 19) || is_agr(w, e, 20)) || !mor_hash_eq(mor_pl_hash(e, 1), coll) || e == x)
			continue;
		if (mor_ending_complete(w, e->id) != 1)
			continue;
		const mor_chain *c = mor_chain_get(w, coll);
		int64_t pe = mor_chain_position_of(c, mor_pl_hash(e, 2));
		const mor_act *b = x->binding ? mor_world_get(w, x->binding) : NULL;
		int64_t pb = b ? mor_chain_position_of(c, b->id) : -1;
		if (pb >= 0 && pb < pe)
			continue;
		const mor_act *tips[MAX_TIPS];
		size_t ntips;
		ending_tips(w, e, coll, tips, &ntips);
		if (pb == pe && in_history_of(w, tips, ntips, x))
			continue;
		return 1;
	}
	return 0;
}

/* ---- a deal's version in force, settled by its chain of judgment (rules 35, 47) ---- */

static int judge_lapsed(const mor_world *w, const uint8_t *judge, const uint8_t *question)
{
	for (size_t i = 0; i < w->nlapsed; i++)
		if (!memcmp(w->lapsed[i].judge, judge, 32) && !memcmp(w->lapsed[i].question, question, 32))
			return 1;
	return 0;
}

static int out_of_turn(const mor_world *w, const uint8_t *id)
{
	for (size_t i = 0; i < w->nout_of_turn; i++)
		if (!memcmp(w->out_of_turn[i], id, 32))
			return 1;
	return 0;
}

/* The links of the judge of forks' chain: the judge first, then the
 * identities the reference's chain of judgment (field 21) names for it. */
static size_t fork_judges(const mor_act *ref, const uint8_t **links, size_t max)
{
	const uint8_t *judge = mor_pl_hash(ref, 27);
	size_t n = 0;
	if (!judge)
		return 0;
	links[n++] = judge;
	const mor_cbor *chains = mor_pl(ref, 21);
	for (uint64_t i = 0; chains && i < chains->v; i++) {
		const mor_cbor *j = &chains->items[i].items[0], *next = &chains->items[i].items[1];
		if (j->items[0].v != 1 || memcmp(j->items[1].data, judge, 32))
			continue;
		for (uint64_t k = 0; k < next->v && n < max; k++)
			links[n++] = next->items[k].items[0].data;
	}
	return n;
}

int mor_deal_version_in_force(mor_world *w, const uint8_t version[32], uint8_t out[32])
{
	const mor_act *ref = terms_of(w, version);
	if (!ref)
		return -1;
	memcpy(out, version, 32);
	const mor_act *cur = ref;
	for (int d = 0; d < 64; d++) {
		const mor_act *kids[MAX_LIST];
		size_t n = complete_children(w, cur->id, kids, MAX_LIST);
		if (n == 0)
			break;
		if (n == 1) {
			cur = kids[0];
			continue;
		}
		/* A fork at cur: the reference, until a settlement counts. A settling
		 * version every party signed: a complete version naming in field 26
		 * the tips it drops, on one branch. */
		const mor_act *settled = NULL;
		for (size_t i = 0; i < w->nacts && !settled; i++) {
			const mor_act *v = &w->acts[i];
			if (is_agr(w, v, 0) && mor_pl(v, 26) && line_depth(w, v, cur->id) > 0 &&
			    complete_version(w, v, 0) == MOR_AGR_EXISTS)
				settled = v;
		}
		if (!settled) {
			/* The judge of forks, activated by a party's request, and the link of
			 * its chain whose stage is open: the first not lapsed on the question. */
			const uint8_t *links[MAX_LIST];
			size_t nl = fork_judges(cur, links, MAX_LIST);
			size_t active = 0;
			while (active < nl && judge_lapsed(w, links[active], cur->id))
				active++;
			for (size_t i = 0; i < w->nacts && active < nl; i++) {
				const mor_act *s = &w->acts[i];
				if (!is_agr(w, s, 23) || !mor_hash_eq(s->signer, links[active]) || out_of_turn(w, s->id))
					continue;
				const mor_act *rq = mor_world_get(w, mor_pl_hash(s, 0));
				int unk = 0;
				if (!rq || !is_agr(w, rq, 22) || !mor_hash_eq(mor_pl_hash(rq, 0), cur->id) ||
				    !signed_by(w, cur->id, rq->signer, &unk))
					continue;
				if (mor_status_act(w, s, 0) != MOR_ST_VALID || mor_status_act(w, rq, 0) != MOR_ST_VALID)
					continue;
				const mor_act *kept = terms_of(w, mor_pl_hash(s, 1));
				if (!kept || line_depth(w, kept, cur->id) <= 0)
					continue;
				settled = kept;
			}
		}
		if (!settled)
			break;
		cur = settled;
	}
	memcpy(out, cur->id, 32);
	return 0;
}
