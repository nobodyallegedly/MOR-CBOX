/* The verdict of an act at a verifier implementing the six MIRs and no cMIP:
 * invalid by any rule it can apply, else unknown under a specification it does
 * not implement, else its standing (Envelopes rule 30, "Invalid beats unknown"). */
#ifndef MOR_VERDICT_H
#define MOR_VERDICT_H

#include "world.h"

int mor_verdict(mor_world *w, const uint8_t id[32]);

#endif
