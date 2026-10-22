/* Agreements, Part four: the format of every act type, and the checks the
 * texts make on terms that are decided from the act's own bytes ("A format
 * rule is one decided from the act's own bytes", Agreements, Dependencies). */
#include <string.h>

#include "formats.h"

#define FAIL(r)                  \
	do {                     \
		if (why)         \
			*why = (r); \
		return 0;        \
	} while (0)

#define MILLION 1000000u

int mor_agreements_format(const mor_world *w, const mor_act *a, const char **why);

static int is_arr(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_ARRAY;
}

static int is_map(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_MAP;
}

static int is_uint(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_UINT;
}

static int is_hash(const mor_cbor *c)
{
	return mor_cbor_is_hash(c);
}

static int hashes1(const mor_cbor *c)
{
	return mor_hash_list(c, 0);
}

static int arr_n(const mor_cbor *c, uint64_t n)
{
	return mor_f_arr(c, n);
}

static int pair_hash_any(const mor_cbor *c)
{
	return arr_n(c, 2) && is_hash(&c->items[0]);
}

/* Ascending by deterministic encoding, none twice. */
static int ascending_enc(const mor_cbor *c)
{
	for (uint64_t i = 1; i < c->v; i++) {
		const mor_cbor *x = &c->items[i - 1], *y = &c->items[i];
		size_t n = x->raw_len < y->raw_len ? x->raw_len : y->raw_len;
		int r = memcmp(x->raw, y->raw, n);
		if (r > 0 || (r == 0 && x->raw_len >= y->raw_len))
			return 0;
	}
	return 1;
}

static int has_hash(const mor_cbor *list, const uint8_t *h)
{
	return mor_act_names_in(list, h);
}

/* ---- terms pieces ---- */

typedef struct terms_ctx {
	const mor_cbor *parties;
	const mor_cbor *stakes;
	int collective; /* field 12 present */
} terms_ctx;

static int stake_index_ok(const terms_ctx *t, const mor_cbor *idx)
{
	return is_uint(idx) && t->stakes && idx->v < t->stakes->v;
}

/* rule = [0] / [1, uint] / [2, [+hash]] / [3, stake, share] / [4, [2* rule]] / [5, stake, share] */
static int f_rule(const terms_ctx *t, const mor_cbor *c, int field, int nested)
{
	if (!is_arr(c) || c->v < 1 || !is_uint(&c->items[0]))
		return 0;
	uint64_t np = t->parties ? t->parties->v : 0;
	switch (c->items[0].v) {
	case 0:
		return c->v == 1;
	case 1:
		return c->v == 2 && is_uint(&c->items[1]) && c->items[1].v >= 1 && c->items[1].v <= np;
	case 2:
		if (c->v != 2 || !hashes1(&c->items[1]))
			return 0;
		for (uint64_t i = 0; i < c->items[1].v; i++)
			if (!t->parties || !has_hash(t->parties, c->items[1].items[i].data))
				return 0;
		return 1;
	case 3:
	case 5:
		/* by share: a collective's change rules only; among holders, field 18 only */
		if (!t->collective || (field != 5 && field != 18) || (c->items[0].v == 5 && field != 18))
			return 0;
		return c->v == 3 && stake_index_ok(t, &c->items[1]) && is_uint(&c->items[2]) && c->items[2].v > 0 &&
		       c->items[2].v <= MILLION;
	case 4:
		if (!t->collective || (field != 5 && field != 18) || nested || c->v != 2 || !is_arr(&c->items[1]) ||
		    c->items[1].v < 2)
			return 0;
		for (uint64_t i = 0; i < c->items[1].v; i++)
			if (!f_rule(t, &c->items[1].items[i], field, 1))
				return 0;
		return 1;
	default:
		return 0;
	}
}

static int f_holders_rule(const mor_cbor *c)
{
	if (!is_arr(c) || c->v < 1 || !is_uint(&c->items[0]))
		return 0;
	if (c->items[0].v == 0)
		return c->v == 1;
	if (c->items[0].v == 3)
		return c->v == 2 && is_uint(&c->items[1]) && c->items[1].v > 0 && c->items[1].v <= MILLION;
	return 0;
}

/* power = [0] / [1] / [2, area] / [3, party] / [4] / [5, stake] */
static int f_power(const mor_cbor *c)
{
	if (!is_arr(c) || c->v < 1 || !is_uint(&c->items[0]))
		return 0;
	switch (c->items[0].v) {
	case 0:
	case 1:
	case 4: return c->v == 1;
	case 2:
	case 5: return c->v == 2 && is_uint(&c->items[1]);
	case 3: return c->v == 2 && is_hash(&c->items[1]);
	default: return 0;
	}
}

/* mark = [+ [ power, signers: [+ hash] ] ], powers ascending, signers ascending */
static int f_mark(const mor_cbor *c)
{
	if (!is_arr(c) || c->v == 0)
		return 0;
	for (uint64_t i = 0; i < c->v; i++) {
		const mor_cbor *e = &c->items[i];
		if (!arr_n(e, 2) || !f_power(&e->items[0]) || !mor_f_hashes_ascending(&e->items[1]))
			return 0;
		if (i) {
			const mor_cbor *pp = &c->items[i - 1].items[0], *pw = &e->items[0];
			size_t n = pp->raw_len < pw->raw_len ? pp->raw_len : pw->raw_len;
			int r = memcmp(pp->raw, pw->raw, n);
			if (r > 0 || (r == 0 && pp->raw_len >= pw->raw_len))
				return 0;
		}
	}
	return 1;
}

static int f_who(const terms_ctx *t, const mor_cbor *c)
{
	return is_hash(c) || (t->collective && mor_cbor_is_null(c));
}

