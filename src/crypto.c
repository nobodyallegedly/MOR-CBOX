#include "crypto.h"

#include <string.h>

#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/params.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>
#include <sodium.h>

void mor_sha256(uint8_t out[32], const uint8_t *p, size_t n)
{
	crypto_hash_sha256(out, p, n);
}

void mor_tagged_hash2(uint8_t out[32], const char *tag, const uint8_t *x1, size_t n1,
		      const uint8_t *x2, size_t n2)
{
	uint8_t th[32];
	crypto_hash_sha256_state st;
	crypto_hash_sha256(th, (const uint8_t *)tag, strlen(tag));
	crypto_hash_sha256_init(&st);
	crypto_hash_sha256_update(&st, th, 32);
	crypto_hash_sha256_update(&st, th, 32);
	if (n1)
		crypto_hash_sha256_update(&st, x1, n1);
	if (n2)
		crypto_hash_sha256_update(&st, x2, n2);
	crypto_hash_sha256_final(&st, out);
}

void mor_tagged_hash(uint8_t out[32], const char *tag, const uint8_t *x, size_t n)
{
	mor_tagged_hash2(out, tag, x, n, NULL, 0);
}

int mor_xchacha_open(uint8_t *plain, const uint8_t *locked, size_t len, const uint8_t nonce[24],
		     const uint8_t key[32])
{
	unsigned long long mlen = 0;
	if (len < crypto_aead_xchacha20poly1305_ietf_ABYTES)
		return -1;
	if (crypto_aead_xchacha20poly1305_ietf_decrypt(plain, &mlen, NULL, locked, len, NULL, 0, nonce, key))
		return -1;
	return 0;
}

void mor_xchacha_seal(uint8_t *locked, const uint8_t *plain, size_t len, const uint8_t nonce[24],
		      const uint8_t key[32])
{
	unsigned long long clen = 0;
	crypto_aead_xchacha20poly1305_ietf_encrypt(locked, &clen, plain, len, NULL, 0, NULL, nonce, key);
}

int mor_schnorr_verify(const uint8_t pk[32], const uint8_t msg[32], const uint8_t sig[64])
{
	secp256k1_xonly_pubkey xpk;
	if (!secp256k1_xonly_pubkey_parse(secp256k1_context_static, &xpk, pk))
		return 0;
	return secp256k1_schnorrsig_verify(secp256k1_context_static, sig, msg, 32, &xpk) == 1;
}

static const char *slh_name(int scheme)
{
	return scheme == 2 ? "SLH-DSA-SHA2-128s" : scheme == 3 ? "SLH-DSA-SHA2-128f" : NULL;
}

size_t mor_slhdsa_sig_len(int scheme)
{
	return scheme == 2 ? 7856 : scheme == 3 ? 17088 : 0;
}

int mor_slhdsa_verify(int scheme, const uint8_t pk[32], const uint8_t *msg, size_t msglen,
		      const uint8_t *sig, size_t siglen, const uint8_t *ctx, size_t ctxlen)
{
	const char *name = slh_name(scheme);
	int ret = -1;
	EVP_PKEY *key = NULL;
	EVP_PKEY_CTX *pctx = NULL;
	EVP_SIGNATURE *alg = NULL;
	if (!name)
		return 0;
	if (siglen != mor_slhdsa_sig_len(scheme))
		return 0;
	key = EVP_PKEY_new_raw_public_key_ex(NULL, name, NULL, pk, 32);
	if (!key)
		goto out;
	pctx = EVP_PKEY_CTX_new_from_pkey(NULL, key, NULL);
	alg = EVP_SIGNATURE_fetch(NULL, name, NULL);
	if (!pctx || !alg)
		goto out;
	uint8_t cbuf[255];
	if (ctxlen > sizeof cbuf)
		goto out;
	if (ctxlen)
		memcpy(cbuf, ctx, ctxlen);
	OSSL_PARAM params[2];
	params[0] = OSSL_PARAM_construct_octet_string(OSSL_SIGNATURE_PARAM_CONTEXT_STRING, cbuf, ctxlen);
	params[1] = OSSL_PARAM_construct_end();
	if (EVP_PKEY_verify_message_init(pctx, alg, params) != 1)
		goto out;
	ret = EVP_PKEY_verify(pctx, sig, siglen, msg, msglen) == 1 ? 1 : 0;
out:
	EVP_SIGNATURE_free(alg);
	EVP_PKEY_CTX_free(pctx);
	EVP_PKEY_free(key);
	return ret;
}

