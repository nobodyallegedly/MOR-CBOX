/* Deterministic CBOR (RFC 8949 section 4.2.1, as Identity Part three,
 * "Encoding and conventions", writes it out), decoded strictly. */
#ifndef MOR_CBOR_H
#define MOR_CBOR_H

#include <stddef.h>
#include <stdint.h>

#include "arena.h"

enum mor_cbor_type {
	MOR_CBOR_UINT,
	MOR_CBOR_NINT, /* value is -1 - v */
	MOR_CBOR_BYTES,
	MOR_CBOR_TEXT,
	MOR_CBOR_ARRAY,
	MOR_CBOR_MAP,
	MOR_CBOR_TAG,
	MOR_CBOR_SIMPLE, /* false 20, true 21, null 22, undefined 23, others */
	MOR_CBOR_FLOAT,
};

enum mor_cbor_err {
	MOR_CBOR_OK = 0,
	MOR_CBOR_E_END,        /* UnexpectedEnd */
	MOR_CBOR_E_NONSHORTEST,/* NonShortestArgument */
	MOR_CBOR_E_INDEFINITE, /* IndefiniteLength (and a stray break) */
	MOR_CBOR_E_UNSORTED,   /* UnsortedMapKeys */
	MOR_CBOR_E_DUPKEY,     /* DuplicateMapKey */
	MOR_CBOR_E_FLOAT,      /* NonPreferredFloat */
	MOR_CBOR_E_UTF8,       /* InvalidUtf8 */
	MOR_CBOR_E_SIMPLE,     /* InvalidSimpleValue */
	MOR_CBOR_E_RESERVED,   /* ReservedAdditionalInfo */
	MOR_CBOR_E_TRAILING,   /* TrailingBytes */
	MOR_CBOR_E_DEPTH,      /* TooDeep */
	MOR_CBOR_E_NOMEM,      /* the caller's arena is full */
};

#define MOR_CBOR_MAX_DEPTH 128

typedef struct mor_cbor {
	uint8_t type;
	uint8_t fwidth;        /* float: 2, 4 or 8 bytes */
	uint64_t v;            /* uint/nint value, string length, item or pair count, tag number, simple value, float bits */
	const uint8_t *raw;    /* the whole encoded item, in the input */
	size_t raw_len;
	const uint8_t *data;   /* string content, in the input */
	struct mor_cbor *items;/* array: v items; map: 2v items (key, value, ...); tag: 1 item */
} mor_cbor;

const char *mor_cbor_err_name(int err);

/* Decode exactly one data item filling buf[0..len). On success *out points to
 * the root, carved from the arena; strings point into buf. */
int mor_cbor_decode(const uint8_t *buf, size_t len, mor_arena *a, const mor_cbor **out);

/* UTF-8 validity as RFC 3629 (no overlongs, no surrogates, nothing above U+10FFFF). */
int mor_utf8_valid(const uint8_t *s, size_t n);

/* Helpers over decoded items. */
const mor_cbor *mor_cbor_map_get(const mor_cbor *map, uint64_t key);
int mor_cbor_is_null(const mor_cbor *c);
int mor_cbor_is_true(const mor_cbor *c);
int mor_cbor_is_hash(const mor_cbor *c); /* bstr .size 32 */
int mor_cbor_bytes_eq(const mor_cbor *c, const uint8_t *p, size_t n);

/* A closed map whose keys are all unsigned integers among the allowed set
 * (bitmask over keys 0..63). Returns 1 when it is so. */
int mor_cbor_map_closed(const mor_cbor *map, uint64_t allowed_mask);

/* Encoder into a caller buffer. */
typedef struct mor_buf {
	uint8_t *p;
	size_t cap;
	size_t len;
	int overflow;
} mor_buf;

void mor_buf_init(mor_buf *b, void *mem, size_t cap);
void mor_buf_put(mor_buf *b, const void *p, size_t n);
void mor_cbor_put_head(mor_buf *b, unsigned major, uint64_t v);
void mor_cbor_put_uint(mor_buf *b, uint64_t v);
void mor_cbor_put_bytes(mor_buf *b, const uint8_t *p, size_t n);
void mor_cbor_put_text(mor_buf *b, const char *p, size_t n);
/* Re-encode a decoded item deterministically. */
void mor_cbor_encode(mor_buf *b, const mor_cbor *c);

#endif
