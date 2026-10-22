#include "json.h"

#include <string.h>

typedef struct jp {
	const char *p, *end;
	mor_arena *a;
	unsigned depth;
} jp;

static void ws(jp *j)
{
	while (j->p < j->end && (*j->p == ' ' || *j->p == '\t' || *j->p == '\n' || *j->p == '\r'))
		j->p++;
}

static int hex4(jp *j, uint32_t *v)
{
	if (j->end - j->p < 4)
		return -1;
	*v = 0;
	for (int i = 0; i < 4; i++) {
		char c = *j->p++;
		uint32_t d;
		if (c >= '0' && c <= '9')
			d = (uint32_t)(c - '0');
		else if (c >= 'a' && c <= 'f')
			d = (uint32_t)(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			d = (uint32_t)(c - 'A' + 10);
		else
			return -1;
		*v = (*v << 4) | d;
	}
	return 0;
}

static size_t put_utf8(char *o, uint32_t c)
{
	if (c < 0x80) {
		o[0] = (char)c;
		return 1;
	}
	if (c < 0x800) {
		o[0] = (char)(0xc0 | (c >> 6));
		o[1] = (char)(0x80 | (c & 0x3f));
		return 2;
	}
	if (c < 0x10000) {
		o[0] = (char)(0xe0 | (c >> 12));
		o[1] = (char)(0x80 | ((c >> 6) & 0x3f));
		o[2] = (char)(0x80 | (c & 0x3f));
		return 3;
	}
	o[0] = (char)(0xf0 | (c >> 18));
	o[1] = (char)(0x80 | ((c >> 12) & 0x3f));
	o[2] = (char)(0x80 | ((c >> 6) & 0x3f));
	o[3] = (char)(0x80 | (c & 0x3f));
	return 4;
}

static int str(jp *j, const char **out, size_t *outlen)
{
	if (j->p >= j->end || *j->p != '"')
		return -1;
	j->p++;
	const char *s = j->p;
	size_t n = 0;
	while (s < j->end && *s != '"') {
		if (*s == '\\')
			s++;
		s++;
		n++;
	}
	if (s >= j->end)
		return -1;
	/* Escapes never lengthen beyond 4 bytes per source character. */
	char *o = mor_arena_alloc(j->a, (size_t)(s - j->p) + 1);
	if (!o)
		return -1;
	size_t k = 0;
	while (*j->p != '"') {
		unsigned char c = (unsigned char)*j->p++;
		if (c < 0x20)
			return -1;
		if (c != '\\') {
			o[k++] = (char)c;
			continue;
		}
		if (j->p >= j->end)
			return -1;
		c = (unsigned char)*j->p++;
		switch (c) {
		case '"': o[k++] = '"'; break;
		case '\\': o[k++] = '\\'; break;
		case '/': o[k++] = '/'; break;
		case 'b': o[k++] = '\b'; break;
		case 'f': o[k++] = '\f'; break;
		case 'n': o[k++] = '\n'; break;
		case 'r': o[k++] = '\r'; break;
		case 't': o[k++] = '\t'; break;
		case 'u': {
			uint32_t v, w;
			if (hex4(j, &v))
				return -1;
			if (v >= 0xd800 && v <= 0xdbff) {
				if (j->end - j->p < 6 || j->p[0] != '\\' || j->p[1] != 'u')
					return -1;
				j->p += 2;
				if (hex4(j, &w) || w < 0xdc00 || w > 0xdfff)
					return -1;
				v = 0x10000 + ((v - 0xd800) << 10) + (w - 0xdc00);
			} else if (v >= 0xdc00 && v <= 0xdfff) {
				return -1;
			}
			k += put_utf8(o + k, v);
			break;
		}
		default:
			return -1;
		}
	}
	j->p++;
	o[k] = 0;
	*out = o;
	*outlen = k;
	(void)n;
	return 0;
}

static int value(jp *j, json *v);

static int list(jp *j, json *v, int obj)
{
	/* Two passes: count, then fill, so storage is contiguous. */
	const char *save = j->p;
	size_t count = 0;
	unsigned d = 0;
	int instr = 0;
	for (const char *s = j->p; s < j->end; s++) {
		if (instr) {
			if (*s == '\\')
				s++;
			else if (*s == '"')
				instr = 0;
			continue;
		}
		if (*s == '"')
			instr = 1;
		else if (*s == '[' || *s == '{')
			d++;
		else if (*s == ']' || *s == '}') {
			if (d == 0)
				break;
			d--;
		} else if (*s == ',' && d == 0)
			count++;
	}
	ws(j);
	if (j->p < j->end && (*j->p == (obj ? '}' : ']'))) {
		j->p++;
		v->len = 0;
		return 0;
	}
	j->p = save;
	count++;
	v->items = mor_arena_array(j->a, count, sizeof *v->items);
	if (obj)
		v->keys = mor_arena_array(j->a, count, sizeof *v->keys);
	if (!v->items || (obj && !v->keys))
		return -1;
	for (size_t i = 0; i < count; i++) {
		ws(j);
		if (obj) {
			size_t kl;
			if (str(j, &v->keys[i], &kl))
				return -1;
			ws(j);
			if (j->p >= j->end || *j->p++ != ':')
				return -1;
		}
		if (value(j, &v->items[i]))
			return -1;
		ws(j);
		if (j->p >= j->end)
			return -1;
		char c = *j->p++;
		if (i + 1 < count) {
			if (c != ',')
				return -1;
		} else if (c != (obj ? '}' : ']')) {
			return -1;
		}
	}
	v->len = count;
	return 0;
}

static int value(jp *j, json *v)
{
	memset(v, 0, sizeof *v);
	ws(j);
	if (j->p >= j->end || ++j->depth > 512)
		return -1;
	int r = 0;
	char c = *j->p;
	if (c == '{') {
		j->p++;
		v->type = JSON_OBJ;
		r = list(j, v, 1);
	} else if (c == '[') {
		j->p++;
		v->type = JSON_ARR;
		r = list(j, v, 0);
	} else if (c == '"') {
		v->type = JSON_STR;
		r = str(j, &v->s, &v->len);
	} else if (j->end - j->p >= 4 && !memcmp(j->p, "null", 4)) {
		v->type = JSON_NULL;
		j->p += 4;
	} else if (j->end - j->p >= 4 && !memcmp(j->p, "true", 4)) {
		v->type = JSON_TRUE;
		j->p += 4;
	} else if (j->end - j->p >= 5 && !memcmp(j->p, "false", 5)) {
		v->type = JSON_FALSE;
		j->p += 5;
	} else if (c == '-' || (c >= '0' && c <= '9')) {
		const char *s = j->p;
		int neg = 0, frac = 0, ovf = 0;
		uint64_t x = 0;
		if (*j->p == '-') {
			neg = 1;
			j->p++;
		}
		if (j->p >= j->end || *j->p < '0' || *j->p > '9')
			return -1;
		while (j->p < j->end && *j->p >= '0' && *j->p <= '9') {
			uint64_t d = (uint64_t)(*j->p - '0');
			if (x > (UINT64_C(9223372036854775807) - d) / 10)
				ovf = 1;
			else
				x = x * 10 + d;
			j->p++;
		}
		while (j->p < j->end && (*j->p == '.' || *j->p == 'e' || *j->p == 'E' || *j->p == '+' ||
					 *j->p == '-' || (*j->p >= '0' && *j->p <= '9'))) {
			frac = 1;
			j->p++;
		}
		v->type = JSON_NUM;
		char *t = mor_arena_alloc(j->a, (size_t)(j->p - s) + 1);
		if (!t)
			return -1;
		memcpy(t, s, (size_t)(j->p - s));
		v->s = t;
		v->is_int = !frac && !ovf;
		v->i = neg ? -(int64_t)x : (int64_t)x;
	} else {
		r = -1;
	}
	j->depth--;
	return r;
}

int json_parse(const char *text, size_t len, mor_arena *a, const json **out)
{
	jp j = { text, text + len, a, 0 };
	json *v = mor_arena_alloc(a, sizeof *v);
	if (!v || value(&j, v))
		return -1;
	ws(&j);
	if (j.p != j.end)
		return -1;
	*out = v;
	return 0;
}

const json *json_get(const json *obj, const char *key)
{
	if (!obj || obj->type != JSON_OBJ)
		return NULL;
	for (size_t i = 0; i < obj->len; i++)
		if (!strcmp(obj->keys[i], key))
			return &obj->items[i];
	return NULL;
}

const json *json_at(const json *arr, size_t i)
{
	if (!arr || (arr->type != JSON_ARR && arr->type != JSON_OBJ) || i >= arr->len)
		return NULL;
	return &arr->items[i];
}

const char *json_str(const json *j)
{
	return j && j->type == JSON_STR ? j->s : NULL;
}

typedef struct rbuf {
	char *p;
	size_t cap, len;
} rbuf;

static void rput(rbuf *b, const char *s, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (b->len + 1 < b->cap)
			b->p[b->len] = s[i];
		b->len++;
	}
}

