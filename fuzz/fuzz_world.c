/* The verdict engine over a set of acts: the input is acts, each prefixed by
 * a two-byte length; every question is asked about every act. */
#include <string.h>

#include "../src/agreements.h"
#include "../src/identity.h"
#include "../src/money.h"
#include "../src/verdict.h"
#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	mor_arena *a = fuzz_arena();
	const uint8_t *b[32];
	size_t n[32], k = 0, i = 0;
	while (i + 2 <= size && k < 32) {
		size_t l = ((size_t)data[i] << 8) | data[i + 1];
		i += 2;
		if (l > size - i)
			l = size - i;
		b[k] = data + i;
		n[k] = l;
		k++;
		i += l;
	}
	mor_world w;
	if (mor_world_init(&w, a, fuzz_mir, b, n, k))
		return 0;
	static const uint8_t rule0[2] = { 0x81, 0x00 };
	const mor_cbor *rule;
	if (mor_cbor_decode(rule0, 2, a, &rule))
		return 0;
	for (size_t j = 0; j < w.nacts; j++) {
		const mor_act *x = &w.acts[j];
		(void)mor_status(&w, x->id, 0);
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
	}
	return 0;
}
