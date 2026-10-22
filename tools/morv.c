/* morv: the MOR verifier's command line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/act.h"
#include "../src/cbor.h"
#include "../src/people.h"
#include "../src/text.h"
#include "hex.h"
#include "vectors.h"

static void usage(void)
{
	fprintf(stderr,
		"usage: morv vectors [--md] [-v] DIR    run every published vector\n"
		"       morv cbor HEX                    decode deterministic CBOR strictly\n"
		"       morv text HEX                    canonical text: the first rule broken\n"
		"       morv act HEX                     decode and open an act, check its signature\n"
		"       morv form STRING [mid|mact|mkey|mspec|mwork]   read a written-for-people form\n");
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
	else if (!strcmp(argv[1], "form"))
		r = cmd_form(argv[2], argc > 3 ? argv[3] : NULL, &a);
	else
		usage();
	free(mem);
	return r;
}
