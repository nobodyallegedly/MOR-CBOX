/* A specification's content (Development, Part three), and a MIR file (rule 26). */
#include "../src/development.h"
#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	mor_arena *a = fuzz_arena();
	mor_world w;
	if (mor_world_init(&w, a, fuzz_mir, NULL, NULL, 0))
		return 0;
	/* hold the input as a specification too, so a Module may name itself */
	mor_spec_content held = { { 0 }, data, size };
	w.specs = &held;
	w.nspecs = 1;
	mor_spec_result r;
	(void)mor_spec_decode(&w, data, size, &r);
	uint8_t h[32];
	const uint8_t *t;
	size_t tl;
	(void)mor_mir_hash(data, size, h, 4, a, &t, &tl);
	return 0;
}
