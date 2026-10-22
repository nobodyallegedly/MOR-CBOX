#include "formats.h"

#include <string.h>

#define FAIL(r)                  \
	do {                     \
		if (why)         \
			*why = (r); \
		return 0;        \
	} while (0)

#define BIT(k) (UINT64_C(1) << (k))

int mor_f_uint(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_UINT;
}

int mor_f_tstr(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_TEXT;
}

static int is_bstr(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_BYTES;
}

static int is_bstr_n(const mor_cbor *c, uint64_t n)
{
	return c && c->type == MOR_CBOR_BYTES && c->v == n;
}

int mor_f_arr(const mor_cbor *c, uint64_t n)
{
	return c && c->type == MOR_CBOR_ARRAY && c->v == n;
}

static int is_arr(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_ARRAY;
}

int mor_f_hash_or_null(const mor_cbor *c)
{
	return mor_cbor_is_hash(c) || mor_cbor_is_null(c);
}

static int hashes(const mor_cbor *c, int allow_empty)
{
	return mor_hash_list(c, allow_empty);
}

int mor_f_hashes_ascending(const mor_cbor *c)
{
	if (!hashes(c, 0))
		return 0;
	for (uint64_t i = 1; i < c->v; i++)
		if (memcmp(c->items[i - 1].data, c->items[i].data, 32) >= 0)
			return 0;
	return 1;
}

/* No hash listed twice (order free). */
static int hashes_distinct(const mor_cbor *c)
{
	for (uint64_t i = 0; i < c->v; i++)
		for (uint64_t j = i + 1; j < c->v; j++)
			if (!memcmp(c->items[i].data, c->items[j].data, 32))
				return 0;
	return 1;
}

/* scheme = 1 / 2 / 3 / hash; founding schemes carry 32-byte keys. */
int mor_f_signing_key(const mor_cbor *c)
{
	if (!mor_f_arr(c, 2) || !is_bstr(&c->items[1]))
		return 0;
	const mor_cbor *s = &c->items[0];
	if (s->type == MOR_CBOR_UINT && s->v >= 1 && s->v <= 3)
		return c->items[1].v == 32;
	return mor_cbor_is_hash(s);
}

int mor_f_chain_commit(const mor_cbor *c)
{
	if (!mor_f_arr(c, 2) || !mor_cbor_is_hash(&c->items[1]))
		return 0;
	const mor_cbor *s = &c->items[0];
	return (s->type == MOR_CBOR_UINT && s->v >= 1 && s->v <= 3) || mor_cbor_is_hash(s);
}

int mor_f_kept_tip(const mor_cbor *c)
{
	return mor_f_arr(c, 3) && mor_cbor_is_hash(&c->items[0]) && mor_f_uint(&c->items[1]) &&
	       mor_cbor_is_hash(&c->items[2]);
}

int mor_f_amount(const mor_cbor *c)
{
	return mor_f_arr(c, 2) && mor_cbor_is_hash(&c->items[0]) && mor_f_uint(&c->items[1]);
}

static int list_of(const mor_cbor *c, int allow_empty, int (*f)(const mor_cbor *))
{
	if (!is_arr(c) || (!allow_empty && c->v == 0))
		return 0;
	for (uint64_t i = 0; i < c->v; i++)
		if (!f(&c->items[i]))
			return 0;
	return 1;
}

/* ---------------- Identity ---------------- */

static int f_home(const mor_cbor *c)
{
	return mor_f_arr(c, 2) && mor_f_hash_or_null(&c->items[0]) && mor_f_tstr(&c->items[1]);
}

static int f_home_rule(const mor_cbor *c)
{
	return mor_f_arr(c, 2) && mor_f_uint(&c->items[0]) && c->items[0].v <= 1 && mor_f_uint(&c->items[1]);
}

static int f_audit(const mor_cbor *c)
{
	return mor_f_arr(c, 2) && mor_f_uint(&c->items[0]) && hashes(&c->items[1], 0);
}

static int f_declaration(const mor_cbor *c)
{
	return mor_f_arr(c, 3) && mor_cbor_is_hash(&c->items[0]) && mor_f_uint(&c->items[1]);
}