/* stakes = [+ [ object: who, [+ [ holder: who, share: uint ]] ] ] */
static int f_stakes(const terms_ctx *t, const mor_cbor *c)
{
	if (!is_arr(c) || c->v == 0)
		return 0;
	for (uint64_t i = 0; i < c->v; i++) {
		const mor_cbor *s = &c->items[i];
		if (!arr_n(s, 2) || !f_who(t, &s->items[0]) || !is_arr(&s->items[1]) || s->items[1].v == 0)
			return 0;
		for (uint64_t j = 0; j < i; j++)
			if (c->items[j].items[0].raw_len == s->items[0].raw_len &&
			    !memcmp(c->items[j].items[0].raw, s->items[0].raw, s->items[0].raw_len))
				return 0; /* an object once */
		uint64_t sum = 0;
		const mor_cbor *hs = &s->items[1];
		for (uint64_t k = 0; k < hs->v; k++) {
			const mor_cbor *h = &hs->items[k];
			if (!arr_n(h, 2) || !f_who(t, &h->items[0]) || !is_uint(&h->items[1]) || h->items[1].v == 0 ||
			    h->items[1].v > MILLION)
				return 0;
			/* the collective holding a stake in itself is invalid */
			if (mor_cbor_is_null(&s->items[0]) && mor_cbor_is_null(&h->items[0]))
				return 0;
			for (uint64_t m = 0; m < k; m++)
				if (hs->items[m].items[0].raw_len == h->items[0].raw_len &&
				    !memcmp(hs->items[m].items[0].raw, h->items[0].raw, h->items[0].raw_len))
					return 0; /* holders distinct */
			sum += h->items[1].v;
		}
		if (sum != MILLION)
			return 0;
	}
	return 1;
}

/* keepers = [ [+ hash], [0] / [1, uint] / [2, [+ hash]] ] */
static int f_keepers(const mor_cbor *c)
{
	if (!arr_n(c, 2) || !hashes1(&c->items[0]))
		return 0;
	const mor_cbor *ops = &c->items[0], *r = &c->items[1];
	for (uint64_t i = 0; i < ops->v; i++)
		for (uint64_t j = i + 1; j < ops->v; j++)
			if (!memcmp(ops->items[i].data, ops->items[j].data, 32))
				return 0;
	if (!is_arr(r) || r->v < 1 || !is_uint(&r->items[0]))
		return 0;
	switch (r->items[0].v) {
	case 0: return r->v == 1;
	case 1: return r->v == 2 && is_uint(&r->items[1]) && r->items[1].v >= 1 && r->items[1].v <= ops->v;
	case 2:
		if (r->v != 2 || !hashes1(&r->items[1]))
			return 0;
		for (uint64_t i = 0; i < r->items[1].v; i++) {
			if (!has_hash(ops, r->items[1].items[i].data))
				return 0;
			for (uint64_t j = i + 1; j < r->items[1].v; j++)
				if (!memcmp(r->items[1].items[i].data, r->items[1].items[j].data, 32))
					return 0;
		}
		return 1;
	default: return 0;
	}
}

/* kind = [0, layer] / [1, spec, type] */
static int f_kind(const mor_world *w, const mor_cbor *c)
{
	if (!is_arr(c) || c->v < 2 || !is_uint(&c->items[0]))
		return 0;
	if (c->items[0].v == 0)
		return c->v == 2 && is_uint(&c->items[1]) && c->items[1].v <= 4;
	if (c->items[0].v == 1) {
		if (c->v != 3 || !is_hash(&c->items[1]) || !is_uint(&c->items[2]))
			return 0;
		/* never a genesis or rotation (Identity's chain acts), nor a record */
		int m = mor_world_mir_of(w, c->items[1].data);
		if (m == MOR_IDENTITY && c->items[2].v <= 1)
			return 0;
		if (m == MOR_AGREEMENTS && c->items[2].v == 17)
			return 0;
		return 1;
	}
	return 0;
}

static int kinds(const mor_world *w, const mor_cbor *c)
{
	if (!is_arr(c) || c->v == 0)
		return 0;
	for (uint64_t i = 0; i < c->v; i++)
		if (!f_kind(w, &c->items[i]))
			return 0;
	return 1;
}

/* field-ref = [0, field] / [1, task] */
static int f_field_ref(const mor_cbor *c)
{
	return arr_n(c, 2) && is_uint(&c->items[0]) && c->items[0].v <= 1 && is_uint(&c->items[1]);
}

/* area = { 0 tstr, 1 [* hash], 2 uint, ? 3 [+ kind], ? 4 [+ field-ref], 5 uint, ? 6 [hash, any] } */
static int f_area(const mor_world *w, const terms_ctx *t, const mor_cbor *c, int clone)
{
	if (!mor_cbor_map_closed(c, 0x7fu))
		return 0;
	const mor_cbor *name = mor_cbor_map_get(c, 0), *holders = mor_cbor_map_get(c, 1),
		       *num = mor_cbor_map_get(c, 2), *k = mor_cbor_map_get(c, 3), *fr = mor_cbor_map_get(c, 4),
		       *id = mor_cbor_map_get(c, 5), *lim = mor_cbor_map_get(c, 6);
	if (!mor_f_tstr(name) || !mor_hash_list(holders, 1) || !is_uint(num) || !is_uint(id))
		return 0;
	if (!clone && holders->v == 0)
		return 0;
	for (uint64_t i = 0; i < holders->v; i++) {
		if (!t->parties || !has_hash(t->parties, holders->items[i].data))
			return 0;
		for (uint64_t j = i + 1; j < holders->v; j++)
			if (!memcmp(holders->items[i].data, holders->items[j].data, 32))
				return 0;
	}
	if (num->v < 1 || (!clone && num->v > holders->v))
		return 0;
	if (!k && !fr)
		return 0;
	if (k && !kinds(w, k))
		return 0;
	if (fr) {
		if (!is_arr(fr) || fr->v == 0)
			return 0;
		for (uint64_t i = 0; i < fr->v; i++)
			if (!f_field_ref(&fr->items[i]))
				return 0;
	}
	if (lim && !pair_hash_any(lim))
		return 0;
	return 1;
}

/* key-grammar = { 0 holding, 1 holding, ? 3 recovery } */
static int f_holding(const terms_ctx *t, const mor_cbor *c)
{
	if (!is_arr(c) || c->v < 2 || !is_uint(&c->items[0]))
		return 0;
	switch (c->items[0].v) {
	case 0: return c->v == 2 && is_hash(&c->items[1]);
	case 1:
		if (c->v != 3 || !is_uint(&c->items[1]) || !hashes1(&c->items[2]))
			return 0;
		for (uint64_t i = 0; i < c->items[2].v; i++)
			if (!t->parties || !has_hash(t->parties, c->items[2].items[i].data))
				return 0;
		return c->items[1].v >= 1 && c->items[1].v <= c->items[2].v;
	case 2: return c->v == 3 && is_hash(&c->items[1]) && is_hash(&c->items[2]);
	default: return 0;
	}
}

