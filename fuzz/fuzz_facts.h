/* For the targets that build a world: facts and the second edition's
 * questions. */
#ifndef MOR_FUZZ_FACTS_H
#define MOR_FUZZ_FACTS_H

#include <string.h>

#include "../src/agreements.h"
#include "../src/identity.h"
#include "../src/money.h"
#include "../src/world.h"

/* Facts beside the acts, derived from the acts themselves so that the input
 * decides them: a valid rail answer for every act; which payment a proof is
 * for acts whose id begins with an even byte (the others left unstated, so
 * that pairs of unknown sameness arise); a judge's period lapsed and a
 * settlement out of turn by other bits of the ids; private acts found at
 * their signer's home. */
static void fuzz_facts(mor_world *w, mor_arena *a)
{
	size_t n = w->nacts;
	if (!n)
		return;
	w->rails = mor_arena_array(a, n, sizeof *w->rails);
	w->payments = mor_arena_array(a, n, sizeof *w->payments);
	w->lapsed = mor_arena_array(a, n, sizeof *w->lapsed);
	w->found = mor_arena_array(a, n, sizeof *w->found);
	uint8_t(*oot)[32] = mor_arena_array(a, n, 32);
	if (!w->rails || !w->payments || !w->lapsed || !w->found || !oot)
		return;
	for (size_t i = 0; i < n; i++) {
		const mor_act *x = &w->acts[i];
		mor_rail_answer *r = &w->rails[w->nrails++];
		memset(r, 0, sizeof *r);
		memcpy(r->act, x->id, 32);
		r->valid = 1;
		const mor_cbor *pr = mor_pl(x, 1);
		if (!(x->id[0] & 1) && pr && pr->type == MOR_CBOR_BYTES && pr->v) {
			mor_rail_payment *p = &w->payments[w->npayments++];
			p->proof = pr->data;
			p->proof_len = (size_t)pr->v;
			p->payment = pr->data;
			p->payment_len = 1;
		}
		if (x->id[1] & 1) {
			mor_lapse *l = &w->lapsed[w->nlapsed++];
			memcpy(l->judge, mor_signer(x), 32);
			memcpy(l->question, w->acts[(i + 1) % n].id, 32);
		}
		if (!(x->id[2] & 3))
			memcpy(oot[w->nout_of_turn++], x->id, 32);
		if (x->signer && !x->key) {
			mor_found *f = &w->found[w->nfound++];
			memcpy(f->act, x->id, 32);
			memcpy(f->operator, x->signer, 32);
		}
	}
	w->out_of_turn = (const uint8_t(*)[32])oot;
}

/* The second edition's questions, about one act. */
static void fuzz_ask_more(mor_world *w, const mor_act *x)
{
	static mor_vow vow;
	mor_succ sc;
	uint8_t v[32];
	const uint8_t *s = mor_signer(x);
	(void)mor_succession(w, s, &sc);
	(void)mor_ending_complete(w, x->id);
	(void)mor_deal_version_in_force(w, x->id, v);
	(void)mor_money_vow(w, x->id, &vow);
	(void)mor_money_refund_repaid(w, x->id, s, x->id);
}

#endif
