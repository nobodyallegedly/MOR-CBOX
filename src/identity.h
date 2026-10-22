/* Identity: identity chains, which rotation counts, receipts and audit, the
 * homeless path and the escape, and the standing of every act (Identity MIR,
 * Parts one and two). */
#ifndef MOR_IDENTITY_H
#define MOR_IDENTITY_H

#include "world.h"

/* What stops a chain (Identity, "What stops a chain"): the seven reasons. */
enum mor_next {
	MOR_NEXT_END = 0,          /* the end of what is held */
	MOR_NEXT_PENDING,
	MOR_NEXT_CONTESTED,
	MOR_NEXT_UNKNOWN,          /* cannot tell */
	MOR_NEXT_NO_GENESIS,
	MOR_NEXT_GENESIS_UNKNOWN,
	MOR_NEXT_GENESIS_INVALID,
};
const char *mor_next_name(int n);

typedef struct mor_chain_pos {
	const mor_act *act;           /* the act counting at this position */
	const mor_cbor *signing_key;  /* [scheme, key] in effect after it */
	const mor_act *key_act;       /* the genesis or rotation that set that key */
	const mor_cbor *commit;       /* the next chain-key commitment */
	const mor_cbor *homes;        /* [+ home] in effect after it */
	const mor_cbor *home_rule;    /* NULL: the default */
	const mor_cbor *audit;        /* NULL: none required */
	int unaudited;                /* counts only through the verifier's own failed attempt */
} mor_chain_pos;

typedef struct mor_dishonest {
	uint8_t op[32];
	uint64_t from;
} mor_dishonest;

typedef struct mor_chain {
	uint8_t id[32];
	int state;            /* 0 none, 1 in progress, 2 done */
	int genesis_status;   /* MOR_ST_VALID, MOR_ST_INVALID or MOR_ST_UNKNOWN (not held) */
	mor_chain_pos *pos;
	size_t npos, cap;
	int next;             /* what stops the chain */
	mor_dishonest dishonest[16];
	size_t ndishonest;
} mor_chain;

/* The identity chain of `id`, as the held acts show it (cached). */
const mor_chain *mor_chain_get(mor_world *w, const uint8_t id[32]);

/* Questions about an act's standing under Identity. */
enum mor_status_flags {
	MOR_Q_KEEPERS = 1, /* "status after keepers": Agreements' keeper records rescue as rule 41 says */
};
int mor_status(mor_world *w, const uint8_t id[32], int flags);
/* The same, for an act already in hand. */
int mor_status_act(mor_world *w, const mor_act *a, int flags);

/* Is `a` an identity-chain act (genesis, rotation, chain signature)? */
int mor_is_chain_act(const mor_world *w, const mor_act *a);
/* The position of an identity-chain act (genesis 0), or -1. */
int64_t mor_chain_act_pos(const mor_world *w, const mor_act *a);
/* The position at which `act` counts in its signer's chain, or -1. */
int64_t mor_chain_position_of(const mor_chain *c, const uint8_t id[32]);
/* Is the identity a collective (its counting chain declares an agreement,
 * Agreements kind 0)? 1 yes, 0 no, -1 unknown. Writes the declared value. */
int mor_identity_agreement(mor_world *w, const uint8_t id[32], const mor_cbor **value);

/* Succession (Identity, section 8): the successor field of the counting
 * rotation with the highest position that carries one; followed only once
 * the successor confirms it, by a link confirmation naming the rotation or a
 * witness act acknowledging it. */
enum mor_succession { MOR_SUCC_NONE = 0, MOR_SUCC_DECLARED, MOR_SUCC_FOLLOWED, MOR_SUCC_UNKNOWN, MOR_SUCC_OTHER };
typedef struct mor_succ {
	int seen;                       /* enum mor_succession */
	const uint8_t *successor;       /* a MOR identity's hash */
	const mor_act *rotation;        /* the rotation declaring it */
	const mor_act *confirmation;
} mor_succ;
int mor_succession(mor_world *w, const uint8_t id[32], mor_succ *out);
const char *mor_succession_name(int s);

/* The rotation that voided an act, when its standing is void or disputed
 * because of one (for Agreements' keepers). */
const mor_act *mor_voiding_rotation(mor_world *w, const mor_act *a);

#endif
