/* morv: command-line verifier for the Agreements layer (draft 19).
 *
 *   morv verify [options] FILE...     apply the grammar rules, print every answer
 *   morv seal DRAFT...                seal a draft (see mor_draft) to stdout
 *   morv fp FILE...                   list what each file holds
 *
 * Options of verify:
 *   --reader ID             on whose behalf the verifier reads (G11.4(d))
 *   --anchoring-place ID    the anchoring the reader accepts where an agreement names no place
 *   --anchoring-clock ID    the same for a clock (G13.4)
 *   --full                  print fingerprints in full
 *   --receiver ROOT[:VER]   the receiver for an act that names that version (G1.4)
 *   --bound WHO:ACT         is WHO bound by ACT (G5.1)
 *   --in-force TERMS        are these Terms in force (G4.6, G4.9)
 *   --order A:B             did A come after B, from citing (G13.3)
 *   --data-path WHO:REPORT  may WHO read REPORT beyond its leg (K8)
 *   --before-paying ROOT    which agreements of the sequence carry a call (G11.4(e))
 *   --sequence PAYMENT      the result of a payment, leg by leg (G11.4)
 */
#include "mor.h"
#include <stdlib.h>
#include <string.h>

#define MAX_FILE ((size_t)64 << 20)
#define MAX_ASKS 64

static unsigned char *slurp(const char *path, size_t *n)
{
	FILE *f = strcmp(path, "-") == 0 ? stdin : fopen(path, "rb");
	size_t cap = 1 << 16, len = 0;
	unsigned char *buf;
	if (!f) return NULL;
	buf = malloc(cap);
	while (buf) {
		size_t got = fread(buf + len, 1, cap - len, f);
		len += got;
		if (got == 0) break;
		if (len == cap) {
			unsigned char *nb;
			if (cap >= MAX_FILE || !(nb = realloc(buf, cap * 2))) {
				free(buf);
				buf = NULL;
				break;
			}
			buf = nb;
			cap *= 2;
		}
	}
	if (f != stdin) fclose(f);
	*n = len;
	return buf;
}

static int usage(void)
{
	fputs("usage: morv verify [options] FILE... | morv seal DRAFT... | morv fp FILE...\n"
	      "see the head of tools/morv.c, or REPORT.md, for the options\n", stderr);
	return 2;
}

/* split "a:b" in place; returns b or NULL */
static char *split_colon(char *s)
{
	/* split at the last colon: what follows it is always a fingerprint */
	char *last = strrchr(s, ':');
	if (!last) return NULL;
	*last = '\0';
	return last + 1;
}

int main(int argc, char **argv)
{
	mor_options opt;
	const char *asks[MAX_ASKS];
	char *askv[MAX_ASKS];
	int nasks = 0, full = 0, rc = 0, i;
	mor_ctx *c;
	memset(&opt, 0, sizeof opt);
	if (argc < 3) return usage();
	if (strcmp(argv[1], "seal") == 0) {
		c = mor_new(NULL);
		if (!c) return 1;
		for (i = 2; i < argc; i++) {
			size_t n;
			unsigned char *b = slurp(argv[i], &n);
			if (!b || mor_draft(c, (const char *)b, n) < 0) {
				fprintf(stderr, "morv: cannot seal %s\n", argv[i]);
				free(b);
				mor_free(c);
				return 1;
			}
			free(b);
		}
		{
			size_t sn;
			const char *s = mor_sealed(c, &sn);
			fwrite(s, 1, sn, stdout);
		}
		mor_free(c);
		return 0;
	}
	if (strcmp(argv[1], "verify") != 0 && strcmp(argv[1], "fp") != 0) return usage();
	for (i = 2; i < argc && argv[i][0] == '-' && argv[i][1] == '-'; i++) {
		const char *o = argv[i];
		if (strcmp(o, "--full") == 0) {
			full = 1;
			continue;
		}
		if (i + 1 >= argc) return usage();
		if (strcmp(o, "--reader") == 0) opt.reader = argv[++i];
		else if (strcmp(o, "--anchoring-place") == 0) opt.anchoring_place = argv[++i];
		else if (strcmp(o, "--anchoring-clock") == 0) opt.anchoring_clock = argv[++i];
		else if (nasks < MAX_ASKS) {
			asks[nasks] = o;
			askv[nasks++] = argv[++i];
		} else return usage();
	}
	if (i >= argc) return usage();
	c = mor_new(&opt);
	if (!c) return 1;
	for (; i < argc; i++) {
		size_t n;
		unsigned char *b = slurp(argv[i], &n);
		if (!b || mor_load(c, b, n) < 0) {
			fprintf(stderr, "morv: cannot read %s\n", argv[i]);
			free(b);
			mor_free(c);
			return 1;
		}
		free(b);
	}
	if (mor_verify(c) != 0) {
		fputs("morv: out of memory\n", stderr);
		mor_free(c);
		return 1;
	}
	if (strcmp(argv[1], "fp") == 0) {
		size_t k;
		for (k = 0; k < mor_finding_count(c); k++) {
			const mor_finding *f = mor_finding_at(c, k);
			if (f->about == MOR_ABOUT_ACT) mor_print_finding(f, stdout, 1);
		}
		mor_free(c);
		return 0;
	}
	for (i = 0; i < nasks; i++) {
		char *a = askv[i], *b = NULL;
		const mor_finding *f = NULL;
		const char *o = asks[i];
		if (strcmp(o, "--in-force") == 0) f = mor_ask_in_force(c, a);
		else if (strcmp(o, "--before-paying") == 0) f = mor_ask_before_paying(c, a);
		else if (strcmp(o, "--sequence") == 0) f = mor_ask_sequence(c, a);
		else if (strcmp(o, "--receiver") == 0) {
			b = split_colon(a);
			f = mor_ask_receiver(c, a, b);
		} else if ((b = split_colon(a)) == NULL) rc = 2;
		else if (strcmp(o, "--bound") == 0) f = mor_ask_bound(c, a, b);
		else if (strcmp(o, "--order") == 0) f = mor_ask_order(c, a, b);
		else if (strcmp(o, "--data-path") == 0) f = mor_ask_data_path(c, a, b);
		else rc = 2;
		if (!f && rc == 0) rc = 1;
	}
	mor_print(c, stdout, full);
	mor_free(c);
	return rc == 2 ? usage() : rc;
}
