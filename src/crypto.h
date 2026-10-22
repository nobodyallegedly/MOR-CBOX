/* The cryptographic primitives MOR uses, each from an established library:
 * SHA-256 and XChaCha20-Poly1305 and X25519 (libsodium), BIP-340 (libsecp256k1),
 * SLH-DSA, ML-KEM-768, SHA3-256 and SHAKE256 (OpenSSL). Nothing here is MOR's own
 * but tagged_hash, which Identity defines. */
#ifndef MOR_CRYPTO_H
#define MOR_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

void mor_sha256(uint8_t out[32], const uint8_t *p, size_t n);
/* tagged_hash(tag, x) = SHA-256(SHA-256(tag) || SHA-256(tag) || x) (Identity, "Hashes"). */
void mor_tagged_hash(uint8_t out[32], const char *tag, const uint8_t *x, size_t n);
/* The same over two concatenated parts, x1 || x2. */
void mor_tagged_hash2(uint8_t out[32], const char *tag, const uint8_t *x1, size_t n1,
		      const uint8_t *x2, size_t n2);

/* XChaCha20-Poly1305, no associated data; locked = ciphertext || tag.
 * open: returns 0 and writes len-16 bytes on success, -1 on failure. */
int mor_xchacha_open(uint8_t *plain, const uint8_t *locked, size_t len, const uint8_t nonce[24],
		     const uint8_t key[32]);
void mor_xchacha_seal(uint8_t *locked, const uint8_t *plain, size_t len, const uint8_t nonce[24],
		      const uint8_t key[32]);

/* Signature verification: 1 valid, 0 not valid. */
int mor_schnorr_verify(const uint8_t pk[32], const uint8_t msg[32], const uint8_t sig[64]);
/* SLH-DSA-SHA2-128s (scheme 2) or -128f (scheme 3), pure, context "MOR".
 * Returns 1 valid, 0 not valid, -1 when the backend failed. */
int mor_slhdsa_verify(int scheme, const uint8_t pk[32], const uint8_t *msg, size_t msglen,
		      const uint8_t *sig, size_t siglen, const uint8_t *ctx, size_t ctxlen);
size_t mor_slhdsa_sig_len(int scheme);

/* X-Wing (draft-connolly-cfrg-xwing-kem-11) over ML-KEM-768 and X25519. */
#define MOR_XWING_PK 1216
#define MOR_XWING_CT 1120
int mor_xwing_public_from_secret(uint8_t pk[MOR_XWING_PK], const uint8_t sk[32]);
int mor_xwing_decaps(uint8_t ss[32], const uint8_t ct[MOR_XWING_CT], const uint8_t sk[32]);
int mor_xwing_encaps_derand(uint8_t ct[MOR_XWING_CT], uint8_t ss[32], const uint8_t pk[MOR_XWING_PK],
			    const uint8_t eseed[64]);

/* For reproducing published values in tests: SLH-DSA key from seeds, and
 * signing (deterministic when opt_rand is NULL). */
int mor_slhdsa_keygen(int scheme, uint8_t pk[32], const uint8_t sk_seed[16], const uint8_t sk_prf[16],
		      const uint8_t pk_seed[16]);
int mor_slhdsa_sign(int scheme, uint8_t *sig, const uint8_t sk_seed[16], const uint8_t sk_prf[16],
		    const uint8_t pk_seed[16], const uint8_t *msg, size_t msglen, const uint8_t *ctx,
		    size_t ctxlen, const uint8_t *opt_rand);

#endif
