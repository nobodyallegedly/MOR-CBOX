/* Development: the specification format (Part three) and the MIR spec hash
 * (rule 26). */
#ifndef MOR_DEVELOPMENT_H
#define MOR_DEVELOPMENT_H

#include <stddef.h>
#include <stdint.h>

#include "world.h"

enum mor_spec_verdict { MOR_SPEC_OK = 0, MOR_SPEC_NONE, MOR_SPEC_UNKNOWN };

typedef struct mor_spec_result {
	int verdict;
	uint8_t hash[32];      /* the spec hash of the content */
	uint8_t missing[32];   /* for UNKNOWN: the cMIP its field 5 names, not held */
	const char *why;       /* for NONE */
	uint64_t kind;
} mor_spec_result;

/* Decode a specification's content, with the specifications the verifier
 * holds (world specs) and the MIR hashes (world mir). */
int mor_spec_decode(mor_world *w, const uint8_t *content, size_t len, mor_spec_result *out);

/* Rule 26: a MIR's spec hash from its file. Returns 0 and the hash, or -1
 * when the text so read is not canonical (refused). `draft` drops a draft's
 * status line from the header. The text read is written to text/textlen. */
int mor_mir_hash(const uint8_t *file, size_t len, uint8_t hash[32], uint64_t kind, mor_arena *a,
		 const uint8_t **text, size_t *textlen);

#endif
