#include "development.h"

#include <string.h>

#include "act.h"
#include "cbor.h"
#include "crypto.h"
#include "text.h"

#define NONE(r)                          \
	do {                             \
		out->verdict = MOR_SPEC_NONE; \
		out->why = (r);          \
		return out->verdict;     \
	} while (0)

static int is_uint(const mor_cbor *c)
{
	return c && c->type == MOR_CBOR_UINT;
}

static int ascending_uints(const mor_cbor *c, uint64_t max, int allow_empty)
{
	if (!c || c->type != MOR_CBOR_ARRAY || (!allow_empty && c->v == 0))
		return 0;
	for (uint64_t i = 0; i < c->v; i++) {
		if (!is_uint(&c->items[i]) || c->items[i].v > max)
			return 0;
		if (i && c->items[i].v <= c->items[i - 1].v)
			return 0;
	}
	return 1;
}

static int holds(const mor_cbor *c, uint64_t v)
{
	for (uint64_t i = 0; i < c->v; i++)
		if (c->items[i].v == v)
			return 1;
	return 0;
}

static const mor_spec_content *held_spec(const mor_world *w, const uint8_t *h)
{
	for (size_t i = 0; i < w->nspecs; i++)
		if (!memcmp(w->specs[i].hash, h, 32))
			return &w->specs[i];
	return NULL;
}

/* Steps 1 to 5 and the by-kind rules that need no other specification. */
static int decode_core(mor_world *w, const uint8_t *content, size_t len, mor_spec_result *out, const mor_cbor **map)
{
	const mor_cbor *m, *f;
	memset(out, 0, sizeof *out);
	mor_tagged_hash(out->hash, "MOR/spec", content, len);
	/* 1. deterministic CBOR holding one map */
	int e = mor_cbor_decode(content, len, w->a, &m);
	if (e == MOR_CBOR_E_NOMEM)
		NONE("out of memory");
	if (e || m->type != MOR_CBOR_MAP)
		NONE("not deterministic CBOR holding one map");
	/* 2. keys 0 to 13 */
	if (!mor_cbor_map_closed(m, 0x3fffu))
		NONE("a key the format does not define");
	/* 3. canonical text everywhere */
	int t = mor_cbor_texts_canonical(m, w->a);
	if (t)
		NONE("a text that is not canonical");
	/* 4. each field's type */
	if (!is_uint(mor_cbor_map_get(m, 0)))
		NONE("field 0");
	if ((f = mor_cbor_map_get(m, 1)) && !mor_cbor_is_hash(f))
		NONE("field 1");
	if ((f = mor_cbor_map_get(m, 2))) {
		if (f->type != MOR_CBOR_ARRAY || f->v == 0)
			NONE("field 2");
		for (uint64_t i = 0; i < f->v; i++) {
			const mor_cbor *r = &f->items[i];
			if (r->type != MOR_CBOR_ARRAY || r->v != 2 || !mor_cbor_is_hash(&r->items[0]) || !is_uint(&r->items[1]))
				NONE("field 2: a task-ref");
		}
	}
	if ((f = mor_cbor_map_get(m, 3))) {
		if (f->type != MOR_CBOR_ARRAY || f->v == 0)
			NONE("field 3");
		for (uint64_t i = 0; i < f->v; i++) {
			const mor_cbor *r = &f->items[i];
			if (r->type != MOR_CBOR_ARRAY || r->v != 3 || !is_uint(&r->items[0]) ||
			    r->items[1].type != MOR_CBOR_TEXT || r->items[2].type != MOR_CBOR_TEXT)
				NONE("field 3: a type-def");
		}
	}
	if ((f = mor_cbor_map_get(m, 4)) && !mor_hash_list(f, 0))
		NONE("field 4");
	if ((f = mor_cbor_map_get(m, 5)) && !mor_cbor_is_hash(f))
		NONE("field 5");
	if ((f = mor_cbor_map_get(m, 6))) {
		if (f->type != MOR_CBOR_ARRAY || f->v == 0)
			NONE("field 6");
		for (uint64_t i = 0; i < f->v; i++) {
			const mor_cbor *r = &f->items[i];
			if (r->type != MOR_CBOR_ARRAY || r->v != 4 || !mor_cbor_is_hash(&r->items[0]) || !is_uint(&r->items[1]) ||
			    !is_uint(&r->items[2]) || !mor_cbor_is_hash(&r->items[3]))
				NONE("field 6: a rule-ref");
		}
	}
	if ((f = mor_cbor_map_get(m, 7))) {
		if (f->type != MOR_CBOR_ARRAY || f->v < 1 || f->v > 2)
			NONE("field 7");
		const mor_cbor *rate = &f->items[0];
		if (rate->type != MOR_CBOR_ARRAY || rate->v != 2 || !is_uint(&rate->items[0]) || rate->items[0].v > 1 ||
		    (rate->items[0].v == 0 && !is_uint(&rate->items[1])))
			NONE("field 7: a rate");
		if (f->v == 2 && !mor_cbor_is_hash(&f->items[1]))
			NONE("field 7: who is paid");
	}
	if ((f = mor_cbor_map_get(m, 8)) && !mor_cbor_is_hash(f))
		NONE("field 8");
	if ((f = mor_cbor_map_get(m, 9)) && f->type != MOR_CBOR_TEXT)
		NONE("field 9");
	if ((f = mor_cbor_map_get(m, 11)) && !is_uint(f))
		NONE("field 11");
	if ((f = mor_cbor_map_get(m, 12)) && !mor_cbor_is_true(f))
		NONE("field 12");
	/* 5. a kind from 0 to 8, and field 9 */
	uint64_t kind = mor_cbor_map_get(m, 0)->v;
	out->kind = kind;
	if (kind > 8)
		NONE("field 0: no such kind");
	if (!mor_cbor_map_get(m, 9))
		NONE("field 9 missing");
	/* 6. by kind */
	int nocreator = kind == 4 || kind == 6 || kind == 7;
	if (nocreator ? mor_cbor_map_get(m, 1) != NULL : mor_cbor_map_get(m, 1) == NULL)
		NONE(nocreator ? "field 1 on a kind that carries no creator" : "field 1 (the creator) missing");
	if ((kind == 1) != (mor_cbor_map_get(m, 5) != NULL))
		NONE(kind == 1 ? "a Module without field 5" : "field 5 on a kind other than a Module");
	if (kind == 8 && !mor_cbor_map_get(m, 8))
		NONE("a MIP without field 8");
	const mor_cbor *f11 = mor_cbor_map_get(m, 11), *f13 = mor_cbor_map_get(m, 13);
	if (!f11 != !f13)
		NONE("fields 11 and 13 not together");
	if (f11) {
		if (kind != 1)
			NONE("fields 11 and 13 on a kind other than a Module");
		if (f11->v > 1)
			NONE("field 11");
		if (!ascending_uints(f13, 2, 1))
			NONE("field 13");
		if (f11->v == 0 && (!holds(f13, 0) || !holds(f13, 1)))
			NONE("a request rail whose field 13 lacks the payee or the purpose");
	}
	if (mor_cbor_map_get(m, 12) && kind != 0)
		NONE("field 12 on a kind other than a cMIP");
	if ((f = mor_cbor_map_get(m, 10)) && !ascending_uints(f, 3, 0))
		NONE("field 10");
	*map = m;
	return MOR_SPEC_OK;
}