static EVP_PKEY *slh_from_seed(int scheme, const uint8_t sk_seed[16], const uint8_t sk_prf[16],
			       const uint8_t pk_seed[16])
{
	const char *name = slh_name(scheme);
	uint8_t seed[48];
	EVP_PKEY *key = NULL;
	EVP_PKEY_CTX *g;
	if (!name)
		return NULL;
	memcpy(seed, sk_seed, 16);
	memcpy(seed + 16, sk_prf, 16);
	memcpy(seed + 32, pk_seed, 16);
	g = EVP_PKEY_CTX_new_from_name(NULL, name, NULL);
	if (!g)
		return NULL;
	OSSL_PARAM params[2];
	params[0] = OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_SLH_DSA_SEED, seed, sizeof seed);
	params[1] = OSSL_PARAM_construct_end();
	if (EVP_PKEY_keygen_init(g) != 1 || EVP_PKEY_CTX_set_params(g, params) != 1 ||
	    EVP_PKEY_generate(g, &key) != 1)
		key = NULL;
	EVP_PKEY_CTX_free(g);
	sodium_memzero(seed, sizeof seed);
	return key;
}

int mor_slhdsa_keygen(int scheme, uint8_t pk[32], const uint8_t sk_seed[16], const uint8_t sk_prf[16],
		      const uint8_t pk_seed[16])
{
	EVP_PKEY *key = slh_from_seed(scheme, sk_seed, sk_prf, pk_seed);
	size_t n = 32;
	int ok;
	if (!key)
		return -1;
	ok = EVP_PKEY_get_raw_public_key(key, pk, &n) == 1 && n == 32;
	EVP_PKEY_free(key);
	return ok ? 0 : -1;
}

int mor_slhdsa_sign(int scheme, uint8_t *sig, const uint8_t sk_seed[16], const uint8_t sk_prf[16],
		    const uint8_t pk_seed[16], const uint8_t *msg, size_t msglen, const uint8_t *ctx,
		    size_t ctxlen, const uint8_t *opt_rand)
{
	EVP_PKEY *key = slh_from_seed(scheme, sk_seed, sk_prf, pk_seed);
	EVP_PKEY_CTX *pctx = NULL;
	EVP_SIGNATURE *alg = NULL;
	size_t siglen = mor_slhdsa_sig_len(scheme);
	int ret = -1, det = 1;
	if (!key)
		return -1;
	pctx = EVP_PKEY_CTX_new_from_pkey(NULL, key, NULL);
	alg = EVP_SIGNATURE_fetch(NULL, slh_name(scheme), NULL);
	if (!pctx || !alg)
		goto out;
	uint8_t cbuf[255], rbuf[16];
	if (ctxlen > sizeof cbuf)
		goto out;
	if (ctxlen)
		memcpy(cbuf, ctx, ctxlen);
	if (opt_rand)
		memcpy(rbuf, opt_rand, 16);
	OSSL_PARAM params[3];
	params[0] = OSSL_PARAM_construct_octet_string(OSSL_SIGNATURE_PARAM_CONTEXT_STRING, cbuf, ctxlen);
	if (opt_rand)
		params[1] = OSSL_PARAM_construct_octet_string(OSSL_SIGNATURE_PARAM_TEST_ENTROPY, rbuf, 16);
	else
		params[1] = OSSL_PARAM_construct_int(OSSL_SIGNATURE_PARAM_DETERMINISTIC, &det);
	params[2] = OSSL_PARAM_construct_end();
	if (EVP_PKEY_sign_message_init(pctx, alg, params) != 1)
		goto out;
	if (EVP_PKEY_sign(pctx, sig, &siglen, msg, msglen) != 1 || siglen != mor_slhdsa_sig_len(scheme))
		goto out;
	ret = 0;
out:
	EVP_SIGNATURE_free(alg);
	EVP_PKEY_CTX_free(pctx);
	EVP_PKEY_free(key);
	return ret;
}

/* ---- X-Wing ---- */

static const uint8_t xwing_label[6] = { 0x5c, 0x2e, 0x2f, 0x2f, 0x5e, 0x5c }; /* "\.//^\" */

static int shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
	EVP_MD_CTX *c = EVP_MD_CTX_new();
	int ok = c && EVP_DigestInit_ex(c, EVP_shake256(), NULL) == 1 && EVP_DigestUpdate(c, in, inlen) == 1 &&
		 EVP_DigestFinalXOF(c, out, outlen) == 1;
	EVP_MD_CTX_free(c);
	return ok ? 0 : -1;
}

static int sha3_256(uint8_t out[32], const uint8_t *in, size_t inlen)
{
	unsigned int n = 32;
	return EVP_Digest(in, inlen, out, &n, EVP_sha3_256(), NULL) == 1 ? 0 : -1;
}

static int xwing_combine(uint8_t ss[32], const uint8_t ss_m[32], const uint8_t ss_x[32], const uint8_t ct_x[32],
			 const uint8_t pk_x[32])
{
	uint8_t buf[32 * 4 + 6];
	memcpy(buf, ss_m, 32);
	memcpy(buf + 32, ss_x, 32);
	memcpy(buf + 64, ct_x, 32);
	memcpy(buf + 96, pk_x, 32);
	memcpy(buf + 128, xwing_label, 6);
	int r = sha3_256(ss, buf, sizeof buf);
	sodium_memzero(buf, sizeof buf);
	return r;
}