static int f_key_grammar(const terms_ctx *t, const mor_cbor *c)
{
	if (!mor_cbor_map_closed(c, 0xbu))
		return 0;
	const mor_cbor *h0 = mor_cbor_map_get(c, 0), *h1 = mor_cbor_map_get(c, 1), *rec = mor_cbor_map_get(c, 3);
	if (!f_holding(t, h0) || !f_holding(t, h1))
		return 0;
	if (rec) {
		if (!is_arr(rec) || rec->v < 2 || !is_uint(&rec->items[0]))
			return 0;
		if (rec->items[0].v == 0 && !(rec->v == 3 && is_hash(&rec->items[1]) && is_hash(&rec->items[2])))
			return 0;
		if (rec->items[0].v == 1 && !(rec->v == 2 && is_hash(&rec->items[1])))
			return 0;
		if (rec->items[0].v > 1)
			return 0;
	}
	/* A way to rotate that needs less than every member: a threshold below the
	 * member count, or a named recovery path (rule 59). */
	if (!rec && h1->items[0].v == 1 && h1->items[1].v >= h1->items[2].v)
		return 0;
	return 1;
}

/* succession-plan = { 0 hash, ? 1 [+ [hash, uint]], ? 2 [+ hash], ? 3 uint } */
static int f_plan(const mor_cbor *c)
{
	if (!mor_cbor_map_closed(c, 0xfu) || !is_hash(mor_cbor_map_get(c, 0)))
		return 0;
	const mor_cbor *s = mor_cbor_map_get(c, 1), *seat = mor_cbor_map_get(c, 2), *e = mor_cbor_map_get(c, 3);
	if (s) {
		if (!is_arr(s) || s->v == 0)
			return 0;
		for (uint64_t i = 0; i < s->v; i++)
			if (!arr_n(&s->items[i], 2) || !is_hash(&s->items[i].items[0]) || !is_uint(&s->items[i].items[1]))
				return 0;
	}
	if (seat && !hashes1(seat))
		return 0;
	if (e && (!is_uint(e) || e->v > 1))
		return 0;
	return 1;
}

/* abandonment = { 0 [0, hash] / [1, uint], 1 [+ uint] ascending, ? 3 [hash, any] } */
static int f_abandonment(const terms_ctx *t, const mor_cbor *c)
{
	if (!mor_cbor_map_closed(c, 0xbu))
		return 0;
	const mor_cbor *au = mor_cbor_map_get(c, 0), *out = mor_cbor_map_get(c, 1), *ap = mor_cbor_map_get(c, 3);
	if (!arr_n(au, 2) || !is_uint(&au->items[0]))
		return 0;
	if (au->items[0].v == 0) {
		if (!is_hash(&au->items[1]))
			return 0;
	} else if (au->items[0].v == 1) {
		/* a threshold of the other parties: in a collective only */
		if (!t->collective || !is_uint(&au->items[1]))
			return 0;
	} else {
		return 0;
	}
	if (!is_arr(out) || out->v == 0)
		return 0;
	for (uint64_t i = 0; i < out->v; i++) {
		if (!is_uint(&out->items[i]) || out->items[i].v > 4)
			return 0;
		if (i && out->items[i].v <= out->items[i - 1].v)
			return 0;
	}
	if (ap && !pair_hash_any(ap))
		return 0;
	return 1;
}

/* split-plan = { 0 [+ share-rule], 1 hash, ? 3 unfilled, ? 4 [+ uint], ? 5 uint, ? 6 amount / uint, ? 7 any, ? 8 [+ metric] } */
static int f_split_plan(const terms_ctx *t, const mor_cbor *c)
{
	if (!mor_cbor_map_closed(c, 0x1fbu))
		return 0;
	const mor_cbor *sr = mor_cbor_map_get(c, 0), *f;
	if (!is_arr(sr) || sr->v == 0 || !is_hash(mor_cbor_map_get(c, 1)))
		return 0;
	for (uint64_t i = 0; i < sr->v; i++) {
		const mor_cbor *s = &sr->items[i];
		if (!arr_n(s, 3) || !is_uint(&s->items[0]) || !is_uint(&s->items[2]))
			return 0;
		switch (s->items[0].v) {
		case 0:
			if (!stake_index_ok(t, &s->items[1]))
				return 0;
			break;
		case 1:
			if (!mor_f_tstr(&s->items[1]))
				return 0;
			break;
		case 2:
			if (!is_hash(&s->items[1]) && !(t->collective && mor_cbor_is_null(&s->items[1])))
				return 0;
			break;
		case 3:
			if (!is_uint(&s->items[1]))
				return 0;
			break;
		default: return 0;
		}
	}
	if ((f = mor_cbor_map_get(c, 3))) {
		if (!is_arr(f) || f->v < 1 || !is_uint(&f->items[0]) || f->items[0].v > 2)
			return 0;
		if (f->items[0].v == 2 ? !(f->v == 2 && is_hash(&f->items[1])) : f->v != 1)
			return 0;
	}
	if ((f = mor_cbor_map_get(c, 4))) {
		if (!is_arr(f) || f->v == 0)
			return 0;
		for (uint64_t i = 0; i < f->v; i++)
			if (!is_uint(&f->items[i]) || f->items[i].v >= sr->v)
				return 0;
	}
	if ((f = mor_cbor_map_get(c, 5)) && (!is_uint(f) || f->v > 1))
		return 0;
	if ((f = mor_cbor_map_get(c, 6)) && !is_uint(f) && !mor_f_amount(f))
		return 0;
	if ((f = mor_cbor_map_get(c, 8))) {
		if (!is_arr(f) || f->v == 0)
			return 0;
		for (uint64_t i = 0; i < f->v; i++) {
			const mor_cbor *m = &f->items[i];
			if (!is_arr(m) || m->v < 2 || m->v > 3 || !is_hash(&m->items[0]) || !is_hash(&m->items[1]))
				return 0;
		}
	}
	return 1;
}

/* fees = [+ [ module: hash, part: uint, ? scope ] ] */
static int f_fees(const mor_cbor *c)
{
	if (!is_arr(c) || c->v == 0)
		return 0;
	for (uint64_t i = 0; i < c->v; i++) {
		const mor_cbor *f = &c->items[i];
		if (!is_arr(f) || f->v < 2 || f->v > 3 || !is_hash(&f->items[0]) || !is_uint(&f->items[1]))
			return 0;
		if (f->v == 3) {
			const mor_cbor *s = &f->items[2];
			if (!arr_n(s, 2) || !is_uint(&s->items[0]) || s->items[0].v > 1 || !is_hash(&s->items[1]))
				return 0;
		}
	}
	return 1;
}

