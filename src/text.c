#include "text.h"

#include <stdint.h>
#include <string.h>

#include <utf8proc.h>

#include "cbor.h"

static int is_space(uint32_t c)
{
	return c == 0x20 || c == 0xa0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200a) || c == 0x202f ||
	       c == 0x205f || c == 0x3000;
}

static int is_nonchar(uint32_t c)
{
	return (c >= 0xfdd0 && c <= 0xfdef) || (c & 0xfffeu) == 0xfffeu;
}

/* Decode valid UTF-8 (checked beforehand) into code points. */
static size_t decode(const uint8_t *s, size_t n, uint32_t *cp)
{
	size_t i = 0, k = 0;
	while (i < n) {
		uint8_t c = s[i];
		uint32_t v;
		size_t need;
		if (c < 0x80) {
			v = c;
			need = 0;
		} else if (c < 0xe0) {
			v = c & 0x1fu;
			need = 1;
		} else if (c < 0xf0) {
			v = c & 0x0fu;
			need = 2;
		} else {
			v = c & 0x07u;
			need = 3;
		}
		for (size_t j = 1; j <= need; j++)
			v = (v << 6) | (s[i + j] & 0x3fu);
		cp[k++] = v;
		i += need + 1;
	}
	return k;
}

const char *mor_text_unicode_version(void)
{
	return utf8proc_unicode_version();
}

/* NFC holds when canonical decomposition, canonical reordering and
 * composition (utf8proc, Unicode 17.0 tables) give back the same code points. */
static int nfc_holds(const uint8_t *s, size_t n, const uint32_t *cp, size_t k, mor_arena *a)
{
	size_t cap = k * 4 + 4;
	utf8proc_int32_t *buf;
	utf8proc_ssize_t got;
	const utf8proc_option_t opt = (utf8proc_option_t)(UTF8PROC_STABLE | UTF8PROC_DECOMPOSE);
	if (n > (size_t)PTRDIFF_MAX / 8)
		return -1;
	for (;;) {
		buf = mor_arena_array(a, cap, sizeof *buf);
		if (!buf)
			return -1;
		got = utf8proc_decompose(s, (utf8proc_ssize_t)n, buf, (utf8proc_ssize_t)cap, opt);
		if (got < 0)
			return 0;
		if ((size_t)got <= cap)
			break;
		cap = (size_t)got;
	}
	/* Canonical reordering and composition. */
	utf8proc_ssize_t m = utf8proc_normalize_utf32(buf, got, UTF8PROC_STABLE | UTF8PROC_COMPOSE);
	if (m < 0)
		return 0;
	if ((size_t)m != k)
		return 0;
	for (size_t i = 0; i < k; i++)
		if ((uint32_t)buf[i] != cp[i])
			return 0;
	return 1;
}

int mor_text_check(const uint8_t *s, size_t n, mor_arena *a)
{
	if (n && !s)
		return MOR_TEXT_UTF8;
	if (!mor_utf8_valid(s, n))
		return MOR_TEXT_UTF8;
	size_t mark = mor_arena_mark(a);
	uint32_t *cp = mor_arena_array(a, n + 1, sizeof *cp);
	if (!cp)
		return MOR_TEXT_NOMEM;
	size_t k = decode(s, n, cp);
	int rule = MOR_TEXT_CANONICAL;

	for (size_t i = 0; i < k && !rule; i++)
		if (cp[i] == 0xfeff)
			rule = MOR_TEXT_UTF8;
	for (size_t i = 0; i < k && !rule; i++)
		if (cp[i] == 0x0d || cp[i] == 0x2028 || cp[i] == 0x2029)
			rule = MOR_TEXT_LINEBREAK;
	for (size_t i = 0; i < k && !rule; i++)
		if ((cp[i] < 0x20 && cp[i] != 0x0a) || (cp[i] >= 0x7f && cp[i] <= 0x9f))
			rule = MOR_TEXT_CONTROL;
	if (!rule) {
		if (k && (cp[k - 1] == 0x0a || is_space(cp[k - 1])))
			rule = MOR_TEXT_TRAILING;
		for (size_t i = 1; i < k && !rule; i++)
			if (cp[i] == 0x0a && is_space(cp[i - 1]))
				rule = MOR_TEXT_TRAILING;
	}
	for (size_t i = 0; i < k && !rule; i++)
		if (is_nonchar(cp[i]))
			rule = MOR_TEXT_NONCHAR;
	if (!rule) {
		int r = nfc_holds(s, n, cp, k, a);
		if (r < 0)
			rule = MOR_TEXT_NOMEM;
		else if (!r)
			rule = MOR_TEXT_NFC;
	}
	mor_arena_reset(a, mark);
	return rule;
}
