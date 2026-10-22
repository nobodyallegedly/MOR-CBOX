/* verdicts.json: for each case, build the verifier's world from the acts and
 * facts it holds, ask the case's question, and compare the answer. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/agreements.h"
#include "../src/development.h"
#include "../src/identity.h"
#include "../src/money.h"
#include "../src/verdict.h"
#include "hex.h"
#include "vectors.h"

static const char *mir_keys[MOR_NMIRS] = { "identity", "envelopes", "text", "money", "agreements", "development" };

/* Build the world of one case: its acts (less any whose id starts with
 * `drop`), and the facts it holds beside them. */
int vbuild_world(mor_arena *ar, const json *root, const json *c, mor_world *w, const char *drop)
{
	struct {
		mor_arena *a;
	} rr = { ar }, *r = &rr;
	uint8_t mir[MOR_NMIRS][32];
	const json *mirs = json_get(root, "mirs"), *pool = json_get(root, "pool");
	for (int i = 0; i < MOR_NMIRS; i++)
		if (hex_fixed(json_str(json_get(mirs, mir_keys[i])), mir[i], 32))
			return -1;
	const json *acts = json_get(c, "acts");
	size_t n = acts ? acts->len : 0;
	const uint8_t **bytes = mor_arena_array(r->a, n + 1, sizeof *bytes);
	size_t *lens = mor_arena_array(r->a, n + 1, sizeof *lens);
	if (!bytes || !lens)
		return -1;
	size_t k = 0;
	for (size_t i = 0; i < n; i++) {
		const char *id = json_str(json_at(acts, i));
		if (drop && !strncmp(id, drop, strlen(drop)))
			continue;
		bytes[k] = hex_decode(json_str(json_get(pool, id)), &lens[k], r->a);
		if (!bytes[k])
			return -1;
		k++;
	}
	if (mor_world_init(w, r->a, (const uint8_t(*)[32])mir, bytes, lens, k))
		return -1;
	const json *facts = json_get(c, "facts");
	const json *an = json_get(facts, "anchors");
	if (an && an->len) {
		w->anchors = mor_arena_array(r->a, an->len, sizeof *w->anchors);
		for (size_t i = 0; i < an->len; i++) {
			const json *x = json_at(an, i);
			mor_anchor *a = &w->anchors[w->nanchors++];
			hex_fixed(json_str(json_get(x, "act")), a->act, 32);
			a->clock = hex_decode(json_str(json_get(x, "clock")), &a->clock_len, r->a);
			a->point = (uint64_t)json_get(x, "point")->i;
		}
	}
	const json *ip = json_get(facts, "inclusion_proofs");
	if (ip && ip->len) {
		w->proofs = mor_arena_array(r->a, ip->len, sizeof *w->proofs);
		for (size_t i = 0; i < ip->len; i++) {
			const json *x = json_at(ip, i);
			mor_incl_proof *p = &w->proofs[w->nproofs++];
			hex_fixed(json_str(json_get(x, "summary")), p->summary, 32);
			if (json_str(json_get(x, "receipt"))) {
				p->has_receipt = 1;
				hex_fixed(json_str(json_get(x, "receipt")), p->receipt, 32);
			}
			p->index = (uint64_t)json_get(x, "index")->i;
			const json *path = json_get(x, "path");
			uint8_t(*pp)[32] = mor_arena_array(r->a, path->len + 1, 32);
			for (size_t q = 0; q < path->len; q++)
				hex_fixed(json_str(json_at(path, q)), pp[q], 32);
			p->path = (const uint8_t(*)[32])pp;
			p->npath = path->len;
		}
	}
	const json *rv = json_get(facts, "rail_valid");
	if (rv && rv->len) {
		w->rails = mor_arena_array(r->a, rv->len, sizeof *w->rails);
		for (size_t i = 0; i < rv->len; i++) {
			const json *x = json_at(rv, i);
			mor_rail_answer *ra = &w->rails[w->nrails++];
			hex_fixed(json_str(json_get(x, "act")), ra->act, 32);
			ra->valid = 1;
			if (json_str(json_get(x, "paid_to_flow"))) {
				ra->has_flow = 1;
				hex_fixed(json_str(json_get(x, "paid_to_flow")), ra->paid_to_flow, 32);
			}
		}
	}
	const json *rp = json_get(facts, "rail_payments");
	if (rp && rp->len) {
		w->payments = mor_arena_array(r->a, rp->len, sizeof *w->payments);
		for (size_t i = 0; i < rp->len; i++) {
			const json *x = json_at(rp, i);
			mor_rail_payment *p = &w->payments[w->npayments++];
			p->proof = hex_decode(json_str(json_get(x, "proof")), &p->proof_len, r->a);
			p->payment = hex_decode(json_str(json_get(x, "payment")), &p->payment_len, r->a);
			if (!p->proof || !p->payment)
				return -1;
		}
	}
	const json *ck = json_get(facts, "content_keys");
	for (size_t i = 0; ck && i < ck->len; i++) {
		uint8_t aid[32], key[32];
		if (hex_fixed(ck->keys[i], aid, 32) || hex_fixed(json_str(&ck->items[i]), key, 32) ||
		    mor_world_open_with(w, aid, key))
			return -1;
	}
	const json *fh = json_get(facts, "found_at_home");
	if (fh && fh->len) {
		w->found = mor_arena_array(r->a, fh->len, sizeof *w->found);
		for (size_t i = 0; i < fh->len; i++) {
			const json *x = json_at(fh, i);
			mor_found *f = &w->found[w->nfound++];
			hex_fixed(json_str(json_get(x, "act")), f->act, 32);
			hex_fixed(json_str(json_get(x, "operator")), f->operator, 32);
		}
	}
	const json *un = json_get(facts, "unreached");
	if (un && un->len) {
		uint8_t(*u)[32] = mor_arena_array(r->a, un->len, 32);
		for (size_t i = 0; i < un->len; i++)
			hex_fixed(json_str(json_at(un, i)), u[i], 32);
		w->unreached = (const uint8_t(*)[32])u;
		w->nunreached = un->len;
	}
	const json *hs = json_get(c, "held_specifications");
	if (hs && hs->len) {
		w->specs = mor_arena_array(r->a, hs->len, sizeof *w->specs);
		for (size_t i = 0; i < hs->len; i++) {
			mor_spec_content *s = &w->specs[w->nspecs++];
			hex_fixed(hs->keys[i], s->hash, 32);
			s->bytes = hex_decode(json_str(&hs->items[i]), &s->len, r->a);
		}
	}
	return 0;
}

