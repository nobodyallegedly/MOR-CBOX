#ifndef MOR_VECTORS_H
#define MOR_VECTORS_H

#include <stddef.h>

#include "../src/arena.h"
#include "json.h"

typedef struct vrun {
	mor_arena *a;      /* scratch, reset per case */
	int markdown;      /* print a RESULTS.md table */
	int verbose;
	unsigned total, agree;
	const char *file;
} vrun;

/* Record one case: expected and got, as text. Returns 1 when they agree. */
int vrecord(vrun *r, const char *casename, const char *expected, const char *got, const char *note);

int run_all_vectors(const char *dir, int markdown, int verbose);

/* Per-file runners. */
int v_tagged_hash(vrun *r, const json *j);
int v_cbor(vrun *r, const json *j);
int v_canonical_text(vrun *r, const json *j);
int v_lock(vrun *r, const json *j);
int v_running_summary(vrun *r, const json *j);
int v_sequence(vrun *r, const json *j);
int v_open_act(vrun *r, const json *j);
int v_textual_forms(vrun *r, const json *j);
int v_signatures(vrun *r, const json *j);
int v_sealed(vrun *r, const json *j);
int v_verdicts(vrun *r, const json *j);

struct mor_world;
int vbuild_world(mor_arena *a, const json *root, const json *c, struct mor_world *w, const char *drop);

#endif
