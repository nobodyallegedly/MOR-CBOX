#include "people.h"

#include <string.h>

#include "cbor.h"

static const char charset[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
static const uint32_t BECH32M = 0x2bc830a3u;

static const char *labels[] = { "", "mid", "mact", "mkey", "mspec", "mwork" };

const char *mor_label_name(int label)
{
	return label > 0 && label <= MOR_L_MWORK ? labels[label] : "?";
}

const char *mor_form_err_name(int e)
{
	switch (e) {
	case MOR_FORM_OK: return "ok";
	case MOR_FORM_CHECKSUM: return "checksum";
	case MOR_FORM_MIXED_CASE: return "mixed case";
	case MOR_FORM_LABEL: return "unknown label";
	case MOR_FORM_DATA: return "data";
	case MOR_FORM_KIND: return "wrong kind";
	case MOR_FORM_SYNTAX: return "syntax";
	default: return "nomem";
	}
}

static uint32_t polymod_step(uint32_t c, uint8_t v)
{
	static const uint32_t gen[5] = { 0x3b6a57b2u, 0x26508e6du, 0x1ea119fau, 0x3d4233ddu, 0x2a1462b3u };
	uint8_t top = (uint8_t)(c >> 25);
	c = ((c & 0x1ffffffu) << 5) ^ v;
	for (int i = 0; i < 5; i++)
		if ((top >> i) & 1u)
			c ^= gen[i];
	return c;
}

static uint32_t hrp_poly(const char *hrp, size_t n)
{
	uint32_t c = 1;
	for (size_t i = 0; i < n; i++)
		c = polymod_step(c, (uint8_t)((unsigned char)hrp[i] >> 5));
	c = polymod_step(c, 0);
	for (size_t i = 0; i < n; i++)
		c = polymod_step(c, (uint8_t)((unsigned char)hrp[i] & 31u));
	return c;
}

size_t mor_form_encode(char *out, size_t cap, int label, const uint8_t *data, size_t n)
{
	if (label <= 0 || label > MOR_L_MWORK)
		return 0;
	const char *hrp = labels[label];
	size_t hl = strlen(hrp);
	size_t dl = (n * 8 + 4) / 5;
	size_t total = hl + 1 + dl + 6;
	if (total + 1 > cap)
		return 0;
	memcpy(out, hrp, hl);
	out[hl] = '1';
	uint32_t c = hrp_poly(hrp, hl);
	size_t k = hl + 1;
	uint32_t acc = 0;
	unsigned bits = 0;
	for (size_t i = 0; i < n; i++) {
		acc = (acc << 8) | data[i];
		bits += 8;
		while (bits >= 5) {
			bits -= 5;
			uint8_t v = (uint8_t)((acc >> bits) & 31u);
			c = polymod_step(c, v);
			out[k++] = charset[v];
		}
	}
	if (bits) {
		uint8_t v = (uint8_t)((acc << (5 - bits)) & 31u);
		c = polymod_step(c, v);
		out[k++] = charset[v];
	}
	for (int i = 0; i < 6; i++)
		c = polymod_step(c, 0);
	c ^= BECH32M;
	for (int i = 0; i < 6; i++)
		out[k++] = charset[(c >> (5 * (5 - i))) & 31u];
	out[k] = 0;
	return k;
}

static int hexval(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int label_of(const char *h, size_t n)
{
	for (int l = 1; l <= MOR_L_MWORK; l++)
		if (strlen(labels[l]) == n && !memcmp(labels[l], h, n))
			return l;
	return MOR_L_NONE;
}

/* mkey data: Identity's [scheme, key], or Envelopes' enc-key, in deterministic CBOR. */
static int key_data_ok(const uint8_t *d, size_t n, mor_arena *a)
{
	size_t mark = mor_arena_mark(a);
	const mor_cbor *c;
	int ok = 0;
	if (mor_cbor_decode(d, n, a, &c) == MOR_CBOR_OK && c->type == MOR_CBOR_ARRAY && c->v == 2 &&
	    c->items[1].type == MOR_CBOR_BYTES) {
		const mor_cbor *s = &c->items[0];
		uint64_t kl = c->items[1].v;
		if (s->type == MOR_CBOR_UINT && s->v >= 1 && s->v <= 3)
			ok = kl == 32;
		else if (s->type == MOR_CBOR_UINT && s->v == 4)
			ok = kl == 1216;
		else
			ok = mor_cbor_is_hash(s);
	}
	mor_arena_reset(a, mark);
	return ok;
}

static int decode_hints(mor_form *f, const char *s, size_t n, mor_arena *a)
{
	/* s holds "?r=x&r=y..." */
	size_t i = 0;
	while (i < n) {
		if (n - i < 3 || s[i] != (f->nhints ? '&' : '?') || s[i + 1] != 'r' || s[i + 2] != '=')
			return MOR_FORM_SYNTAX;
		i += 3;
		size_t j = i;
		while (j < n && s[j] != '&')
			j++;
		if (f->nhints >= sizeof f->hints / sizeof f->hints[0])
			return MOR_FORM_SYNTAX;
		char *h = mor_arena_alloc(a, j - i + 1);
		if (!h)
			return MOR_FORM_NOMEM;
		size_t k = 0;
		for (size_t p = i; p < j; p++) {
			if (s[p] == '%') {
				if (j - p < 3)
					return MOR_FORM_SYNTAX;
				int hi = hexval(s[p + 1]), lo = hexval(s[p + 2]);
				if (hi < 0 || lo < 0)
					return MOR_FORM_SYNTAX;
				h[k++] = (char)(hi * 16 + lo);
				p += 2;
			} else {
				h[k++] = s[p];
			}
		}
		h[k] = 0;
		f->hints[f->nhints++] = h;
		i = j;
	}
	return MOR_FORM_OK;
}

int mor_form_decode(mor_form *f, const char *s, size_t n, int want, mor_arena *a)
{
	memset(f, 0, sizeof *f);
	/* Bare hexadecimal: 64 characters, 0 to 9 and a to f. */
	if (n == 64) {
		int hex = 1;
		for (size_t i = 0; i < 64; i++)
			if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f')))
				hex = 0;
		if (hex) {
			if (want == MOR_L_MKEY || want == MOR_L_NONE)
				return MOR_FORM_DATA;
			f->data = mor_arena_alloc(a, 32);
			if (!f->data)
				return MOR_FORM_NOMEM;
			for (size_t i = 0; i < 32; i++)
				f->data[i] = (uint8_t)(hexval(s[2 * i]) * 16 + hexval(s[2 * i + 1]));
			f->len = 32;
			f->label = want;
			return MOR_FORM_OK;
		}
	}
	/* The where-it-lives hints lie outside the checksum. */
	size_t body = n;
	for (size_t i = 0; i < n; i++)
		if (s[i] == '?') {
			body = i;
			break;
		}
	int lower = 0, upper = 0;
	for (size_t i = 0; i < body; i++) {
		unsigned char c = (unsigned char)s[i];
		if (c < 33 || c > 126)
			return MOR_FORM_SYNTAX;
		if (c >= 'a' && c <= 'z')
			lower = 1;
		if (c >= 'A' && c <= 'Z')
			upper = 1;
	}
	if (lower && upper)
		return MOR_FORM_MIXED_CASE;
	size_t sep = 0;
	int found = 0;
	for (size_t i = body; i > 0; i--)
		if (s[i - 1] == '1') {
			sep = i - 1;
			found = 1;
			break;
		}
	if (!found || sep == 0 || body - sep - 1 < 6 || sep > 83)
		return MOR_FORM_SYNTAX;
	char hrp[84];
	for (size_t i = 0; i < sep; i++)
		hrp[i] = (char)((s[i] >= 'A' && s[i] <= 'Z') ? s[i] + 32 : s[i]);
	uint32_t c = hrp_poly(hrp, sep);
	size_t dl = body - sep - 1;
	uint8_t *v = mor_arena_alloc(a, dl);
	if (!v)
		return MOR_FORM_NOMEM;
	for (size_t i = 0; i < dl; i++) {
		char ch = s[sep + 1 + i];
		if (ch >= 'A' && ch <= 'Z')
			ch = (char)(ch + 32);
		const char *p = strchr(charset, ch);
		if (!p || !ch)
			return MOR_FORM_SYNTAX;
		v[i] = (uint8_t)(p - charset);
		c = polymod_step(c, v[i]);
	}
	if (c != BECH32M)
		return MOR_FORM_CHECKSUM;
	int label = label_of(hrp, sep);
	if (label == MOR_L_NONE)
		return MOR_FORM_LABEL;
	f->label = label;
	if (want != MOR_L_NONE && want != label)
		return MOR_FORM_KIND;
	/* 5-bit groups to bytes, padding at most 4 zero bits. */
	size_t nd = dl - 6;
	uint8_t *d = mor_arena_alloc(a, nd * 5 / 8 + 1);
	if (!d)
		return MOR_FORM_NOMEM;
	uint32_t acc = 0;
	unsigned bits = 0;
	size_t k = 0;
	for (size_t i = 0; i < nd; i++) {
		acc = ((acc << 5) | v[i]) & 0xfffu;
		bits += 5;
		if (bits >= 8) {
			bits -= 8;
			d[k++] = (uint8_t)(acc >> bits);
		}
	}
	if (bits >= 5 || (acc & ((1u << bits) - 1u)))
		return MOR_FORM_DATA;
	f->data = d;
	f->len = k;
	if (label == MOR_L_MKEY) {
		if (!key_data_ok(d, k, a))
			return MOR_FORM_DATA;
	} else if (k != 32) {
		return MOR_FORM_DATA;
	}
	if (body < n) {
		if (label != MOR_L_MID)
			return MOR_FORM_SYNTAX;
		return decode_hints(f, s + body, n - body, a);
	}
	return MOR_FORM_OK;
}