/* Is the content a payment cMIP: a cMIP whose field 2 lists [MONEY, 6]? */
static int is_payment_cmip(mor_world *w, const mor_spec_content *c)
{
	mor_spec_result r;
	const mor_cbor *m = NULL;
	size_t mark = mor_arena_mark(w->a);
	int ok = 0;
	if (decode_core(w, c->bytes, c->len, &r, &m) == MOR_SPEC_OK && r.kind == 0) {
		const mor_cbor *t = mor_cbor_map_get(m, 2);
		for (uint64_t i = 0; t && i < t->v; i++)
			if (!memcmp(t->items[i].items[0].data, w->mir[MOR_MONEY], 32) && t->items[i].items[1].v == 6)
				ok = 1;
	}
	mor_arena_reset(w->a, mark);
	return ok;
}

int mor_spec_decode(mor_world *w, const uint8_t *content, size_t len, mor_spec_result *out)
{
	const mor_cbor *m = NULL;
	if (decode_core(w, content, len, out, &m) != MOR_SPEC_OK)
		return out->verdict;
	/* A Module whose field 5 names a payment cMIP carries fields 11 and 13:
	 * decided by decoding the cMIP field 5 names. */
	if (out->kind == 1 && !mor_cbor_map_get(m, 11)) {
		const uint8_t *target = mor_cbor_map_get(m, 5)->data;
		if (mor_world_mir_of(w, target) != MOR_MIR_NONE)
			return out->verdict = MOR_SPEC_OK; /* works under a MIR */
		const mor_spec_content *c = held_spec(w, target);
		if (!c) {
			memcpy(out->missing, target, 32);
			return out->verdict = MOR_SPEC_UNKNOWN;
		}
		if (is_payment_cmip(w, c)) {
			out->why = "a rail Module under a payment cMIP without fields 11 and 13";
			return out->verdict = MOR_SPEC_NONE;
		}
	}
	return out->verdict = MOR_SPEC_OK;
}

int mor_mir_hash(const uint8_t *file, size_t len, uint8_t hash[32], uint64_t kind, mor_arena *a,
		 const uint8_t **text, size_t *textlen)
{
	/* A file may end with exactly one line break, which is not part of the text. */
	if (len && file[len - 1] == '\n')
		len--;
	uint8_t *t = mor_arena_alloc(a, len + 1);
	if (!t)
		return -1;
	/* Nothing else is dropped: a draft's status line is part of the text,
	 * like every line of the file (Development rule 26). */
	size_t n = len;
	memcpy(t, file, len);
	size_t mark = mor_arena_mark(a);
	int r = mor_text_check(t, n, a);
	mor_arena_reset(a, mark);
	if (r)
		return -1;
	uint8_t *enc = mor_arena_alloc(a, n + 32);
	if (!enc)
		return -1;
	mor_buf b;
	mor_buf_init(&b, enc, n + 32);
	mor_cbor_put_head(&b, 5, 2);
	mor_cbor_put_uint(&b, 0);
	mor_cbor_put_uint(&b, kind);
	mor_cbor_put_uint(&b, 9);
	mor_cbor_put_text(&b, (const char *)t, n);
	if (b.overflow)
		return -1;
	mor_tagged_hash(hash, "MOR/spec", enc, b.len);
	*text = t;
	*textlen = n;
	return 0;
}