static int f_successor(const mor_cbor *c)
{
	return mor_f_arr(c, 2) && mor_f_tstr(&c->items[0]) && is_bstr(&c->items[1]);
}

static int f_route(const mor_cbor *c)
{
	if (!is_arr(c) || c->v < 2 || c->v > 3)
		return 0;
	if (!mor_f_hash_or_null(&c->items[0]) || !list_of(&c->items[1], 0, mor_f_tstr))
		return 0;
	/* kind 0 is written as absent, never as 0 */
	if (c->v == 3 && !(mor_f_uint(&c->items[2]) && c->items[2].v == 1))
		return 0;
	return 1;
}

/* A versioned chain's head: version 0 is none; version 1 has no previous; a
 * later version names it (routes, encryption key, payee pointer). */
static int versioned(const mor_act *a, uint64_t vkey, uint64_t prevkey)
{
	const mor_cbor *v = mor_pl(a, vkey), *p = mor_pl(a, prevkey);
	if (!mor_f_uint(v) || v->v == 0)
		return 0;
	if (v->v == 1)
		return p == NULL;
	return mor_cbor_is_hash(p);
}

static int identity_format(const mor_act *a, const char **why)
{
	const mor_cbor *p = a->payload;
	switch (a->type) {
	case 0: /* genesis */
		if (!mor_cbor_map_closed(p, BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5)))
			FAIL("genesis payload keys");
		if (!mor_f_signing_key(mor_pl(a, 0)) || !mor_f_chain_commit(mor_pl(a, 1)) ||
		    !list_of(mor_pl(a, 2), 0, f_home))
			FAIL("genesis payload fields");
		if (mor_pl(a, 3) && !f_home_rule(mor_pl(a, 3)))
			FAIL("genesis home rule");
		if (mor_pl(a, 4) && !list_of(mor_pl(a, 4), 0, f_declaration))
			FAIL("genesis declarations");
		if (mor_pl(a, 5) && !f_audit(mor_pl(a, 5)))
			FAIL("genesis audit");
		return 1;
	case 1: { /* rotation */
		if (!mor_cbor_map_closed(p, 0x1fffu))
			FAIL("rotation payload keys");
		if (!mor_cbor_is_hash(mor_pl(a, 0)) || !mor_f_uint(mor_pl(a, 1)) || !mor_f_signing_key(mor_pl(a, 2)) ||
		    !mor_f_chain_commit(mor_pl(a, 3)) || !list_of(mor_pl(a, 4), 1, mor_f_kept_tip))
			FAIL("rotation payload fields");
		const mor_cbor *f;
		if ((f = mor_pl(a, 5)) && !hashes(f, 0))
			FAIL("rotation disowned");
		if ((f = mor_pl(a, 6)) && !list_of(f, 0, f_home))
			FAIL("rotation homes");
		if ((f = mor_pl(a, 7)) && !mor_cbor_is_null(f) && !f_home_rule(f))
			FAIL("rotation home rule");
		if ((f = mor_pl(a, 8)) && !list_of(f, 0, f_declaration))
			FAIL("rotation declarations");
		if ((f = mor_pl(a, 9)) && !mor_cbor_is_null(f) && !f_successor(f))
			FAIL("rotation successor");
		if ((f = mor_pl(a, 10)) && !mor_cbor_is_null(f) && !f_audit(f))
			FAIL("rotation audit");
		if ((f = mor_pl(a, 11)) && !mor_cbor_is_true(f))
			FAIL("rotation homeless flag");
		if ((f = mor_pl(a, 12)) && !mor_cbor_is_true(f))
			FAIL("rotation closure flag");
		return 1;
	}
	case 2: /* receipt */
		if (!mor_cbor_map_closed(p, 0xfu) || !mor_cbor_is_hash(mor_pl(a, 0)) || !mor_cbor_is_hash(mor_pl(a, 1)) ||
		    !mor_f_uint(mor_pl(a, 2)) || !mor_f_uint(mor_pl(a, 3)))
			FAIL("receipt payload");
		return 1;
	case 3: /* routes */
		if (!mor_cbor_map_closed(p, 0x7u) || !list_of(mor_pl(a, 2), 1, f_route))
			FAIL("routes payload");
		if (!versioned(a, 0, 1))
			FAIL("routes version");
		return 1;
	case 4: /* name */
		if (!mor_cbor_map_closed(p, 0x3u) || !mor_f_tstr(mor_pl(a, 0)) || !mor_cbor_is_hash(mor_pl(a, 1)))
			FAIL("name payload");
		return 1;
	case 5:  /* name withdrawal */
	case 7:  /* link confirmation */
	case 8:  /* link termination */
	case 10: /* cosignature */
		if (p->v != 0)
			FAIL("payload not empty");
		if (!a->objects)
			FAIL("names no act in objects");
		return 1;
	case 6: /* link claim */
		if (!mor_cbor_map_closed(p, 0x3u) || !f_successor(mor_pl(a, 0)) || (mor_pl(a, 1) && !is_bstr(mor_pl(a, 1))))
			FAIL("link claim payload");
		return 1;
	case 9: /* log summary */
		if (!mor_cbor_map_closed(p, 0x7u) || !mor_f_uint(mor_pl(a, 0)) || !mor_cbor_is_hash(mor_pl(a, 1)) ||
		    (mor_pl(a, 2) && !mor_cbor_is_hash(mor_pl(a, 2))))
			FAIL("log summary payload");
		return 1;
	case 12: /* objection */
	case 13: /* absence statement */
		if (!mor_cbor_map_closed(p, a->type == 12 ? 0x1u : 0x3u) || !mor_cbor_is_hash(mor_pl(a, 0)) ||
		    (mor_pl(a, 1) && !mor_cbor_is_hash(mor_pl(a, 1))))
			FAIL("objection or absence payload");
		/* exactly the entry [identity, homeless rotation], the chain that identity */
		if (!a->objects || a->objects->v != 1 ||
		    memcmp(a->objects->items[0].items[0].data, mor_pl(a, 0)->data, 32))
			FAIL("objection or absence objects");
		return 1;
	case 14: /* escape endorsement */
		if (!mor_cbor_map_closed(p, 0x1u) || (mor_pl(a, 0) && !hashes(mor_pl(a, 0), 0)))
			FAIL("escape endorsement payload");
		if (!a->objects || a->objects->v != 1)
			FAIL("escape endorsement objects");
		return 1;
	case 15: /* witness: acks at least one, no objects, empty payload (rule 10) */
		if (p->v != 0 || !a->acks || a->objects)
			FAIL("witness shape");
		return 1;
	case 16: /* chain signature */
		if (!mor_cbor_map_closed(p, 0xfu) || !mor_cbor_is_hash(mor_pl(a, 0)) || !mor_f_uint(mor_pl(a, 1)) ||
		    !mor_f_chain_commit(mor_pl(a, 2)) || !mor_cbor_is_hash(mor_pl(a, 3)))
			FAIL("chain signature payload");
		return 1;
	default:
		FAIL("a type Identity does not define");
	}
}