/* judge = [0, task] / [1, identity] ; chain = [ judge, [+ [hash, period]] ] */
static int f_chain(const mor_cbor *c)
{
	if (!arr_n(c, 2))
		return 0;
	const mor_cbor *j = &c->items[0], *next = &c->items[1];
	if (!arr_n(j, 2) || !is_uint(&j->items[0]))
		return 0;
	if (j->items[0].v == 0 ? !is_uint(&j->items[1]) : j->items[0].v == 1 ? !is_hash(&j->items[1]) : 1)
		return 0;
	if (!is_arr(next) || next->v == 0)
		return 0;
	for (uint64_t i = 0; i < next->v; i++) {
		const mor_cbor *l = &next->items[i];
		if (!arr_n(l, 2) || !is_hash(&l->items[0]) || !is_uint(&l->items[1]) || l->items[1].v == 0)
			return 0;
		for (uint64_t k = 0; k < i; k++)
			if (!memcmp(next->items[k].items[0].data, l->items[0].data, 32))
				return 0;
		if (j->items[0].v == 1 && !memcmp(l->items[0].data, j->items[1].data, 32))
			return 0;
	}
	return 1;
}

static int f_refund_terms(const mor_cbor *c)
{
	if (!mor_cbor_map_closed(c, 0x1u))
		return 0;
	const mor_cbor *p = mor_cbor_map_get(c, 0);
	return arr_n(p, 2) && is_uint(&p->items[0]) && p->items[0].v == 0;
}

