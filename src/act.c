#include "act.h"

#include <string.h>

#include "crypto.h"
#include "text.h"

const char *mor_open_name(int r)
{
	switch (r) {
	case MOR_OPEN_OK: return "ok";
	case MOR_OPEN_CBOR: return "cbor";
	case MOR_OPEN_SHAPE: return "shape";
	case MOR_OPEN_LOCKED_HASH: return "locked_hash";
	case MOR_OPEN_NO_KEY: return "no_key";
	case MOR_OPEN_UNLOCK: return "unlock";
	case MOR_OPEN_INSIDE_COMMITMENT: return "inside_commitment";
	case MOR_OPEN_TEXT: return "text";
	case MOR_OPEN_NOMEM: return "nomem";
	default: return "?";
	}
}

int mor_hash_list(const mor_cbor *c, int allow_empty)
{
	if (!c || c->type != MOR_CBOR_ARRAY)
		return 0;
	if (!allow_empty && c->v == 0)
		return 0;
	for (uint64_t i = 0; i < c->v; i++)
		if (!mor_cbor_is_hash(&c->items[i]))
			return 0;
	return 1;
}

int mor_act_names_in(const mor_cbor *hashlist, const uint8_t id[32])
{
	if (!hashlist || hashlist->type != MOR_CBOR_ARRAY)
		return 0;
	for (uint64_t i = 0; i < hashlist->v; i++)
		if (mor_cbor_bytes_eq(&hashlist->items[i], id, 32))
			return 1;
	return 0;
}

int mor_cbor_texts_canonical(const mor_cbor *c, mor_arena *a)
{
	uint64_t n = 0;
	switch (c->type) {
	case MOR_CBOR_TEXT: {
		int r = mor_text_check(c->data, (size_t)c->v, a);
		if (r == MOR_TEXT_NOMEM)
			return -1;
		return r ? 6 + r : 0;
	}
	case MOR_CBOR_ARRAY: n = c->v; break;
	case MOR_CBOR_MAP: n = 2 * c->v; break;
	case MOR_CBOR_TAG: n = 1; break;
	default: return 0;
	}
	for (uint64_t i = 0; i < n; i++) {
		int r = mor_cbor_texts_canonical(&c->items[i], a);
		if (r)
			return r;
	}
	return 0;
}

static int bstr_size(const mor_cbor *c, uint64_t n)
{
	return c && c->type == MOR_CBOR_BYTES && c->v == n;
}

/* outside = { ? 0 hash, ? 1 hash, 2 hash, 3 hash, 4 bstr24, ? 5 bstr32, ? 6 [+ hash] } */
static int outside_shape(mor_act *act, const mor_cbor *o)
{
	if (!mor_cbor_map_closed(o, 0x7fu))
		return 0;
	const mor_cbor *f;
	if ((f = mor_cbor_map_get(o, 0))) {
		if (!mor_cbor_is_hash(f))
			return 0;
		act->signer = f->data;
	}
	if ((f = mor_cbor_map_get(o, 1))) {
		if (!mor_cbor_is_hash(f))
			return 0;
		act->binding = f->data;
	}
	f = mor_cbor_map_get(o, 2);
	if (!mor_cbor_is_hash(f))
		return 0;
	act->commit = f->data;
	f = mor_cbor_map_get(o, 3);
	if (!mor_cbor_is_hash(f))
		return 0;
	act->locked_hash = f->data;
	f = mor_cbor_map_get(o, 4);
	if (!bstr_size(f, 24))
		return 0;
	act->nonce = f->data;
	if ((f = mor_cbor_map_get(o, 5))) {
		if (!bstr_size(f, 32))
			return 0;
		act->key = f->data;
	}
	if ((f = mor_cbor_map_get(o, 6))) {
		if (!mor_hash_list(f, 0))
			return 0;
		act->to = f;
	}
	act->outside = o;
	return 1;
}

/* signature = [ scheme: 1 / 2 / 3 / hash, key: bstr, sig: bstr ], founding
 * schemes with the lengths of Identity's table. */
