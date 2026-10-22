/* Money: double entry (rule 11) and what counts toward an obligation. The
 * core reads no rail: the rail Module's answer is a fact the verifier is
 * given (world rails). */
#ifndef MOR_MONEY_H
#define MOR_MONEY_H

#include "world.h"

/* How many open questions a receiver shows: claims of a payment whose
 * receipts by the receiver disagree with them and none matches (rule 11). */
int mor_money_disagreements(mor_world *w, const uint8_t receiver[32]);

/* What counts toward an obligation: the sum, in its unit, of the payments
 * whose receipt or claim names it in "fulfils", to its creditor, with a valid
 * rail answer, each payment counted once at its greater amount. -1 unknown. */
int64_t mor_money_paid_toward(mor_world *w, const uint8_t obligation[32]);

#endif
