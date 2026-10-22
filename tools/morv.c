/* morv: the MOR verifier's command line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/act.h"
#include "../src/cbor.h"
#include "../src/development.h"
#include "../src/people.h"
#include "../src/text.h"
#include "diag.h"
#include "hex.h"
#include "json.h"
#include "vectors.h"

static void usage(void)
{
	fprintf(stderr,
		"usage: morv vectors [--md] [-v] DIR    run every published vector\n"
		"       morv cbor HEX                    decode deterministic CBOR strictly\n"
		"       morv text HEX                    canonical text: the first rule broken\n"
		"       morv act HEX                     decode and open an act, check its signature\n"
		"       morv form STRING [mid|mact|mkey|mspec|mwork]   read a written-for-people form\n"
		"       morv mir-hash FILE [KIND]        Development rule 26: a MIR's spec hash (kind 4; 6 MICS, 7 suite)\n"
		"       morv pool VERDICTS.json          dump the pool of the verdict vectors, decoded\n");
}

static int cmd_cbor(const char *h, mor_arena *a)
{
	size_t n;
	const mor_cbor *c;
	const uint8_t *d = hex_decode(h, &n, a);
	if (!d) {
		fprintf(stderr, "bad hex\n");
		return 2;
	}
	int e = mor_cbor_decode(d, n, a, &c);
	printf("%s\n", e ? mor_cbor_err_name(e) : "ok: deterministic CBOR");
	return e ? 1 : 0;
}

static int cmd_text(const char *h, mor_arena *a)
{
	size_t n;
	const uint8_t *d = hex_decode(h, &n, a);
	if (!d) {
		fprintf(stderr, "bad hex\n");
		return 2;
	}
	int r = mor_text_check(d, n, a);
	if (r)
		printf("not canonical: Text rule %d\n", 6 + r);
	else
		printf("canonical\n");
	return r ? 1 : 0;
}

static int cmd_act(const char *h, mor_arena *a)
{
	size_t n;
	mor_act act;
	const uint8_t *d = hex_decode(h, &n, a);
	if (!d) {
		fprintf(stderr, "bad hex\n");
		return 2;
	}
	int o = mor_act_decode(&act, d, n, NULL, a);
	printf("open: %s\n", mor_open_name(o));
	if (act.outside) {
		printf("id: ");
		hex_print(act.id, 32);
		printf("\n");
	}
	if (o == MOR_OPEN_OK) {
		printf("spec: ");
		hex_print(act.spec, 32);
		printf("\ntype: %llu\n", (unsigned long long)act.type);
		int s = mor_act_sig_verify(&act);
		printf("signature: %s\n", s == 1 ? "valid" : s == 0 ? "invalid" : "scheme not implemented");
	}
	return o ? 1 : 0;
}

static int cmd_form(const char *s, const char *want, mor_arena *a)
{
	static const char *n[] = { "", "mid", "mact", "mkey", "mspec", "mwork" };
	int w = MOR_L_NONE;
	for (int i = 1; want && i <= MOR_L_MWORK; i++)
		if (!strcmp(want, n[i]))
			w = i;
	mor_form f;
	int e = mor_form_decode(&f, s, strlen(s), w, a);
	if (e) {
		if (e == MOR_FORM_KIND)
			printf("refused: this is %s, not %s\n", mor_label_name(f.label), want);
		else
			printf("refused: %s\n", mor_form_err_name(e));
		return 1;
	}
	printf("%s ", mor_label_name(f.label));
	hex_print(f.data, f.len);
	printf("\n");
	for (size_t i = 0; i < f.nhints; i++)
		printf("hint: %s\n", f.hints[i]);
	return 0;
}

/* Dump every act of a verdicts.json pool, decoded, with short names. */
static int cmd_pool(const char *path, mor_arena *a)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return 2;
	static char text[1 << 21];
	size_t n = fread(text, 1, sizeof text, f);
	fclose(f);
	const json *root;
	if (json_parse(text, n, a, &root))
		return 2;
	const json *pool = json_get(root, "pool"), *mirs = json_get(root, "mirs");
	size_t cap = (pool ? pool->len : 0) + 8;
	uint8_t(*hs)[32] = mor_arena_array(a, cap, 32);
	const char **names = mor_arena_array(a, cap, sizeof *names);
	size_t k = 0;
	for (size_t i = 0; mirs && i < mirs->len; i++) {
		hex_fixed(json_str(&mirs->items[i]), hs[k], 32);
		names[k++] = mirs->keys[i];
	}
	for (size_t i = 0; pool && i < pool->len; i++) {
		hex_fixed(pool->keys[i], hs[k], 32);
		char *nm = mor_arena_alloc(a, 9);
		memcpy(nm, pool->keys[i], 8);
		names[k++] = nm;
	}
	diag_names dn = { (const uint8_t(*)[32])hs, names, k };
	for (size_t i = 0; pool && i < pool->len; i++) {
		size_t bn;
		mor_act act;
		const uint8_t *b = hex_decode(json_str(&pool->items[i]), &bn, a);
		int o = mor_act_decode(&act, b, bn, NULL, a);
		printf("== %.8s open=%s", pool->keys[i], mor_open_name(o));
		if (act.outside) {
			printf(" outside=");
			diag_print(stdout, act.outside, &dn);
		}
		if (o == MOR_OPEN_OK) {
			printf("\n   inside=");
			diag_print(stdout, act.inside, &dn);
			int s = mor_act_sig_verify(&act);
			printf("\n   sig scheme=%llu keylen=%zu sig=%s", (unsigned long long)act.sig.num, act.sig.key_len,
			       s == 1 ? "valid" : s == 0 ? "INVALID" : "unknown-scheme");
			if (act.sig.key_len == 32) {
				printf(" key=");
				hex_print(act.sig.key, 8);
			}
		}
		printf("\n");
	}
	return 0;
}

