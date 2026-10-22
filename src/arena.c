#include "arena.h"

#include <string.h>

void mor_arena_init(mor_arena *a, void *buf, size_t cap)
{
	a->base = buf;
	a->cap = cap;
	a->used = 0;
	a->high = 0;
}

void *mor_arena_alloc(mor_arena *a, size_t n)
{
	size_t start = (a->used + 15u) & ~(size_t)15u;
	if (start < a->used || start > a->cap || n > a->cap - start)
		return NULL;
	a->used = start + n;
	if (a->used > a->high)
		a->high = a->used;
	if (n)
		memset(a->base + start, 0, n);
	return a->base + start;
}

void *mor_arena_array(mor_arena *a, size_t n, size_t size)
{
	if (size && n > SIZE_MAX / size)
		return NULL;
	return mor_arena_alloc(a, n * size);
}

size_t mor_arena_mark(const mor_arena *a)
{
	return a->used;
}

void mor_arena_reset(mor_arena *a, size_t mark)
{
	if (mark <= a->used)
		a->used = mark;
}
