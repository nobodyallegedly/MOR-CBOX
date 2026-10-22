/* Test-only: build acts from CBOR diagnostic notation, lock and sign them,
 * so tests can make exactly the acts a rule is about. Allocates freely (it is
 * test code, outside the library's memory discipline). */
#ifndef MOR_TEST_BUILDER_H
#define MOR_TEST_BUILDER_H

#include <stddef.h>
#include <stdint.h>

/* Encode a subset of CBOR diagnostic notation deterministically: unsigned and
 * negative integers, h'hex', "text" (no escapes), [..], {..} (keys sorted by
 * their encoding), true, false, null. Returns the length, 0 on error. */
size_t tb_diag(const char *s, uint8_t *out, size_t cap);

typedef struct tb_key {
	int scheme;        /* 1: Schnorr; 3: SLH-DSA-SHA2-128f */
	uint8_t sk[32];    /* Schnorr secret */
	uint8_t seed[48];  /* SLH-DSA sk_seed || sk_prf || pk_seed */
	uint8_t pk[32];
} tb_key;

void tb_key_schnorr(tb_key *k, const char *label);
void tb_key_slh(tb_key *k, const char *label);
/* The chain-key commitment [3, tagged_hash("MOR/safety", 3 || pk)] in hex. */
void tb_commit_hex(const tb_key *k, char out[65]);
void tb_hex(const uint8_t *p, size_t n, char *out);

typedef struct tb_act {
	uint8_t bytes[65536];
	size_t len;
	uint8_t id[32];
	char idhex[65];
} tb_act;

/* Build an act from its inside (diagnostic notation) and its outside fields
 * (signer and binding as hex, or NULL; `to` as a CBOR diag list or NULL). The
 * act is public unless private_ is set. Optional overrides for tests of the
 * envelope: a signature corrupted, a scheme by hash. */
typedef struct tb_opts {
	int private_;
	int corrupt_sig;
	int scheme_hash;    /* sign under a scheme named by a 32-byte hash */
	const char *to;     /* diag of a [+ hash] list */
} tb_opts;

int tb_make(tb_act *out, const char *inside_diag, const char *signer_hex, const char *binding_hex, const tb_key *k,
	    const tb_opts *o);

#endif
