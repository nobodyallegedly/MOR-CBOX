/* Arena: the library's one source of memory.
 *
 * Memory discipline (stated once, kept everywhere): the library never calls
 * malloc, calloc, realloc or free. Every structure it builds is carved from a
 * caller-owned arena (a fixed byte region the caller provides) or written to a
 * caller-owned buffer. Decoded values point into the caller's input bytes or
 * into the arena; their lifetime is the shorter of the two. Running out of
 * arena is an ordinary error (MOR_E_NOMEM), never a crash. The only memory the
 * library does not own is inside OpenSSL's EVP layer (SLH-DSA, ML-KEM, SHA-3),
 * confined to crypto.c and released before each call returns.
 */
#ifndef MOR_ARENA_H
#define MOR_ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct mor_arena {
	uint8_t *base;
	size_t cap;
	size_t used;
	size_t high; /* high-water mark, for reporting */
} mor_arena;

void mor_arena_init(mor_arena *a, void *buf, size_t cap);
/* Returns NULL when the arena cannot hold n more bytes (aligned to 16). */
void *mor_arena_alloc(mor_arena *a, size_t n);
/* Allocates n * size bytes, refusing a multiplication that overflows. */
void *mor_arena_array(mor_arena *a, size_t n, size_t size);
/* Save and restore a position, to release scratch use in LIFO order. */
size_t mor_arena_mark(const mor_arena *a);
void mor_arena_reset(mor_arena *a, size_t mark);

#endif
