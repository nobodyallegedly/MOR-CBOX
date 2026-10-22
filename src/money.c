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

static int standing_ok(mor_world *w, const mor_act *a)
{
	int st = mor_status_act(w, a, 0);
	return st == MOR_ST_VALID || st == MOR_ST_SCOPED;
}

/* Rule 10: a receipt or claim is treated as valid only when its rail answer is. */
static int counts(mor_world *w, const mor_act *a)
{
	const mor_rail_answer *r = rail_of(w, a->id);
	return r && r->valid && standing_ok(w, a);
}

/* ---- the same payment (rule 12) ---- */

/* Which payment a record's proof is, as the rail Module's answer says, or NULL. */
static const mor_rail_payment *payment_of(const mor_world *w, const mor_act *a)
{
	const mor_cbor *pr = mor_pl(a, 1);
	for (size_t i = 0; pr && i < w->npayments; i++)
		if (w->payments[i].proof_len == pr->v && !memcmp(w->payments[i].proof, pr->data, (size_t)pr->v))
			return &w->payments[i];
	return NULL;
}

/* Is a pair of records one payment? 1 or 0 from the rail's answer; -1 where
 * the answer is not held for both, whatever the proofs' bytes. Records on
 * different rail Modules are never one payment. */
static int rail_same(const mor_world *w, const mor_act *x, const mor_act *y)
{
	if (x == y)
		return 1;
	if (memcmp(mor_pl(x, 0)->data, mor_pl(y, 0)->data, 32))
		return 0;
	const mor_rail_payment *px = payment_of(w, x), *py = payment_of(w, y);
	if (!px || !py)
		return -1;
	return px->payment_len == py->payment_len && !memcmp(px->payment, py->payment, px->payment_len);
}

/* An answer is computed under a context: the one pair whose sameness is
 * taken one way (assume), and the other unknown pairs the answer asked. */
#define MAX_PAIRS 4
typedef struct same_ctx {
	const mor_act *ox, *oy;
	int assume;
	const mor_act *ux[MAX_PAIRS], *uy[MAX_PAIRS];
	size_t nu;
} same_ctx;

static int same(const mor_world *w, same_ctx *c, const mor_act *x, const mor_act *y)
{
	int k = rail_same(w, x, y);
	if (k >= 0)
		return k;
	if ((x == c->ox && y == c->oy) || (x == c->oy && y == c->ox))
		return c->assume;
	for (size_t i = 0; i < c->nu; i++)
		if ((c->ux[i] == x && c->uy[i] == y) || (c->ux[i] == y && c->uy[i] == x))
			return 0;
	if (c->nu < MAX_PAIRS) {
		c->ux[c->nu] = x;
		c->uy[c->nu] = y;
	}
	c->nu++;
	return 0;
}

#define UNKNOWN_ANSWER (-1)

/* Rule 12, "Where the rail's answer is not held": an answer resting on no
 * unknown pair stands; on one, it is judged both ways and stands where both
 * agree; on two or more, it is unknown. */
typedef int64_t (*scalar_fn)(mor_world *w, same_ctx *c, const void *arg);

static int64_t both_ways(mor_world *w, scalar_fn fn, const void *arg)
{
	same_ctx c0;
	memset(&c0, 0, sizeof c0);
	int64_t v = fn(w, &c0, arg);
	if (c0.nu == 0 || v == UNKNOWN_ANSWER)
		return v;
	if (c0.nu > 1)
		return UNKNOWN_ANSWER;
	int64_t r[2];
	for (int k = 0; k < 2; k++) {
		same_ctx c;
		memset(&c, 0, sizeof c);
		c.ox = c0.ux[0];
		c.oy = c0.uy[0];
		c.assume = k;
		r[k] = fn(w, &c, arg);
		if (c.nu)
			return UNKNOWN_ANSWER; /* a second pair, asked under one way */
	}
	return r[0] == r[1] ? r[0] : UNKNOWN_ANSWER;
}

/* ---- double entry (rule 11) ---- */

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

