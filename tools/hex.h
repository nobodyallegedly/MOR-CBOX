#ifndef MOR_HEX_H
#define MOR_HEX_H

#include <stddef.h>
#include <stdint.h>

#include "../src/arena.h"

/* Decode lowercase or uppercase hex into the arena; NULL on bad input. A
 * zero-length string gives a non-NULL pointer and *n = 0. */
uint8_t *hex_decode(const char *s, size_t *n, mor_arena *a);
/* Fixed-size decode into out; returns 0 on success. */
int hex_fixed(const char *s, uint8_t *out, size_t n);
/* Encode into the arena, NUL-terminated. */
char *hex_encode(const uint8_t *p, size_t n, mor_arena *a);
void hex_print(const uint8_t *p, size_t n);

#endif
