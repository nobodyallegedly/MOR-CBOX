/* Written for people (Identity, Part three): bech32m forms under the labels
 * mid, mact, mkey, mspec and mwork; the where-it-lives form; bare hex. */
#ifndef MOR_PEOPLE_H
#define MOR_PEOPLE_H

#include <stddef.h>
#include <stdint.h>

#include "arena.h"

enum mor_label { MOR_L_NONE = 0, MOR_L_MID, MOR_L_MACT, MOR_L_MKEY, MOR_L_MSPEC, MOR_L_MWORK };

enum mor_form_err {
	MOR_FORM_OK = 0,
	MOR_FORM_CHECKSUM,   /* the checksum does not hold: mistyped or cut */
	MOR_FORM_MIXED_CASE, /* capitals and small letters mixed */
	MOR_FORM_LABEL,      /* a label no text defines */
	MOR_FORM_DATA,       /* data of the wrong length or shape for its label */
	MOR_FORM_KIND,       /* a string of another kind than the one asked */
	MOR_FORM_SYNTAX,     /* not a bech32m string at all */
	MOR_FORM_NOMEM,
};

const char *mor_label_name(int label);
const char *mor_form_err_name(int e);

/* Write data under a label, lowercase, NUL-terminated. Returns the length,
 * or 0 when out is too small. */
size_t mor_form_encode(char *out, size_t cap, int label, const uint8_t *data, size_t n);

typedef struct mor_form {
	int label;           /* the label found */
	uint8_t *data;       /* in the arena */
	size_t len;
	const char *hints[16];/* the where-it-lives hints, decoded, in the arena */
	size_t nhints;
} mor_form;

/* Read a string, wanting a kind (MOR_L_NONE for any). Bare hex (64
 * characters) is read as the kind wanted. */
int mor_form_decode(mor_form *f, const char *s, size_t n, int want, mor_arena *a);

#endif
