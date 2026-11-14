/* fuzz_engine: a structure-aware harness. The input bytes are read as a small
 * program that writes a draft: Terms, Statements, reports, payments, places and
 * the rest, naming one another by label, so that the acts reach the grammar
 * rules with real fingerprints that really cite each other. The draft is then
 * assembled, decoded, verified and questioned. libFuzzer. */
#include "mor.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

#define MAXL 96
#define NPRE 13

typedef struct { const uint8_t *p; size_t n; } in;
static unsigned nx(in *s)
{
	if (!s->n) return 0;
	s->n--;
	return *s->p++;
}

static char buf[1 << 17];
static size_t len;
static FILE *sink;

static void ap(const char *fmt, ...)
{
	va_list a;
	va_start(a, fmt);
	if (len < sizeof buf) {
		int w = vsnprintf(buf + len, sizeof buf - len, fmt, a);
		if (w > 0) len += (size_t)w;
		if (len > sizeof buf) len = sizeof buf;
	}
	va_end(a);
}

static const char *const PRE =
	"=L0\nmor-act 1 cmip\nstub yes\n\n=L1\nmor-act 1 cmip\nstub no\n\n=L2\nmor-act 1 cmip\nstub count\n\n"
	"=L3\nmor-act 1 cmip\nstub signer\n\n=L4\nmor-act 1 cmip\nstub author\n\n=L5\nmor-act 1 cmip\nstub witness\n\n"
	"=L6\nmor-act 1 cmip\nstub plain\n\n=L7\nmor-act 1 cmip\nstub after\n\n=L8\nmor-act 1 cmip\nstub readers\n\n"
	"=L9\nmor-act 1 cmip\nstub kind\n\n=L10\nmor-act 1 words\nline share @a 50\nline share @b 50\nline passes @b\n\n"
	"=L11\nmor-act 1 words\nline delegation @a @b\nline end none\n\n=L12\nmor-act 1 words\nline delegation @a @b\nline end agent-only\n\n";

static const char *const WHO[4] = { "@a", "@b", "@c", "@d" };
static unsigned nl;
static int root_of[MAXL];       /* for Terms and Statements: the label of the root */
static int last_act[4][MAXL];   /* per signer and root: the signer's last act there */

static const char *ref(in *s, char out[16])
{
	unsigned b = nx(s);
	if (b == 255) snprintf(out, 16, "~m%u", nx(s) % 4);
	else snprintf(out, 16, "$L%u", b % nl);
	return out;
}

static void previous(in *s, unsigned who, int root, unsigned smart)
{
	char r[16];
	if (smart && root >= 0 && last_act[who][root] >= 0) ap("previous $L%d\n", last_act[who][root]);
	else if (smart || (nx(s) & 1)) ap("previous none\n");
	else ap("previous %s\n", ref(s, r));
}

static void sock(in *s, const char *key)
{
	static const char *const pick[8] = { "$L0", "$L1", "$L2 1 @a", "$L2 2 @a @b @c", "$L3 @a", "$L3 @d", "$L7 5", "~absent" };
	ap("%s %s\n", key, pick[nx(s) % 8]);
}

static void resolves(in *s)
{
	char a[16], b[16];
	ref(s, a);
	ref(s, b);
	ap("resolves %s\nresolves %s\n", a, b);
	if (nx(s) % 4) ap("continues %s\n", (nx(s) & 1) ? a : b);
}

static void op_terms(in *s, int update)
{
	char r[16];
	unsigned who = nx(s) % 4, f = nx(s), g = nx(s), mask = nx(s);
	int root = (int)nl, of = -1;
	if (nl >= MAXL) return;
	if (update) {
		of = (int)(nx(s) % nl);
		root = root_of[of] >= 0 ? root_of[of] : of;
	}
	ap("=L%u\nmor-act 1 terms\nsigner %s\n", nl, WHO[who]);
	previous(s, who, update ? root : -1, f & 1);
	if (update) ap("replaces $L%d\nagreement $L%d\n", of, (f & 2) ? root : (int)(nx(s) % nl));
	if (!(update && (g & 64))) {
		for (unsigned i = 0; i < 4; i++)
			if (mask & (1u << i)) ap("party signer %s\n", WHO[i]);
		if (!(mask & 15)) ap("party signer @a\n");
		if (mask & 16) ap("party role r kind=%s what=~thing cmip=$L5 @d %s\n", (mask & 128) ? "future" : "done", (g & 128) ? "many" : "one max 2");
		if (mask & 32) ap("party agreement %s\n", ref(s, r));
		ap("author %s\n", (mask & 64) ? "role:r" : WHO[nx(s) % 4]);
		ap("body $L%u $L6\n", 10 + nx(s) % 3);
	} else {
		resolves(s);
	}
	if (f & 4) sock(s, "in_force");
	if (f & 8) sock(s, "condition");
	if (f & 16) ap("power %s %s\n", (nx(s) & 1) ? "assent" : ((nx(s) & 1) ? "resolves" : "$L9"), (nx(s) & 1) ? "$L3 @b @d" : "$L0");
	if (f & 32) ap("place place:p\n");
	if (f & 64) ap("clock clock:k\n");
	if (f & 128) {
		unsigned k = nx(s) % 4;
		if (k == 0) ap("receiver role:r\n");
		else if (k == 1) ap("receiver %s\n", ref(s, r));
		else ap("receiver %s\n", WHO[nx(s) % 4]);
	}
	if (g & 1) sock(s, "control");
	if (g & 2) ap("call control %s\n", (nx(s) & 1) ? "both" : "doer");
	if (g & 4) ap("data %s\n", (nx(s) & 1) ? "$L8 @c" : "$L0");
	if (g & 8) ap("envelope %s\n", ref(s, r));
	if (g & 16) ap("relies_on %s\n", ref(s, r));
	if (g & 32 && !(update && (g & 64))) resolves(s);
	ap("\n");
	root_of[nl] = root;
	last_act[who][root] = (int)nl;
	nl++;
}

