#ifndef MOR_DIAG_H
#define MOR_DIAG_H

#include <stdint.h>
#include <stdio.h>

#include "../src/cbor.h"

typedef struct diag_names {
	const uint8_t (*hash)[32];
	const char **name;
	size_t n;
} diag_names;

void diag_print(FILE *f, const mor_cbor *c, const diag_names *nm);

#endif
