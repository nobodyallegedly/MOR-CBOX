/* Sealed container (Envelopes, Part three): sealed = [to, locked-act]. */
#ifndef MOR_SEALED_H
#define MOR_SEALED_H

#include <stddef.h>
#include <stdint.h>

#include "act.h"
#include "arena.h"

enum mor_sealed_err {
	MOR_SEALED_OK = 0,
	MOR_SEALED_FORMAT,     /* not in the container's format (rule 12) */
	MOR_SEALED_NOT_FOR_US, /* no capsule for this recipient */
	MOR_SEALED_CAPSULE,    /* the capsule does not open (step 4) */
	MOR_SEALED_CONTENTS,   /* the contents do not open or decode (step 4) */
	MOR_SEALED_INNER,      /* the inner act does not open (step 4) */
	MOR_SEALED_KEY_PUBLIC, /* a key given for a public act (step 4) */
	MOR_SEALED_ADDRESSING, /* the inner act's own `to` does not name the recipient (step 5) */
	MOR_SEALED_NOMEM,
};
const char *mor_sealed_err_name(int e);

typedef struct mor_sealed_open {
	const uint8_t *inner;  /* the inner act's bytes */
	size_t inner_len;
	const uint8_t *key;    /* its content key, if private */
	mor_act act;           /* the inner act, opened */
} mor_sealed_open;

/* Open a container as the recipient `recipient` (an identity hash, or NULL
 * for a bare key), holding the X-Wing secret `xsk`. */
int mor_sealed_open_as(mor_sealed_open *out, const uint8_t *sealed, size_t len, const uint8_t *recipient,
		       const uint8_t xsk[32], mor_arena *a);

#endif
