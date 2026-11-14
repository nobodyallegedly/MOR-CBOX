/* standin.c: the stand-ins for the core's fingerprint and the Sign layer's
 * signature (STANDINS.md, S2 and S3). Both sit behind mor_options and can be
 * replaced without touching the grammar.
 *
 * The one cryptographic primitive used is SHA-256, from OpenSSL's libcrypto
 * (EVP interface), pinned to the 3.x series: see the Makefile and REPORT.md. */
#include "internal.h"
#include <openssl/evp.h>
#include <openssl/opensslv.h>

#if OPENSSL_VERSION_MAJOR != 3
#error "this build is pinned to OpenSSL 3.x (tested with 3.0.13)"
#endif

static const char SIG_DOMAIN[] = "mor-standin-sig-1\n";

static int sha256_hex2(const unsigned char *a, size_t an, const unsigned char *b, size_t bn,
                       char out[MOR_FP_LEN + 1])
{
	static const char hex[] = "0123456789abcdef";
	unsigned char md[EVP_MAX_MD_SIZE];
	unsigned int len = 0;
	int ok = 0;
	EVP_MD_CTX *m = EVP_MD_CTX_new();
	if (!m) return -1;
	if (EVP_DigestInit_ex(m, EVP_sha256(), NULL) == 1 && (an == 0 || EVP_DigestUpdate(m, a, an) == 1) &&
	    (bn == 0 || EVP_DigestUpdate(m, b, bn) == 1) && EVP_DigestFinal_ex(m, md, &len) == 1 && len == 32)
		ok = 1;
	EVP_MD_CTX_free(m);
	if (!ok) return -1;
	for (unsigned i = 0; i < 32; i++) {
		out[2 * i] = hex[md[i] >> 4];
		out[2 * i + 1] = hex[md[i] & 15];
	}
	out[MOR_FP_LEN] = '\0';
	return 0;
}

int mor_standin_fingerprint(const unsigned char *bytes, size_t n, char out[MOR_FP_LEN + 1])
{
	return sha256_hex2(bytes, n, NULL, 0, out);
}

/* The stand-in signature proves nothing: it is a keyless tag over the bytes,
 * which anyone can compute. It keeps the shape "an act is signed, and a
 * verifier checks it" so that a real Sign layer can be plugged in. */
void mor_standin_sig_make(const unsigned char *bytes, size_t n, char out[MOR_FP_LEN + 1])
{
	if (sha256_hex2((const unsigned char *)SIG_DOMAIN, sizeof SIG_DOMAIN - 1, bytes, n, out) != 0)
		memset(out, 0, MOR_FP_LEN + 1);
}

int mor_standin_sig_check(const char *signer, const unsigned char *bytes, size_t n, const char *sig)
{
	char want[MOR_FP_LEN + 1];
	(void)signer; /* the signer line is inside the bytes */
	mor_standin_sig_make(bytes, n, want);
	return want[0] != '\0' && strcmp(want, sig) == 0;
}

static bool is_document_type(const char *body)
{
	static const char *const docs[] = { "mor-act 1 cmip\n", "mor-act 1 words\n", "mor-act 1 envelope\n" };
	for (size_t i = 0; i < sizeof docs / sizeof docs[0]; i++)
		if (strncmp(body, docs[i], strlen(docs[i])) == 0) return true;
	return false;
}

size_t mor_seal(const mor_options *opt, const char *body, char *out, size_t cap, char fp[MOR_FP_LEN + 1])
{
	size_t n = strlen(body);
	char sig[MOR_FP_LEN + 1];
	if (n == 0 || body[n - 1] != '\n' || strncmp(body, "mor-act 1 ", 10) != 0) return 0;
	size_t need = n + 4 + 1;
	bool doc = is_document_type(body);
	if (!doc) need += 4 + MOR_FP_LEN + 1;
	if (need > cap) return 0;
	memcpy(out, body, n);
	size_t o = n;
	if (!doc) {
		mor_standin_sig_make((const unsigned char *)body, n, sig);
		memcpy(out + o, "sig ", 4);
		memcpy(out + o + 4, sig, MOR_FP_LEN);
		out[o + 4 + MOR_FP_LEN] = '\n';
		o += 4 + MOR_FP_LEN + 1;
	}
	memcpy(out + o, "end\n", 5);
	o += 4;
	int (*fpf)(const unsigned char *, size_t, char *) =
		(opt && opt->fingerprint) ? opt->fingerprint : mor_standin_fingerprint;
	if (fpf((const unsigned char *)out, o, fp) != 0) return 0;
	return o;
}