static void op_statement(in *s)
{
	static const char *const kinds[4] = { "assent", "counter_signature", "contest", "$L9" };
	char r[16];
	unsigned who = nx(s) % 4, f = nx(s), t = nx(s) % nl;
	int root = root_of[t] >= 0 ? root_of[t] : (int)t;
	if (nl >= MAXL) return;
	ap("=L%u\nmor-act 1 statement\nsigner %s\n", nl, WHO[who]);
	previous(s, who, root, f & 1);
	ap("agreement $L%d\nterms $L%u\nkind %s\n", (f & 2) ? root : (int)(nx(s) % nl), t, kinds[(f >> 2) & 3]);
	if (f & 16) ap("for %s\nunder %s\n", (nx(s) & 1) ? WHO[nx(s) % 4] : ref(s, r), (nx(s) & 1) ? "$L0" : ref(s, r));
	if (f & 32) ap("relies_on %s\n", ref(s, r));
	if (f & 64) ap("content fills r %s\n", WHO[nx(s) % 4]);
	if (f & 128) {
		unsigned g = nx(s);
		if (g & 1) ap("answers %s\n", ref(s, r));
		if (g & 2) ap("settlement %s\n", ref(s, r));
		if (g & 4) resolves(s);
	}
	ap("\n");
	root_of[nl] = root;
	last_act[who][root] = (int)nl;
	nl++;
}

static void op_other(in *s, unsigned kind)
{
	char r[16], q[16];
	if (nl >= MAXL) return;
	ap("=L%u\n", nl);
	switch (kind) {
	case 0:
		ap("mor-act 1 report\nsigner %s\ncall %s\n", WHO[nx(s) % 4], ref(s, r));
		for (unsigned i = nx(s) % 4; i > 0; i--) {
			unsigned k = nx(s) % 5;
			if (k == 0) ap("received %u EUR %s\n", nx(s) % 3, ref(s, r));
			else if (k == 1) ap("passed %u EUR %s %s\n", nx(s) % 3, (nx(s) & 1) ? WHO[nx(s) % 4] : ref(s, q), ref(s, r));
			else if (k == 2) ap("own %u EUR\n", nx(s));
			else if (k == 3) ap("kept %u EUR\n", nx(s));
			else ap("owed %u EUR %s\n", nx(s), WHO[nx(s) % 4]);
		}
		break;
	case 1:
		ap("mor-act 1 payment\nsigner %s\nto %s\n", WHO[nx(s) % 4], ref(s, r));
		if (nx(s) & 1) ap("version %s\n", ref(s, r));
		ap("settlement %s\namount %u EUR\n", ref(s, r), nx(s) % 3);
		break;
	case 2:
		ap("mor-act 1 settlement\nsigner @d\npayment p%u\n", nx(s) % 3);
		break;
	case 3:
		ap("mor-act 1 place\nplace place:p\n");
		for (unsigned i = nx(s) % 24, pos = nx(s) % 3; i > 0; i--, pos += 1 + nx(s) % 2) ap("entry %u %s\n", pos, ref(s, r));
		break;
	case 4:
		ap("mor-act 1 clock\nclock clock:k\ntime %u\n", nx(s));
		break;
	default:
		ap("mor-act 1 envelope\nbaker %s\nwork ~w%u\n", WHO[nx(s) % 4], nx(s) % 2);
		break;
	}
	ap("\n");
	nl++;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	in s = { data, size };
	mor_options opt;
	memset(&opt, 0, sizeof opt);
	unsigned o = nx(&s);
	if (o & 1) opt.reader = WHO[(o >> 1) & 3];
	if (o & 8) opt.anchoring_place = "place:p";
	if (o & 16) opt.anchoring_clock = "clock:k";
	if (o & 32) opt.max_chain = 2;
	if (o & 64) opt.max_receiver_hops = 1;
	if (o & 128) opt.max_power_depth = 1;
	len = 0;
	nl = NPRE;
	for (unsigned i = 0; i < MAXL; i++) {
		root_of[i] = -1;
		for (unsigned w = 0; w < 4; w++) last_act[w][i] = -1;
	}
	ap("%s", PRE);
	while (s.n > 0 && nl < MAXL) {
		unsigned op = nx(&s) % 16;
		if (op < 3) op_terms(&s, 0);
		else if (op < 6) op_terms(&s, 1);
		else if (op < 12) op_statement(&s);
		else op_other(&s, nx(&s) % 6);
	}
	if (!sink) sink = fopen("/dev/null", "w");
	mor_ctx *c = mor_new(&opt);
	if (!c) return 0;
	if (mor_draft(c, buf, len) >= 0 && mor_verify(c) == 0) {
		char name[16];
		for (unsigned i = NPRE; i < nl; i++) {
			snprintf(name, sizeof name, "L%u", i);
			const char *fp = mor_label(c, name);
			if (!fp) continue;
			snprintf(name, sizeof name, "L%u", NPRE + (i * 7) % (nl - NPRE));
			const char *other = mor_label(c, name);
			mor_ask_receiver(c, fp, (i & 1) ? other : NULL);
			mor_ask_in_force(c, fp);
			mor_ask_bound(c, WHO[i % 4], fp);
			if (other) mor_ask_order(c, fp, other);
			mor_ask_data_path(c, WHO[(i + 1) % 4], fp);
			mor_ask_before_paying(c, fp);
			mor_ask_sequence(c, fp);
		}
		if (sink) mor_print(c, sink, 0);
	}
	mor_free(c);
	return 0;
}
