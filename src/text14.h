/* Text rule 14: what a text act carries, its `objects` above all. */
#ifndef MOR_TEXT14_H
#define MOR_TEXT14_H

#include "world.h"

/* For a text act (Text, type 0): VALID, INVALID, or UNKNOWN when the
 * verifier does not hold what the rule needs (the signer's chain, the
 * decision named, the grant or member signature its binding names). */
int mor_text14(mor_world *w, const mor_act *a);

/* Is `id` a decision of collective C: a link of its identity chain or one of
 * its records (Agreements rule 58)? 1 yes, 0 no, -1 not held. */
int mor_is_decision(mor_world *w, const uint8_t c[32], const uint8_t id[32]);

#endif