static void render_chain(mor_world *w, const uint8_t id[32], char *out, size_t cap)
{
	const mor_chain *c = mor_chain_get(w, id);
	size_t n = 0;
	n += (size_t)snprintf(out + n, cap - n, "{\"counting\":[");
	for (size_t i = 0; c && i < c->npos && n < cap; i++) {
		char *h = hex_encode(c->pos[i].act->id, 32, w->a);
		n += (size_t)snprintf(out + n, cap - n, "%s\"%s\"", i ? "," : "", h);
	}
	n += (size_t)snprintf(out + n, cap - n, "],\"dishonest\":[");
	for (size_t i = 0; c && i < c->ndishonest && n < cap; i++) {
		char *h = hex_encode(c->dishonest[i].op, 32, w->a);
		n += (size_t)snprintf(out + n, cap - n, "%s{\"from_position\":%llu,\"operator\":\"%s\"}", i ? "," : "",
				      (unsigned long long)c->dishonest[i].from, h);
	}
	snprintf(out + n, cap - n, "],\"next\":\"%s\"}", c ? mor_next_name(c->next) : "unknown");
}

static int cmp_act_id(const void *x, const void *y)
{
	return memcmp((*(const mor_act *const *)x)->id, (*(const mor_act *const *)y)->id, 32);
}

static int cmp_sale(const void *x, const void *y)
{
	return memcmp(((const mor_vow_sale *)x)->act->id, ((const mor_vow_sale *)y)->act->id, 32);
}

static void render_ids(const mor_act **l, size_t n, mor_arena *a, char *out, size_t cap, size_t *pos)
{
	qsort(l, n, sizeof *l, cmp_act_id);
	for (size_t i = 0; i < n && *pos < cap; i++)
		*pos += (size_t)snprintf(out + *pos, cap - *pos, "%s\"%s\"", i ? "," : "", hex_encode(l[i]->id, 32, a));
}

