/* The vector runner's JSON reader (it reads the published vector files). */
#include "../tools/json.h"
#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	const json *j;
	static char out[1 << 16];
	if (json_parse((const char *)data, size, fuzz_arena(), &j) == 0)
		(void)json_render(j, out, sizeof out);
	return 0;
}