/* Development rule 26: a MIR's (kind 4) or the MICS's (6) or a test
 * suite's (7) spec hash from its file. */
static int cmd_mir_hash(const char *path, uint64_t kind, mor_arena *a)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return 2;
	static uint8_t buf[1 << 21];
	size_t n = fread(buf, 1, sizeof buf, f);
	fclose(f);
	uint8_t h[32];
	const uint8_t *t;
	size_t tl;
	if (mor_mir_hash(buf, n, h, kind, a, &t, &tl)) {
		printf("refused: the text so read is not canonical\n");
		return 1;
	}
	hex_print(h, 32);
	printf("  %s\n", path);
	return 0;
}

/* Write engine-fuzzer seeds: one file per verdict case, each act as the
 * record fuzz/fuzz_engine.c reads (its outside fields and its inside). */
static int cmd_engine_seeds(const char *path, const char *outdir, mor_arena *a)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return 2;
	static char text[1 << 21];
	size_t n = fread(text, 1, sizeof text, f);
	fclose(f);
	const json *root;
	if (json_parse(text, n, a, &root))
		return 2;
	const json *pool = json_get(root, "pool"), *cases = json_get(root, "cases");
	static uint8_t out[1 << 20];
	int files = 0;
	for (size_t l = 0; l < cases->len; l++)
		for (size_t i = 0; i < cases->items[l].len; i++) {
			const json *acts = json_get(json_at(&cases->items[l], i), "acts");
			size_t o = 0;
			for (size_t k = 0; acts && k < acts->len; k++) {
				size_t bn;
				mor_act act;
				const uint8_t *b = hex_decode(json_str(json_get(pool, json_str(json_at(acts, k)))), &bn, a);
				if (mor_act_decode(&act, b, bn, NULL, a) != MOR_OPEN_OK || !act.sig_shape_ok ||
				    act.sig.kind != MOR_SCHEME_NUM || act.sig.key_len != 32)
					continue;
				size_t rl = 1 + (act.signer ? 32 : 0) + (act.binding ? 32 : 0) + 24 + 32 + 1 + 32 + act.inside_len;
				if (o + 2 + rl > sizeof out || rl > 0xffff)
					break;
				out[o++] = (uint8_t)(rl >> 8);
				out[o++] = (uint8_t)rl;
				out[o++] = (uint8_t)((act.signer ? 1 : 0) | (act.binding ? 2 : 0));
				if (act.signer) {
					memcpy(out + o, act.signer, 32);
					o += 32;
				}
				if (act.binding) {
					memcpy(out + o, act.binding, 32);
					o += 32;
				}
				memcpy(out + o, act.nonce, 24);
				memcpy(out + o + 24, act.key, 32);
				out[o + 56] = (uint8_t)(act.sig.num - 1);
				memcpy(out + o + 57, act.sig.key, 32);
				memcpy(out + o + 89, act.inside_bytes, act.inside_len);
				o += 89 + act.inside_len;
			}
			char name[4096];
			snprintf(name, sizeof name, "%s/%s-%zu", outdir, cases->keys[l], i);
			FILE *g = fopen(name, "wb");
			if (g) {
				fwrite(out, 1, o, g);
				fclose(g);
				files++;
			}
		}
	printf("%d seeds\n", files);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage();
		return 2;
	}
	if (!strcmp(argv[1], "vectors")) {
		int md = 0, verbose = 0;
		const char *dir = NULL;
		for (int i = 2; i < argc; i++) {
			if (!strcmp(argv[i], "--md"))
				md = 1;
			else if (!strcmp(argv[i], "-v"))
				verbose = 1;
			else
				dir = argv[i];
		}
		if (!dir) {
			usage();
			return 2;
		}
		return run_all_vectors(dir, md, verbose);
	}
	if (argc < 3) {
		usage();
		return 2;
	}
	size_t cap = (size_t)64 << 20;
	void *mem = malloc(cap);
	mor_arena a;
	int r = 2;
	if (!mem)
		return 2;
	mor_arena_init(&a, mem, cap);
	if (!strcmp(argv[1], "cbor"))
		r = cmd_cbor(argv[2], &a);
	else if (!strcmp(argv[1], "text"))
		r = cmd_text(argv[2], &a);
	else if (!strcmp(argv[1], "act"))
		r = cmd_act(argv[2], &a);
	else if (!strcmp(argv[1], "engine-seeds") && argc > 3)
		r = cmd_engine_seeds(argv[2], argv[3], &a);
	else if (!strcmp(argv[1], "mir-hash"))
		r = cmd_mir_hash(argv[2], argc > 3 ? (uint64_t)strtoull(argv[3], NULL, 10) : 4, &a);
	else if (!strcmp(argv[1], "pool"))
		r = cmd_pool(argv[2], &a);
	else if (!strcmp(argv[1], "form"))
		r = cmd_form(argv[2], argc > 3 ? argv[3] : NULL, &a);
	else
		usage();
	free(mem);
	return r;
}
