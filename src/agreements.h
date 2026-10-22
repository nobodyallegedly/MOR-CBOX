/* Agreements: the questions the verifier answers about agreements, keepers,
 * collectives' consent, grants and deadlines (Agreements MIR). */
#ifndef MOR_AGREEMENTS_H
#define MOR_AGREEMENTS_H

#include "world.h"

/* Agreements rule 41 with Identity rule 34: do the keepers of an agreement
 * the act belongs to rescue it from `ending` (a rotation that voided it, or a
 * grant's revocation), each keeper's record of the act lying before its own
 * record of the ending, counted by the agreement's keepers' rule? */
/* 1: a keeper's record rescues the act; 0: none; -1: cannot tell. */
int mor_keepers_rescue(mor_world *w, const mor_act *a, const mor_act *ending);

/* Agreements rule 1: an agreement exists once its signing rule is met. */
enum mor_exists { MOR_AGR_EXISTS = 1, MOR_AGR_NOT = 0, MOR_AGR_UNKNOWN = -1, MOR_AGR_INVALID = -2 };
int mor_agreement_exists(mor_world *w, const uint8_t terms[32]);

/* Agreements rule 47: two complete versions of one version of a deal. */
typedef struct mor_fork {
	int branches;
	uint8_t reference[32];
	int tangled;
} mor_fork;
int mor_deal_fork(mor_world *w, const uint8_t version[32], mor_fork *out);

/* Agreements rules 57, 58, 60, 65, 72, 73: does an act of a collective have
 * the consent its areas need? 1 counts, 0 does not, -1 unknown. */
int mor_consent_counts(mor_world *w, const uint8_t act[32]);

/* Agreements rules 73 to 80: an act signed with a grant key. */
enum mor_backing {
	MOR_BK_BACKED = 0,  /* within the grant, no ending held */
	MOR_BK_BINDS,       /* the ending's history holds it, or the grantor adopted it */
	MOR_BK_VOID,        /* racing or after an ending, relied on by nobody */
	MOR_BK_DISPUTED,    /* racing an ending, acknowledged by another or recorded by a keeper first */
	MOR_BK_NOT_BACKED,  /* not within the grant (reach, key, citation, acceptance) */
	MOR_BK_UNKNOWN,
};
const char *mor_backing_name(int b);
int mor_backing(mor_world *w, const uint8_t act[32]);

/* Agreements rule 33: a deadline on a clock, under the keepers named and
 * their rule (terms field 3's form). */
enum mor_deadline { MOR_DL_MET_ANCHOR, MOR_DL_MET_KEEPERS, MOR_DL_MISSED, MOR_DL_UNDETERMINED };
const char *mor_deadline_name(int d);
int mor_deadline(mor_world *w, const uint8_t act[32], const uint8_t (*keepers)[32], size_t nkeepers,
		 const mor_cbor *rule, const uint8_t *clock, size_t clock_len, uint64_t point);

/* A collective's fork (type 19) or closing (type 20): complete 1, not 0,
 * unknown -1 (rule 91; "Fork (type 19)", "Closing (type 20)"). */
int mor_ending_complete(mor_world *w, const uint8_t id[32]);

/* "Belongs to": the agreements an act belongs to (up to max), by id. */
size_t mor_belongs_to(mor_world *w, const mor_act *a, const uint8_t **out, size_t max, int *unknown);

/* Does `from`'s history (what it cites through prev and objects on `chain`,
 * transitively) hold `id`? */
int mor_history_holds(mor_world *w, const mor_act *from, const uint8_t chain[32], const uint8_t id[32]);

int mor_acknowledged_by_other(mor_world *w, const mor_act *a, const uint8_t *except);

#endif
