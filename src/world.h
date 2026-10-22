/* The verifier's world for one question: the acts it holds and the facts it
 * holds beside them (anchors, inclusion proofs, rail answers, specifications'
 * content). Every verdict is computed from this and nothing else. */
#ifndef MOR_WORLD_H
#define MOR_WORLD_H

#include <stddef.h>
#include <stdint.h>

#include "act.h"
#include "arena.h"

/* The six MIRs, by their spec hashes (stand-ins until the freeze). */
enum mor_mir { MOR_MIR_NONE = -1, MOR_IDENTITY = 0, MOR_ENVELOPES, MOR_TEXT, MOR_MONEY, MOR_AGREEMENTS,
	       MOR_DEVELOPMENT, MOR_NMIRS };

/* Standings, the words the texts use. */
enum mor_status {
	MOR_ST_VALID = 0,
	MOR_ST_INVALID,
	MOR_ST_UNKNOWN,
	MOR_ST_PENDING,
	MOR_ST_VOID,
	MOR_ST_DISPUTED,
	MOR_ST_SCOPED,
	MOR_ST_CONTESTED,
};
const char *mor_status_name(int st);

typedef struct mor_anchor {
	uint8_t act[32];
	const uint8_t *clock; /* the clock, as given (bytes) */
	size_t clock_len;
	uint64_t point;
} mor_anchor;

typedef struct mor_incl_proof {
	uint8_t summary[32];  /* the log summary's act id */
	int has_receipt;
	uint8_t receipt[32];  /* the receipt, when the proof names it */
	uint64_t index;
	const uint8_t (*path)[32];
	size_t npath;
} mor_incl_proof;

typedef struct mor_rail_answer {
	uint8_t act[32];      /* a receipt or claim */
	int valid;            /* the rail Module's answer: valid */
	int has_flow;
	uint8_t paid_to_flow[32]; /* the payee pointer it was paid to */
} mor_rail_answer;

typedef struct mor_spec_content {
	uint8_t hash[32];
	const uint8_t *bytes;
	size_t len;
} mor_spec_content;

typedef struct mor_world {
	mor_arena *a;
	uint8_t mir[MOR_NMIRS][32];
	mor_act *acts;        /* sorted by id */
	size_t nacts;
	mor_anchor *anchors;
	size_t nanchors;
	mor_incl_proof *proofs;
	size_t nproofs;
	mor_rail_answer *rails;
	size_t nrails;
	mor_spec_content *specs;
	size_t nspecs;
	/* Identity homes the verifier tried and failed to reach (by operator). */
	const uint8_t (*unreached)[32];
	size_t nunreached;
	/* Caches, built lazily. */
	struct mor_chain **chains;
	size_t nchains, chains_cap;
	unsigned depth;       /* recursion guard for resolving operators and agreements */
} mor_world;

/* Build a world from encoded acts. Each act is decoded (and opened with its
 * public key); acts that do not decode are kept, so that a question about
 * them is answered invalid. Duplicate ids keep the copy whose signature is
 * valid (Envelopes, "How it fits together", step 3). */
int mor_world_init(mor_world *w, mor_arena *a, const uint8_t mir[MOR_NMIRS][32], const uint8_t *const *acts,
		   const size_t *lens, size_t n);

const mor_act *mor_world_get(const mor_world *w, const uint8_t id[32]);
int mor_world_mir_of(const mor_world *w, const uint8_t spec[32]);
int mor_world_is_mir(const mor_world *w, const mor_act *a, int mir);

/* Payload helpers. */
const mor_cbor *mor_pl(const mor_act *a, uint64_t key);
const uint8_t *mor_pl_hash(const mor_act *a, uint64_t key);
int mor_hash_eq(const uint8_t *x, const uint8_t *y);
/* Does the act name `id` in its objects (as chain or predecessor)? */
int mor_objects_name(const mor_act *a, const uint8_t id[32]);
/* Does the act name the entry [chain, pred] in its objects? */
int mor_objects_has(const mor_act *a, const uint8_t chain[32], const uint8_t pred[32]);
/* Does the act acknowledge id? */
int mor_acks_name(const mor_act *a, const uint8_t id[32]);
/* The single prev entry, or NULL. */
const uint8_t *mor_prev(const mor_act *a);
/* The signer: outside key 0, or for a genesis its own id. */
const uint8_t *mor_signer(const mor_act *a);

#endif