static int64_t disagreements_fn(mor_world *w, same_ctx *c, const void *arg)
{
	const uint8_t *receiver = arg;
	int64_t questions = 0;
	for (size_t i = 0; i < w->nacts; i++) {
		const mor_act *cl = &w->acts[i];
		if (!is_money(w, cl, 3) || !counts(w, cl))
			continue;
		int any = 0, match = 0;
		for (size_t j = 0; j < w->nacts; j++) {
			const mor_act *rc = &w->acts[j];
			if (!is_money(w, rc, 2) || !mor_hash_eq(rc->signer, receiver) || !counts(w, rc))
				continue;
			if (!same(w, c, rc, cl))
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

int mor_money_disagreements(mor_world *w, const uint8_t receiver[32])
{
	return (int)both_ways(w, disagreements_fn, receiver);
}

/* ---- what counts toward an obligation ---- */

#define MAX_PAY 256

/* Group records into payments (by `same`) and sum each payment once, at the
 * greater amount its records show (rule 11). */
static int64_t sum_payments(mor_world *w, same_ctx *c, const mor_act *const *rec, const uint64_t *amount, size_t n)
{
	const mor_act *pay[MAX_PAY];
	uint64_t amt[MAX_PAY];
	size_t np = 0;
	for (size_t i = 0; i < n; i++) {
		size_t k;
		for (k = 0; k < np; k++)
			if (same(w, c, pay[k], rec[i]))
				break;
		if (k == np) {
			if (np >= MAX_PAY)
				continue;
			pay[np] = rec[i];
			amt[np] = 0;
			np++;
		}
		if (amount[i] > amt[k])
			amt[k] = amount[i];
	}
	uint64_t sum = 0;
	for (size_t k = 0; k < np; k++)
		sum = sum + amt[k] < sum ? UINT64_MAX : sum + amt[k];
	return sum > INT64_MAX ? INT64_MAX : (int64_t)sum;
}

static int64_t paid_toward_fn(mor_world *w, same_ctx *c, const void *arg)
{
	const mor_act *ob = arg;
	const uint8_t *creditor = mor_pl_hash(ob, 1);
	const mor_cbor *unit = &mor_pl(ob, 2)->items[0];
	const mor_act *rec[MAX_PAY];
	uint64_t amt[MAX_PAY];
	size_t n = 0;
	for (size_t i = 0; i < w->nacts && n < MAX_PAY; i++) {
		const mor_act *a = &w->acts[i];
		int rcpt = is_money(w, a, 2), clm = is_money(w, a, 3);
		if (!rcpt && !clm)
			continue;
		/* What the payment fulfils: only an act to which a rule gives a payment
		 * following it a meaning; here, the obligation itself. */
		if (!mor_hash_eq(mor_pl_hash(a, rcpt ? 5 : 4), ob->id))
			continue;
		if (!mor_hash_eq(mor_pl_hash(a, rcpt ? 3 : 2), creditor) || !counts(w, a))
			continue;
		const mor_cbor *am = mor_pl(a, rcpt ? 4 : 3);
		if (memcmp(am->items[0].data, unit->data, 32))
			continue;
		rec[n] = a;
		amt[n] = am->items[1].v;
		n++;
	}
	return sum_payments(w, c, rec, amt, n);
}

int64_t mor_money_paid_toward(mor_world *w, const uint8_t obligation[32])
{
	const mor_act *ob = mor_world_get(w, obligation);
	if (!ob)
		return -1;
	if (!is_money(w, ob, 1))
		return 0;
	return both_ways(w, paid_toward_fn, ob);
}

/* ---- refunds: what is repaid toward a refund owed on a payment ---- */

typedef struct refund_arg {
	const mor_act *payment;
	const uint8_t *to;
	const uint8_t *unit;
} refund_arg;

static int64_t refund_fn(mor_world *w, same_ctx *c, const void *arg)
{
	const refund_arg *ra = arg;
	const mor_act *rec[MAX_PAY];
	uint64_t amt[MAX_PAY];
	size_t n = 0;
	for (size_t i = 0; i < w->nacts && n < MAX_PAY; i++) {
		const mor_act *a = &w->acts[i];
		int rcpt = is_money(w, a, 2), clm = is_money(w, a, 3);
		if (!rcpt && !clm)
			continue;
		/* paid to the one owed: a receipt or claim naming it as payee */
		if (!mor_hash_eq(mor_pl_hash(a, rcpt ? 3 : 2), ra->to) || !counts(w, a))
			continue;
		const mor_cbor *am = mor_pl(a, rcpt ? 4 : 3);
		if (memcmp(am->items[0].data, ra->unit, 32))
			continue;
		/* naming a record of the payment the refund is owed on */
		const mor_act *f = mor_world_get(w, mor_pl_hash(a, rcpt ? 5 : 4));
		if (!f || !(is_money(w, f, 2) || is_money(w, f, 3)) || f == a)
			continue;
		if (!same(w, c, f, ra->payment))
			continue;
		rec[n] = a;
		amt[n] = am->items[1].v;
		n++;
	}
	return sum_payments(w, c, rec, amt, n);
}

int64_t mor_money_refund_repaid(mor_world *w, const uint8_t payment[32], const uint8_t to[32],
				const uint8_t unit[32])
{
	const mor_act *p = mor_world_get(w, payment);
	if (!p || !(is_money(w, p, 2) || is_money(w, p, 3)))
		return -1;
	refund_arg ra = { p, to, unit };
	return both_ways(w, refund_fn, &ra);
}

/* ---- vows: the state of each sale (Agreements rule 13) ---- */

/* An offer selling the vow: an Agreements standing offer whose field 1 lists [3, vow]. */
static int sells_vow(const mor_world *w, const mor_act *o, const uint8_t vow[32])
{
	if (!mor_world_is_mir(w, o, MOR_AGREEMENTS) || o->type != 6)
		return 0;
	const mor_cbor *sold = mor_pl(o, 1);
	for (uint64_t i = 0; sold && i < sold->v; i++) {
		const mor_cbor *s = &sold->items[i];
		if (s->items[0].v == 3 && !memcmp(s->items[1].data, vow, 32))
			return 1;
	}
	return 0;
}

static int list_names(const mor_cbor *l, const uint8_t id[32])
{
	for (uint64_t i = 0; l && l->type == MOR_CBOR_ARRAY && i < l->v; i++) {
		const mor_cbor *x = &l->items[i];
		if (x->type == MOR_CBOR_BYTES && x->v == 32 && !memcmp(x->data, id, 32))
			return 1;
	}
	return 0;
}

/* An act about the vow: the vow itself, an act naming it in its refs or
 * objects, or an offer selling it; held and valid. */
static int about_vow(mor_world *w, const uint8_t id[32], const uint8_t vow[32])
{
	const mor_act *x = mor_world_get(w, id);
	if (!x || !standing_ok(w, x))
		return 0;
	return !memcmp(x->id, vow, 32) || list_names(x->refs, vow) || mor_objects_name(x, vow) || sells_vow(w, x, vow);
}

typedef struct vow_result {
	mor_vow_sale sales[MOR_VOW_MAX];
	size_t nsales;
	const mor_act *unsigned_[MOR_VOW_MAX];
	size_t nunsigned;
} vow_result;

static void vow_compute(mor_world *w, same_ctx *c, const uint8_t vow[32], vow_result *out)
{
	memset(out, 0, sizeof *out);
	const mor_act *rec[MOR_VOW_MAX];
	size_t n = 0;
	for (size_t i = 0; i < w->nacts && n < MOR_VOW_MAX; i++) {
		const mor_act *a = &w->acts[i];
		int rcpt = is_money(w, a, 2), clm = is_money(w, a, 3);
		if (!rcpt && !clm)
			continue;
		const mor_act *o = mor_world_get(w, mor_pl_hash(a, rcpt ? 5 : 4));
		if (!o || !sells_vow(w, o, vow) || !standing_ok(w, o) || !standing_ok(w, a))
			continue;
		rec[n++] = a;
	}
	/* Payments: each record joins the first payment it is the same as. */
	size_t group[MOR_VOW_MAX], ng = 0;
	const mor_act *rep[MOR_VOW_MAX];
	for (size_t i = 0; i < n; i++) {
		size_t k;
		for (k = 0; k < ng; k++)
			if (same(w, c, rep[k], rec[i]))
				break;
		if (k == ng)
			rep[ng++] = rec[i];
		group[i] = k;
	}
	for (size_t k = 0; k < ng; k++) {
		const mor_act *first = NULL, *confirm = NULL;
		for (size_t i = 0; i < n; i++) {
			const mor_act *a = rec[i];
			if (group[i] != k || !is_money(w, a, 3))
				continue;
			/* the earliest claim: one whose prev names no claim of this payment */
			int later = 0;
			for (size_t j = 0; j < n; j++)
				if (group[j] == k && rec[j] != a && is_money(w, rec[j], 3) &&
				    mor_hash_eq(mor_prev(a), rec[j]->id))
					later = 1;
			if (!later && !first)
				first = a;
			for (uint64_t q = 0; a->acks && q < a->acks->v; q++)
				if (about_vow(w, a->acks->items[q].data, vow))
					confirm = a;
		}
		if (!first && !confirm) {
			for (size_t i = 0; i < n && out->nunsigned < MOR_VOW_MAX; i++)
				if (group[i] == k)
					out->unsigned_[out->nunsigned++] = rec[i];
			continue;
		}
		if (!first)
			first = confirm;
		mor_vow_sale *s = &out->sales[out->nsales++];
		s->buyer = first->signer;
		s->act = confirm ? confirm : first;
		s->state = confirm ? MOR_SALE_CONFIRMED : MOR_SALE_PENDING;
		if (!confirm) {
			/* contested: the buyer signs a contest naming the sale */
			for (size_t i = 0; i < w->nacts; i++) {
				const mor_act *x = &w->acts[i];
				if (!mor_world_is_mir(w, x, MOR_AGREEMENTS) || x->type != 14 ||
				    !mor_hash_eq(x->signer, s->buyer) || !standing_ok(w, x))
					continue;
				for (size_t j = 0; j < n; j++)
					if (group[j] == k && mor_hash_eq(mor_pl_hash(x, 0), rec[j]->id))
						s->state = MOR_SALE_CONTESTED;
			}
		}
	}
}

static int sale_eq(const mor_vow_sale *a, const mor_vow_sale *b)
{
	return a->act == b->act && a->state == b->state && mor_hash_eq(a->buyer, b->buyer);
}

static void add_unknown(mor_vow *out, const mor_act *a)
{
	for (size_t i = 0; i < out->nunknown; i++)
		if (out->unknown[i] == a)
			return;
	if (out->nunknown < MOR_VOW_MAX)
		out->unknown[out->nunknown++] = a;
}

int mor_money_vow(mor_world *w, const uint8_t vow[32], mor_vow *out)
{
	memset(out, 0, sizeof *out);
	same_ctx c0;
	memset(&c0, 0, sizeof c0);
	static vow_result r0, r[2];
	vow_compute(w, &c0, vow, &r0);
	if (c0.nu > 1) {
		out->unknown_all = 1;
		return 0;
	}
	if (c0.nu == 0) {
		memcpy(out->sales, r0.sales, sizeof r0.sales);
		out->nsales = r0.nsales;
		memcpy(out->unsigned_, r0.unsigned_, sizeof r0.unsigned_);
		out->nunsigned = r0.nunsigned;
		return 0;
	}
	/* One pair: each entry found both ways stands, one found one way only is
	 * unknown (rule 12, a list is one answer). */
	for (int k = 0; k < 2; k++) {
		same_ctx c;
		memset(&c, 0, sizeof c);
		c.ox = c0.ux[0];
		c.oy = c0.uy[0];
		c.assume = k;
		vow_compute(w, &c, vow, &r[k]);
		if (c.nu) {
			out->unknown_all = 1;
			return 0;
		}
	}
	for (int k = 0; k < 2; k++) {
		const vow_result *a = &r[k], *b = &r[1 - k];
		for (size_t i = 0; i < a->nsales; i++) {
			int both = 0;
			for (size_t j = 0; j < b->nsales; j++)
				both |= sale_eq(&a->sales[i], &b->sales[j]);
			if (both) {
				int have = 0;
				for (size_t j = 0; j < out->nsales; j++)
					have |= sale_eq(&out->sales[j], &a->sales[i]);
				if (!have && out->nsales < MOR_VOW_MAX)
					out->sales[out->nsales++] = a->sales[i];
			} else {
				add_unknown(out, a->sales[i].act);
			}
		}
		for (size_t i = 0; i < a->nunsigned; i++) {
			int both = 0;
			for (size_t j = 0; j < b->nunsigned; j++)
				both |= a->unsigned_[i] == b->unsigned_[j];
			if (!both) {
				add_unknown(out, a->unsigned_[i]);
				continue;
			}
			int have = 0;
			for (size_t j = 0; j < out->nunsigned; j++)
				have |= out->unsigned_[j] == a->unsigned_[i];
			if (!have && out->nunsigned < MOR_VOW_MAX)
				out->unsigned_[out->nunsigned++] = a->unsigned_[i];
		}
	}
	return 0;
}
