/* An act: decode, open, shape, signature, and its standing alone in a world. */
#include "../src/act.h"
#include "../src/identity.h"
#include "../src/verdict.h"
#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	mor_arena *a = fuzz_arena();
	mor_act act;
	if (mor_act_decode(&act, data, size, NULL, a) == MOR_OPEN_OK)
		(void)mor_act_sig_verify(&act);
	mor_world w;
	const uint8_t *b[1] = { data };
	size_t n[1] = { size };
	if (mor_world_init(&w, a, fuzz_mir, b, n, 1) == 0 && w.nacts) {
		(void)mor_status(&w, w.acts[0].id, MOR_Q_KEEPERS);
		(void)mor_verdict(&w, w.acts[0].id);
	}
	return 0;
}