static void rputs(rbuf *b, const char *s)
{
	rput(b, s, strlen(s));
}

static void rstr(rbuf *b, const char *s, size_t n)
{
	rput(b, "\"", 1);
	for (size_t i = 0; i < n; i++) {
		char c = s[i];
		if (c == '"' || c == '\\') {
			rput(b, "\\", 1);
			rput(b, &c, 1);
		} else if ((unsigned char)c < 0x20) {
			char e[8];
			e[0] = '\\';
			e[1] = 'u';
			e[2] = '0';
			e[3] = '0';
			e[4] = "0123456789abcdef"[(unsigned char)c >> 4];
			e[5] = "0123456789abcdef"[(unsigned char)c & 15];
			rput(b, e, 6);
		} else {
			rput(b, &c, 1);
		}
	}
	rput(b, "\"", 1);
}

static void rval(rbuf *b, const json *j)
{
	switch (j->type) {
	case JSON_NULL: rputs(b, "null"); break;
	case JSON_TRUE: rputs(b, "true"); break;
	case JSON_FALSE: rputs(b, "false"); break;
	case JSON_NUM: rputs(b, j->s); break;
	case JSON_STR: rstr(b, j->s, j->len); break;
	case JSON_ARR:
		rput(b, "[", 1);
		for (size_t i = 0; i < j->len; i++) {
			if (i)
				rput(b, ",", 1);
			rval(b, &j->items[i]);
		}
		rput(b, "]", 1);
		break;
	case JSON_OBJ: {
		/* keys sorted; small objects, so selection is enough */
		size_t done[64];
		size_t nd = 0;
		rput(b, "{", 1);
		for (size_t k = 0; k < j->len && k < 64; k++) {
			size_t best = j->len;
			for (size_t i = 0; i < j->len; i++) {
				int used = 0;
				for (size_t d = 0; d < nd; d++)
					if (done[d] == i)
						used = 1;
				if (used)
					continue;
				if (best == j->len || strcmp(j->keys[i], j->keys[best]) < 0)
					best = i;
			}
			done[nd++] = best;
			if (k)
				rput(b, ",", 1);
			rstr(b, j->keys[best], strlen(j->keys[best]));
			rput(b, ":", 1);
			rval(b, &j->items[best]);
		}
		rput(b, "}", 1);
		break;
	}
	default: break;
	}
}

size_t json_render(const json *j, char *out, size_t cap)
{
	rbuf b = { out, cap, 0 };
	if (j->type == JSON_STR) {
		rput(&b, j->s, j->len);
	} else {
		rval(&b, j);
	}
	if (cap)
		out[b.len < cap ? b.len : cap - 1] = 0;
	return b.len;
}
