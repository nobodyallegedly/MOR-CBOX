#include "world.h"

#include <string.h>

const char *mor_status_name(int st)
{
	switch (st) {
	case MOR_ST_VALID: return "valid";
	case MOR_ST_INVALID: return "invalid";
	case MOR_ST_UNKNOWN: return "unknown";
	case MOR_ST_PENDING: return "pending";
	case MOR_ST_VOID: return "void";
	case MOR_ST_DISPUTED: return "disputed";
	case MOR_ST_SCOPED: return "scoped";
	case MOR_ST_CONTESTED: return "contested";
	default: return "?";
	}
}

int mor_hash_eq(const uint8_t *x, const uint8_t *y)
{
	return x && y && memcmp(x, y, 32) == 0;
}

static int cmp_id(const mor_act *x, const mor_act *y)
{
	return memcmp(x->id, y->id, 32);
}

int mor_world_init(mor_world *w, mor_arena *a, const uint8_t mir[MOR_NMIRS][32], const uint8_t *const *acts,
		   const size_t *lens, size_t n)
{
	memset(w, 0, sizeof *w);
	w->a = a;
	memcpy(w->mir, mir, sizeof w->mir);
	w->acts = mor_arena_array(a, n ? n : 1, sizeof *w->acts);
	if (!w->acts)
		return -1;
	size_t k = 0;
	for (size_t i = 0; i < n; i++) {
		mor_act tmp;
		int r = mor_act_decode(&tmp, acts[i], lens[i], NULL, a);
		if (r == MOR_OPEN_NOMEM)
			return -1;
		if (!tmp.outside)
			continue; /* no id can be computed: not an act anyone can name */
		/* Insert sorted; on a duplicate id keep the copy whose signature verifies. */
		size_t lo = 0, hi = k;
		while (lo < hi) {
			size_t mid = lo + (hi - lo) / 2;
			int c = cmp_id(&w->acts[mid], &tmp);
			if (c < 0)
				lo = mid + 1;
			else
				hi = mid;
		}
		if (lo < k && !cmp_id(&w->acts[lo], &tmp)) {
			if (mor_act_sig_verify(&w->acts[lo]) != 1 && mor_act_sig_verify(&tmp) == 1)
				w->acts[lo] = tmp;
			continue;
		}
		memmove(&w->acts[lo + 1], &w->acts[lo], (k - lo) * sizeof *w->acts);
		w->acts[lo] = tmp;
		k++;
	}
	w->nacts = k;
	w->chains_cap = k + 1;
	w->chains = mor_arena_array(a, w->chains_cap, sizeof *w->chains);
	return w->chains ? 0 : -1;
}

const mor_act *mor_world_get(const mor_world *w, const uint8_t id[32])
{
	size_t lo = 0, hi = w->nacts;
	if (!id)
		return NULL;
	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		int c = memcmp(w->acts[mid].id, id, 32);
		if (c == 0)
			return &w->acts[mid];
		if (c < 0)
			lo = mid + 1;
		else
			hi = mid;
	}
	return NULL;
}

int mor_world_mir_of(const mor_world *w, const uint8_t spec[32])
{
	if (!spec)
		return MOR_MIR_NONE;
	for (int i = 0; i < MOR_NMIRS; i++)
		if (!memcmp(w->mir[i], spec, 32))
			return i;
	return MOR_MIR_NONE;
}

int mor_world_is_mir(const mor_world *w, const mor_act *a, int mir)
{
	return a && a->open == MOR_OPEN_OK && mor_world_mir_of(w, a->spec) == mir;
}

const mor_cbor *mor_pl(const mor_act *a, uint64_t key)
{
	return a && a->payload ? mor_cbor_map_get(a->payload, key) : NULL;
}

const uint8_t *mor_pl_hash(const mor_act *a, uint64_t key)
{
	const mor_cbor *c = mor_pl(a, key);
	return mor_cbor_is_hash(c) ? c->data : NULL;
}

int mor_objects_name(const mor_act *a, const uint8_t id[32])
{
	if (!a || !a->objects)
		return 0;
	for (uint64_t i = 0; i < a->objects->v; i++) {
		const mor_cbor *o = &a->objects->items[i];
		if (!memcmp(o->items[0].data, id, 32) || !memcmp(o->items[1].data, id, 32))
			return 1;
	}
	return 0;
}

int mor_objects_has(const mor_act *a, const uint8_t chain[32], const uint8_t pred[32])
{
	if (!a || !a->objects)
		return 0;
	for (uint64_t i = 0; i < a->objects->v; i++) {
		const mor_cbor *o = &a->objects->items[i];
		if (!memcmp(o->items[0].data, chain, 32) && !memcmp(o->items[1].data, pred, 32))
			return 1;
	}
	return 0;
}

int mor_acks_name(const mor_act *a, const uint8_t id[32])
{
	return a && a->acks && mor_act_names_in(a->acks, id);
}

const uint8_t *mor_prev(const mor_act *a)
{
	if (!a || !a->prev || a->prev->v != 1)
		return NULL;
	return a->prev->items[0].data;
}

const uint8_t *mor_signer(const mor_act *a)
{
	if (!a)
		return NULL;
	return a->signer ? a->signer : a->id;
}
