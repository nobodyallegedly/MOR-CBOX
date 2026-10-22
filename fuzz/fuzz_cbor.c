/* Deterministic CBOR: decode, and every accepted input re-encodes to itself. */
#include <stdlib.h>
#include <string.h>

#include "../src/cbor.h"
#include "fuzz_common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	static uint8_t out[1 << 20];
	mor_arena *a = fuzz_arena();
	const mor_cbor *c;
	if (mor_cbor_decode(data, size, a, &c) == MOR_CBOR_OK) {
		mor_buf b;
		mor_buf_init(&b, out, sizeof out);
		mor_cbor_encode(&b, c);
		if (!b.overflow && (b.len != size || memcmp(out, data, size)))
			abort(); /* deterministic: one encoding per value */
	}
	return 0;
}
