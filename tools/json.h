/* A small strict JSON reader for the vector files (RFC 8259), arena-backed. */
#ifndef MOR_JSON_H
#define MOR_JSON_H

#include <stddef.h>
#include <stdint.h>

#include "../src/arena.h"

enum { JSON_NULL, JSON_FALSE, JSON_TRUE, JSON_NUM, JSON_STR, JSON_ARR, JSON_OBJ };

typedef struct json {
	int type;
	const char *s;     /* string (NUL-terminated, decoded) or number text */
	size_t len;        /* string byte length, or item count */
	int64_t i;         /* number as an integer, when integral */
	int is_int;
	struct json *items;/* array items, or object values */
	const char **keys; /* object keys */
} json;

int json_parse(const char *text, size_t len, mor_arena *a, const json **out);
const json *json_get(const json *obj, const char *key);
const json *json_at(const json *arr, size_t i);
const char *json_str(const json *j); /* NULL unless a string */
/* Compact rendering, object keys sorted bytewise; strings unescaped where safe. */
size_t json_render(const json *j, char *out, size_t cap);

#endif
