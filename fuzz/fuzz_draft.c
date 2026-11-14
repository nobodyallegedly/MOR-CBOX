/* fuzz_draft: untrusted bytes into the draft assembler (labels, sealing), then
 * the grammar rules. libFuzzer. */
#include "mor.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static FILE *sink;

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (!sink) sink = fopen("/dev/null", "w");
	mor_ctx *c = mor_new(NULL);
	if (!c) return 0;
	if (mor_draft(c, (const char *)data, size) >= 0 && mor_verify(c) == 0) {
		size_t n;
		(void)mor_sealed(c, &n);
		(void)mor_label(c, "T");
		if (sink) mor_print(c, sink, 1);
	}
	mor_free(c);
	return 0;
}
