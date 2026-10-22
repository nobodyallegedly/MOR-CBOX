#include "money.h"

#include <string.h>

#include "identity.h"

static const mor_rail_answer *rail_of(const mor_world *w, const uint8_t id[32])
{
	for (size_t i = 0; i < w->nrails; i++)
		if (!memcmp(w->rails[i].act, id, 32))
			return &w->rails[i];
	return NULL;
}

static int is_money(const mor_world *w, const mor_act *a, uint64_t type)
{
	return mor_world_is_mir(w, a, MOR_MONEY) && a->type == type;
}

/* Rule 10: a receipt or claim is treated as valid only when its rail answer is. */
static int counts(mor_world *w, const mor_act *a)
{
	const mor_rail_answer *r = rail_of(w, a->id);
	if (!r || !r->valid)
		return 0;
	int st = mor_status_act(w, a, 0);
	return st == MOR_ST_VALID || st == MOR_ST_SCOPED;
}

/* One payment: what the rail Module says. The core reads no rail; the rail
 * Module and the proof's bytes stand for it (see QUESTIONS.md). */
static int same_payment(const mor_act *x, const mor_act *y)
{
	const mor_cbor *rx = mor_pl(x, 0), *ry = mor_pl(y, 0), *px = mor_pl(x, 1), *py = mor_pl(y, 1);
	return !memcmp(rx->data, ry->data, 32) && px->v == py->v && !memcmp(px->data, py->data, (size_t)px->v);
}

static int cbor_eq(const mor_cbor *a, const mor_cbor *b)
{
	if (!a || !b)
		return a == b;
	return a->raw_len == b->raw_len && !memcmp(a->raw, b->raw, a->raw_len);
}

/* Does a receipt match a claim of the same payment in amount, payee, purpose
 * and previous hop? */
static int matches(const mor_act *rc, const mor_act *cl)
{
	return cbor_eq(mor_pl(rc, 4), mor_pl(cl, 3)) && cbor_eq(mor_pl(rc, 3), mor_pl(cl, 2)) &&
	       cbor_eq(mor_pl(rc, 5), mor_pl(cl, 4)) && cbor_eq(mor_pl(rc, 6), mor_pl(cl, 10));
}

int mor_money_disagreements(mor_world *w, const uint8_t receiver[32])
{
	int questions = 0;
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *cl = &w->acts[i];
		if (!is_money(w, cl, 3) || !counts(w, cl))
			continue;
		int any = 0, match = 0;
		for (size_t j = 0; j < w->nacts; j++) {
			const mor_act *rc = &w->acts[j];
			if (!is_money(w, rc, 2) || !mor_hash_eq(rc->signer, receiver) || !counts(w, rc))
				continue;
			if (!same_payment(rc, cl))
				continue;
			any = 1;
			if (matches(rc, cl))
				match = 1;
		}
		if (any && !match)
			questions++;
	}
	return questions;
}

int64_t mor_money_paid_toward(mor_world *w, const uint8_t obligation[32])
{
	const mor_act *ob = mor_world_get(w, obligation);
	if (!ob)
		return -1;
	if (!is_money(w, ob, 1))
		return 0;
	const uint8_t *creditor = mor_pl_hash(ob, 1);
	const mor_cbor *unit = &mor_pl(ob, 2)->items[0];
	const mor_act *pay[256];
	uint64_t amt[256];
	size_t np = 0;
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *a = &w->acts[i];
		int rcpt = is_money(w, a, 2), clm = is_money(w, a, 3);
		if (!rcpt && !clm)
			continue;
		/* What the payment fulfils: only an act to which a rule gives a payment
		 * following it a meaning; here, the obligation itself. */
		if (!mor_hash_eq(mor_pl_hash(a, rcpt ? 5 : 4), obligation))
			continue;
		if (!mor_hash_eq(mor_pl_hash(a, rcpt ? 3 : 2), creditor) || !counts(w, a))
			continue;
		const mor_cbor *am = mor_pl(a, rcpt ? 4 : 3);
		if (memcmp(am->items[0].data, unit->data, 32))
			continue;
		size_t k;
		for (k = 0; k < np; k++)
			if (same_payment(pay[k], a))
				break;
		if (k == np) {
			if (np >= 256)
				continue;
			pay[np] = a;
			amt[np] = 0;
			np++;
		}
		if (am->items[1].v > amt[k])
			amt[k] = am->items[1].v; /* the greater amount counts (rule 11) */
	}
	uint64_t sum = 0;
	for (size_t k = 0; k < np; k++)
		sum += amt[k];
	return sum > INT64_MAX ? INT64_MAX : (int64_t)sum;
}
