/* Canonical text: the six rules, NFC with the pinned tables. */
#include "../src/text.h"
#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	(void)mor_text_check(data, size, fuzz_arena());
	return 0;
}
