/* The verdict engine, structure-aware: each record is an act's outside
 * fields and its inside; the harness locks it and assembles the act, so
 * mutations reach the rules behind the lock. Built with MOR_FUZZ_NOSIG, where
 * a well-shaped signature is taken as valid.
 *
 * record = len:2 flags:1 [signer:32] [binding:32] nonce:24 key:32 scheme:1 pubkey:32 inside:(len - fixed) */
#include <string.h>

#include "../src/agreements.h"
#include "../src/cbor.h"
#include "../src/crypto.h"
#include "../src/identity.h"
#include "../src/money.h"
#include "../src/verdict.h"
#include "fuzz_common.h"
#include "fuzz_facts.h"

static size_t build(uint8_t *out, size_t cap, const uint8_t *r, size_t n)
{
	size_t fixed = 1 + 24 + 32 + 1 + 32, i = 1;
	uint8_t flags = r[0];
	const uint8_t *signer = NULL, *binding = NULL;
	if (n < fixed)
		return 0;
	if (flags & 1) {
		if (n < fixed + 32)
			return 0;
		signer = r + i;
		i += 32;
		fixed += 32;
	}
	if (flags & 2) {
		if (n < fixed + 32)
			return 0;
		binding = r + i;
		i += 32;
		fixed += 32;
	}
	const uint8_t *nonce = r + i, *key = r + i + 24;
	uint8_t scheme = r[i + 56];
	const uint8_t *pk = r + i + 57;
	const uint8_t *inside = r + fixed;
	size_t in = n - fixed;
	static uint8_t locked[1 << 16];
	if (in + 16 > sizeof locked)
		return 0;
	mor_xchacha_seal(locked, inside, in, nonce, key);
	uint8_t commit[32], lh[32];
	mor_tagged_hash(commit, "MOR/inside", inside, in);
	mor_sha256(lh, locked, in + 16);
	mor_buf b;
	mor_buf_init(&b, out, cap);
	mor_cbor_put_head(&b, 4, 3);
	mor_cbor_put_head(&b, 5, 3u + (signer != NULL) + (binding != NULL) + (flags & 4 ? 0u : 1u));
	if (signer) {
		mor_cbor_put_uint(&b, 0);
		mor_cbor_put_bytes(&b, signer, 32);
	}
	if (binding) {
		mor_cbor_put_uint(&b, 1);
		mor_cbor_put_bytes(&b, binding, 32);
	}
	mor_cbor_put_uint(&b, 2);
	mor_cbor_put_bytes(&b, commit, 32);
	mor_cbor_put_uint(&b, 3);
	mor_cbor_put_bytes(&b, lh, 32);
	mor_cbor_put_uint(&b, 4);
	mor_cbor_put_bytes(&b, nonce, 24);
	if (!(flags & 4)) {
		mor_cbor_put_uint(&b, 5);
		mor_cbor_put_bytes(&b, key, 32);
	}
	mor_cbor_put_bytes(&b, locked, in + 16);
	static const uint8_t zero[17088];
	scheme = scheme % 3 + 1;
	mor_cbor_put_head(&b, 4, 3);
	mor_cbor_put_uint(&b, scheme);
	mor_cbor_put_bytes(&b, pk, 32);
	mor_cbor_put_bytes(&b, zero, scheme == 1 ? 64 : scheme == 2 ? 7856 : 17088);
	return b.overflow ? 0 : b.len;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	mor_arena *a = fuzz_arena();
	const uint8_t *acts[24];
	size_t lens[24], k = 0, i = 0;
	while (i + 2 <= size && k < 24) {
		size_t l = ((size_t)data[i] << 8) | data[i + 1];
		i += 2;
		if (l > size - i)
			l = size - i;
		uint8_t *buf = mor_arena_alloc(a, l + 18000 + 256);
		if (!buf)
			return 0;
		size_t n = build(buf, l + 18000 + 256, data + i, l);
		i += l;
		if (!n)
			continue;
		acts[k] = buf;
		lens[k] = n;
		k++;
	}
	mor_world w;
	if (mor_world_init(&w, a, fuzz_mir, acts, lens, k))
		return 0;
	fuzz_facts(&w, a);
	static const uint8_t rule0[2] = { 0x81, 0x00 };
	const mor_cbor *rule;
	if (mor_cbor_decode(rule0, 2, a, &rule))
		return 0;
	for (size_t j = 0; j < w.nacts; j++) {
		const mor_act *x = &w.acts[j];
		(void)mor_status(&w, x->id, MOR_Q_KEEPERS);
		(void)mor_verdict(&w, x->id);
		(void)mor_backing(&w, x->id);
		(void)mor_consent_counts(&w, x->id);
		(void)mor_agreement_exists(&w, x->id);
		mor_fork f;
		(void)mor_deal_fork(&w, x->id, &f);
		(void)mor_money_paid_toward(&w, x->id);
		const uint8_t *s = mor_signer(x);
		(void)mor_chain_get(&w, s);
		(void)mor_money_disagreements(&w, s);
		uint8_t keepers[1][32];
		memcpy(keepers[0], s, 32);
		(void)mor_deadline(&w, x->id, (const uint8_t(*)[32])keepers, 1, rule, (const uint8_t *)"c", 1, 100);
		fuzz_ask_more(&w, x);
	}
	return 0;
}