static int sig_shape(mor_sigval *s, const mor_cbor *c)
{
	if (!c || c->type != MOR_CBOR_ARRAY || c->v != 3)
		return 0;
	const mor_cbor *sc = &c->items[0], *k = &c->items[1], *sg = &c->items[2];
	if (k->type != MOR_CBOR_BYTES || sg->type != MOR_CBOR_BYTES)
		return 0;
	if (sc->type == MOR_CBOR_UINT && sc->v >= 1 && sc->v <= 3) {
		s->kind = MOR_SCHEME_NUM;
		s->num = sc->v;
		static const size_t klen[4] = { 0, 32, 32, 32 };
		static const size_t slen[4] = { 0, 64, 7856, 17088 };
		if (k->v != klen[sc->v] || sg->v != slen[sc->v])
			return 0;
	} else if (mor_cbor_is_hash(sc)) {
		s->kind = MOR_SCHEME_HASH;
		s->hash = sc->data;
	} else {
		return 0;
	}
	s->key = k->data;
	s->key_len = (size_t)k->v;
	s->sig = sg->data;
	s->sig_len = (size_t)sg->v;
	return 1;
}

static int is_object(const mor_cbor *c)
{
	return c->type == MOR_CBOR_ARRAY && c->v == 2 && mor_cbor_is_hash(&c->items[0]) &&
	       mor_cbor_is_hash(&c->items[1]);
}

static int is_ref(const mor_cbor *c)
{
	if (mor_cbor_is_hash(c))
		return 1;
	if (c->type != MOR_CBOR_ARRAY)
		return 0;
	if (c->v == 2 && mor_cbor_is_hash(&c->items[0]) && mor_cbor_is_hash(&c->items[1]))
		return 1;
	if ((c->v == 1 || c->v == 2) && c->items[0].type == MOR_CBOR_TEXT)
		return c->v == 1 || mor_cbor_is_hash(&c->items[1]);
	return 0;
}

/* inside = { 0 hash, 1 uint, ? 2 [* hash], ? 3 [+ object], 4 map, ? 5 uint,
 *            ? 6 hash, ? 7 [+ hash], ? 8 [+ ref], ? 9 tstr, 10 bstr16 } */
static int inside_shape(mor_act *act, const mor_cbor *in)
{
	if (!mor_cbor_map_closed(in, 0x7ffu))
		return 0;
	const mor_cbor *f = mor_cbor_map_get(in, 0);
	if (!mor_cbor_is_hash(f))
		return 0;
	act->spec = f->data;
	f = mor_cbor_map_get(in, 1);
	if (!f || f->type != MOR_CBOR_UINT)
		return 0;
	act->type = f->v;
	if ((f = mor_cbor_map_get(in, 2))) {
		if (!mor_hash_list(f, 1))
			return 0;
		act->prev = f;
	}
	if ((f = mor_cbor_map_get(in, 3))) {
		if (f->type != MOR_CBOR_ARRAY || f->v == 0)
			return 0;
		for (uint64_t i = 0; i < f->v; i++)
			if (!is_object(&f->items[i]))
				return 0;
		act->objects = f;
	}
	f = mor_cbor_map_get(in, 4);
	if (!f || f->type != MOR_CBOR_MAP)
		return 0;
	act->payload = f;
	if ((f = mor_cbor_map_get(in, 5))) {
		if (f->type != MOR_CBOR_UINT)
			return 0;
		act->has_pos = 1;
		act->pos = f->v;
	}
	if ((f = mor_cbor_map_get(in, 6))) {
		if (!mor_cbor_is_hash(f))
			return 0;
		act->summary = f->data;
	}
	if ((f = mor_cbor_map_get(in, 7))) {
		if (!mor_hash_list(f, 0))
			return 0;
		act->acks = f;
	}
	if ((f = mor_cbor_map_get(in, 8))) {
		if (f->type != MOR_CBOR_ARRAY || f->v == 0)
			return 0;
		for (uint64_t i = 0; i < f->v; i++)
			if (!is_ref(&f->items[i]))
				return 0;
		act->refs = f;
	}
	if ((f = mor_cbor_map_get(in, 9))) {
		if (f->type != MOR_CBOR_TEXT)
			return 0;
		act->hint = f;
	}
	f = mor_cbor_map_get(in, 10);
	if (!bstr_size(f, 16))
		return 0;
	act->salt = f->data;
	act->inside = in;
	return 1;
}

