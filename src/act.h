/* An act (Envelopes, Part three, "The act"): decoded, opened and checked for
 * shape. Nothing here judges standing; that is the layers' work. */
#ifndef MOR_ACT_H
#define MOR_ACT_H

#include <stddef.h>
#include <stdint.h>

#include "arena.h"
#include "cbor.h"

/* The first check that fails when an act is decoded and opened, in the order
 * open-act.json gives: locked hash, key, unlock, inside commitment, inside
 * CBOR, shape, canonical text. */
enum mor_open {
	MOR_OPEN_OK = 0,
	MOR_OPEN_CBOR,             /* the act or its inside is not deterministic CBOR (Envelopes rules 1, 2) */
	MOR_OPEN_SHAPE,            /* not in the shape of "The act" (rule 1) */
	MOR_OPEN_LOCKED_HASH,      /* locked bytes do not hash to the locked hash (rule 3) */
	MOR_OPEN_NO_KEY,           /* a private act, with no key to open it (rule 10) */
	MOR_OPEN_UNLOCK,           /* the content key does not open the locked bytes (rule 7) */
	MOR_OPEN_INSIDE_COMMITMENT,/* the inside does not match its commitment (rule 3) */
	MOR_OPEN_TEXT,             /* a text field that is not canonical (rule 5, Text rule 1) */
	MOR_OPEN_NOMEM,
};
const char *mor_open_name(int r);

enum mor_scheme_kind { MOR_SCHEME_BAD = 0, MOR_SCHEME_NUM, MOR_SCHEME_HASH };

typedef struct mor_sigval {
	int kind;            /* MOR_SCHEME_NUM or MOR_SCHEME_HASH */
	uint64_t num;        /* 1, 2 or 3 */
	const uint8_t *hash; /* 32-byte spec hash */
	const uint8_t *key;
	size_t key_len;
	const uint8_t *sig;
	size_t sig_len;
} mor_sigval;

typedef struct mor_act {
	const uint8_t *bytes;
	size_t len;
	uint8_t id[32];
	int open;            /* enum mor_open */
	int text_rule;       /* the Text rule broken when open == MOR_OPEN_TEXT */
	int cbor_err;

	/* outside */
	const mor_cbor *outside;
	const uint8_t *signer, *binding, *commit, *locked_hash, *nonce, *key;
	const mor_cbor *to;
	const mor_cbor *locked;
	mor_sigval sig;
	int sig_shape_ok;    /* the signature array is in shape */

	/* inside, once opened */
	uint8_t *inside_bytes;
	size_t inside_len;
	const mor_cbor *inside;
	const uint8_t *spec;
	uint64_t type;
	const mor_cbor *prev, *objects, *payload, *acks, *refs, *hint;
	int has_pos;
	uint64_t pos;
	const uint8_t *summary;
	const uint8_t *salt;

	/* Set by the world: the act is in its MIR's format (always 1 for an act
	 * of a specification that is no MIR). Typed reads require it. */
	int fmt_ok;
} mor_act;

/* Decode an act's bytes and, if public (or a key is given), open it. Every
 * pointer in *act points into bytes or the arena. */
int mor_act_decode(mor_act *act, const uint8_t *bytes, size_t len, const uint8_t *key, mor_arena *a);

/* Open-act vector form: an outside and locked bytes given apart. */
int mor_act_open_parts(mor_act *act, const uint8_t *outside, size_t outside_len, const uint8_t *locked,
		       size_t locked_len, mor_arena *a);

/* Every text string anywhere in the item is canonical text. Returns 0, or the
 * Text rule broken (7 to 12), or -1 when out of arena. */
int mor_cbor_texts_canonical(const mor_cbor *c, mor_arena *a);

/* Signature check over the act id: 1 valid, 0 invalid, -1 scheme not
 * implemented (a scheme named by hash). Lengths of founding schemes are shape. */
int mor_act_sig_verify(const mor_act *act);

/* Chain-key commitment: tagged_hash("MOR/safety", scheme || key). */
void mor_chain_commit(uint8_t out[32], const mor_sigval *s);

/* Small accessors. */
int mor_hash_list(const mor_cbor *c, int allow_empty); /* [* hash] / [+ hash] */
int mor_act_names_in(const mor_cbor *hashlist, const uint8_t id[32]);

#endif
