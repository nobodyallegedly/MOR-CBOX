#include "cbor.h"

#include <string.h>

const char *mor_cbor_err_name(int err)
{
	switch (err) {
	case MOR_CBOR_OK: return "ok";
	case MOR_CBOR_E_END: return "UnexpectedEnd";
	case MOR_CBOR_E_NONSHORTEST: return "NonShortestArgument";
	case MOR_CBOR_E_INDEFINITE: return "IndefiniteLength";
	case MOR_CBOR_E_UNSORTED: return "UnsortedMapKeys";
	case MOR_CBOR_E_DUPKEY: return "DuplicateMapKey";
	case MOR_CBOR_E_FLOAT: return "NonPreferredFloat";
	case MOR_CBOR_E_UTF8: return "InvalidUtf8";
	case MOR_CBOR_E_SIMPLE: return "InvalidSimpleValue";
	case MOR_CBOR_E_RESERVED: return "ReservedAdditionalInfo";
	case MOR_CBOR_E_TRAILING: return "TrailingBytes";
	case MOR_CBOR_E_DEPTH: return "TooDeep";
	case MOR_CBOR_E_NOMEM: return "OutOfArena";
	default: return "unknown";
	}
}

int mor_utf8_valid(const uint8_t *s, size_t n)
{
	size_t i = 0;
	while (i < n) {
		uint8_t c = s[i];
		if (c < 0x80) {
			i++;
			continue;
		}
		size_t need;
		uint32_t cp, min;
		if (c >= 0xc2 && c <= 0xdf) {
			need = 1; cp = c & 0x1fu; min = 0x80;
		} else if (c >= 0xe0 && c <= 0xef) {
			need = 2; cp = c & 0x0fu; min = 0x800;
		} else if (c >= 0xf0 && c <= 0xf4) {
			need = 3; cp = c & 0x07u; min = 0x10000;
		} else {
			return 0;
		}
		if (n - i - 1 < need)
			return 0;
		for (size_t k = 1; k <= need; k++) {
			uint8_t cc = s[i + k];
			if ((cc & 0xc0) != 0x80)
				return 0;
			cp = (cp << 6) | (cc & 0x3fu);
		}
		if (cp < min || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
			return 0;
		i += need + 1;
	}
	return 1;
}

/* Is the value M * 2^E exactly representable in a binary format with p
 * significand bits (hidden bit included) and normal exponents emin..emax? */
static int fits(uint64_t m, int e, int p, int emin, int emax)
{
	if (m == 0)
		return 1;
	while ((m & 1u) == 0) {
		m >>= 1;
		e++;
	}
	int bits = 0;
	for (uint64_t t = m; t; t >>= 1)
		bits++;
	if (bits > p)
		return 0;
	if (e + bits - 1 > emax)
		return 0;
	if (e < emin - (p - 1))
		return 0;
	return 1;
}

static int single_fits_half(uint32_t b)
{
	uint32_t ex = (b >> 23) & 0xffu, m = b & 0x7fffffu;
	if (ex == 0xff)
		return (m & 0x1fffu) == 0;
	if (ex == 0)
		return fits(m, -149, 11, -14, 15);
	return fits(m | 0x800000u, (int)ex - 150, 11, -14, 15);
}

static int double_fits_single(uint64_t b)
{
	uint64_t ex = (b >> 52) & 0x7ffu, m = b & ((UINT64_C(1) << 52) - 1);
	if (ex == 0x7ff)
		return (m & ((UINT64_C(1) << 29) - 1)) == 0;
	if (ex == 0)
		return fits(m, -1074, 24, -126, 127);
	return fits(m | (UINT64_C(1) << 52), (int)ex - 1075, 24, -126, 127);
}

typedef struct dec {
	const uint8_t *p;
	const uint8_t *end;
	mor_arena *a;
} dec;

static int key_cmp(const mor_cbor *x, const mor_cbor *y)
{
	size_t n = x->raw_len < y->raw_len ? x->raw_len : y->raw_len;
	int c = memcmp(x->raw, y->raw, n);
	if (c)
		return c;
	if (x->raw_len == y->raw_len)
		return 0;
	return x->raw_len < y->raw_len ? -1 : 1;
}

static int read_arg(dec *d, unsigned ai, uint64_t *v)
{
	size_t n;
	if (ai < 24) {
		*v = ai;
		return MOR_CBOR_OK;
	}
	switch (ai) {
	case 24: n = 1; break;
	case 25: n = 2; break;
	case 26: n = 4; break;
	case 27: n = 8; break;
	default: return MOR_CBOR_E_RESERVED;
	}
	if ((size_t)(d->end - d->p) < n)
		return MOR_CBOR_E_END;
	uint64_t x = 0;
	for (size_t i = 0; i < n; i++)
		x = (x << 8) | d->p[i];
	d->p += n;
	*v = x;
	return MOR_CBOR_OK;
}

static int shortest(unsigned ai, uint64_t v)
{
	switch (ai) {
	case 24: return v >= 24;
	case 25: return v > 0xff;
	case 26: return v > 0xffff;
	case 27: return v > 0xffffffffu;
	default: return 1;
	}
}

static int decode_item(dec *d, mor_cbor *out, unsigned level)
{
	if (level > MOR_CBOR_MAX_DEPTH)
		return MOR_CBOR_E_DEPTH;
	const uint8_t *start = d->p;
	if (d->p >= d->end)
		return MOR_CBOR_E_END;
	uint8_t ib = *d->p++;
	unsigned major = ib >> 5, ai = ib & 0x1fu;
	uint64_t v = 0;
	int err;

	memset(out, 0, sizeof *out);
	out->raw = start;

	if (major == 7) {
		if (ai < 24) {
			out->type = MOR_CBOR_SIMPLE;
			out->v = ai;
		} else if (ai == 24) {
			if ((err = read_arg(d, ai, &v)))
				return err;
			if (v < 32)
				return MOR_CBOR_E_SIMPLE;
			out->type = MOR_CBOR_SIMPLE;
			out->v = v;
		} else if (ai >= 25 && ai <= 27) {
			if ((err = read_arg(d, ai, &v)))
				return err;
			out->type = MOR_CBOR_FLOAT;
			out->v = v;
			if (ai == 25) {
				out->fwidth = 2;
			} else if (ai == 26) {
				out->fwidth = 4;
				if (single_fits_half((uint32_t)v))
					return MOR_CBOR_E_FLOAT;
			} else {
				out->fwidth = 8;
				if (double_fits_single(v))
					return MOR_CBOR_E_FLOAT;
			}
		} else if (ai == 31) {
			return MOR_CBOR_E_INDEFINITE;
		} else {
			return MOR_CBOR_E_RESERVED;
		}
		out->raw_len = (size_t)(d->p - start);
		return MOR_CBOR_OK;
	}

	if (ai == 31)
		return (major >= 2 && major <= 5) ? MOR_CBOR_E_INDEFINITE : MOR_CBOR_E_RESERVED;
	if ((err = read_arg(d, ai, &v)))
		return err;
	if (!shortest(ai, v))
		return MOR_CBOR_E_NONSHORTEST;
	out->v = v;

	switch (major) {
	case 0:
		out->type = MOR_CBOR_UINT;
		break;
	case 1:
		out->type = MOR_CBOR_NINT;
		break;
	case 2:
	case 3:
		if ((uint64_t)(d->end - d->p) < v)
			return MOR_CBOR_E_END;
		out->type = major == 2 ? MOR_CBOR_BYTES : MOR_CBOR_TEXT;
		out->data = d->p;
		if (major == 3 && !mor_utf8_valid(d->p, (size_t)v))
			return MOR_CBOR_E_UTF8;
		d->p += v;
		break;
	case 4:
	case 5: {
		uint64_t n = v;
		/* Every item takes at least one byte: a count beyond what is left
		 * cannot be met, and is refused before anything is allocated. */
		uint64_t left = (uint64_t)(d->end - d->p);
		if (major == 5) {
			if (n > left / 2)
				return MOR_CBOR_E_END;
			n *= 2;
		} else if (n > left) {
			return MOR_CBOR_E_END;
		}
		out->type = major == 4 ? MOR_CBOR_ARRAY : MOR_CBOR_MAP;
		if (n) {
			out->items = mor_arena_array(d->a, (size_t)n, sizeof *out->items);
			if (!out->items)
				return MOR_CBOR_E_NOMEM;
		}
		for (uint64_t i = 0; i < n; i++) {
			if ((err = decode_item(d, &out->items[i], level + 1)))
				return err;
			if (major == 5 && (i & 1u) == 0 && i >= 2) {
				int c = key_cmp(&out->items[i - 2], &out->items[i]);
				if (c == 0)
					return MOR_CBOR_E_DUPKEY;
				if (c > 0)
					return MOR_CBOR_E_UNSORTED;
			}
		}
		break;
	}
	case 6:
		out->type = MOR_CBOR_TAG;
		out->items = mor_arena_alloc(d->a, sizeof *out->items);
		if (!out->items)
			return MOR_CBOR_E_NOMEM;
		if ((err = decode_item(d, out->items, level + 1)))
			return err;
		break;
	default:
		return MOR_CBOR_E_RESERVED;
	}
	out->raw_len = (size_t)(d->p - start);
	return MOR_CBOR_OK;
}

int mor_cbor_decode(const uint8_t *buf, size_t len, mor_arena *a, const mor_cbor **out)
{
	dec d = { buf, buf + len, a };
	mor_cbor *root = mor_arena_alloc(a, sizeof *root);
	*out = NULL;
	if (!root)
		return MOR_CBOR_E_NOMEM;
	if (!buf && len)
		return MOR_CBOR_E_END;
	if (len == 0)
		return MOR_CBOR_E_END;
	int err = decode_item(&d, root, 0);
	if (err)
		return err;
	if (d.p != d.end)
		return MOR_CBOR_E_TRAILING;
	*out = root;
	return MOR_CBOR_OK;
}

const mor_cbor *mor_cbor_map_get(const mor_cbor *map, uint64_t key)
{
	if (!map || map->type != MOR_CBOR_MAP)
		return NULL;
	for (uint64_t i = 0; i < map->v; i++) {
		const mor_cbor *k = &map->items[2 * i];
		if (k->type == MOR_CBOR_UINT && k->v == key)
			return &map->items[2 * i + 1];
	}
	return NULL;
}

int mor_cbor_is_null(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_SIMPLE && c->v == 22;
}

int mor_cbor_is_true(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_SIMPLE && c->v == 21;
}

int mor_cbor_is_hash(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_BYTES && c->v == 32;
}

int mor_cbor_bytes_eq(const mor_cbor *c, const uint8_t *p, size_t n)
{
	return c && c->type == MOR_CBOR_BYTES && c->v == n && (n == 0 || memcmp(c->data, p, n) == 0);
}

int mor_cbor_map_closed(const mor_cbor *map, uint64_t allowed_mask)
{
	if (!map || map->type != MOR_CBOR_MAP)
		return 0;
	for (uint64_t i = 0; i < map->v; i++) {
		const mor_cbor *k = &map->items[2 * i];
		if (k->type != MOR_CBOR_UINT || k->v > 63)
			return 0;
		if (!((allowed_mask >> k->v) & 1u))
			return 0;
	}
	return 1;
}

void mor_buf_init(mor_buf *b, void *mem, size_t cap)
{
	b->p = mem;
	b->cap = cap;
	b->len = 0;
	b->overflow = 0;
}

void mor_buf_put(mor_buf *b, const void *p, size_t n)
{
	if (b->overflow || n > b->cap - b->len) {
		b->overflow = 1;
		return;
	}
	if (n)
		memcpy(b->p + b->len, p, n);
	b->len += n;
}

void mor_cbor_put_head(mor_buf *b, unsigned major, uint64_t v)
{
	uint8_t h[9];
	size_t n;
	major <<= 5;
	if (v < 24) {
		h[0] = (uint8_t)(major | v);
		n = 1;
	} else if (v <= 0xff) {
		h[0] = (uint8_t)(major | 24);
		h[1] = (uint8_t)v;
		n = 2;
	} else if (v <= 0xffff) {
		h[0] = (uint8_t)(major | 25);
		h[1] = (uint8_t)(v >> 8);
		h[2] = (uint8_t)v;
		n = 3;
	} else if (v <= 0xffffffffu) {
		h[0] = (uint8_t)(major | 26);
		for (int i = 0; i < 4; i++)
			h[1 + i] = (uint8_t)(v >> (24 - 8 * i));
		n = 5;
	} else {
		h[0] = (uint8_t)(major | 27);
		for (int i = 0; i < 8; i++)
			h[1 + i] = (uint8_t)(v >> (56 - 8 * i));
		n = 9;
	}
	mor_buf_put(b, h, n);
}

void mor_cbor_put_uint(mor_buf *b, uint64_t v)
{
	mor_cbor_put_head(b, 0, v);
}

void mor_cbor_put_bytes(mor_buf *b, const uint8_t *p, size_t n)
{
	mor_cbor_put_head(b, 2, n);
	mor_buf_put(b, p, n);
}

void mor_cbor_put_text(mor_buf *b, const char *p, size_t n)
{
	mor_cbor_put_head(b, 3, n);
	mor_buf_put(b, p, n);
}

void mor_cbor_encode(mor_buf *b, const mor_cbor *c)
{
	switch (c->type) {
	case MOR_CBOR_UINT: mor_cbor_put_head(b, 0, c->v); break;
	case MOR_CBOR_NINT: mor_cbor_put_head(b, 1, c->v); break;
	case MOR_CBOR_BYTES: mor_cbor_put_bytes(b, c->data, (size_t)c->v); break;
	case MOR_CBOR_TEXT: mor_cbor_put_text(b, (const char *)c->data, (size_t)c->v); break;
	case MOR_CBOR_ARRAY:
		mor_cbor_put_head(b, 4, c->v);
		for (uint64_t i = 0; i < c->v; i++)
			mor_cbor_encode(b, &c->items[i]);
		break;
	case MOR_CBOR_MAP:
		mor_cbor_put_head(b, 5, c->v);
		for (uint64_t i = 0; i < 2 * c->v; i++)
			mor_cbor_encode(b, &c->items[i]);
		break;
	case MOR_CBOR_TAG:
		mor_cbor_put_head(b, 6, c->v);
		mor_cbor_encode(b, c->items);
		break;
	case MOR_CBOR_SIMPLE:
		if (c->v < 24) {
			uint8_t x = (uint8_t)(0xe0u | c->v);
			mor_buf_put(b, &x, 1);
		} else {
			uint8_t x[2] = { 0xf8, (uint8_t)c->v };
			mor_buf_put(b, x, 2);
		}
		break;
	case MOR_CBOR_FLOAT: {
		uint8_t x[9];
		x[0] = c->fwidth == 2 ? 0xf9 : c->fwidth == 4 ? 0xfa : 0xfb;
		for (unsigned i = 0; i < c->fwidth; i++)
			x[1 + i] = (uint8_t)(c->v >> (8 * (c->fwidth - 1 - i)));
		mor_buf_put(b, x, 1u + c->fwidth);
		break;
	}
	default:
		b->overflow = 1;
	}
}