static void render_vow(mor_vow *v, mor_arena *a, char *out, size_t cap)
{
	static const char *states[] = { "pending", "confirmed", "contested" };
	if (v->unknown_all) {
		snprintf(out, cap, "unknown");
		return;
	}
	if (v->nsales)
		qsort(v->sales, v->nsales, sizeof *v->sales, cmp_sale);
	size_t n = (size_t)snprintf(out, cap, "{\"sales\":[");
	for (size_t i = 0; i < v->nsales && n < cap; i++)
		n += (size_t)snprintf(out + n, cap - n, "%s{\"act\":\"%s\",\"buyer\":\"%s\",\"state\":\"%s\"}", i ? "," : "",
				      hex_encode(v->sales[i].act->id, 32, a), hex_encode(v->sales[i].buyer, 32, a),
				      states[v->sales[i].state]);
	n += (size_t)snprintf(out + n, cap - n, "],\"unknown\":[");
	render_ids(v->unknown, v->nunknown, a, out, cap, &n);
	n += (size_t)snprintf(out + n, cap - n, "],\"unsigned\":[");
	render_ids(v->unsigned_, v->nunsigned, a, out, cap, &n);
	snprintf(out + n, cap - n, "]}");
}

static void answer(vrun *r, const json *root, const json *c, char *got, size_t cap)
{
	mor_world w;
	const json *ask = json_get(c, "ask");
	const char *q = json_str(json_get(ask, "question"));
	uint8_t id[32];
	snprintf(got, cap, "(no answer)");
	/* Development and MIR-hash cases carry no acts. */
	if (json_get(c, "content")) {
		const json *empty_root = root;
		if (vbuild_world(r->a, empty_root, c, &w, NULL)) {
			snprintf(got, cap, "(world failed)");
			return;
		}
		size_t n;
		const uint8_t *content = hex_decode(json_str(json_get(c, "content")), &n, r->a);
		mor_spec_result sr;
		mor_spec_decode(&w, content, n, &sr);
		if (sr.verdict == MOR_SPEC_OK)
			snprintf(got, cap, "a specification");
		else if (sr.verdict == MOR_SPEC_NONE)
			snprintf(got, cap, "no specification");
		else
			snprintf(got, cap, "unknown: the cMIP %s is not held", hex_encode(sr.missing, 32, r->a));
		return;
	}
	if (json_get(c, "file")) {
		const json *f = json_get(c, "file");
		uint8_t h[32];
		const uint8_t *text;
		size_t tl;
		if (mor_mir_hash((const uint8_t *)f->s, f->len, h, 4, r->a, &text, &tl))
			snprintf(got, cap, "null");
		else
			snprintf(got, cap, "%s", hex_encode(h, 32, r->a));
		return;
	}
	if (vbuild_world(r->a, root, c, &w, NULL)) {
		snprintf(got, cap, "(world failed)");
		return;
	}
	const char *aid = json_str(json_get(ask, "act"));
	if (aid)
		hex_fixed(aid, id, 32);
	if (!q) {
		return;
	} else if (!strcmp(q, "status")) {
		snprintf(got, cap, "%s", mor_status_name(mor_status(&w, id, 0)));
	} else if (!strcmp(q, "status after keepers")) {
		snprintf(got, cap, "%s", mor_status_name(mor_status(&w, id, MOR_Q_KEEPERS)));
	} else if (!strcmp(q, "verdict, implementing the MIRs only")) {
		snprintf(got, cap, "%s", mor_status_name(mor_verdict(&w, id)));
	} else if (!strcmp(q, "chain")) {
		hex_fixed(json_str(json_get(ask, "identity")), id, 32);
		render_chain(&w, id, got, cap);
	} else if (!strcmp(q, "succession")) {
		hex_fixed(json_str(json_get(ask, "identity")), id, 32);
		mor_succ sc;
		mor_succession(&w, id, &sc);
		size_t n = 0;
		n += (size_t)snprintf(got + n, cap - n, "{");
		if (sc.confirmation)
			n += (size_t)snprintf(got + n, cap - n, "\"confirmation\":\"%s\",",
					      hex_encode(sc.confirmation->id, 32, r->a));
		n += (size_t)snprintf(got + n, cap - n, "\"seen\":\"%s\"", mor_succession_name(sc.seen));
		if (sc.successor)
			n += (size_t)snprintf(got + n, cap - n, ",\"successor\":\"%s\"", hex_encode(sc.successor, 32, r->a));
		snprintf(got + n, cap - n, "}");
	} else if (!strcmp(q, "backing")) {
		snprintf(got, cap, "%s", mor_backing_name(mor_backing(&w, id)));
	} else if (!strcmp(q, "consent counts")) {
		int k = mor_consent_counts(&w, id);
		snprintf(got, cap, "%s", k > 0 ? "true" : k == 0 ? "false" : "unknown");
	} else if (!strcmp(q, "agreement exists")) {
		int e = mor_agreement_exists(&w, id);
		snprintf(got, cap, "%s", e == MOR_AGR_EXISTS ? "exists" : e == MOR_AGR_NOT ? "does not exist" :
					 e == MOR_AGR_INVALID ? "invalid" : "unknown");
	} else if (!strcmp(q, "closing") || !strcmp(q, "fork")) {
		int e = mor_ending_complete(&w, id);
		snprintf(got, cap, "%s", e > 0 ? "{\"complete\":true}" : e == 0 ? "{\"complete\":false}" : "unknown");
	} else if (!strcmp(q, "deal fork")) {
		mor_fork f;
		mor_deal_fork(&w, id, &f);
		snprintf(got, cap, "{\"branches\":%d,\"reference\":\"%s\",\"tangled\":%s}", f.branches,
			 hex_encode(f.reference, 32, r->a), f.tangled ? "true" : "false");
	} else if (!strcmp(q, "deadline")) {
		const json *ks = json_get(ask, "keepers");
		uint8_t(*k)[32] = mor_arena_array(r->a, ks->len + 1, 32);
		for (size_t i = 0; i < ks->len; i++)
			hex_fixed(json_str(json_at(ks, i)), k[i], 32);
		/* the rule, in terms field 3's form: 0 is [0] */
		const json *rj = json_get(ask, "rule");
		static const uint8_t rule0[2] = { 0x81, 0x00 };
		const mor_cbor *rule = NULL;
		if (rj && rj->type == JSON_NUM && rj->i == 0)
			mor_cbor_decode(rule0, 2, r->a, &rule);
		size_t clen;
		const uint8_t *clock = hex_decode(json_str(json_get(ask, "clock")), &clen, r->a);
		int d = mor_deadline(&w, id, (const uint8_t(*)[32])k, ks->len, rule, clock, clen,
				     (uint64_t)json_get(ask, "point")->i);
		snprintf(got, cap, "%s", mor_deadline_name(d));
	} else if (!strcmp(q, "disagreements")) {
		hex_fixed(json_str(json_get(ask, "receiver")), id, 32);
		int d = mor_money_disagreements(&w, id);
		if (d < 0)
			snprintf(got, cap, "unknown");
		else
			snprintf(got, cap, "%d", d);
	} else if (!strcmp(q, "refund repaid")) {
		uint8_t to[32], unit[32];
		hex_fixed(json_str(json_get(ask, "payment")), id, 32);
		hex_fixed(json_str(json_get(ask, "to")), to, 32);
		hex_fixed(json_str(json_get(ask, "unit")), unit, 32);
		int64_t v = mor_money_refund_repaid(&w, id, to, unit);
		if (v < 0)
			snprintf(got, cap, "unknown");
		else
			snprintf(got, cap, "%lld", (long long)v);
	} else if (!strcmp(q, "vow")) {
		hex_fixed(json_str(json_get(ask, "vow")), id, 32);
		static mor_vow vw;
		mor_money_vow(&w, id, &vw);
		render_vow(&vw, r->a, got, cap);
	} else if (!strcmp(q, "paid toward")) {
		int64_t v = mor_money_paid_toward(&w, id);
		if (v < 0)
			snprintf(got, cap, "unknown");
		else
			snprintf(got, cap, "%lld", (long long)v);
	} else {
		snprintf(got, cap, "(question not implemented: %s)", q);
	}
}

int v_verdicts(vrun *r, const json *j)
{
	const json *cases = json_get(j, "cases");
	static char exp[8192], got[8192], name[512];
	for (size_t l = 0; cases && l < cases->len; l++) {
		const json *layer = &cases->items[l];
		for (size_t i = 0; i < layer->len; i++) {
			const json *c = json_at(layer, i);
			const json *v = json_get(c, "verdict");
			if (!v)
				v = json_get(c, "spec_hash");
			json_render(v, exp, sizeof exp);
			answer(r, j, c, got, sizeof got);
			snprintf(name, sizeof name, "%s[%zu] %s", cases->keys[l], i, json_str(json_get(c, "name")));
			vrecord(r, name, exp, got, json_str(json_get(c, "rule")));
			mor_arena_reset(r->a, 0);
		}
	}
	return 0;
}