static int open_locked(mor_act *act, const uint8_t *locked, size_t llen, const uint8_t *key, mor_arena *a)
{
	uint8_t h[32];
	mor_sha256(h, locked, llen);
	if (memcmp(h, act->locked_hash, 32))
		return MOR_OPEN_LOCKED_HASH;
	if (!key)
		key = act->key;
	if (!key)
		return MOR_OPEN_NO_KEY;
	if (llen < 16)
		return MOR_OPEN_UNLOCK;
	act->inside_len = llen - 16;
	act->inside_bytes = mor_arena_alloc(a, act->inside_len + 1);
	if (!act->inside_bytes)
		return MOR_OPEN_NOMEM;
	if (mor_xchacha_open(act->inside_bytes, locked, llen, act->nonce, key))
		return MOR_OPEN_UNLOCK;
	mor_tagged_hash(h, "MOR/inside", act->inside_bytes, act->inside_len);
	if (memcmp(h, act->commit, 32))
		return MOR_OPEN_INSIDE_COMMITMENT;
	const mor_cbor *in;
	int e = mor_cbor_decode(act->inside_bytes, act->inside_len, a, &in);
	if (e == MOR_CBOR_E_NOMEM)
		return MOR_OPEN_NOMEM;
	if (e) {
		act->cbor_err = e;
		return MOR_OPEN_CBOR;
	}
	if (!inside_shape(act, in))
		return MOR_OPEN_SHAPE;
	int t = mor_cbor_texts_canonical(in, a);
	if (t < 0)
		return MOR_OPEN_NOMEM;
	if (t) {
		act->text_rule = t;
		return MOR_OPEN_TEXT;
	}
	return MOR_OPEN_OK;
}

int mor_act_decode(mor_act *act, const uint8_t *bytes, size_t len, const uint8_t *key, mor_arena *a)
{
	const mor_cbor *root;
	memset(act, 0, sizeof *act);
	act->bytes = bytes;
	act->len = len;
	int e = mor_cbor_decode(bytes, len, a, &root);
	if (e == MOR_CBOR_E_NOMEM)
		return act->open = MOR_OPEN_NOMEM;
	if (e) {
		act->cbor_err = e;
		return act->open = MOR_OPEN_CBOR;
	}
	if (root->type != MOR_CBOR_ARRAY || root->v != 3 || root->items[0].type != MOR_CBOR_MAP ||
	    root->items[1].type != MOR_CBOR_BYTES)
		return act->open = MOR_OPEN_SHAPE;
	if (!outside_shape(act, &root->items[0]))
		return act->open = MOR_OPEN_SHAPE;
	mor_tagged_hash(act->id, "MOR/act", act->outside->raw, act->outside->raw_len);
	act->locked = &root->items[1];
	act->sig_shape_ok = sig_shape(&act->sig, &root->items[2]);
	int r = open_locked(act, act->locked->data, (size_t)act->locked->v, key, a);
	if (r == MOR_OPEN_OK && !act->sig_shape_ok)
		r = MOR_OPEN_SHAPE;
	return act->open = r;
}

int mor_act_open_parts(mor_act *act, const uint8_t *outside, size_t outside_len, const uint8_t *locked,
		       size_t locked_len, mor_arena *a)
{
	const mor_cbor *o;
	memset(act, 0, sizeof *act);
	int e = mor_cbor_decode(outside, outside_len, a, &o);
	if (e == MOR_CBOR_E_NOMEM)
		return act->open = MOR_OPEN_NOMEM;
	if (e) {
		act->cbor_err = e;
		return act->open = MOR_OPEN_CBOR;
	}
	if (!outside_shape(act, o))
		return act->open = MOR_OPEN_SHAPE;
	mor_tagged_hash(act->id, "MOR/act", outside, outside_len);
	return act->open = open_locked(act, locked, locked_len, NULL, a);
}

int mor_act_sig_verify(const mor_act *act)
{
	const mor_sigval *s = &act->sig;
	if (!act->sig_shape_ok)
		return 0;
	if (s->kind == MOR_SCHEME_HASH)
		return -1;
#ifdef MOR_FUZZ_NOSIG
	/* Fuzz-only build: signatures are taken as valid so the engine's logic
	 * behind them can be explored. Never built into the verifier. */
	return 1;
#else
	if (s->num == 1)
		return mor_schnorr_verify(s->key, act->id, s->sig);
	int r = mor_slhdsa_verify((int)s->num, s->key, act->id, 32, s->sig, s->sig_len,
				  (const uint8_t *)"MOR", 3);
	return r == 1;
#endif
}

void mor_chain_commit(uint8_t out[32], const mor_sigval *s)
{
	uint8_t sch[32];
	size_t n;
	if (s->kind == MOR_SCHEME_NUM) {
		sch[0] = (uint8_t)s->num;
		n = 1;
	} else {
		memcpy(sch, s->hash, 32);
		n = 32;
	}
	mor_tagged_hash2(out, "MOR/safety", sch, n, s->key, s->key_len);
}
