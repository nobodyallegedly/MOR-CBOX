#include "hex.h"

#include <stdio.h>
#include <string.h>

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

uint8_t *hex_decode(const char *s, size_t *n, mor_arena *a)
{
	size_t l = s ? strlen(s) : 0;
	if (!s || (l & 1u))
		return NULL;
	uint8_t *p = mor_arena_alloc(a, l / 2 + 1);
	if (!p)
		return NULL;
	for (size_t i = 0; i < l / 2; i++) {
		int hi = nib(s[2 * i]), lo = nib(s[2 * i + 1]);
		if (hi < 0 || lo < 0)
			return NULL;
		p[i] = (uint8_t)(hi * 16 + lo);
	}
	*n = l / 2;
	return p;
}

int hex_fixed(const char *s, uint8_t *out, size_t n)
{
	if (!s || strlen(s) != 2 * n)
		return -1;
	for (size_t i = 0; i < n; i++) {
		int hi = nib(s[2 * i]), lo = nib(s[2 * i + 1]);
		if (hi < 0 || lo < 0)
			return -1;
		out[i] = (uint8_t)(hi * 16 + lo);
	}
	return 0;
}

char *hex_encode(const uint8_t *p, size_t n, mor_arena *a)
{
	static const char d[] = "0123456789abcdef";
	char *s = mor_arena_alloc(a, 2 * n + 1);
	if (!s)
		return NULL;
	for (size_t i = 0; i < n; i++) {
		s[2 * i] = d[p[i] >> 4];
		s[2 * i + 1] = d[p[i] & 15u];
	}
	s[2 * n] = 0;
	return s;
}

void hex_print(const uint8_t *p, size_t n)
{
	for (size_t i = 0; i < n; i++)
		printf("%02x", p[i]);
}