/* Expand an X-Wing decapsulation key: ML-KEM-768 key from (d, z), X25519 secret. */
static EVP_PKEY *xwing_expand(const uint8_t sk[32], uint8_t sk_x[32], uint8_t pk_x[32])
{
	uint8_t ex[96];
	EVP_PKEY *key = NULL;
	EVP_PKEY_CTX *g = NULL;
	if (shake256(ex, sizeof ex, sk, 32))
		return NULL;
	memcpy(sk_x, ex + 64, 32);
	if (crypto_scalarmult_base(pk_x, sk_x))
		goto out;
	g = EVP_PKEY_CTX_new_from_name(NULL, "ML-KEM-768", NULL);
	if (!g)
		goto out;
	OSSL_PARAM params[2];
	params[0] = OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_ML_KEM_SEED, ex, 64);
	params[1] = OSSL_PARAM_construct_end();
	if (EVP_PKEY_keygen_init(g) != 1 || EVP_PKEY_CTX_set_params(g, params) != 1 ||
	    EVP_PKEY_generate(g, &key) != 1)
		key = NULL;
out:
	EVP_PKEY_CTX_free(g);
	sodium_memzero(ex, sizeof ex);
	return key;
}

int mor_xwing_public_from_secret(uint8_t pk[MOR_XWING_PK], const uint8_t sk[32])
{
	uint8_t sk_x[32];
	EVP_PKEY *key = xwing_expand(sk, sk_x, pk + 1184);
	size_t n = 1184;
	int ok;
	sodium_memzero(sk_x, sizeof sk_x);
	if (!key)
		return -1;
	ok = EVP_PKEY_get_raw_public_key(key, pk, &n) == 1 && n == 1184;
	EVP_PKEY_free(key);
	return ok ? 0 : -1;
}

int mor_xwing_decaps(uint8_t ss[32], const uint8_t ct[MOR_XWING_CT], const uint8_t sk[32])
{
	uint8_t sk_x[32], pk_x[32], ss_m[32], ss_x[32];
	size_t n = 32;
	int ret = -1;
	EVP_PKEY *key = xwing_expand(sk, sk_x, pk_x);
	EVP_PKEY_CTX *c = NULL;
	if (!key)
		goto out;
	c = EVP_PKEY_CTX_new_from_pkey(NULL, key, NULL);
	if (!c || EVP_PKEY_decapsulate_init(c, NULL) != 1 ||
	    EVP_PKEY_decapsulate(c, ss_m, &n, ct, 1088) != 1 || n != 32)
		goto out;
	if (crypto_scalarmult(ss_x, sk_x, ct + 1088))
		goto out;
	ret = xwing_combine(ss, ss_m, ss_x, ct + 1088, pk_x);
out:
	EVP_PKEY_CTX_free(c);
	EVP_PKEY_free(key);
	sodium_memzero(sk_x, sizeof sk_x);
	sodium_memzero(ss_m, sizeof ss_m);
	sodium_memzero(ss_x, sizeof ss_x);
	return ret;
}

int mor_xwing_encaps_derand(uint8_t ct[MOR_XWING_CT], uint8_t ss[32], const uint8_t pk[MOR_XWING_PK],
			    const uint8_t eseed[64])
{
	uint8_t ss_m[32], ss_x[32];
	size_t ctlen = 1088, sslen = 32;
	int ret = -1;
	EVP_PKEY *key = EVP_PKEY_new_raw_public_key_ex(NULL, "ML-KEM-768", NULL, pk, 1184);
	EVP_PKEY_CTX *c = NULL;
	if (!key)
		goto out;
	c = EVP_PKEY_CTX_new_from_pkey(NULL, key, NULL);
	uint8_t m[32];
	memcpy(m, eseed, 32);
	OSSL_PARAM params[2];
	params[0] = OSSL_PARAM_construct_octet_string(OSSL_KEM_PARAM_IKME, m, 32);
	params[1] = OSSL_PARAM_construct_end();
	if (!c || EVP_PKEY_encapsulate_init(c, params) != 1 ||
	    EVP_PKEY_encapsulate(c, ct, &ctlen, ss_m, &sslen) != 1 || ctlen != 1088 || sslen != 32)
		goto out;
	if (crypto_scalarmult_base(ct + 1088, eseed + 32))
		goto out;
	if (crypto_scalarmult(ss_x, eseed + 32, pk + 1184))
		goto out;
	ret = xwing_combine(ss, ss_m, ss_x, ct + 1088, pk + 1184);
out:
	EVP_PKEY_CTX_free(c);
	EVP_PKEY_free(key);
	sodium_memzero(ss_m, sizeof ss_m);
	sodium_memzero(ss_x, sizeof ss_x);
	return ret;
}
