#include "builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

#include "../src/cbor.h"
#include "../src/crypto.h"

typedef struct vec {
	uint8_t *p;
	size_t n, cap;
} vec;

static void vput(vec *v, const void *p, size_t n)
{
	if (v->n + n > v->cap) {
		size_t c = (v->cap ? v->cap * 2 : 64) + n;
		uint8_t *q = realloc(v->p, c);
		if (!q)
			abort();
		v->p = q;
		v->cap = c;
	}
	memcpy(v->p + v->n, p, n);
	v->n += n;
}

static void vhead(vec *v, unsigned major, uint64_t x)
{
	uint8_t buf[9];
	mor_buf b;
	mor_buf_init(&b, buf, sizeof buf);
	mor_cbor_put_head(&b, major, x);
	vput(v, buf, b.len);
}

typedef struct ps {
	const char *s;
	int err;
} ps;

static void ws(ps *p)
{
	while (*p->s == ' ' || *p->s == '\n' || *p->s == '\t' || *p->s == ',')
		p->s++;
}

static int nib(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static void item(ps *p, vec *out);

typedef struct pair {
	vec k, v;
} pair;

static int pair_cmp(const void *x, const void *y)
{
	const pair *a = x, *b = y;
	size_t n = a->k.n < b->k.n ? a->k.n : b->k.n;
	int c = memcmp(a->k.p, b->k.p, n);
	if (c)
		return c;
	return a->k.n < b->k.n ? -1 : a->k.n > b->k.n;
}

static void item(ps *p, vec *out)
{
	ws(p);
	char c = *p->s;
	if (c == '[') {
		p->s++;
		vec items = { 0 };
		uint64_t n = 0;
		ws(p);
		while (*p->s && *p->s != ']') {
			item(p, &items);
			n++;
			ws(p);
		}
		if (*p->s != ']')
			p->err = 1;
		else
			p->s++;
		vhead(out, 4, n);
		if (items.n)
			vput(out, items.p, items.n);
		free(items.p);
	} else if (c == '{') {
		p->s++;
		pair *pairs = NULL;
		size_t n = 0, cap = 0;
		ws(p);
		while (*p->s && *p->s != '}') {
			if (n == cap) {
				cap = cap ? cap * 2 : 8;
				pairs = realloc(pairs, cap * sizeof *pairs);
				if (!pairs)
					abort();
			}
			memset(&pairs[n], 0, sizeof pairs[n]);
			item(p, &pairs[n].k);
			ws(p);
			if (*p->s != ':') {
				p->err = 1;
				break;
			}
			p->s++;
			item(p, &pairs[n].v);
			n++;
			ws(p);
		}
		if (*p->s != '}')
			p->err = 1;
		else
			p->s++;
		if (n)
			qsort(pairs, n, sizeof *pairs, pair_cmp);
		vhead(out, 5, n);
		for (size_t i = 0; i < n; i++) {
			vput(out, pairs[i].k.p, pairs[i].k.n);
			vput(out, pairs[i].v.p, pairs[i].v.n);
			free(pairs[i].k.p);
			free(pairs[i].v.p);
		}
		free(pairs);
	} else if (c == 'h' && p->s[1] == '\'') {
		p->s += 2;
		vec b = { 0 };
		while (*p->s && *p->s != '\'') {
			int hi = nib(p->s[0]), lo = nib(p->s[1]);
			if (hi < 0 || lo < 0) {
				p->err = 1;
				break;
			}
			uint8_t x = (uint8_t)(hi * 16 + lo);
			vput(&b, &x, 1);
			p->s += 2;
		}
		if (*p->s == '\'')
			p->s++;
		vhead(out, 2, b.n);
		if (b.n)
			vput(out, b.p, b.n);
		free(b.p);
	} else if (c == '"') {
		p->s++;
		const char *st = p->s;
		while (*p->s && *p->s != '"')
			p->s++;
		size_t n = (size_t)(p->s - st);
		if (*p->s == '"')
			p->s++;
		vhead(out, 3, n);
		vput(out, st, n);
	} else if (!strncmp(p->s, "true", 4)) {
		p->s += 4;
		uint8_t x = 0xf5;
		vput(out, &x, 1);
	} else if (!strncmp(p->s, "false", 5)) {
		p->s += 5;
		uint8_t x = 0xf4;
		vput(out, &x, 1);
	} else if (!strncmp(p->s, "null", 4)) {
		p->s += 4;
		uint8_t x = 0xf6;
		vput(out, &x, 1);
	} else if (c == '-' || (c >= '0' && c <= '9')) {
		int neg = c == '-';
		if (neg)
			p->s++;
		uint64_t v = 0;
		while (*p->s >= '0' && *p->s <= '9')
			v = v * 10 + (uint64_t)(*p->s++ - '0');
		if (neg)
			vhead(out, 1, v - 1);
		else
			vhead(out, 0, v);
	} else {
		p->err = 1;
	}
}

size_t tb_diag(const char *s, uint8_t *out, size_t cap)
{
	ps p = { s, 0 };
	vec v = { 0 };
	item(&p, &v);
	ws(&p);
	size_t n = 0;
	if (!p.err && !*p.s && v.n <= cap) {
		memcpy(out, v.p, v.n);
		n = v.n;
	}
	free(v.p);
	return n;
}

void tb_hex(const uint8_t *p, size_t n, char *out)
{
	for (size_t i = 0; i < n; i++)
		sprintf(out + 2 * i, "%02x", p[i]);
	out[2 * n] = 0;
}

void tb_key_schnorr(tb_key *k, const char *label)
{
	memset(k, 0, sizeof *k);
	k->scheme = 1;
	mor_tagged_hash(k->sk, "test/secret", (const uint8_t *)label, strlen(label));
	secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	secp256k1_keypair kp;
	secp256k1_xonly_pubkey x;
	if (!secp256k1_keypair_create(ctx, &kp, k->sk) || !secp256k1_keypair_xonly_pub(ctx, &x, NULL, &kp))
		abort();
	secp256k1_xonly_pubkey_serialize(ctx, k->pk, &x);
	secp256k1_context_destroy(ctx);
}

void tb_key_slh(tb_key *k, const char *label)
{
	uint8_t h[32];
	memset(k, 0, sizeof *k);
	k->scheme = 3;
	mor_tagged_hash(h, "test/seed1", (const uint8_t *)label, strlen(label));
	memcpy(k->seed, h, 32);
	mor_tagged_hash(h, "test/seed2", (const uint8_t *)label, strlen(label));
	memcpy(k->seed + 32, h, 16);
	if (mor_slhdsa_keygen(3, k->pk, k->seed, k->seed + 16, k->seed + 32))
		abort();
}

void tb_commit_hex(const tb_key *k, char out[65])
{
	uint8_t c[32], s = (uint8_t)k->scheme;
	mor_tagged_hash2(c, "MOR/safety", &s, 1, k->pk, 32);
	tb_hex(c, 32, out);
}

int tb_make(tb_act *out, const char *inside_diag, const char *signer_hex, const char *binding_hex, const tb_key *k,
	    const tb_opts *o)
{
	static uint8_t inside[65536], locked[65536 + 16];
	static const tb_opts none = { 0 };
	if (!o)
		o = &none;
	size_t in = tb_diag(inside_diag, inside, sizeof inside);
	if (!in)
		return -1;
	uint8_t key[32], nonce[32], commit[32], lh[32];
	mor_tagged_hash(key, "test/key", inside, in);
	mor_tagged_hash(nonce, "test/nonce", inside, in);
	mor_xchacha_seal(locked, inside, in, nonce, key);
	mor_tagged_hash(commit, "MOR/inside", inside, in);
	mor_sha256(lh, locked, in + 16);
	char ch[65], lhh[65], kh[65], nh[49];
	tb_hex(commit, 32, ch);
	tb_hex(lh, 32, lhh);
	tb_hex(key, 32, kh);
	tb_hex(nonce, 24, nh);
	char *od = malloc(2048 + (o->to ? strlen(o->to) : 0));
	if (!od)
		abort();
	int n = sprintf(od, "{");
	if (signer_hex)
		n += sprintf(od + n, "0: h'%s', ", signer_hex);
	if (binding_hex)
		n += sprintf(od + n, "1: h'%s', ", binding_hex);
	n += sprintf(od + n, "2: h'%s', 3: h'%s', 4: h'%s'", ch, lhh, nh);
	if (!o->private_)
		n += sprintf(od + n, ", 5: h'%s'", kh);
	if (o->to)
		n += sprintf(od + n, ", 6: %s", o->to);
	sprintf(od + n, "}");
	static uint8_t outside[8192];
	size_t on = tb_diag(od, outside, sizeof outside);
	free(od);
	if (!on)
		return -1;
	mor_tagged_hash(out->id, "MOR/act", outside, on);
	tb_hex(out->id, 32, out->idhex);
	/* signature over the act id */
	static uint8_t sig[17088];
	size_t sl;
	if (k->scheme == 1) {
		secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
		secp256k1_keypair kp;
		if (!secp256k1_keypair_create(ctx, &kp, k->sk) || !secp256k1_schnorrsig_sign32(ctx, sig, out->id, &kp, NULL))
			abort();
		secp256k1_context_destroy(ctx);
		sl = 64;
	} else {
		sl = mor_slhdsa_sig_len(k->scheme);
		if (mor_slhdsa_sign(k->scheme, sig, k->seed, k->seed + 16, k->seed + 32, out->id, 32,
				    (const uint8_t *)"MOR", 3, NULL))
			abort();
	}
	if (o->corrupt_sig)
		sig[0] ^= 1;
	mor_buf b;
	mor_buf_init(&b, out->bytes, sizeof out->bytes);
	mor_cbor_put_head(&b, 4, 3);
	mor_buf_put(&b, outside, on);
	mor_cbor_put_bytes(&b, locked, in + 16);
	mor_cbor_put_head(&b, 4, 3);
	if (o->scheme_hash) {
		uint8_t sh[32];
		mor_sha256(sh, (const uint8_t *)"a scheme by hash", 16);
		mor_cbor_put_bytes(&b, sh, 32);
	} else {
		mor_cbor_put_uint(&b, (uint64_t)k->scheme);
	}
	mor_cbor_put_bytes(&b, k->pk, 32);
	mor_cbor_put_bytes(&b, sig, sl);
	if (b.overflow)
		return -1;
	out->len = b.len;
	return 0;
}