static int terms_format(const mor_world *w, const mor_act *a, const char **why)
{
	const mor_cbor *p = a->payload, *f;
	/* keys 0-9, 11-24, 26-32: 10 and 25 are withdrawn */
	uint64_t allowed = 0;
	for (unsigned k = 0; k <= 32; k++)
		if (k != 10 && k != 25)
			allowed |= UINT64_C(1) << k;
	if (!mor_cbor_map_closed(p, allowed))
		FAIL("terms: a key the format does not define");
	terms_ctx t = { mor_pl(a, 0), mor_pl(a, 7), mor_pl(a, 12) != NULL };
	int clone = mor_pl(a, 11) != NULL;
	if (!hashes1(t.parties))
		FAIL("terms: parties");
	if (!mor_f_tstr(mor_pl(a, 1)))
		FAIL("terms: words");
	f = mor_pl(a, 2);
	if (!is_map(f))
		FAIL("terms: cMIPs");
	for (uint64_t i = 0; i < f->v; i++)
		if (!is_uint(&f->items[2 * i]) || !is_hash(&f->items[2 * i + 1]))
			FAIL("terms: cMIPs");
	if (t.stakes && !f_stakes(&t, t.stakes))
		FAIL("terms: stakes");
	/* field 4: founding terms carry the rule [0]; a clone carries its mark */
	f = mor_pl(a, 4);
	if (clone) {
		if (!f_mark(f))
			FAIL("terms: a clone's field 4 is not a mark");
	} else if (!(arr_n(f, 1) && is_uint(&f->items[0]) && f->items[0].v == 0)) {
		FAIL("terms: founding terms' field 4 is not [0]");
	}
	if (!f_rule(&t, mor_pl(a, 5), 5, 0))
		FAIL("terms: clone rule");
	if (!t.collective) {
		/* a deal: every party is its clone rule; no collective-only fields */
		f = mor_pl(a, 5);
		if (!(f->v == 1 && f->items[0].v == 0))
			FAIL("terms: a deal whose field 5 is not [0]");
		if (mor_pl(a, 18) || mor_pl(a, 19) || mor_pl(a, 20) || mor_pl(a, 22) || mor_pl(a, 23) || mor_pl(a, 30) ||
		    mor_pl(a, 32))
			FAIL("terms: a deal carrying a collective's field");
	} else {
		if (mor_pl(a, 26) || mor_pl(a, 27) || mor_pl(a, 31))
			FAIL("terms: a collective carrying a deal's field");
		if (!f_key_grammar(&t, mor_pl(a, 12)))
			FAIL("terms: key grammar");
	}
	if ((f = mor_pl(a, 15)) && !hashes1(f))
		FAIL("terms: extensions");
	if ((f = mor_pl(a, 16))) {
		if (!is_arr(f) || f->v == 0)
			FAIL("terms: succession plans");
		for (uint64_t i = 0; i < f->v; i++)
			if (!f_plan(&f->items[i]))
				FAIL("terms: succession plan");
	}
	if ((f = mor_pl(a, 3)) && !f_keepers(f))
		FAIL("terms: keepers");
	if ((f = mor_pl(a, 18)) && !f_rule(&t, f, 18, 0))
		FAIL("terms: constitutional change rule");
	if ((f = mor_pl(a, 19))) {
		if (!is_arr(f) || f->v == 0)
			FAIL("terms: areas");
		for (uint64_t i = 0; i < f->v; i++) {
			if (!f_area(w, &t, &f->items[i], clone))
				FAIL("terms: area");
			for (uint64_t j = 0; j < i; j++)
				if (mor_cbor_map_get(&f->items[j], 5)->v == mor_cbor_map_get(&f->items[i], 5)->v)
					FAIL("terms: two areas with one id");
		}
	}
	if ((f = mor_pl(a, 20))) {
		const mor_cbor *areas = mor_pl(a, 19);
		if (!is_map(f) || f->v == 0 || !areas)
			FAIL("terms: area words");
		for (uint64_t i = 0; i < f->v; i++) {
			const mor_cbor *k = &f->items[2 * i];
			int found = 0;
			if (!is_uint(k) || !mor_f_tstr(&f->items[2 * i + 1]))
				FAIL("terms: area words");
			for (uint64_t j = 0; j < areas->v; j++)
				if (mor_cbor_map_get(&areas->items[j], 5)->v == k->v)
					found = 1;
			if (!found)
				FAIL("terms: words for an area that does not exist");
		}
	}
	if ((f = mor_pl(a, 6)) && !pair_hash_any(f))
		FAIL("terms: time reference");
	{
		/* one time reference: field 6 and task 10 name the same specification */
		const mor_cbor *tr = mor_pl(a, 6), *t10 = mor_cbor_map_get(mor_pl(a, 2), 10);
		if (tr && t10 && memcmp(tr->items[0].data, t10->data, 32))
			FAIL("terms: field 6 and task 10 name different time references");
	}
	if ((f = mor_pl(a, 8)) && !f_split_plan(&t, f))
		FAIL("terms: split plan");
	if ((f = mor_pl(a, 28)) && !f_fees(f))
		FAIL("terms: fees");
	if ((f = mor_pl(a, 9)) && !f_abandonment(&t, f))
		FAIL("terms: abandonment clause");
	if ((f = mor_pl(a, 11)) && !is_hash(f))
		FAIL("terms: parent");
	if ((f = mor_pl(a, 13)) && !hashes1(f))
		FAIL("terms: arbitrators");
	if ((f = mor_pl(a, 14)) && !is_hash(f) && !hashes1(f))
		FAIL("terms: split service");
	if ((f = mor_pl(a, 17)) && !f_refund_terms(f))
		FAIL("terms: refund terms");
	if ((f = mor_pl(a, 21))) {
		if (!is_arr(f) || f->v == 0)
			FAIL("terms: chain of judgment");
		for (uint64_t i = 0; i < f->v; i++) {
			if (!f_chain(&f->items[i]))
				FAIL("terms: chain of judgment");
			if (i) {
				const mor_cbor *x = &f->items[i - 1].items[0], *y = &f->items[i].items[0];
				size_t n = x->raw_len < y->raw_len ? x->raw_len : y->raw_len;
				int r = memcmp(x->raw, y->raw, n);
				if (r > 0 || (r == 0 && x->raw_len >= y->raw_len))
					FAIL("terms: chain links out of order");
			}
		}
	}
	if ((f = mor_pl(a, 22))) {
		if (!hashes1(f))
			FAIL("terms: departed members");
		for (uint64_t i = 0; i < f->v; i++)
			if (has_hash(t.parties, f->items[i].data))
				FAIL("terms: a departed member who is a party");
	}
	if ((f = mor_pl(a, 23)) && (!is_hash(f) || clone))
		FAIL("terms: forked from");
	if ((f = mor_pl(a, 24))) {
		if (!f_holders_rule(f) && !(arr_n(f, 2) && is_uint(&f->items[0]) && f->items[0].v == 2 && hashes1(&f->items[1])))
			FAIL("terms: release rule");
	}
	if ((f = mor_pl(a, 26)) && (!clone || !mor_f_hashes_ascending(f)))
		FAIL("terms: settles");
	if ((f = mor_pl(a, 26))) {
		for (uint64_t i = 0; i < f->v; i++)
			if (!mor_objects_has(a, f->items[i].data, f->items[i].data))
				FAIL("terms: a settled tip not cited as chain and predecessor");
	}
	if ((f = mor_pl(a, 27)) && (!is_hash(f) || !mor_pl(a, 13) || !has_hash(mor_pl(a, 13), f->data)))
		FAIL("terms: judge of forks");
	if ((f = mor_pl(a, 29))) {
		if (!mor_pl(a, 14) || !mor_cbor_map_closed(f, 0x3u) || !is_arr(mor_cbor_map_get(f, 0)) ||
		    mor_cbor_map_get(f, 0)->v == 0)
			FAIL("terms: backups");
	}
	if ((f = mor_pl(a, 30))) {
		if (!is_arr(f) || f->v == 0)
			FAIL("terms: holders' rules");
		for (uint64_t i = 0; i < f->v; i++) {
			const mor_cbor *e = &f->items[i];
			if (!is_arr(e) || e->v < 2 || e->v > 3 || !stake_index_ok(&t, &e->items[0]) ||
			    !f_holders_rule(&e->items[1]) || (e->v == 3 && !mor_cbor_is_true(&e->items[2])))
				FAIL("terms: holders' rule");
			if (i && e->items[0].v <= f->items[i - 1].items[0].v)
				FAIL("terms: holders' rules out of order");
		}
	}
	if ((f = mor_pl(a, 31)) && (!clone || !mor_f_hashes_ascending(f)))
		FAIL("terms: transfers counted");
	if ((f = mor_pl(a, 32)) && (!clone || !(is_hash(f) || (arr_n(f, 1) && is_uint(&f->items[0]) && f->items[0].v == 0))))
		FAIL("terms: issues under");
	/* A judge never handles what it judges: a specification named for a
	 * judicial task (field 2's 9, 10, 11; field 6, a naming for 10; the
	 * abandonment clause's key 3, for 14) is named for no other task and as
	 * no extension. */
	{
		const mor_cbor *m = mor_pl(a, 2), *ext = mor_pl(a, 15), *ab = mor_pl(a, 9);
		const uint8_t *js[8];
		uint64_t jt[8];
		size_t nj = 0;
		for (uint64_t i = 0; i < m->v && nj < 6; i++) {
			uint64_t task = m->items[2 * i].v;
			if (task == 9 || task == 10 || task == 11) {
				js[nj] = m->items[2 * i + 1].data;
				jt[nj++] = task;
			}
		}
		if ((f = mor_pl(a, 6))) {
			js[nj] = f->items[0].data;
			jt[nj++] = 10;
		}
		if (ab && (f = mor_cbor_map_get(ab, 3))) {
			js[nj] = f->items[0].data;
			jt[nj++] = 14;
		}
		for (size_t j = 0; j < nj; j++) {
			for (size_t k = 0; k < nj; k++)
				if (!memcmp(js[j], js[k], 32) && jt[j] != jt[k])
					FAIL("terms: one judge named for two tasks");
			for (uint64_t i = 0; i < m->v; i++)
				if (!memcmp(m->items[2 * i + 1].data, js[j], 32) && m->items[2 * i].v != jt[j])
					FAIL("terms: a judge named for another task");
			if (ext && has_hash(ext, js[j]))
				FAIL("terms: a judge named as an extension");
		}
	}
	return 1;
}

/* sold = [0, hash] / [2, [hash, any]] / [3, hash] */
static int f_sold(const mor_cbor *c)
{
	if (!arr_n(c, 2) || !is_uint(&c->items[0]))
		return 0;
	switch (c->items[0].v) {
	case 0:
	case 3: return is_hash(&c->items[1]);
	case 2: return pair_hash_any(&c->items[1]);
	default: return 0;
	}
}

