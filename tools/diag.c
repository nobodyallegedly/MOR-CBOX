/* CBOR diagnostic notation, for reading acts. 32-byte strings that name a
 * known object are printed by name. */
#include "diag.h"

#include <stdio.h>
#include <string.h>

static const char *lookup(const diag_names *nm, const uint8_t *h)
{
	for (size_t i = 0; nm && i < nm->n; i++)
		if (!memcmp(nm->hash[i], h, 32))
			return nm->name[i];
	return NULL;
}

void diag_print(FILE *f, const mor_cbor *c, const diag_names *nm)
{
	switch (c->type) {
	case MOR_CBOR_UINT: fprintf(f, "%llu", (unsigned long long)c->v); break;
	case MOR_CBOR_NINT: fprintf(f, "-%llu", (unsigned long long)c->v + 1ULL); break;
	case MOR_CBOR_BYTES: {
		const char *n = c->v == 32 ? lookup(nm, c->data) : NULL;
		if (n) {
			fprintf(f, "<%s>", n);
		} else if (c->v > 48) {
			fprintf(f, "h'");
			for (size_t i = 0; i < 12; i++)
				fprintf(f, "%02x", c->data[i]);
			fprintf(f, "..'(%llu)", (unsigned long long)c->v);
		} else {
			fprintf(f, "h'");
			for (size_t i = 0; i < c->v; i++)
				fprintf(f, "%02x", c->data[i]);
			fprintf(f, "'");
		}
		break;
	}
	case MOR_CBOR_TEXT: fprintf(f, "\"%.*s\"", (int)c->v, (const char *)c->data); break;
	case MOR_CBOR_ARRAY:
		fprintf(f, "[");
		for (uint64_t i = 0; i < c->v; i++) {
			if (i)
				fprintf(f, ", ");
			diag_print(f, &c->items[i], nm);
		}
		fprintf(f, "]");
		break;
	case MOR_CBOR_MAP:
		fprintf(f, "{");
		for (uint64_t i = 0; i < c->v; i++) {
			if (i)
				fprintf(f, ", ");
			diag_print(f, &c->items[2 * i], nm);
			fprintf(f, ": ");
			diag_print(f, &c->items[2 * i + 1], nm);
		}
		fprintf(f, "}");
		break;
	case MOR_CBOR_TAG:
		fprintf(f, "%llu(", (unsigned long long)c->v);
		diag_print(f, c->items, nm);
		fprintf(f, ")");
		break;
	case MOR_CBOR_SIMPLE:
		if (c->v == 20)
			fprintf(f, "false");
		else if (c->v == 21)
			fprintf(f, "true");
		else if (c->v == 22)
			fprintf(f, "null");
		else if (c->v == 23)
			fprintf(f, "undefined");
		else
			fprintf(f, "simple(%llu)", (unsigned long long)c->v);
		break;
	case MOR_CBOR_FLOAT: fprintf(f, "float(0x%llx)", (unsigned long long)c->v); break;
	default: fprintf(f, "?");
	}
}
