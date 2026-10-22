/* Payload formats of every act type the six MIRs define (each MIR's Part
 * three), and the shape rules decided from an act's own bytes. An act outside
 * its specification's format is invalid (Envelopes rule 30). */
#ifndef MOR_FORMATS_H
#define MOR_FORMATS_H

#include "world.h"

/* Is the act in its MIR's format? Returns 1 in format, 0 invalid. Also sets
 * *why to a short reason when not. Only for acts whose spec is a MIR. */
int mor_format_ok(const mor_world *w, const mor_act *a, const char **why);

/* Shared CDDL pieces (Identity Part three, "Shared parts"). */
int mor_f_signing_key(const mor_cbor *c);
int mor_f_chain_commit(const mor_cbor *c);
int mor_f_kept_tip(const mor_cbor *c);
int mor_f_amount(const mor_cbor *c);
int mor_f_uint(const mor_cbor *c);
int mor_f_tstr(const mor_cbor *c);
int mor_f_arr(const mor_cbor *c, uint64_t n);
int mor_f_hash_or_null(const mor_cbor *c);
/* An ascending list of hashes, none twice. */
int mor_f_hashes_ascending(const mor_cbor *c);

/* Agreements: may acts of this type carry acknowledgements? Identity, Money
 * and Agreements types may (Envelopes rule 18). */
int mor_type_may_ack(const mor_world *w, const mor_act *a);

#endif
