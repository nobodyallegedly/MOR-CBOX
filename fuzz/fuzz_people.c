/* Written for people: bech32m forms, where-it-lives hints, bare hex. */
#include "../src/people.h"
#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	mor_arena *a = fuzz_arena();
	mor_form f;
	for (int want = MOR_L_NONE; want <= MOR_L_MWORK; want++) {
		size_t mark = mor_arena_mark(a);
		(void)mor_form_decode(&f, (const char *)data, size, want, a);
		mor_arena_reset(a, mark);
	}
	return 0;
}