/* ---------------- Envelopes ---------------- */

static int f_enc_key(const mor_cbor *c)
{
	if (!mor_f_arr(c, 2) || !is_bstr(&c->items[1]))
		return 0;
	const mor_cbor *s = &c->items[0];
	if (s->type == MOR_CBOR_UINT && s->v == 4)
		return c->items[1].v == 1216;
	return mor_cbor_is_hash(s);
}

static int envelopes_format(const mor_act *a, const char **why)
{
	const mor_cbor *p = a->payload, *f;
	switch (a->type) {
	case 0: /* publication: the media map */
		if (!mor_cbor_map_closed(p, 0x1ffu) || !mor_cbor_is_hash(mor_pl(a, 0)) || !mor_cbor_is_hash(mor_pl(a, 1)) ||
		    !mor_cbor_is_hash(mor_pl(a, 2)) || !mor_f_uint(mor_pl(a, 3)) || !is_bstr_n(mor_pl(a, 4), 24))
			FAIL("publication payload");
		if ((f = mor_pl(a, 5)) && !is_bstr_n(f, 32))
			FAIL("publication media key");
		if ((f = mor_pl(a, 6)) && !list_of(f, 0, mor_f_tstr))
			FAIL("publication locations");
		if ((f = mor_pl(a, 8)) && !mor_cbor_is_hash(f))
			FAIL("publication for");
		return 1;
	case 1: /* key delivery */
		if (!mor_cbor_map_closed(p, 0x7u) || !mor_cbor_is_hash(mor_pl(a, 0)) || !is_bstr_n(mor_pl(a, 1), 32) ||
		    ((f = mor_pl(a, 2)) && !mor_cbor_is_true(f)))
			FAIL("key delivery payload");
		return 1;
	case 3: /* withdrawal: {} and [publication, publication] */
		if (p->v != 0)
			FAIL("withdrawal payload not empty");
		if (!a->objects)
			FAIL("withdrawal names no publication");
		for (uint64_t i = 0; i < a->objects->v; i++)
			if (!memcmp(a->objects->items[i].items[0].data, a->objects->items[i].items[1].data, 32))
				return 1;
		FAIL("withdrawal objects");
	case 4: /* encryption key */
		if (!mor_cbor_map_closed(p, 0x7u) || !f_enc_key(mor_pl(a, 2)) || !versioned(a, 0, 1))
			FAIL("encryption key payload");
		return 1;
	case 5: /* vow */
		if (!mor_cbor_map_closed(p, 0x3u) || !mor_f_tstr(mor_pl(a, 0)))
			FAIL("vow payload");
		if ((f = mor_pl(a, 1)) && !(mor_f_arr(f, 2) && mor_cbor_is_hash(&f->items[0])))
			FAIL("vow cMIP");
		return 1;
	default: /* type 2 is retired */
		FAIL("a type Envelopes does not define");
	}
}

