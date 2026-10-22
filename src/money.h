/* Money: double entry (rule 11) and what counts toward an obligation. The
 * core reads no rail: the rail Module's answer is a fact the verifier is
 * given (world rails). */
#ifndef MOR_MONEY_H
#define MOR_MONEY_H

#include "world.h"

/* Which payment a receipt or claim is, is the rail Module's answer, given as
 * a fact (world payments). An answer resting on one pair of records whose
 * payment is not stated is judged both ways and stands where both agree; on
 * two or more, it is unknown (rule 12). */

/* How many open questions a receiver shows: claims of a payment whose
 * receipts by the receiver disagree with them and none matches (rule 11).
 * -1 unknown. */
int mor_money_disagreements(mor_world *w, const uint8_t receiver[32]);

/* What counts toward an obligation: the sum, in its unit, of the payments
 * whose receipt or claim names it in "fulfils", to its creditor, with a valid
 * rail answer, each payment counted once at its greater amount. -1 unknown. */
int64_t mor_money_paid_toward(mor_world *w, const uint8_t obligation[32]);

/* What is repaid toward a refund owed to `to` on a payment (named by one of
 * its records): receipts and claims paying `to` in the unit and naming a
 * record of that payment. -1 unknown. */
int64_t mor_money_refund_repaid(mor_world *w, const uint8_t payment[32], const uint8_t to[32],
				const uint8_t unit[32]);

/* A vow's sales (Agreements rule 13): each payment under an offer selling the
 * vow that a payer's claim signs onto is a sale, pending, confirmed (a claim
 * of the buyer acknowledging an act about the vow) or contested (the buyer's
 * contest naming it); a payment no claim signs onto is unsigned. */
#define MOR_VOW_MAX 64
enum mor_sale_state { MOR_SALE_PENDING = 0, MOR_SALE_CONFIRMED, MOR_SALE_CONTESTED };
typedef struct mor_vow_sale {
	const mor_act *act;      /* the claim that signs on, or the confirming one */
	const uint8_t *buyer;
	int state;
} mor_vow_sale;
typedef struct mor_vow {
	int unknown_all;         /* the whole answer is unknown */
	mor_vow_sale sales[MOR_VOW_MAX];
	size_t nsales;
	const mor_act *unsigned_[MOR_VOW_MAX];
	size_t nunsigned;
	const mor_act *unknown[MOR_VOW_MAX]; /* records whose place is unknown */
	size_t nunknown;
} mor_vow;
int mor_money_vow(mor_world *w, const uint8_t vow[32], mor_vow *out);

#endif