static int f_payout(const mor_cbor *c)
{
	if (!mor_cbor_map_closed(c, 0xffu) || !is_hash(mor_cbor_map_get(c, 0)) || !is_uint(mor_cbor_map_get(c, 1)))
		return 0;
	const mor_cbor *k2 = mor_cbor_map_get(c, 2), *k3 = mor_cbor_map_get(c, 3), *k4 = mor_cbor_map_get(c, 4),
		       *k5 = mor_cbor_map_get(c, 5), *k6 = mor_cbor_map_get(c, 6), *k7 = mor_cbor_map_get(c, 7);
	if ((k2 && !is_uint(k2)) || (k3 && !mor_f_tstr(k3)) || (k4 && !is_hash(k4)) || (k5 && !is_hash(k5)) ||
	    (k6 && !is_uint(k6)) || (k7 && !is_uint(k7)))
		return 0;
	if (k4 && !k3)
		return 0;
	if (k5 && (k2 || k3))
		return 0;
	if (k7 && (k2 || k3 || k5))
		return 0;
	return 1;
}

int mor_agreements_format(const mor_world *w, const mor_act *a, const char **why)
{
	const mor_cbor *p = a->payload, *f;
	switch (a->type) {
	case 0:
		return terms_format(w, a, why);
	case 1: /* signature: { 0 hash, ? 1 signing-key }, objects [signed, signed] */
		if (!mor_cbor_map_closed(p, 0x3u) || !is_hash(mor_pl(a, 0)) ||
		    ((f = mor_pl(a, 1)) && !mor_f_signing_key(f)))
			FAIL("signature payload");
		if (!mor_objects_has(a, mor_pl(a, 0)->data, mor_pl(a, 0)->data))
			FAIL("signature does not name the act signed as chain and predecessor");
		return 1;
	case 2: /* keeper record: { 0 [act, signer] }, no objects */
		f = mor_pl(a, 0);
		if (!mor_cbor_map_closed(p, 0x1u) || !arr_n(f, 2) || !is_hash(&f->items[0]) || !is_hash(&f->items[1]))
			FAIL("keeper record payload");
		if (a->objects)
			FAIL("keeper record names a chain");
		return 1;
	case 3: /* work claim */
		if (!mor_cbor_map_closed(p, 0x7u) || !is_hash(mor_pl(a, 0)) || !mor_f_hashes_ascending(mor_pl(a, 1)) ||
		    ((f = mor_pl(a, 2)) && !pair_hash_any(f)))
			FAIL("work claim payload");
		if (!a->signer || !has_hash(mor_pl(a, 1), a->signer))
			FAIL("work claim: its signer is not among the creators");
		return 1;
	case 4: /* stake transfer */
		if (!mor_cbor_map_closed(p, 0x1fu) || !is_hash(mor_pl(a, 0)) || !is_uint(mor_pl(a, 1)) ||
		    !is_hash(mor_pl(a, 2)) || !is_uint(mor_pl(a, 3)) || mor_pl(a, 3)->v == 0 ||
		    mor_pl(a, 3)->v > MILLION || ((f = mor_pl(a, 4)) && !hashes1(f)))
			FAIL("stake transfer payload");
		if (!mor_objects_has(a, mor_pl(a, 0)->data, mor_pl(a, 0)->data))
			FAIL("stake transfer does not name its agreement");
		return 1;
	case 5: /* public domain release */
		if (!mor_cbor_map_closed(p, 0x1fu) || !is_hash(mor_pl(a, 0)))
			FAIL("release payload");
		f = mor_pl(a, 1);
		if (!is_arr(f) || f->v == 0)
			FAIL("release stakes");
		for (uint64_t i = 0; i < f->v; i++)
			if (!arr_n(&f->items[i], 2) || !is_hash(&f->items[i].items[0]) || !is_uint(&f->items[i].items[1]))
				FAIL("release stakes");
		if ((f = mor_pl(a, 2)) && !hashes1(f))
			FAIL("release claims");
		if ((f = mor_pl(a, 3))) {
			if (!is_arr(f) || f->v == 0)
				FAIL("release keys");
			for (uint64_t i = 0; i < f->v; i++)
				if (!arr_n(&f->items[i], 2) || !is_hash(&f->items[i].items[0]) ||
				    !(f->items[i].items[1].type == MOR_CBOR_BYTES && f->items[i].items[1].v == 32))
					FAIL("release keys");
		}
		if ((f = mor_pl(a, 4)) && !(arr_n(f, 2) && is_hash(&f->items[1])))
			FAIL("release timing");
		if (!mor_pl(a, 3) && !mor_pl(a, 4))
			FAIL("release carries neither keys nor a timed release");
		return 1;
	case 6: /* standing offer */
		if (!mor_cbor_map_closed(p, 0xffu) || !is_arr(mor_pl(a, 1)) || !mor_f_amount(mor_pl(a, 2)))
			FAIL("offer payload");
		f = mor_pl(a, 1);
		for (uint64_t i = 0; i < f->v; i++)
			if (!f_sold(&f->items[i]))
				FAIL("offer: what it sells");
		if (!ascending_enc(f))
			FAIL("offer: what it sells is not ascending");
		if ((f = mor_pl(a, 0)) && !is_hash(f))
			FAIL("offer: under");
		if ((f = mor_pl(a, 3)) && !is_hash(f) && !(is_uint(f) && f->v == 0))
			FAIL("offer: payee");
		if (mor_pl(a, 0) && !mor_pl(a, 3))
			FAIL("offer: field 3 required with field 0");
		if ((f = mor_pl(a, 4)) && !mor_f_tstr(f))
			FAIL("offer: words");
		if ((f = mor_pl(a, 6)) && !pair_hash_any(f))
			FAIL("offer: time reference");
		if ((f = mor_pl(a, 7)) && !f_refund_terms(f))
			FAIL("offer: refund terms");
		f = mor_pl(a, 1);
		for (uint64_t i = 0; i < f->v; i++)
			if (f->items[i].items[0].v == 3 && !mor_pl(a, 4))
				FAIL("offer naming a vow without words");
		return 1;
	case 8: /* split */
		if (!mor_cbor_map_closed(p, 0x1ffu) || !is_hash(mor_pl(a, 0)) || !is_arr(mor_pl(a, 1)) ||
		    mor_pl(a, 1)->v == 0 || !is_hash(mor_pl(a, 2)) || !is_hash(mor_pl(a, 3)))
			FAIL("split payload");
		f = mor_pl(a, 1);
		for (uint64_t i = 0; i < f->v; i++)
			if (!f_payout(&f->items[i]))
				FAIL("split payout");
		if ((f = mor_pl(a, 5)) && (!is_uint(f) || f->v == 0))
			FAIL("split number");
		if ((f = mor_pl(a, 6)) && !hashes1(f))
			FAIL("split modules");
		if ((f = mor_pl(a, 7)) && !hashes1(f))
			FAIL("split metric records");
		return 1;
	case 9: { /* grant */
		if (!mor_cbor_map_closed(p, 0x3ffu) || !is_hash(mor_pl(a, 0)) || !is_uint(mor_pl(a, 1)) ||
		    mor_pl(a, 1)->v > 2 || !mor_f_signing_key(mor_pl(a, 9)))
			FAIL("grant payload");
		const mor_cbor *f2 = mor_pl(a, 2), *f5 = mor_pl(a, 5), *f6 = mor_pl(a, 6), *f7 = mor_pl(a, 7),
			       *f8 = mor_pl(a, 8);
		if (f2 && !mor_cbor_is_null(f2) && !hashes1(f2))
			FAIL("grant agreements");
		if (f2 && mor_cbor_is_null(f2) && (mor_pl(a, 1)->v != 1 || f5 || f7 || f8))
			FAIL("grant: null agreements only with scope 1");
		if ((f = mor_pl(a, 4)) && !is_hash(f))
			FAIL("grant limits cMIP");
		if (f5 && !is_uint(f5))
			FAIL("grant area");
		if (f6 && !kinds(w, f6))
			FAIL("grant kinds");
		if (f5 && !f6)
			FAIL("grant: field 6 required with field 5");
		if (f7 && (!is_hash(f7) || !f5))
			FAIL("grant reinstatement");
		if (f8 && (!mor_cbor_is_null(f8) || f5 || f7))
			FAIL("grant: field 8");
		return 1;
	}
	case 10: /* revocation */
	case 12: /* liveness */
	case 22: /* settlement request */
	case 28: /* revocation of a proxy */
		if (!mor_cbor_map_closed(p, 0x1u) || !is_hash(mor_pl(a, 0)))
			FAIL("payload { 0 hash }");
		if ((a->type == 12 || a->type == 22) && !mor_objects_has(a, mor_pl(a, 0)->data, mor_pl(a, 0)->data))
			FAIL("does not name its agreement as chain and predecessor");
		if (a->type == 28 && !mor_objects_has(a, mor_pl(a, 0)->data, mor_pl(a, 0)->data))
			FAIL("does not name the proxy as chain and predecessor");
		return 1;
	case 13: /* abandonment declaration */
		if (!mor_cbor_map_closed(p, 0xfu) || !is_hash(mor_pl(a, 0)) || !is_hash(mor_pl(a, 1)) ||
		    !is_hash(mor_pl(a, 2)) || !is_arr(mor_pl(a, 3)))
			FAIL("declaration payload");
		f = mor_pl(a, 3);
		for (uint64_t i = 0; i < f->v; i++) {
			if (!is_uint(&f->items[i]) || f->items[i].v > 4)
				FAIL("declaration outcomes");
			if (i && f->items[i].v <= f->items[i - 1].v)
				FAIL("declaration outcomes not ascending");
		}
		if (!mor_objects_has(a, mor_pl(a, 0)->data, mor_pl(a, 0)->data))
			FAIL("declaration does not name its agreement");
		return 1;
	case 14: /* contest */
		if (!mor_cbor_map_closed(p, 0x3u) || !is_hash(mor_pl(a, 0)))
			FAIL("contest payload");
		if ((f = mor_pl(a, 1)) && !mor_f_tstr(f) && !pair_hash_any(f))
			FAIL("contest promise");
		if (!a->objects)
			FAIL("contest names nothing as chain and predecessor");
		return 1;
	case 16: /* resignation */
		if (!mor_cbor_map_closed(p, 0x7u) || !is_hash(mor_pl(a, 0)) || ((f = mor_pl(a, 1)) && !is_uint(f)) ||
		    ((f = mor_pl(a, 2)) && !mor_f_hashes_ascending(f)))
			FAIL("resignation payload");
		if (!mor_objects_has(a, mor_pl(a, 0)->data, mor_pl(a, 0)->data))
			FAIL("resignation does not name its agreement");
		return 1;
	case 17: { /* record */
		if (!mor_cbor_map_closed(p, 0xfu) || !is_arr(mor_pl(a, 1)))
			FAIL("record payload");
		f = mor_pl(a, 1);
		for (uint64_t i = 0; i < f->v; i++)
			if (!mor_f_kept_tip(&f->items[i]))
				FAIL("record tips");
		const mor_cbor *c0 = mor_pl(a, 0), *c2 = mor_pl(a, 2), *c3 = mor_pl(a, 3);
		if ((c0 && !is_hash(c0)) || (c2 && !hashes1(c2)) || (c3 && !hashes1(c3)))
			FAIL("record fields");
		if (!c0 != !c2)
			FAIL("record: field 2 present exactly when field 0 is");
		if (!c0 && !c3)
			FAIL("record: neither a clone nor registrations");
		return 1;
	}
	case 18: /* negotiation message */
		if (!mor_cbor_map_closed(p, 0x3u) || !mor_f_tstr(mor_pl(a, 0)) || ((f = mor_pl(a, 1)) && !is_hash(f)))
			FAIL("negotiation payload");
		return 1;
	case 19: { /* fork */
		if (!mor_cbor_map_closed(p, 0x7fu) || !is_hash(mor_pl(a, 0)) || !is_hash(mor_pl(a, 1)) ||
		    !is_hash(mor_pl(a, 2)) || !is_arr(mor_pl(a, 3)) || !is_arr(mor_pl(a, 4)) || mor_pl(a, 4)->v < 2)
			FAIL("fork payload");
		f = mor_pl(a, 3);
		for (uint64_t i = 0; i < f->v; i++)
			if (!mor_f_kept_tip(&f->items[i]))
				FAIL("fork tips");
		f = mor_pl(a, 4);
		for (uint64_t i = 0; i < f->v; i++) {
			const mor_cbor *s = &f->items[i];
			if (!arr_n(s, 2) || !is_hash(&s->items[0]) || !hashes1(&s->items[1]) ||
			    !memcmp(s->items[0].data, mor_pl(a, 1)->data, 32))
				FAIL("fork side");
			for (uint64_t j = 0; j < i; j++)
				if (!memcmp(f->items[j].items[0].data, s->items[0].data, 32))
					FAIL("fork: a successor twice");
		}
		uint64_t nsides = f->v;
		if ((f = mor_pl(a, 5))) {
			if (!is_arr(f) || f->v == 0)
				FAIL("fork shares");
			for (uint64_t i = 0; i < f->v; i++) {
				const mor_cbor *s = &f->items[i];
				if (!arr_n(s, 3) || !is_hash(&s->items[0]) || !is_uint(&s->items[1]) || !is_arr(&s->items[2]) ||
				    s->items[2].v != nsides)
					FAIL("fork share");
				uint64_t sum = 0;
				for (uint64_t k = 0; k < s->items[2].v; k++) {
					if (!is_uint(&s->items[2].items[k]))
						FAIL("fork share");
					sum += s->items[2].items[k].v;
				}
				if (sum != MILLION)
					FAIL("fork shares do not sum to 1,000,000");
			}
		}
		if ((f = mor_pl(a, 6))) {
			if (!is_arr(f) || f->v == 0)
				FAIL("fork obligations");
			for (uint64_t i = 0; i < f->v; i++) {
				const mor_cbor *o = &f->items[i];
				if (!arr_n(o, 2) || !is_hash(&o->items[0]) || !is_arr(&o->items[1]) || o->items[1].v == 0)
					FAIL("fork obligation");
				for (uint64_t k = 0; k < o->items[1].v; k++) {
					const mor_cbor *sv = &o->items[1].items[k];
					if (!is_uint(sv) || sv->v >= nsides || (k && sv->v <= o->items[1].items[k - 1].v))
						FAIL("fork obligation sides");
				}
				for (uint64_t j = 0; j < i; j++)
					if (!memcmp(f->items[j].items[0].data, o->items[0].data, 32))
						FAIL("fork: an obligation twice");
			}
		}
		return 1;
	}
	case 20: /* closing */
		if (!mor_cbor_map_closed(p, 0x1fu) || !is_hash(mor_pl(a, 0)) || !is_hash(mor_pl(a, 1)) ||
		    !is_hash(mor_pl(a, 2)) || !is_arr(mor_pl(a, 3)))
			FAIL("closing payload");
		f = mor_pl(a, 3);
		for (uint64_t i = 0; i < f->v; i++)
			if (!mor_f_kept_tip(&f->items[i]))
				FAIL("closing tips");
		if ((f = mor_pl(a, 4))) {
			if (!is_arr(f) || f->v == 0)
				FAIL("closing left open");
			for (uint64_t i = 0; i < f->v; i++) {
				const mor_cbor *l = &f->items[i];
				if (!is_arr(l) || l->v < 2 || l->v > 3 || !is_hash(&l->items[0]) ||
				    !mor_f_hash_or_null(&l->items[1]) || (l->v == 3 && !is_hash(&l->items[2])))
					FAIL("closing left open");
				if (i && memcmp(f->items[i - 1].items[0].data, l->items[0].data, 32) >= 0)
					FAIL("closing left open not ascending");
			}
		}
		return 1;
	case 23: /* fork settlement */
		if (!mor_cbor_map_closed(p, 0x7u) || !is_hash(mor_pl(a, 0)) || !is_hash(mor_pl(a, 1)) ||
		    !mor_f_hashes_ascending(mor_pl(a, 2)) || has_hash(mor_pl(a, 2), mor_pl(a, 1)->data))
			FAIL("fork settlement payload");
		if (!mor_objects_has(a, mor_pl(a, 0)->data, mor_pl(a, 0)->data))
			FAIL("fork settlement does not name its request");
		return 1;
	case 24: /* notice */
		if (!mor_cbor_map_closed(p, 0x3u) || !is_hash(mor_pl(a, 0)) || !pair_hash_any(mor_pl(a, 1)))
			FAIL("notice payload");
		return 1;
	case 25: /* request to a judge */
		f = mor_pl(a, 1);
		if (!mor_cbor_map_closed(p, 0x7u) || !is_hash(mor_pl(a, 0)) || !is_hash(mor_pl(a, 2)) || !arr_n(f, 2) ||
		    !is_uint(&f->items[0]) || f->items[0].v > 1 ||
		    (f->items[0].v == 0 ? !is_uint(&f->items[1]) : !is_hash(&f->items[1])))
			FAIL("request to a judge payload");
		return 1;
	case 26: { /* issuance offer */
		if (!mor_cbor_map_closed(p, 0xffu) || !is_hash(mor_pl(a, 0)) || !is_uint(mor_pl(a, 1)) ||
		    !is_uint(mor_pl(a, 2)) || mor_pl(a, 2)->v == 0 || mor_pl(a, 2)->v >= MILLION || !is_arr(mor_pl(a, 3)) ||
		    mor_pl(a, 3)->v == 0 || !mor_pl(a, 4))
			FAIL("issuance offer payload");
		f = mor_pl(a, 3);
		uint64_t sum = 0;
		for (uint64_t i = 0; i < f->v; i++) {
			const mor_cbor *h = &f->items[i];
			if (!arr_n(h, 2) || !is_hash(&h->items[0]) || !is_uint(&h->items[1]))
				FAIL("issuance offer holders");
			if (i && memcmp(f->items[i - 1].items[0].data, h->items[0].data, 32) >= 0)
				FAIL("issuance offer holders not ascending");
			sum += h->items[1].v;
		}
		if (sum != MILLION)
			FAIL("issuance offer parts do not sum to 1,000,000");
		if ((f = mor_pl(a, 5)) && !mor_f_amount(f))
			FAIL("issuance offer price");
		if ((f = mor_pl(a, 6)) && !mor_f_tstr(f))
			FAIL("issuance offer words");
		if ((f = mor_pl(a, 7)) && !pair_hash_any(f))
			FAIL("issuance offer cMIP");
		return 1;
	}
	case 27: /* proxy */
		if (!mor_cbor_map_closed(p, 0xfu) || !is_hash(mor_pl(a, 0)) || !is_uint(mor_pl(a, 1)) ||
		    !is_hash(mor_pl(a, 2)) || ((f = mor_pl(a, 3)) && !is_hash(f)))
			FAIL("proxy payload");
		if (!mor_objects_has(a, mor_pl(a, 0)->data, mor_pl(a, 0)->data))
			FAIL("proxy does not name its agreement");
		return 1;
	default: /* 7, 11, 15 and 21 are retired; nothing above 28 is defined */
		FAIL("a type Agreements does not define");
	}
}