/* ---------------- Text ---------------- */

static int text_format(const mor_act *a, const char **why)
{
	if (a->type != 0)
		FAIL("a type Text does not define");
	if (!mor_cbor_map_closed(a->payload, 0x3u) || !mor_f_tstr(mor_pl(a, 0)) ||
	    (mor_pl(a, 1) && !mor_cbor_is_hash(mor_pl(a, 1))))
		FAIL("text payload");
	return 1;
}

/* ---------------- Money ---------------- */

static int f_rail(const mor_cbor *c)
{
	return mor_f_arr(c, 2) && mor_cbor_is_hash(&c->items[0]) && is_bstr(&c->items[1]);
}

static int f_purchase(const mor_cbor *c)
{
	return mor_f_arr(c, 2) && mor_cbor_is_hash(&c->items[0]) && mor_cbor_is_hash(&c->items[1]);
}

static int money_format(const mor_act *a, const char **why)
{
	const mor_cbor *p = a->payload, *f;
	switch (a->type) {
	case 0: /* payee pointer, signed by the identity it is for */
		if (!mor_cbor_map_closed(p, 0xfu) || !mor_cbor_is_hash(mor_pl(a, 0)) || !list_of(mor_pl(a, 3), 0, f_rail) ||
		    !versioned(a, 1, 2))
			FAIL("payee pointer payload");
		if (!a->signer || memcmp(a->signer, mor_pl(a, 0)->data, 32))
			FAIL("payee pointer signed by another identity than its payee");
		return 1;
	case 1: /* obligation, signed by the debtor */
		if (!mor_cbor_map_closed(p, 0x1fu) || !mor_cbor_is_hash(mor_pl(a, 0)) || !mor_cbor_is_hash(mor_pl(a, 1)) ||
		    !mor_f_amount(mor_pl(a, 2)) || !mor_cbor_is_hash(mor_pl(a, 3)) ||
		    ((f = mor_pl(a, 4)) && !mor_cbor_is_hash(f)))
			FAIL("obligation payload");
		if (!a->signer || memcmp(a->signer, mor_pl(a, 0)->data, 32))
			FAIL("obligation not signed by its debtor");
		return 1;
	case 2: /* settlement receipt, signed by the hop's payee */
		if (!mor_cbor_map_closed(p, 0x3ffu) || !mor_cbor_is_hash(mor_pl(a, 0)) || !is_bstr(mor_pl(a, 1)) ||
		    !mor_cbor_is_hash(mor_pl(a, 3)) || !mor_f_amount(mor_pl(a, 4)) || !mor_cbor_is_hash(mor_pl(a, 5)))
			FAIL("receipt payload");
		if ((f = mor_pl(a, 2)) && !mor_cbor_is_hash(f) && !mor_f_signing_key(f))
			FAIL("receipt payer");
		if ((f = mor_pl(a, 6)) && !mor_cbor_is_hash(f))
			FAIL("receipt previous hop");
		if ((f = mor_pl(a, 7)) && !(mor_f_arr(f, 3) && mor_cbor_is_hash(&f->items[0]) &&
					   mor_f_amount(&f->items[1]) && mor_cbor_is_hash(&f->items[2])))
			FAIL("receipt forward");
		if ((f = mor_pl(a, 8)) && !mor_cbor_is_hash(f))
			FAIL("receipt batch");
		if ((f = mor_pl(a, 9)) && !f_purchase(f))
			FAIL("receipt purchase");
		if (!a->signer || memcmp(a->signer, mor_pl(a, 3)->data, 32))
			FAIL("receipt signed by another than the hop's receiver");
		return 1;
	case 3: /* payment claim */
		if (!mor_cbor_map_closed(p, 0x7ffu) || !mor_cbor_is_hash(mor_pl(a, 0)) || !is_bstr(mor_pl(a, 1)) ||
		    !mor_cbor_is_hash(mor_pl(a, 2)) || !mor_f_amount(mor_pl(a, 3)) || !mor_cbor_is_hash(mor_pl(a, 4)))
			FAIL("claim payload");
		if ((f = mor_pl(a, 5)) && !mor_cbor_is_hash(f))
			FAIL("claim disputed receipt");
		if ((f = mor_pl(a, 6)) && !f_purchase(f))
			FAIL("claim referral");
		if ((f = mor_pl(a, 7)) && !f_rail(f))
			FAIL("claim refund rail");
		if ((f = mor_pl(a, 8)) && !(mor_f_arr(f, 2) && mor_f_signing_key(&f->items[0]) && is_bstr(&f->items[1])))
			FAIL("claim anonymous");
		if ((f = mor_pl(a, 9)) && !f_purchase(f))
			FAIL("claim purchase");
		if ((f = mor_pl(a, 10)) && !mor_cbor_is_hash(f))
			FAIL("claim previous hop");
		return 1;
	case 4: /* creditor's release */
		if (!mor_cbor_map_closed(p, 0x3u) || !mor_cbor_is_hash(mor_pl(a, 0)))
			FAIL("release payload");
		if ((f = mor_pl(a, 1)) && (!hashes(f, 0) || !hashes_distinct(f)))
			FAIL("release field 1");
		return 1;
	default:
		FAIL("a type Money does not define");
	}
}

int mor_agreements_format(const mor_world *w, const mor_act *a, const char **why);

int mor_format_ok(const mor_world *w, const mor_act *a, const char **why)
{
	if (why)
		*why = NULL;
	switch (mor_world_mir_of(w, a->spec)) {
	case MOR_IDENTITY: return identity_format(a, why);
	case MOR_ENVELOPES: return envelopes_format(a, why);
	case MOR_TEXT: return text_format(a, why);
	case MOR_MONEY: return money_format(a, why);
	case MOR_AGREEMENTS: return mor_agreements_format(w, a, why);
	case MOR_DEVELOPMENT: FAIL("Development defines no act types");
	default: return 1;
	}
}

int mor_type_may_ack(const mor_world *w, const mor_act *a)
{
	int m = mor_world_mir_of(w, a->spec);
	return m == MOR_IDENTITY || m == MOR_MONEY || m == MOR_AGREEMENTS;
}
