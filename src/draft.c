/* draft.c: a small assembler for writing acts by hand (tests and the tool).
 * It is a convenience of the stand-in format, not part of the grammar. */
#include "internal.h"

static sym label_get(const mor_ctx *c, const char *name, size_t n)
{
	for (size_t i = 0; i < c->labels.n; i++)
		if (strlen(c->labels.v[i].name) == n && memcmp(c->labels.v[i].name, name, n) == 0) return c->labels.v[i].fp;
	return NULL;
}

static void put(mor_ctx *c, charvec *b, const char *s, size_t n)
{
	for (size_t i = 0; i < n; i++) vec_push(c, *b, s[i]);
}

/* copy one line, replacing $name and ~name at token starts or after '=' */
static bool expand_line(mor_ctx *c, charvec *b, const char *s, size_t n)
{
	size_t i = 0;
	while (i < n) {
		bool at_start = i == 0 || s[i - 1] == ' ' || s[i - 1] == '=';
		if ((s[i] == '$' || s[i] == '~') && at_start) {
			size_t e = i + 1;
			while (e < n && s[e] != ' ') e++;
			if (e == i + 1) return false;
			if (s[i] == '$') {
				sym fp = label_get(c, s + i + 1, e - i - 1);
				if (!fp) return false;
				put(c, b, fp, MOR_FP_LEN);
			} else {
				char fp[MOR_FP_LEN + 1];
				if (c->opt.fingerprint((const unsigned char *)s + i, e - i, fp) != 0) return false;
				put(c, b, fp, MOR_FP_LEN);
			}
			i = e;
		} else {
			vec_push(c, *b, s[i]);
			i++;
		}
	}
	vec_push(c, *b, '\n');
	return true;
}

static bool flush(mor_ctx *c, charvec *body, sym *label, int *count)
{
	if (body->n == 0) return true;
	vec_push(c, *body, '\0');
	size_t cap = body->n + 2 * MOR_FP_LEN + 32;
	char *out = ar_alloc(c, cap);
	char fp[MOR_FP_LEN + 1];
	size_t len = mor_seal(&c->opt, body->v, out, cap, fp);
	body->n = 0;
	if (len == 0) return false;
	if (*label) {
		struct mlabel l = { *label, intern(c, fp, MOR_FP_LEN) };
		vec_push(c, c->labels, l);
		*label = NULL;
	}
	put(c, &c->sealed, out, len);
	decode_all(c, (const unsigned char *)out, len);
	(*count)++;
	return true;
}

int mor_draft(mor_ctx *c, const char *text, size_t n)
{
	if (!c || c->oom) return -1;
	if (setjmp(c->jb)) return -1;
	c->verified = false;
	charvec body = { 0 };
	sym label = NULL;
	int count = 0;
	size_t pos = 0;
	while (pos < n) {
		const char *nl = memchr(text + pos, '\n', n - pos);
		size_t len = nl ? (size_t)(nl - (text + pos)) : n - pos;
		const char *line = text + pos;
		pos += len + (nl ? 1 : 0);
		if (len == 0) {
			if (!flush(c, &body, &label, &count)) return -1;
			continue;
		}
		if (body.n == 0 && line[0] == '#') continue;
		if (line[0] == '=') {
			if (!flush(c, &body, &label, &count) || len < 2 || label) return -1;
			label = intern(c, line + 1, len - 1);
			continue;
		}
		if (len > MOR_MAX_LINE || !expand_line(c, &body, line, len)) return -1;
	}
	if (!flush(c, &body, &label, &count)) return -1;
	return count;
}

const char *mor_label(const mor_ctx *c, const char *name)
{
	return (c && name) ? label_get(c, name, strlen(name)) : NULL;
}

const char *mor_sealed(const mor_ctx *c, size_t *n)
{
	*n = c->sealed.n;
	return c->sealed.v ? c->sealed.v : "";
}
