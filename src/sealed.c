#include "sealed.h"

#include <string.h>

#include "cbor.h"
#include "crypto.h"

const char *mor_sealed_err_name(int e)
{
	switch (e) {
	case MOR_SEALED_OK: return "ok";
	case MOR_SEALED_FORMAT: return "format";
	case MOR_SEALED_NOT_FOR_US: return "not addressed to this recipient";
	case MOR_SEALED_CAPSULE: return "capsule";
	case MOR_SEALED_CONTENTS: return "contents";
	case MOR_SEALED_INNER: return "inner act";
	case MOR_SEALED_KEY_PUBLIC: return "key given for a public act";
	case MOR_SEALED_ADDRESSING: return "addressing";
	default: return "nomem";
	}
}

static int is_bstr(const mor_cbor *c, uint64_t n)
{
	return c->type == MOR_CBOR_BYTES && c->v == n;
}

int mor_sealed_open_as(mor_sealed_open *out, const uint8_t *sealed, size_t len, const uint8_t *recipient,
		       const uint8_t xsk[32], mor_arena *a)
{
	const mor_cbor *c, *lc, *ct;
	memset(out, 0, sizeof *out);
	int e = mor_cbor_decode(sealed, len, a, &c);
	if (e == MOR_CBOR_E_NOMEM)
		return MOR_SEALED_NOMEM;
	/* sealed = [ to: [* hash], locked-act: bstr ] */
	if (e || c->type != MOR_CBOR_ARRAY || c->v != 2 || !mor_hash_list(&c->items[0], 1) ||
	    c->items[1].type != MOR_CBOR_BYTES)
		return MOR_SEALED_FORMAT;
	const mor_cbor *to = &c->items[0];
	e = mor_cbor_decode(c->items[1].data, (size_t)c->items[1].v, a, &lc);
	if (e == MOR_CBOR_E_NOMEM)
		return MOR_SEALED_NOMEM;
	/* locked-act-content = [ capsules: [+ capsule], nonce: bstr .size 24, locked: bstr ] */
	if (e || lc->type != MOR_CBOR_ARRAY || lc->v != 3 || lc->items[0].type != MOR_CBOR_ARRAY ||
	    lc->items[0].v == 0 || !is_bstr(&lc->items[1], 24) || lc->items[2].type != MOR_CBOR_BYTES)
		return MOR_SEALED_FORMAT;
	const mor_cbor *caps = &lc->items[0];
	for (uint64_t i = 0; i < caps->v; i++) {
		const mor_cbor *k = &caps->items[i];
		if (k->type != MOR_CBOR_ARRAY || k->v != 2 || !is_bstr(&k->items[0], MOR_XWING_CT) ||
		    !is_bstr(&k->items[1], 48))
			return MOR_SEALED_FORMAT;
	}
	/* Step 1: one capsule per recipient, in order; a bare key has none listed and one capsule. */
	if (to->v == 0 ? caps->v != 1 : caps->v != to->v)
		return MOR_SEALED_FORMAT;
	uint64_t idx = 0;
	if (recipient) {
		int found = 0;
		for (uint64_t i = 0; i < to->v; i++)
			if (!memcmp(to->items[i].data, recipient, 32)) {
				idx = i;
				found = 1;
				break;
			}
		if (!found)
			return MOR_SEALED_NOT_FOR_US;
	} else if (to->v != 0) {
		return MOR_SEALED_NOT_FOR_US;
	}
	/* Step 4: decapsulate, unwrap the container key, open the contents. */
	uint8_t ss[32], wk[32], ck[32];
	static const uint8_t zero_nonce[24];
	const mor_cbor *cap = &caps->items[idx];
	if (mor_xwing_decaps(ss, cap->items[0].data, xsk))
		return MOR_SEALED_CAPSULE;
	mor_tagged_hash(wk, "MOR/sealed/wrap", ss, 32);
	if (mor_xchacha_open(ck, cap->items[1].data, 48, zero_nonce, wk))
		return MOR_SEALED_CAPSULE;
	size_t ln = (size_t)lc->items[2].v;
	if (ln < 16)
		return MOR_SEALED_CONTENTS;
	uint8_t *plain = mor_arena_alloc(a, ln);
	if (!plain)
		return MOR_SEALED_NOMEM;
	if (mor_xchacha_open(plain, lc->items[2].data, ln, lc->items[1].data, ck))
		return MOR_SEALED_CONTENTS;
	e = mor_cbor_decode(plain, ln - 16, a, &ct);
	if (e == MOR_CBOR_E_NOMEM)
		return MOR_SEALED_NOMEM;
	/* contents = [ act: bstr, ? key: bstr .size 32 ] */
	if (e || ct->type != MOR_CBOR_ARRAY || ct->v < 1 || ct->v > 2 || ct->items[0].type != MOR_CBOR_BYTES ||
	    (ct->v == 2 && !is_bstr(&ct->items[1], 32)))
		return MOR_SEALED_CONTENTS;
	out->inner = ct->items[0].data;
	out->inner_len = (size_t)ct->items[0].v;
	out->key = ct->v == 2 ? ct->items[1].data : NULL;
	int r = mor_act_decode(&out->act, out->inner, out->inner_len, out->key, a);
	if (r == MOR_OPEN_NOMEM)
		return MOR_SEALED_NOMEM;
	if (out->key && out->act.key)
		return MOR_SEALED_KEY_PUBLIC;
	if (r != MOR_OPEN_OK)
		return MOR_SEALED_INNER;
	/* Step 5: addressing. */
	if (recipient) {
		if (!mor_act_names_in(out->act.to, recipient))
			return MOR_SEALED_ADDRESSING;
	} else if (out->act.to) {
		return MOR_SEALED_ADDRESSING;
	}
	return MOR_SEALED_OK;
}
