/* test_mor.c: test cases written from the text alone (no vectors exist).
 * Each case cites the sentence of spec/agreements-draft-19.md it tests.
 * `test_mor --list` prints the table of sentences and cases (tests/MUSTS.md). */
#include "mor.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static mor_ctx *C;
static int g_checks, g_fails;
static const char *g_case = "";

static void fail_at(int line, const char *fmt, ...)
{
	va_list ap;
	g_fails++;
	fprintf(stderr, "FAIL %s (line %d): ", g_case, line);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

#define CHECK(cond, ...) \
	do { \
		g_checks++; \
		if (!(cond)) fail_at(__LINE__, __VA_ARGS__); \
	} while (0)

#define CM(label, stub) "=" label "\nmor-act 1 cmip\nstub " stub "\n\n"
static const char PRE[] = CM("yes", "yes") CM("no", "no") CM("count", "count") CM("signer", "signer") CM("author", "author")
	CM("witness", "witness") CM("plain", "plain") CM("after", "after") CM("readers", "readers") CM("kind", "kind")
	"=w0\nmor-act 1 words\ntext plain words\n\n";

#define TERMS(l, s, prev) "=" l "\nmor-act 1 terms\nsigner " s "\nprevious " prev "\n"
#define UPD(of, root) "replaces $" of "\nagreement $" root "\n"
#define AB "party signer @a\nparty signer @b\nauthor @a\nbody $w0 $plain\n"
#define ABC "party signer @a\nparty signer @b\nparty signer @c\nauthor @a\nbody $w0 $plain\n"
#define STMT(l, s, prev, root, terms, kind) \
	"=" l "\nmor-act 1 statement\nsigner " s "\nprevious " prev "\nagreement $" root "\nterms $" terms "\nkind " kind "\n"
#define ASSENT(l, s, prev, root, terms) STMT(l, s, prev, root, terms, "assent") "\n"
#define E "\n"
/* Terms T by @a for parties @a and @b, in force by both assents */
#define BASE TERMS("T", "@a", "none") AB E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
/* an update U of T signed by both, and a rival update U2 signed by both: a fork */
#define UPDATE_U TERMS("U", "@a", "$A1") UPD("T", "T") AB E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U")
#define RIVAL_U2 TERMS("U2", "@b", "$B2") UPD("T", "T") AB E ASSENT("A3", "@a", "$A2", "T", "U2") ASSENT("B3", "@b", "$U2", "T", "U2")

/* with MOR_DUMP_DIR set, every case leaves its drafts and its sealed acts there: the seeds of the fuzzers */
static int g_ctx_no;
static void dump(const char *ext, const char *bytes, size_t n, const char *mode)
{
	const char *dir = getenv("MOR_DUMP_DIR");
	char path[512];
	if (!dir) return;
	snprintf(path, sizeof path, "%s/%s-%d.%s", dir, g_case, g_ctx_no, ext);
	FILE *f = fopen(path, mode);
	if (!f) return;
	fwrite(bytes, 1, n, f);
	fclose(f);
}

static void D(const char *fmt, ...)
{
	static char buf[32768];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	g_checks++;
	if (n < 0 || (size_t)n >= sizeof buf || mor_draft(C, buf, (size_t)n) < 0) fail_at(0, "draft did not assemble:\n%s", buf);
	dump("draft", buf, n > 0 ? (size_t)n : 0, "a");
}

static void begin(const mor_options *o)
{
	C = mor_new(o);
	if (!C) {
		fprintf(stderr, "out of memory\n");
		exit(2);
	}
	D("%s", PRE);
}

static void V(void)
{
	g_checks++;
	if (mor_verify(C) != 0) fail_at(0, "verify failed");
}

static void end(void)
{
	size_t n;
	const char *sealed = mor_sealed(C, &n);
	dump("acts", sealed, n, "w");
	g_ctx_no++;
	mor_free(C);
	C = NULL;
}

/* a label's fingerprint, or the string itself */
static const char *X(const char *name)
{
	const char *fp = name ? mor_label(C, name) : NULL;
	return fp ? fp : name;
}

/* the made-up fingerprint a draft gives to ~name */
static const char *MU(const char *token)
{
	static char fp[MOR_FP_LEN + 1];
	mor_standin_fingerprint((const unsigned char *)token, strlen(token), fp);
	return fp;
}

/* first finding about this subject (and object), whose text holds `needle` if given */
static const mor_finding *FT(mor_about about, const char *subj, const char *obj, const char *needle)
{
	const char *s = X(subj), *o = X(obj);
	for (size_t i = 0; i < mor_finding_count(C); i++) {
		const mor_finding *f = mor_finding_at(C, i);
		if (f->about != about || strcmp(f->subject, s) != 0) continue;
		if (o && strcmp(f->object, o) != 0) continue;
		if (needle && !strstr(f->text, needle)) continue;
		return f;
	}
	return NULL;
}
#define F(about, subj, obj) FT(about, subj, obj, NULL)

static void is_at(int line, const mor_finding *f, mor_answer a, unsigned quals, const char *needle)
{
	g_checks++;
	if (!f) {
		fail_at(line, "no such finding (wanted %s)", mor_answer_name(a));
		return;
	}
	if (f->answer != a) fail_at(line, "answer is `%s`, wanted `%s`: %s", mor_answer_name(f->answer), mor_answer_name(a), f->text);
	if ((f->quals & quals) != quals) fail_at(line, "qualifiers %#x lack %#x: %s", f->quals, quals, f->text);
	if (needle && !strstr(f->text, needle)) fail_at(line, "text lacks \"%s\": %s", needle, f->text);
}
#define IS(f, a, quals, needle) is_at(__LINE__, f, a, quals, needle)

/* no finding anywhere says this */
static void nowhere_at(int line, const char *needle)
{
	g_checks++;
	for (size_t i = 0; i < mor_finding_count(C); i++) {
		const mor_finding *f = mor_finding_at(C, i);
		if (strstr(f->text, needle) || strstr(mor_answer_name(f->answer), needle)) {
			fail_at(line, "a finding says \"%s\": %s", needle, f->text);
			return;
		}
	}
}
#define NOWHERE(needle) nowhere_at(__LINE__, needle)

static int count_about(mor_about about, const char *subj, mor_answer a)
{
	int n = 0;
	const char *s = X(subj);
	for (size_t i = 0; i < mor_finding_count(C); i++) {
		const mor_finding *f = mor_finding_at(C, i);
		if (f->about == about && f->answer == a && (!s || strcmp(f->subject, s) == 0)) n++;
	}
	return n;
}

/* ====================== G1 ====================== */

/* an agreement X of one signer @x, in force; Terms T that need X's signing */
#define AGR_X TERMS("X", "@x", "none") "party signer @x\nauthor @x\nbody $w0 $plain\n" E ASSENT("XA", "@x", "$X", "X", "X")
#define T_NEEDS_X TERMS("T", "@a", "none") "party signer @a\nparty agreement $X\nauthor @a\nbody $w0 $plain\nin_force $count 2 @a $X\n" E \
	ASSENT("A1", "@a", "$T", "T", "T")

static void g1_2_under_other_terms(void)
{
	begin(NULL);
	D(AGR_X TERMS("O", "@x", "none") "party signer @x\nauthor @x\nbody $w0 $plain\nrelies_on ~another\n" E ASSENT("OA", "@x", "$O", "O", "O") T_NEEDS_X
	  STMT("S", "@x", "none", "T", "T", "assent") "for $X\nunder $O\n" E);
	V();
	IS(F(MOR_ABOUT_POWER, "S", "X"), MOR_NOT_SHOWN, 0, NULL);
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_NOT_SHOWN, 0, NULL);
	end();
	/* the same act under Terms of X is X's signature: one act by each of its signers */
	begin(NULL);
	D(AGR_X T_NEEDS_X STMT("S", "@x", "none", "T", "T", "assent") "for $X\nunder $X\n" E);
	V();
	IS(F(MOR_ABOUT_POWER, "S", "X"), MOR_SHOWN, 0, "signature");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, MOR_Q_AS_SHOWN, NULL);
	end();
}

static void g1_2_one_level(void)
{
	/* X names an agreement Y as a party; X's signing needs only X's own signers */
	begin(NULL);
	D(TERMS("Y", "@y", "none") "party signer @y\nauthor @y\nbody $w0 $plain\n" E
	  TERMS("X", "@x", "none") "party signer @x\nparty agreement $Y\nauthor @x\nbody $w0 $plain\n" E ASSENT("XA", "@x", "$X", "X", "X")
	  T_NEEDS_X STMT("S", "@x", "none", "T", "T", "assent") "for $X\nunder $X\n" E);
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "X", "X"), MOR_SHOWN, 0, NULL); /* Y is not counted by the default (G4.4) */
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL); /* nobody inside Y had to sign */
	end();
}

static void g1_2_named_not_bound(void)
{
	/* naming an agreement asks nothing of it: T is in force without X, and X's people are not bound */
	begin(NULL);
	D(AGR_X TERMS("T", "@a", "none") "party signer @a\nparty agreement $X\nauthor @a\nbody $w0 $plain\n" E ASSENT("A1", "@a", "$T", "T", "T"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	IS(mor_ask_bound(C, "@x", X("T")), MOR_NOT_SHOWN, 0, NULL);
	IS(mor_ask_bound(C, "@a", X("T")), MOR_SHOWN, 0, NULL);
	end();
}

static void g1_2_power_signature(void)
{
	/* X gives a power of kind assent to @d: @d's act for X is X's signature */
	begin(NULL);
	D(TERMS("X", "@x", "none") "party signer @x\nauthor @x\nbody $w0 $plain\npower assent $signer @d\n" E ASSENT("XA", "@x", "$X", "X", "X")
	  T_NEEDS_X STMT("S", "@d", "none", "T", "T", "assent") "for $X\nunder $X\n" E
	  STMT("S2", "@z", "none", "T", "T", "assent") "for $X\nunder $X\n" E);
	V();
	IS(F(MOR_ABOUT_POWER, "S", "X"), MOR_SHOWN, 0, "@d");
	IS(F(MOR_ABOUT_POWER, "S2", "X"), MOR_NOT_SHOWN, 0, NULL); /* the rule does not accept @z */
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	/* it binds every party bound by X's Terms (G5.1(b)), and no one else */
	IS(mor_ask_bound(C, "@x", X("S")), MOR_SHOWN, 0, NULL);
	IS(mor_ask_bound(C, "@q", X("S")), MOR_NOT_SHOWN, 0, NULL);
	end();
}

#define RCV_T(extra) TERMS("T", "@a", "none") AB "receiver @b\n" extra E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")

static void g1_4_no_version_qualified(void)
{
	begin(NULL);
	D(RCV_T(""));
	V();
	IS(mor_ask_receiver(C, X("T"), NULL), MOR_SHOWN, MOR_Q_AS_SHOWN, "@b");
	end();
	begin(NULL);
	D(RCV_T("place place:p\n") "=PL\nmor-act 1 place\nplace place:p\nentry 1 $T\nentry 2 $A1\nentry 3 $B1\n" E);
	V();
	const mor_finding *f = mor_ask_receiver(C, X("T"), NULL);
	IS(f, MOR_SHOWN, MOR_Q_PER_PLACE, "@b");
	CHECK(f && strcmp(f->anchoring, "place:p") == 0, "the place is named with the answer");
	end();
	mor_options o;
	memset(&o, 0, sizeof o);
	o.anchoring_place = "place:mine";
	begin(&o);
	D(RCV_T("") "=PL\nmor-act 1 place\nplace place:mine\nentry 1 $T\nentry 2 $A1\nentry 3 $B1\n" E);
	V();
	f = mor_ask_receiver(C, X("T"), NULL);
	IS(f, MOR_SHOWN, MOR_Q_PER_ANCHORING, "@b");
	CHECK(f && strcmp(f->anchoring, "place:mine") == 0, "the anchoring is named with the answer");
	end();
}

static void g1_4_fork_receiver(void)
{
	/* two updates name other receivers; the receiver is the one of the Terms both replace */
	begin(NULL);
	D(RCV_T("") TERMS("U", "@a", "$A1") UPD("T", "T") AB "receiver @a\n" E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U")
	  TERMS("U2", "@b", "$B2") UPD("T", "T") AB "receiver @a\nrelies_on ~other\n" E ASSENT("A3", "@a", "$A2", "T", "U2") ASSENT("B3", "@b", "$U2", "T", "U2"));
	V();
	IS(F(MOR_ABOUT_REPLACED_BY, "T", NULL), MOR_FORK, 0, NULL);
	const mor_finding *f = mor_ask_receiver(C, X("T"), NULL);
	IS(f, MOR_SHOWN, MOR_Q_WITH_FORK, "@b");
	CHECK(f && strstr(f->text, "no branch is chosen"), "says no branch is chosen");
	/* an act that names a tip: read from the Terms both replace, shown as built on a tip */
	IS(mor_ask_receiver(C, X("T"), X("U")), MOR_SHOWN, MOR_Q_WITH_FORK | MOR_Q_BUILT_ON_TIP, "@b");
	end();
}

static void g1_4_not_shown_cases(void)
{
	begin(NULL);
	D(BASE);
	V();
	IS(mor_ask_receiver(C, X("T"), NULL), MOR_NOT_SHOWN, 0, "names no receiver");
	end();
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "receiver @z\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(mor_ask_receiver(C, X("T"), NULL), MOR_NOT_SHOWN, 0, "not among its own parties");
	IS(mor_ask_receiver(C, X("T"), X("A1")), MOR_UNCHECKED, 0, NULL);  /* names no Terms held as that version */
	IS(mor_ask_receiver(C, "~nowhere", NULL), MOR_UNCHECKED, 0, "not held");
	end();
	/* a loop: P points at Q and Q at P: no one receives */
	begin(NULL);
	D(TERMS("P", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\nreceiver ~q\n" E ASSENT("PA", "@a", "$P", "P", "P"));
	V();
	IS(mor_ask_receiver(C, X("P"), NULL), MOR_UNCHECKED, 0, "not held"); /* lacks that agreement's acts */
	end();
	/* a version that is no version of that agreement */
	begin(NULL);
	D(BASE TERMS("O", "@a", "none") AB "receiver @a\n" E);
	V();
	IS(mor_ask_receiver(C, X("T"), X("O")), MOR_NOT_SHOWN, 0, "not shown as a version");
	end();
}

static void g1_4_through_agreements(void)
{
	/* T's receiver is agreement S, whose receiver is its own party @s: @s receives */
	begin(NULL);
	D(TERMS("S", "@s", "none") "party signer @s\nauthor @s\nbody $w0 $plain\nreceiver @s\n" E ASSENT("SA", "@s", "$S", "S", "S")
	  TERMS("T", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\nreceiver $S\n" E ASSENT("A1", "@a", "$T", "T", "T"));
	V();
	IS(mor_ask_receiver(C, X("T"), NULL), MOR_SHOWN, 0, "@s");
	end();
}

/* a spot `r` of kind done, confirmed by witness @w */
#define ROLE(mode) "party role r kind=done what=~thing cmip=$witness @w " mode "\n"
#define FILL(l, prev, root, terms, who) STMT(l, "@w", prev, root, terms, "$kind") "content fills r " who "\n" E

static void g1_4_role_receiver(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") "party signer @a\n" ROLE("many") "author @a\nbody $w0 $plain\nin_force $yes\nreceiver role:r\n" E);
	V();
	IS(mor_ask_receiver(C, X("T"), NULL), MOR_NOT_SHOWN, 0, "not filled");
	D(FILL("W1", "none", "T", "T", "@p"));
	V();
	IS(mor_ask_receiver(C, X("T"), NULL), MOR_SHOWN, 0, "@p");
	D(FILL("W2", "$W1", "T", "T", "@q"));
	V();
	IS(mor_ask_receiver(C, X("T"), NULL), MOR_NOT_SHOWN, 0, "filled more than once");
	end();
}

/* ====================== G2, G3 ====================== */

static void g2_1_fingerprints(void)
{
	begin(NULL);
	D(TERMS("T1", "@a", "none") "party signer @a\nauthor @a\nbody some-words.txt $plain\n" E
	  TERMS("T2", "@a", "none") "party signer @a\nauthor @a\nbody $w0 not-a-fingerprint\n" E
	  TERMS("T3", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\ncondition my-condition 2\n" E
	  TERMS("T4", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\npower assent rule.txt\n" E
	  TERMS("T5", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\nrelies_on http://example.invalid/side-piece\n" E
	  ASSENT("A5", "@a", "$T5", "T5", "T5"));
	V();
	IS(F(MOR_ABOUT_ACT, "T1", NULL), MOR_INVALID, 0, "body");
	CHECK(F(MOR_ABOUT_ACT, "T1", NULL) && strcmp(F(MOR_ABOUT_ACT, "T1", NULL)->rule, "G2.1") == 0, "names G2.1 with the answer");
	IS(F(MOR_ABOUT_ACT, "T2", NULL), MOR_INVALID, 0, "cMIP");
	IS(F(MOR_ABOUT_ACT, "T3", NULL), MOR_INVALID, 0, "cMIP");
	IS(F(MOR_ABOUT_ACT, "T4", NULL), MOR_INVALID, 0, "cMIP");
	/* an entry of relies_on that is not a fingerprint: not read, not invalid (G2.2) */
	CHECK(F(MOR_ABOUT_ACT, "T5", NULL) == NULL, "not invalid");
	IS(F(MOR_ABOUT_UNREAD, "T5", NULL), MOR_NOTE, 0, "left unread");
	IS(F(MOR_ABOUT_IN_FORCE, "T5", "T5"), MOR_SHOWN, 0, NULL);
	/* an invalid act counts for nothing */
	IS(F(MOR_ABOUT_IN_FORCE, "T1", NULL), MOR_NOT_SHOWN, 0, "invalid");
	end();
}

static void g2_2_not_part(void)
{
	/* a field section 3 does not give is not read as part of the Terms */
	begin(NULL);
	D(TERMS("T", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\nannex $w0\nreceiver_too @z\n" E ASSENT("A1", "@a", "$T", "T", "T"));
	V();
	IS(FT(MOR_ABOUT_UNREAD, "T", "annex", NULL), MOR_NOTE, 0, "left unread");
	IS(FT(MOR_ABOUT_UNREAD, "T", "receiver_too", NULL), MOR_NOTE, 0, "does not make the act invalid");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	IS(mor_ask_receiver(C, X("T"), NULL), MOR_NOT_SHOWN, 0, "names no receiver");
	end();
}

static void g3_1_authors(void)
{
	begin(NULL);
	D(TERMS("T1", "@a", "none") "party signer @a\nauthor @z\nbody $w0 $plain\n" E
	  TERMS("T2", "@a", "none") "party signer @a\nparty agreement ~x\nauthor ~x\nbody $w0 $plain\n" E
	  TERMS("T3", "@a", "none") "party signer @a\n" ROLE("one") "author role:r\nbody $w0 $plain\nin_force $yes\n" E);
	V();
	IS(F(MOR_ABOUT_ACT, "T1", NULL), MOR_INVALID, 0, "authors");
	CHECK(F(MOR_ABOUT_ACT, "T1", NULL) && strcmp(F(MOR_ABOUT_ACT, "T1", NULL)->rule, "G3.1") == 0, "names G3.1");
	IS(F(MOR_ABOUT_ACT, "T2", NULL), MOR_INVALID, 0, "authors"); /* a party named as an agreement is no author */
	CHECK(F(MOR_ABOUT_ACT, "T3", NULL) == NULL, "a role may be an author");
	end();
}

static void g3_2_authors_input(void)
{
	/* a rule in a power's socket is handed `authors`: this stub accepts only an author's act */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "power $kind $author\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  STMT("J1", "@a", "$A1", "T", "T", "$kind") "for @b\nunder $T\ncontent decided\n" E
	  STMT("J2", "@b", "$B1", "T", "T", "$kind") "for @a\nunder $T\ncontent decided\n" E);
	V();
	IS(F(MOR_ABOUT_POWER, "J1", "@b"), MOR_SHOWN, 0, NULL);
	IS(F(MOR_ABOUT_POWER, "J2", "@a"), MOR_NOT_SHOWN, 0, "author");
	end();
}

/* ====================== G4 ====================== */

static void g4_1_default(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") AB E ASSENT("A1", "@a", "$T", "T", "T"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_NOT_SHOWN, MOR_Q_AS_SHOWN, "@b");
	IS(F(MOR_ABOUT_ASSENT, "A1", "T"), MOR_NOT_SHOWN, 0, "not in force"); /* G4.3, G4.10 */
	D(ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, MOR_Q_AS_SHOWN, NULL);
	IS(F(MOR_ABOUT_ASSENT, "A1", "T"), MOR_SHOWN, 0, NULL);
	end();
	/* no party named as a signer, no in_force: in force at once */
	begin(NULL);
	D(TERMS("T", "@a", "none") "party agreement ~x\nauthor @a\nbody $w0 $plain\n" E);
	V();
	IS(F(MOR_ABOUT_ACT, "T", NULL), MOR_INVALID, 0, NULL); /* its author is no party (G3.1) */
	end();
}

static void g4_2_role_needs_in_force(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") "party signer @a\n" ROLE("one") "author @a\nbody $w0 $plain\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_ACT, "T", NULL);
	IS(f, MOR_INVALID, 0, "in_force");
	CHECK(f && strcmp(f->rule, "G4.2") == 0, "names G4.2");
	end();
}

static void g4_6_qualified(void)
{
	/* per the agreement's place */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "place place:p\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  "=PL\nmor-act 1 place\nplace place:p\nentry 1 $T\nentry 2 $A1\nentry 3 $B1\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_IN_FORCE, "T", "T");
	IS(f, MOR_SHOWN, MOR_Q_PER_PLACE, "stands on");
	CHECK(f && strcmp(f->anchoring, "place:p") == 0, "place named");
	CHECK(f && !(f->quals & MOR_Q_AS_SHOWN), "not also as shown");
	end();
	/* no place: as shown, saying what the answer stands on */
	begin(NULL);
	D(BASE);
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, MOR_Q_AS_SHOWN, "stands on");
	/* every answer about what is in force carries a qualifier */
	for (size_t i = 0; i < mor_finding_count(C); i++) {
		const mor_finding *g = mor_finding_at(C, i);
		if (g->about == MOR_ABOUT_IN_FORCE)
			CHECK(g->quals & (MOR_Q_AS_SHOWN | MOR_Q_PER_PLACE | MOR_Q_PER_ANCHORING), "an in-force answer without a qualifier");
	}
	end();
}

static void g4_7_fork_of_terms(void)
{
	begin(NULL);
	D(BASE UPDATE_U RIVAL_U2);
	V();
	const mor_finding *f = F(MOR_ABOUT_REPLACED_BY, "T", NULL);
	IS(f, MOR_FORK, MOR_Q_AS_SHOWN, "no tip is chosen");
	char a[13], b[13];
	snprintf(a, sizeof a, "%.12s", X("U"));
	snprintf(b, sizeof b, "%.12s", X("U2"));
	CHECK(f && strstr(f->text, a) && strstr(f->text, b), "both tips are shown");
	/* the last shared version stays in force, shown with the fork */
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, MOR_Q_WITH_FORK, NULL);
	IS(mor_ask_in_force(C, X("U")), MOR_FORK, MOR_Q_WITH_FORK, NULL);
	IS(mor_ask_in_force(C, X("U2")), MOR_FORK, MOR_Q_WITH_FORK, NULL);
	/* an assent to a tip counts for whether the tip was signed, and for nothing else */
	IS(F(MOR_ABOUT_ASSENT, "A2", "U"), MOR_NOT_SHOWN, MOR_Q_WITH_FORK, "binds no one");
	end();
	/* a place gives positions: still no tip is chosen by position */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "place place:p\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") AB "place place:p\n" E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U")
	  TERMS("U2", "@b", "$B2") UPD("T", "T") AB "place place:p\n" E ASSENT("A3", "@a", "$A2", "T", "U2") ASSENT("B3", "@b", "$U2", "T", "U2")
	  "=PL\nmor-act 1 place\nplace place:p\nentry 1 $T\nentry 2 $A1\nentry 3 $B1\nentry 4 $U\nentry 5 $A2\nentry 6 $B2\nentry 7 $U2\nentry 8 $A3\nentry 9 $B3\n" E);
	V();
	IS(F(MOR_ABOUT_REPLACED_BY, "T", NULL), MOR_FORK, MOR_Q_PER_PLACE, NULL);
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, MOR_Q_WITH_FORK | MOR_Q_PER_PLACE, NULL);
	end();
}

static void g4_7_answers_from_shared(void)
{
	/* T has a spot and a receiver; two updates drop them; answers come from T, with the fork */
	begin(NULL);
	D(TERMS("T", "@a", "none") "party signer @a\nparty signer @b\n" ROLE("many") "author @a\nbody $w0 $plain\nin_force $count 2 @a @b\nreceiver @b\n" E
	  ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T") FILL("W1", "none", "T", "T", "@p")
	  TERMS("U", "@a", "$A1") UPD("T", "T") AB "receiver @a\n" E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U")
	  TERMS("U2", "@b", "$B2") UPD("T", "T") AB E ASSENT("A3", "@a", "$A2", "T", "U2") ASSENT("B3", "@b", "$U2", "T", "U2"));
	V();
	IS(F(MOR_ABOUT_REPLACED_BY, "T", NULL), MOR_FORK, 0, NULL);
	IS(F(MOR_ABOUT_FILLING, "T", "role:r"), MOR_SHOWN, MOR_Q_WITH_FORK | MOR_Q_AS_SHOWN, "@p");
	IS(F(MOR_ABOUT_RECEIVER, "T", "T"), MOR_SHOWN, MOR_Q_WITH_FORK, "@b");
	IS(F(MOR_ABOUT_SPOT_OPEN, "T", "role:r"), MOR_YES, MOR_Q_WITH_FORK, NULL);
	end();
}

static void g4_8_default_update(void)
{
	begin(NULL);
	D(BASE TERMS("U", "@a", "$A1") UPD("T", "T") AB E ASSENT("A2", "@a", "$U", "T", "U"));
	V();
	/* lacks @b's signature: not shown as a replacement, never invalid, never not valid */
	IS(F(MOR_ABOUT_REPLACES, "U", "T"), MOR_NOT_SHOWN, MOR_Q_AS_SHOWN, "may exist");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	D(ASSENT("B2", "@b", "$B1", "T", "U"));
	V();
	IS(F(MOR_ABOUT_REPLACES, "U", "T"), MOR_VALID, MOR_Q_BY_DEFAULT, "by default");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, 0, NULL);
	/* a stranger's assent makes no one a party, and is not asked for */
	D(ASSENT("Z1", "@z", "none", "T", "U"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, 0, NULL);
	IS(mor_ask_bound(C, "@z", X("U")), MOR_NOT_SHOWN, 0, NULL);
	end();
}

static void g4_8_e_unchecked_update(void)
{
	/* two Terms acts name T; one cannot be checked (its in_force cMIP is not held) */
	begin(NULL);
	D(BASE UPDATE_U TERMS("U2", "@b", "$B2") UPD("T", "T") AB "in_force ~absent\n" E ASSENT("A3", "@a", "$A2", "T", "U2")
	  ASSENT("B3", "@b", "$U2", "T", "U2"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_UNCHECKED, 0, "last version established");
	end();
}

static void g4_8_new_version_in_force(void)
{
	/* an update the condition answers valid must still be in force by its own in_force (section 8.3, item 1) */
	begin(NULL);
	D(TERMS("T", "@a", "none") ABC "condition $count 2 @a @b @c\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  ASSENT("C1", "@c", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") ABC E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U"));
	V();
	IS(F(MOR_ABOUT_REPLACES, "U", "T"), MOR_VALID, 0, NULL);
	IS(F(MOR_ABOUT_VERSION, "U", "T"), MOR_NOT_SHOWN, 0, "@c");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	end();
}

static void g4_9_never_ended(void)
{
	begin(NULL);
	D(BASE UPDATE_U);
	V();
	const mor_finding *f = mor_ask_in_force(C, X("T"));
	IS(f, MOR_NOT_SHOWN, 0, "goes on in its last version");
	CHECK(f && strcmp(f->object, X("U")) == 0, "names the Terms that replace them");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, 0, NULL);
	NOWHERE("ended");
	NOWHERE("is over");
	NOWHERE("terminated");
	end();
}

#define PLACE3 "=PL\nmor-act 1 place\nplace place:p\n"

static void g4_10_counter_place(void)
{
	/* @a assents, cancels before @b's assent: per the place, the assent is not counted */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "place place:p\n" E ASSENT("A1", "@a", "$T", "T", "T")
	  STMT("K", "@a", "$A1", "T", "T", "counter_signature") E ASSENT("B1", "@b", "none", "T", "T")
	  PLACE3 "entry 1 $T\nentry 2 $A1\nentry 3 $K\nentry 4 $B1\n" E);
	V();
	IS(F(MOR_ABOUT_COUNTER, "K", "A1"), MOR_SHOWN, MOR_Q_PER_PLACE, "cancels");
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_NOT_SHOWN, MOR_Q_PER_PLACE, NULL);
	end();
	/* made after the Terms were in force: it changes nothing */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "place place:p\n" E ASSENT("A1", "@a", "$T", "T", "T")
	  STMT("K", "@a", "$A1", "T", "T", "counter_signature") E ASSENT("B1", "@b", "none", "T", "T")
	  PLACE3 "entry 1 $T\nentry 2 $A1\nentry 3 $B1\nentry 4 $K\n" E);
	V();
	IS(F(MOR_ABOUT_COUNTER, "K", "A1"), MOR_NOT_SHOWN, MOR_Q_PER_PLACE, "changes nothing");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, MOR_Q_PER_PLACE, NULL);
	end();
}

static void g4_10_counter_citing(void)
{
	/* no place: the counter signature names the completing assent: it came after, and changes nothing */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  STMT("K", "@a", "$A1", "T", "T", "counter_signature") "relies_on $B1\n" E);
	V();
	IS(F(MOR_ABOUT_COUNTER, "K", "A1"), MOR_NOT_SHOWN, 0, "changes nothing");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	end();
	/* the completing assent names the counter signature: it came after it; the assent is not counted */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB E ASSENT("A1", "@a", "$T", "T", "T") STMT("K", "@a", "$A1", "T", "T", "counter_signature") E
	  STMT("B1", "@b", "none", "T", "T", "assent") "relies_on $K\n" E);
	V();
	IS(F(MOR_ABOUT_COUNTER, "K", "A1"), MOR_SHOWN, 0, "cancels");
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_NOT_SHOWN, 0, NULL);
	/* a later assent by the same signer is counted as any assent is */
	D(ASSENT("A9", "@a", "$K", "T", "T"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	end();
	/* neither names the other: whether the Terms are in force is not shown, and the verifier says why */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB E ASSENT("A1", "@a", "$T", "T", "T") STMT("K", "@a", "$A1", "T", "T", "counter_signature") E
	  ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_NOT_SHOWN, 0, "no order is shown");
	IS(F(MOR_ABOUT_COUNTER, "K", "A1"), MOR_NOT_SHOWN, 0, "not shown");
	end();
	/* it cancels its own signer's assent, and no one else's */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  STMT("K", "@z", "none", "T", "T", "counter_signature") E);
	V();
	IS(F(MOR_ABOUT_COUNTER, "K", NULL), MOR_NOT_SHOWN, 0, "cancels nothing");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	end();
}

static void g4_11_c_not_valid(void)
{
	/* a named condition that counts signatures: two of three */
	begin(NULL);
	D(TERMS("T", "@a", "none") ABC "condition $count 2 @a @b @c\nin_force $count 2 @a @b @c\n" E ASSENT("A1", "@a", "$T", "T", "T")
	  ASSENT("B1", "@b", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") ABC "in_force $yes\n" E ASSENT("A2", "@a", "$U", "T", "U"));
	V();
	const mor_finding *f = F(MOR_ABOUT_REPLACES, "U", "T");
	IS(f, MOR_NOT_VALID, MOR_Q_AS_SHOWN, "replaces nothing");
	CHECK(f && strstr(f->text, X("count")), "the condition is named beside the answer");
	CHECK(f && strstr(f->text, "@a") && strstr(f->text, "reported by its signer"), "the ground: the assents held, by who signed");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	/* it may differ once an assent the verifier did not hold is shown */
	D(ASSENT("B2", "@b", "$B1", "T", "U"));
	V();
	f = F(MOR_ABOUT_REPLACES, "U", "T");
	IS(f, MOR_VALID, MOR_Q_AS_SHOWN, NULL);
	CHECK(f && strstr(f->text, X("count")), "valid is named in the same way");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, 0, NULL);
	/* the third is bound by an update it did not sign (G5.1(c)) */
	IS(mor_ask_bound(C, "@a", X("U")), MOR_SHOWN, 0, NULL);
	end();
}

static void g4_11_c_time_ground(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "condition $after 100\nclock clock:k\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") AB E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U")
	  "=CK\nmor-act 1 clock\nclock clock:k\ntime 50\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_REPLACES, "U", "T");
	/* everyone signed, and still the named condition's output is the answer (P21) */
	IS(f, MOR_NOT_VALID, MOR_Q_PER_CLOCK, "a time");
	CHECK(f && strcmp(f->anchoring, "clock:k") == 0, "per the clock the agreement names");
	end();
}

static void g4_11_i_no_lock(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "condition $no\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T") UPDATE_U);
	V();
	IS(F(MOR_ABOUT_REPLACES, "U", "T"), MOR_NOT_VALID, 0, NULL);
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	NOWHERE("locked");
	NOWHERE("never be updated");
	NOWHERE("can never");
	end();
	/* a condition that answers valid to an update fewer than all signed: it replaces */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "condition $yes\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") "party signer @a\nauthor @a\nbody $w0 $plain\n" E ASSENT("A2", "@a", "$U", "T", "U"));
	V();
	IS(F(MOR_ABOUT_REPLACES, "U", "T"), MOR_VALID, 0, NULL);
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, 0, NULL);
	end();
}

static void g4_11_h_lacks_cmip(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "condition ~absent\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T") UPDATE_U);
	V();
	IS(F(MOR_ABOUT_REPLACES, "U", "T"), MOR_UNCHECKED, 0, NULL);
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_UNCHECKED, 0, NULL);
	end();
}

/* ====================== G5 ====================== */

static void g5_2_no_power_for_replacement(void)
{
	/* "any two of us" written as a power of kind replaces: left unread; only everyone updates */
	begin(NULL);
	D(TERMS("T", "@a", "none") ABC "power replaces $yes\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  ASSENT("C1", "@c", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") ABC "in_force $yes\n" E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U"));
	V();
	IS(FT(MOR_ABOUT_UNREAD, "T", "replaces", NULL), MOR_NOTE, 0, "gives no power");
	/* the body is readable and is no delegation: not shown, by default */
	IS(F(MOR_ABOUT_REPLACES, "U", "T"), MOR_NOT_SHOWN, 0, "@c");
	end();
}

static void g5_1_c_bound_by_update(void)
{
	/* two of three make an update valid; the third signed the original, whose condition says so, and is bound */
	begin(NULL);
	D(TERMS("T", "@a", "none") ABC "condition $count 2 @a @b @c\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  ASSENT("C1", "@c", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") ABC "in_force $yes\n" E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, 0, NULL);
	const mor_finding *f = mor_ask_bound(C, "@c", X("U"));
	IS(f, MOR_SHOWN, 0, "no assent of their own");
	CHECK(f && strcmp(f->rule, "G5.1(c)") == 0, "names G5.1(c)");
	IS(mor_ask_bound(C, "@z", X("U")), MOR_NOT_SHOWN, 0, NULL);
	end();
}

static void g5_powers_through_powers(void)
{
	/* @p is bound by W only through an assent @g made for it under W0; @x then acts for @p under W */
	for (int limited = 0; limited < 2; limited++) {
		mor_options o;
		memset(&o, 0, sizeof o);
		o.max_power_depth = limited ? 1 : 0;
		begin(&o);
		D(TERMS("W0", "@p", "none") "party signer @p\nparty signer @g\nauthor @p\nbody $w0 $plain\npower assent $signer @g\n" E
		  ASSENT("P0", "@p", "$W0", "W0", "W0") ASSENT("G0", "@g", "none", "W0", "W0")
		  TERMS("W", "@x", "none") "party signer @p\nparty signer @x\nauthor @x\nbody $w0 $plain\npower assent $signer @x\n" E
		  ASSENT("X0", "@x", "$W", "W", "W") STMT("GP", "@g", "none", "W", "W", "assent") "for @p\nunder $W0\n" E
		  TERMS("T", "@q", "none") "party signer @p\nparty signer @q\nauthor @q\nbody $w0 $plain\n" E ASSENT("Q1", "@q", "$T", "T", "T")
		  STMT("XP", "@x", "none", "T", "T", "assent") "for @p\nunder $W\n" E);
		V();
		if (!limited) {
			IS(F(MOR_ABOUT_IN_FORCE, "W", "W"), MOR_SHOWN, 0, NULL);
			IS(F(MOR_ABOUT_POWER, "XP", "@p"), MOR_SHOWN, 0, NULL);
			IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
		} else {
			/* it stopped: unchecked beyond where it stopped, never not shown or invalid because it stopped */
			const mor_finding *f = F(MOR_ABOUT_POWER, "XP", "@p");
			IS(f, MOR_UNCHECKED, 0, "stopped following powers through powers");
			IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_UNCHECKED, 0, NULL);
		}
		end();
	}
}

static void g5_3_own_name_only(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") AB E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("C1", "@c", "none", "T", "T"));
	V();
	/* @c's assent has no `for`: it is never counted as made for @b */
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_NOT_SHOWN, 0, "@b");
	const mor_finding *f = mor_ask_bound(C, "@b", X("C1"));
	IS(f, MOR_NOT_SHOWN, 0, "own name only");
	CHECK(f && strcmp(f->rule, "G5.3") == 0, "names G5.3");
	end();
}

/* a delegation W: @p gives @g a power to assent for it; then Terms T of @p and @q, assented for @p by @g */
#define DELEG_BODY(endline) "=dw\nmor-act 1 words\nline delegation @p @g\n" endline E
#define DELEG(extra) TERMS("W", "@p", "none") "party signer @p\nparty signer @g\nauthor @p\nbody $dw $plain\npower assent $signer @g\n" extra E \
	ASSENT("PW", "@p", "$W", "W", "W") ASSENT("GW", "@g", "none", "W", "W")
#define T_PQ(extra) TERMS("T", "@q", "none") "party signer @p\nparty signer @q\nauthor @q\nbody $w0 $plain\n" extra E ASSENT("Q1", "@q", "$T", "T", "T")
#define W2_NO_POWER(prev, extra) TERMS("W2", "@p", prev) UPD("W", "W") "party signer @p\nparty signer @g\nauthor @p\nbody $dw $plain\n" extra E

static void g5_1_b_power(void)
{
	begin(NULL);
	D(DELEG_BODY("line end none\n") DELEG("") T_PQ("") STMT("G1", "@g", "none", "T", "T", "assent") "for @p\nunder $W\n" E);
	V();
	IS(F(MOR_ABOUT_POWER, "G1", "@p"), MOR_SHOWN, 0, "@g");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	IS(mor_ask_bound(C, "@p", X("G1")), MOR_SHOWN, 0, NULL);
	IS(mor_ask_bound(C, "@p", X("T")), MOR_SHOWN, 0, NULL);
	IS(mor_ask_bound(C, "@g", X("T")), MOR_NOT_SHOWN, 0, NULL); /* acting for someone does not bind the agent */
	end();
	/* no power the person bound had signed for */
	begin(NULL);
	D(DELEG_BODY("line end none\n") DELEG("") T_PQ("") STMT("G1", "@g", "none", "T", "T", "assent") "for @q\nunder $W\n" E
	  STMT("G2", "@z", "none", "T", "T", "assent") "for @p\nunder $W\n" E);
	V();
	IS(F(MOR_ABOUT_POWER, "G1", "@q"), MOR_NOT_SHOWN, 0, "not shown as bound");
	IS(F(MOR_ABOUT_POWER, "G2", "@p"), MOR_NOT_SHOWN, 0, "does not accept");
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_NOT_SHOWN, 0, NULL);
	end();
}

static void g5_4_replaced_power_place(void)
{
	/* per the place the Terms that gave the power name: the act's position against the replacement's */
	static const char *const orders[2] = { "entry 4 $G1\nentry 5 $W2\nentry 6 $PW2\nentry 7 $GW2\n",
		                               "entry 4 $W2\nentry 5 $PW2\nentry 6 $GW2\nentry 7 $G1\n" };
	for (int k = 0; k < 2; k++) {
		begin(NULL);
		D(DELEG_BODY("line end none\n") DELEG("place place:d\n") T_PQ("")
		  STMT("G1", "@g", "none", "T", "T", "assent") "for @p\nunder $W\n" E
		  W2_NO_POWER("$PW", "place place:d\n") ASSENT("PW2", "@p", "$W2", "W", "W2") ASSENT("GW2", "@g", "$GW", "W", "W2")
		  "=PL\nmor-act 1 place\nplace place:d\nentry 1 $W\nentry 2 $PW\nentry 3 $GW\n%s" E, orders[k]);
		V();
		IS(F(MOR_ABOUT_IN_FORCE, "W", "W2"), MOR_SHOWN, MOR_Q_PER_PLACE, NULL);
		if (k == 0) {
			IS(F(MOR_ABOUT_POWER, "G1", "@p"), MOR_SHOWN, 0, NULL);
			IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
		} else {
			IS(F(MOR_ABOUT_POWER, "G1", "@p"), MOR_NOT_SHOWN, 0, "per place");
			IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_NOT_SHOWN, 0, NULL);
		}
		end();
	}
}

static void g5_4_replaced_power_citing(void)
{
	/* no place: by citing alone. (0) the act names the replacement: after. (1) the replacement names the act: before.
	 * (2) neither names the other: not shown. */
	for (int k = 0; k < 3; k++) {
		begin(NULL);
		D(DELEG_BODY("line end none\n") DELEG("") T_PQ(""));
		if (k == 1) D(STMT("G1", "@g", "none", "T", "T", "assent") "for @p\nunder $W\n" E);
		D(W2_NO_POWER("$PW", "%s") ASSENT("PW2", "@p", "$W2", "W", "W2") ASSENT("GW2", "@g", "$GW", "W", "W2"), k == 1 ? "relies_on $G1\n" : "");
		if (k != 1) D(STMT("G1", "@g", "none", "T", "T", "assent") "for @p\nunder $W\n%s" E, k == 0 ? "relies_on $W2\n" : "");
		V();
		IS(F(MOR_ABOUT_IN_FORCE, "W", "W2"), MOR_SHOWN, MOR_Q_AS_SHOWN, NULL);
		if (k == 1) IS(F(MOR_ABOUT_POWER, "G1", "@p"), MOR_SHOWN, 0, NULL);
		else IS(F(MOR_ABOUT_POWER, "G1", "@p"), MOR_NOT_SHOWN, 0, k == 0 ? "made after" : "neither the act nor the replacement names the other");
		end();
	}
}

static void g5_5_delegation(void)
{
	/* by default the delegator ends it alone: the delegator's part is all that is asked */
	begin(NULL);
	D(DELEG_BODY("line end none\n") DELEG("") W2_NO_POWER("$PW", "") ASSENT("PW2", "@p", "$W2", "W", "W2"));
	V();
	IS(F(MOR_ABOUT_REPLACES, "W2", "W"), MOR_VALID, MOR_Q_BY_DEFAULT, "delegator's part alone");
	IS(F(MOR_ABOUT_IN_FORCE, "W", "W2"), MOR_SHOWN, 0, NULL); /* in force without the agent's assent */
	end();
	/* the agent alone does not end it by default */
	begin(NULL);
	D(DELEG_BODY("line end none\n") DELEG("") W2_NO_POWER("$PW", "") ASSENT("GW2", "@g", "$GW", "W", "W2"));
	V();
	IS(F(MOR_ABOUT_REPLACES, "W2", "W"), MOR_NOT_SHOWN, 0, NULL);
	end();
	/* a signed condition that leaves ending to the agent alone */
	begin(NULL);
	D(DELEG_BODY("line end agent-only\n") DELEG("") W2_NO_POWER("$PW", "") ASSENT("PW2", "@p", "$W2", "W", "W2"));
	V();
	IS(F(MOR_ABOUT_REPLACES, "W2", "W"), MOR_NOT_SHOWN, 0, "whether the agent will end it is not shown");
	IS(F(MOR_ABOUT_IN_FORCE, "W", "W"), MOR_SHOWN, 0, NULL);
	NOWHERE("never be ended");
	NOWHERE("can never");
	D(ASSENT("GW2", "@g", "$GW", "W", "W2"));
	V();
	IS(F(MOR_ABOUT_REPLACES, "W2", "W"), MOR_VALID, 0, NULL);
	end();
	/* Terms that carry powers and whose body cannot be read: unchecked, not "not shown" */
	begin(NULL);
	D(TERMS("W", "@p", "none") "party signer @p\nparty signer @g\nauthor @p\nbody ~words-not-held $plain\npower assent $signer @g\n" E
	  ASSENT("PW", "@p", "$W", "W", "W") ASSENT("GW", "@g", "none", "W", "W")
	  TERMS("W2", "@p", "$PW") UPD("W", "W") "party signer @p\nparty signer @g\nauthor @p\nbody ~words-not-held $plain\n" E
	  ASSENT("PW2", "@p", "$W2", "W", "W2"));
	V();
	IS(F(MOR_ABOUT_REPLACES, "W2", "W"), MOR_UNCHECKED, 0, "delegation");
	end();
	/* a named update condition answers, as for any update */
	begin(NULL);
	D(DELEG_BODY("line end none\n") DELEG("condition $no\n") W2_NO_POWER("$PW", "") ASSENT("PW2", "@p", "$W2", "W", "W2"));
	V();
	IS(F(MOR_ABOUT_REPLACES, "W2", "W"), MOR_NOT_VALID, 0, NULL);
	end();
}

static void g5_6_powers_cannot_touch_judicial(void)
{
	/* T gives @j a power to resolve, and a power for Statements of a kind. A Statement made under that
	 * power "removes" the power to resolve and "names a judge": it is read as changing nothing. */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "power resolves $signer @j\npower $kind $yes\n" E ASSENT("A1", "@a", "$T", "T", "T")
	  ASSENT("B1", "@b", "none", "T", "T")
	  STMT("P1", "@b", "$B1", "T", "T", "$kind") "for @a\nunder $T\ncontent remove-power resolves judge @z contest-reaches @z\n" E
	  TERMS("U", "@a", "$A1") UPD("T", "T") AB E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$P1", "T", "U")
	  TERMS("U2", "@b", "$B2") UPD("T", "T") AB E ASSENT("A3", "@a", "$A2", "T", "U2") ASSENT("B3", "@b", "$U2", "T", "U2")
	  TERMS("R", "@j", "none") UPD("T", "T") "resolves $U\nresolves $U2\ncontinues $U2\n" E
	  STMT("K1", "@z", "none", "T", "T", "contest") E);
	V();
	IS(F(MOR_ABOUT_POWER, "P1", "@a"), MOR_SHOWN, 0, NULL);
	/* the power to resolve is still there and still @j's */
	IS(F(MOR_ABOUT_REPLACES, "R", "T"), MOR_SHOWN, 0, "@j");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U2"), MOR_SHOWN, 0, NULL);
	/* a contest still reaches the signers the version names, not whoever the Statement named */
	const mor_finding *f = F(MOR_ABOUT_CONTEST, "K1", "T");
	IS(f, MOR_SHOWN, 0, "@a, @b");
	end();
}

/* ====================== G6, G12 (lines) ====================== */

static void g6_1_previous(void)
{
	begin(NULL);
	D(BASE STMT("S1", "@a", "$B1", "T", "T", "$kind") "content x\n" E        /* previous: an act of another signer */
	  TERMS("O", "@a", "none") AB "relies_on ~another\n" E STMT("S2", "@a", "$O", "T", "T", "$kind") "content y\n" E /* of another agreement */
	  STMT("S3", "@b", "~not-held", "T", "T", "$kind") "content z\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_ACT, "S1", NULL);
	IS(f, MOR_INVALID, 0, "another signer");
	CHECK(f && strcmp(f->rule, "G6.1") == 0, "names G6.1");
	IS(F(MOR_ABOUT_ACT, "S2", NULL), MOR_INVALID, 0, "another agreement");
	/* previous not held: the act counts, and the line is shown as unchecked before it */
	IS(FT(MOR_ABOUT_LINE, "@b", "T", "not held"), MOR_UNCHECKED, 0, NULL);
	IS(FT(MOR_ABOUT_ACT, "S3", "kind", NULL), MOR_SHOWN, 0, NULL);
	end();
}

static void g6_2_line_fork(void)
{
	begin(NULL);
	D(BASE STMT("S1", "@b", "$B1", "T", "T", "$kind") "content one\n" E STMT("S2", "@b", "$B1", "T", "T", "$kind") "content two\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_LINE, "@b", "T");
	IS(f, MOR_FORK, 0, "no tip is chosen");
	char a[13], b[13];
	snprintf(a, sizeof a, "%.12s", X("S1"));
	snprintf(b, sizeof b, "%.12s", X("S2"));
	CHECK(f && strstr(f->text, a) && strstr(f->text, b), "both tips shown");
	/* both tips stop: neither counts */
	IS(F(MOR_ABOUT_ACT, "S1", NULL), MOR_NOT_SHOWN, MOR_Q_WITH_FORK, NULL);
	IS(F(MOR_ABOUT_ACT, "S2", NULL), MOR_NOT_SHOWN, MOR_Q_WITH_FORK, NULL);
	IS(F(MOR_ABOUT_ASSENT, "B1", "T"), MOR_SHOWN, 0, NULL); /* the shared act still counts */
	end();
	/* two acts that both name no previous are a fork with no shared act */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  STMT("B0", "@b", "none", "T", "T", "$kind") "content other\n" E);
	V();
	IS(F(MOR_ABOUT_LINE, "@b", "T"), MOR_FORK, 0, "none");
	/* an assent that is a tip is not counted while the fork stands (P9) */
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_NOT_SHOWN, 0, NULL);
	end();
}

static void g12_3_line_resolved(void)
{
	begin(NULL);
	D(BASE STMT("S1", "@b", "$B1", "T", "T", "$kind") "content one\n" E STMT("S2", "@b", "$B1", "T", "T", "$kind") "content two\n" E
	  STMT("R", "@b", "$B1", "T", "T", "$kind") "content resolving\nresolves $S1\nresolves $S2\ncontinues $S1\n" E
	  STMT("N", "@b", "$R", "T", "T", "$kind") "content next\n" E);
	V();
	IS(F(MOR_ABOUT_LINE, "@b", "T"), MOR_SHOWN, 0, "set aside");
	IS(FT(MOR_ABOUT_ACT, "S1", "kind", NULL), MOR_SHOWN, 0, NULL);
	IS(F(MOR_ABOUT_ACT, "S2", NULL), MOR_NOT_SHOWN, 0, "set aside");
	/* the next act names the resolving update: read, and no fork */
	IS(FT(MOR_ABOUT_ACT, "N", "kind", NULL), MOR_SHOWN, 0, NULL);
	CHECK(count_about(MOR_ABOUT_LINE, "@b", MOR_FORK) == 0, "no fork is left");
	end();
	/* names `resolves` and no `continues`: resolves nothing, and is itself a tip */
	begin(NULL);
	D(BASE STMT("S1", "@b", "$B1", "T", "T", "$kind") "content one\n" E STMT("S2", "@b", "$B1", "T", "T", "$kind") "content two\n" E
	  STMT("R", "@b", "$B1", "T", "T", "$kind") "content resolving\nresolves $S1\nresolves $S2\n" E);
	V();
	IS(F(MOR_ABOUT_LINE, "@b", "T"), MOR_FORK, 0, NULL);
	IS(F(MOR_ABOUT_ACT, "R", NULL), MOR_NOT_SHOWN, MOR_Q_WITH_FORK, NULL);
	end();
}

static void g12_3_must_name_every_tip(void)
{
	/* three tips; a resolving update that names two of them resolves nothing */
	begin(NULL);
	D(BASE STMT("S1", "@b", "$B1", "T", "T", "$kind") "content one\n" E STMT("S2", "@b", "$B1", "T", "T", "$kind") "content two\n" E
	  STMT("S3", "@b", "$B1", "T", "T", "$kind") "content three\n" E
	  STMT("R", "@b", "$B1", "T", "T", "$kind") "content r\nresolves $S1\nresolves $S2\ncontinues $S1\n" E);
	V();
	IS(F(MOR_ABOUT_LINE, "@b", "T"), MOR_FORK, 0, NULL);
	/* one that names every tip, and more acts than the tips, resolves */
	D(STMT("R2", "@b", "$B1", "T", "T", "$kind") "content r2\nresolves $S1\nresolves $S2\nresolves $S3\nresolves $R\nresolves $A1\ncontinues $S3\n" E);
	V();
	IS(F(MOR_ABOUT_LINE, "@b", "T"), MOR_SHOWN, 0, "continues");
	IS(FT(MOR_ABOUT_ACT, "S3", "kind", NULL), MOR_SHOWN, 0, NULL);
	IS(F(MOR_ABOUT_ACT, "S1", NULL), MOR_NOT_SHOWN, 0, "set aside");
	end();
}

static void g12_7_rival_resolvers(void)
{
	begin(NULL);
	D(BASE STMT("S1", "@b", "$B1", "T", "T", "$kind") "content one\n" E STMT("S2", "@b", "$B1", "T", "T", "$kind") "content two\n" E
	  STMT("R1", "@b", "$B1", "T", "T", "$kind") "content r1\nresolves $S1\nresolves $S2\ncontinues $S1\n" E
	  STMT("R2", "@b", "$B1", "T", "T", "$kind") "content r2\nresolves $S1\nresolves $S2\ncontinues $S2\n" E);
	V();
	/* two that continue different tips are themselves a fork: every tip stays stopped */
	IS(F(MOR_ABOUT_LINE, "@b", "T"), MOR_FORK, 0, "rival");
	IS(F(MOR_ABOUT_ACT, "S1", NULL), MOR_NOT_SHOWN, MOR_Q_WITH_FORK, NULL);
	/* resolved the same way: an update that names the two rivals and continues one */
	D(STMT("RR", "@b", "$B1", "T", "T", "$kind") "content rr\nresolves $R1\nresolves $R2\ncontinues $R2\n" E);
	V();
	IS(F(MOR_ABOUT_LINE, "@b", "T"), MOR_SHOWN, 0, NULL);
	IS(FT(MOR_ABOUT_ACT, "S2", "kind", NULL), MOR_SHOWN, 0, NULL);
	IS(F(MOR_ABOUT_ACT, "S1", NULL), MOR_NOT_SHOWN, 0, "set aside");
	end();
	/* two that continue the same tip resolve it, and are no fork */
	begin(NULL);
	D(BASE STMT("S1", "@b", "$B1", "T", "T", "$kind") "content one\n" E STMT("S2", "@b", "$B1", "T", "T", "$kind") "content two\n" E
	  STMT("R1", "@b", "$B1", "T", "T", "$kind") "content r1\nresolves $S1\nresolves $S2\ncontinues $S1\n" E
	  STMT("R2", "@b", "$B1", "T", "T", "$kind") "content r2\nresolves $S1\nresolves $S2\ncontinues $S1\n" E);
	V();
	IS(F(MOR_ABOUT_LINE, "@b", "T"), MOR_SHOWN, 0, NULL);
	end();
}

static void g12_7_rival_resolvers_terms(void)
{
	begin(NULL);
	D(BASE UPDATE_U RIVAL_U2
	  TERMS("R1", "@a", "$A3") UPD("T", "T") "resolves $U\nresolves $U2\ncontinues $U\n" E
	  ASSENT("A4", "@a", "$R1", "T", "R1") ASSENT("B4", "@b", "$B3", "T", "R1")
	  TERMS("R2", "@b", "$B4") UPD("T", "T") "resolves $U\nresolves $U2\ncontinues $U2\n" E
	  ASSENT("A5", "@a", "$A4", "T", "R2") ASSENT("B5", "@b", "$R2", "T", "R2"));
	V();
	/* neither resolves the first fork while the other stands: every tip stays stopped */
	IS(F(MOR_ABOUT_REPLACED_BY, "T", NULL), MOR_FORK, 0, "rival");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, MOR_Q_WITH_FORK, NULL);
	/* resolved the same way: a resolving update that names the two rival ones and continues one */
	D(TERMS("RR", "@a", "$A5") UPD("T", "T") "resolves $R1\nresolves $R2\ncontinues $R1\n" E
	  ASSENT("A6", "@a", "$RR", "T", "RR") ASSENT("B6", "@b", "$B5", "T", "RR"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, 0, NULL);
	IS(mor_ask_in_force(C, X("U2")), MOR_NOT_SHOWN, 0, NULL);
	/* the next version names the resolving update: read as following the continued branch */
	D(TERMS("V3", "@a", "$A6") UPD("RR", "T") AB "receiver @a\n" E ASSENT("A7", "@a", "$V3", "T", "V3") ASSENT("B7", "@b", "$B6", "T", "V3"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "V3"), MOR_SHOWN, 0, NULL);
	end();
}

static void g1_4_loop(void)
{
	/* P's new version names Q as receiver, and Q names P: an agreement is met a second time, no one receives */
	begin(NULL);
	D(TERMS("P", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\n" E ASSENT("PA", "@a", "$P", "P", "P")
	  TERMS("Q", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\nreceiver $P\n" E ASSENT("QA", "@a", "$Q", "Q", "Q")
	  TERMS("P2", "@a", "$PA") UPD("P", "P") "party signer @a\nauthor @a\nbody $w0 $plain\nreceiver $Q\n" E ASSENT("PA2", "@a", "$P2", "P", "P2"));
	V();
	IS(mor_ask_receiver(C, X("P"), NULL), MOR_NOT_SHOWN, 0, "second time");
	IS(mor_ask_receiver(C, X("Q"), NULL), MOR_NOT_SHOWN, 0, "second time");
	end();
}

static void g12_2_built_on_tip(void)
{
	begin(NULL);
	D(BASE UPDATE_U RIVAL_U2
	  STMT("S", "@z", "none", "T", "U", "$kind") "content made-within-a-tip\n" E
	  STMT("S9", "@y", "none", "T", "T", "$kind") "content relies\nrelies_on $U2\n" E
	  TERMS("U3", "@a", "$A3") UPD("U", "T") AB E);
	V();
	const mor_finding *f = F(MOR_ABOUT_REPLACED_BY, "T", NULL);
	IS(f, MOR_FORK, 0, "no tip is chosen");
	/* what is built on a tip is shown with the fork, and counts for nothing */
	IS(F(MOR_ABOUT_ACT, "S", "U"), MOR_NOT_SHOWN, MOR_Q_BUILT_ON_TIP, "counts for nothing");
	IS(F(MOR_ABOUT_ACT, "S9", "T"), MOR_NOT_SHOWN, MOR_Q_BUILT_ON_TIP, NULL);
	IS(F(MOR_ABOUT_REPLACES, "U3", "U"), MOR_NOT_SHOWN, MOR_Q_WITH_FORK, "follows a tip");
	/* neither tip is shown as resolved: not as in force, not as the next version (see also G12.8) */
	IS(mor_ask_in_force(C, X("U")), MOR_FORK, 0, NULL);
	CHECK(F(MOR_ABOUT_IN_FORCE, "T", "U") == NULL && F(MOR_ABOUT_IN_FORCE, "T", "U2") == NULL, "no tip in force");
	end();
}

static void g12_3_terms_resolved(void)
{
	/* a resolving update signed by everyone who signed the shared version (the default) */
	begin(NULL);
	D(BASE UPDATE_U RIVAL_U2 TERMS("R", "@a", "$A3") UPD("T", "T") "resolves $U\nresolves $U2\ncontinues $U\n" E
	  ASSENT("A4", "@a", "$R", "T", "R") ASSENT("B4", "@b", "$B3", "T", "R"));
	V();
	IS(F(MOR_ABOUT_REPLACED_BY, "T", "U"), MOR_SHOWN, 0, "set aside");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, 0, NULL);
	IS(F(MOR_ABOUT_ASSENT, "A2", "U"), MOR_SHOWN, 0, NULL);
	end();
	/* lacking a signature it resolves nothing: the fork stands */
	begin(NULL);
	D(BASE UPDATE_U RIVAL_U2 TERMS("R", "@a", "$A3") UPD("T", "T") "resolves $U\nresolves $U2\ncontinues $U\n" E
	  ASSENT("A4", "@a", "$R", "T", "R"));
	V();
	IS(F(MOR_ABOUT_REPLACED_BY, "T", NULL), MOR_FORK, 0, NULL);
	IS(F(MOR_ABOUT_REPLACES, "R", "T"), MOR_NOT_SHOWN, 0, NULL);
	end();
}

static void g12_4_power_to_resolve(void)
{
	/* Terms: a power of kind resolves; its rule accepts @j */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "power resolves $signer @j\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  UPDATE_U RIVAL_U2 TERMS("R", "@j", "none") UPD("T", "T") "resolves $U\nresolves $U2\ncontinues $U2\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_REPLACES, "R", "T");
	IS(f, MOR_SHOWN, 0, "power to resolve");
	CHECK(f && strstr(f->text, "@j") && strstr(f->text, "rule"), "reported with the power and whoever it rests on");
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U2"), MOR_SHOWN, 0, NULL);
	end();
	/* a power of another kind does not resolve */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "power assent $signer @j\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  UPDATE_U RIVAL_U2 TERMS("R", "@j", "none") UPD("T", "T") "resolves $U\nresolves $U2\ncontinues $U2\n" E);
	V();
	IS(F(MOR_ABOUT_REPLACED_BY, "T", NULL), MOR_FORK, 0, NULL);
	end();
	/* a fork in one signer's line, resolved by another under a power the signer signed for */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "power resolves $signer @j\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  STMT("S1", "@b", "$B1", "T", "T", "$kind") "content one\n" E STMT("S2", "@b", "$B1", "T", "T", "$kind") "content two\n" E
	  STMT("R", "@j", "none", "T", "T", "$kind") "content r\nfor @b\nunder $T\nresolves $S1\nresolves $S2\ncontinues $S2\n" E);
	V();
	IS(F(MOR_ABOUT_LINE, "@b", "T"), MOR_SHOWN, 0, "under a power");
	f = FT(MOR_ABOUT_POWER, "R", "T", NULL);
	IS(f, MOR_SHOWN, 0, "@j");
	IS(FT(MOR_ABOUT_ACT, "S2", "kind", NULL), MOR_SHOWN, 0, NULL);
	end();
}

static void g12_5_disputed(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "receiver @a\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") AB "receiver @b\n" E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U")
	  TERMS("U2", "@b", "$B2") UPD("T", "T") AB "receiver @a\nrelies_on ~x\n" E ASSENT("A3", "@a", "$A2", "T", "U2") ASSENT("B3", "@b", "$U2", "T", "U2")
	  TERMS("R", "@a", "$A3") UPD("T", "T") "resolves $U\nresolves $U2\ncontinues $U2\n" E
	  ASSENT("A4", "@a", "$R", "T", "R") ASSENT("B4", "@b", "$B3", "T", "R")
	  /* a stranger relied on the tip now set aside */
	  STMT("S", "@z", "none", "T", "U", "$kind") "content relied\n" E
	  ASSENT("Z1", "@z", "$S", "T", "U")
	  "=PAY\nmor-act 1 payment\nsigner @buyer\nto $T\nversion $U\nsettlement ~s1\namount 10 EUR\n" E
	  /* @a signed a Terms act of that branch: not protected for what they built on it */
	  STMT("S5", "@a", "$A4", "T", "U", "$kind") "content built-by-a-signer-of-the-branch\n" E);
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U2"), MOR_SHOWN, 0, NULL);
	IS(FT(MOR_ABOUT_ACT, "S", "kind", NULL), MOR_SHOWN, MOR_Q_DISPUTED, NULL);
	IS(F(MOR_ABOUT_ASSENT, "Z1", "U"), MOR_SHOWN, MOR_Q_DISPUTED, "binds no one");
	/* a payment that names a version of that branch is read as if the tip had stood alone */
	IS(F(MOR_ABOUT_RECEIVER, "PAY", "T"), MOR_SHOWN, MOR_Q_DISPUTED, "@b");
	IS(F(MOR_ABOUT_PAYMENT, "PAY", NULL), MOR_ONE_SIDE_ONLY, MOR_Q_DISPUTED, NULL);
	IS(F(MOR_ABOUT_ACT, "S5", "U"), MOR_NOT_SHOWN, 0, NULL);
	end();
}

static void g12_8_client(void)
{
	/* a client's duty leaves no act and no verifier can check it; what a verifier can do is never to
	 * hand a client a tip as resolved: no answer shows a tip as in force or as the next version */
	begin(NULL);
	D(BASE UPDATE_U RIVAL_U2);
	V();
	for (size_t i = 0; i < mor_finding_count(C); i++) {
		const mor_finding *f = mor_finding_at(C, i);
		if (f->about == MOR_ABOUT_IN_FORCE)
			CHECK(strcmp(f->object, X("U")) != 0 && strcmp(f->object, X("U2")) != 0, "a tip shown as in force");
		if (f->about == MOR_ABOUT_REPLACED_BY) CHECK(f->answer == MOR_FORK, "a tip shown as the next version");
	}
	end();
}

/* ====================== G7, G13 ====================== */

static void g7_2_place(void)
{
	/* an act counts only once the place shows it: @b's assent is held but the place does not show it */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "place place:p\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  PLACE3 "entry 1 $T\nentry 2 $A1\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_IN_FORCE, "T", NULL);
	IS(f, MOR_NOT_SHOWN, MOR_Q_PER_PLACE, NULL);
	CHECK(f && strcmp(f->anchoring, "place:p") == 0, "the place is named");
	end();
	/* two answers of one place that contradict each other: no answer from that place */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "place place:p\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  PLACE3 "entry 1 $T\nentry 2 $A1\nentry 3 $B1\n" E "=PL2\nmor-act 1 place\nplace place:p\nentry 1 $T\nentry 2 $B1\n" E);
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_UNCHECKED, 0, NULL);
	end();
	/* no answer at all from the named place */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "place place:p\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_UNCHECKED, 0, "place");
	end();
	/* a new version names another place: whether it replaced is judged per the place the original names */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "place place:p\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") AB E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U")
	  PLACE3 "entry 1 $T\nentry 2 $A1\nentry 3 $B1\nentry 4 $U\nentry 5 $A2\n" E);
	V();
	IS(F(MOR_ABOUT_REPLACES, "U", "T"), MOR_NOT_SHOWN, MOR_Q_PER_PLACE, NULL); /* the place does not show @b's assent */
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, MOR_Q_PER_PLACE, NULL);
	end();
}

static void g7_3_as_shown(void)
{
	begin(NULL);
	D(BASE UPDATE_U);
	V();
	for (size_t i = 0; i < mor_finding_count(C); i++) {
		const mor_finding *f = mor_finding_at(C, i);
		if (f->about == MOR_ABOUT_IN_FORCE || f->about == MOR_ABOUT_REPLACES || f->about == MOR_ABOUT_VERSION)
			CHECK(f->quals & MOR_Q_AS_SHOWN, "an answer about what is latest lacks `as shown`: %s", f->text);
	}
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, MOR_Q_AS_SHOWN, "an act not held may be later");
	end();
	mor_options o;
	memset(&o, 0, sizeof o);
	o.anchoring_place = "place:mine";
	begin(&o);
	D(BASE "=PL\nmor-act 1 place\nplace place:mine\nentry 1 $T\nentry 2 $A1\nentry 3 $B1\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_IN_FORCE, "T", "T");
	IS(f, MOR_SHOWN, MOR_Q_PER_ANCHORING, NULL);
	CHECK(f && strcmp(f->anchoring, "place:mine") == 0, "the anchoring is named");
	end();
}

static void g7_4_one_place_for_all(void)
{
	/* the reader accepts another anchoring, which shows both assents; the Terms name a place that shows one.
	 * The answer is per the place the Terms name. */
	mor_options o;
	memset(&o, 0, sizeof o);
	o.anchoring_place = "place:mine";
	o.anchoring_clock = "clock:mine";
	begin(&o);
	D(TERMS("T", "@a", "none") AB "place place:p\nclock clock:k\ncondition $after 100\n" E ASSENT("A1", "@a", "$T", "T", "T")
	  ASSENT("B1", "@b", "none", "T", "T")
	  PLACE3 "entry 1 $T\nentry 2 $A1\n" E "=PM\nmor-act 1 place\nplace place:mine\nentry 1 $T\nentry 2 $A1\nentry 3 $B1\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_IN_FORCE, "T", NULL);
	IS(f, MOR_NOT_SHOWN, MOR_Q_PER_PLACE, NULL);
	CHECK(f && strcmp(f->anchoring, "place:p") == 0 && !(f->quals & MOR_Q_PER_ANCHORING), "not per the reader's anchoring");
	end();
	/* the same for the clock (G13.4): the time is taken from the clock the Terms name */
	o.anchoring_place = NULL;
	begin(&o);
	D(TERMS("T", "@a", "none") AB "clock clock:k\ncondition $after 100\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  UPDATE_U "=CK\nmor-act 1 clock\nclock clock:k\ntime 50\n" E "=CM\nmor-act 1 clock\nclock clock:mine\ntime 500\n" E);
	V();
	f = F(MOR_ABOUT_REPLACES, "U", "T");
	IS(f, MOR_NOT_VALID, MOR_Q_PER_CLOCK, NULL);
	CHECK(f && strcmp(f->anchoring, "clock:k") == 0, "per the clock the Terms name");
	end();
	/* only where the agreement names none does the reader's anchoring answer */
	o.anchoring_place = "place:mine";
	begin(&o);
	D(TERMS("T", "@a", "none") AB "condition $after 100\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  UPDATE_U "=CM\nmor-act 1 clock\nclock clock:mine\ntime 500\n" E
	  "=PM\nmor-act 1 place\nplace place:mine\nentry 1 $T\nentry 2 $A1\nentry 3 $B1\nentry 4 $U\nentry 5 $A2\nentry 6 $B2\n" E);
	V();
	f = F(MOR_ABOUT_REPLACES, "U", "T");
	IS(f, MOR_VALID, MOR_Q_PER_ANCHORING, NULL);
	CHECK(f && strcmp(f->anchoring, "clock:mine") == 0, "per anchoring, with the anchoring named");
	end();
}

static void g13_order(void)
{
	begin(NULL);
	D(BASE UPDATE_U);
	V();
	const mor_finding *f = mor_ask_order(C, X("A2"), X("T"));
	IS(f, MOR_SHOWN, 0, "came after");
	CHECK(f && f->quals == 0, "given with no place and no qualifier");
	/* between two acts of which neither names the other, no order is shown */
	IS(mor_ask_order(C, X("A1"), X("B1")), MOR_NOT_SHOWN, 0, "no order");
	IS(mor_ask_order(C, X("T"), X("A2")), MOR_NOT_SHOWN, 0, NULL);
	end();
}

static void g13_1_read_against_named(void)
{
	/* a payment names the version it read: a later version does not change what it did */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "receiver @a\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") AB "receiver @b\n" E ASSENT("A2", "@a", "$U", "T", "U") ASSENT("B2", "@b", "$B1", "T", "U")
	  "=PAY\nmor-act 1 payment\nsigner @buyer\nto $T\nversion $T\nsettlement ~s1\namount 10 EUR\n" E
	  "=PAY2\nmor-act 1 payment\nsigner @buyer\nto $T\nsettlement ~s2\namount 10 EUR\n" E);
	V();
	IS(F(MOR_ABOUT_RECEIVER, "PAY", "T"), MOR_SHOWN, 0, "@a");
	/* an act is not read as having relied on a version it does not name: read from the Terms in force */
	IS(F(MOR_ABOUT_RECEIVER, "PAY2", "T"), MOR_SHOWN, MOR_Q_AS_SHOWN, "@b");
	end();
}

/* ====================== G8, G9 ====================== */

static void g8_who_said_it(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "receiver @a\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  "=SET\nmor-act 1 settlement\nsigner @bank\npayment pay-1\n" E
	  "=PAY\nmor-act 1 payment\nsigner @buyer\nto $T\nversion $T\nsettlement $SET\namount 10 EUR\n" E
	  STMT("S", "@b", "$B1", "T", "T", "$kind") "content delivered-at 1700000000\nsettlement $SET\n" E
	  STMT("K", "@buyer", "none", "T", "T", "contest") "settlement $SET\n" E);
	V();
	/* G8.1: who stated it */
	IS(F(MOR_ABOUT_CONTEST, "K", "T"), MOR_SHOWN, 0, "the word of @buyer");
	IS(FT(MOR_ABOUT_ACT, "S", "kind", NULL), MOR_SHOWN, 0, "@b");
	/* G8.5: a settlement named is reported on the word of whoever gave it */
	IS(FT(MOR_ABOUT_ACT, "S", "SET", NULL), MOR_NOTE, 0, "@bank");
	/* G8.3: value is never reported as moved on the strength of an Agreements act alone */
	const mor_finding *f = F(MOR_ABOUT_PAYMENT, "PAY", NULL);
	IS(f, MOR_ONE_SIDE_ONLY, 0, "not shown as received");
	CHECK(f && strstr(f->text, "the payer @buyer states"), "the payer's word");
	NOWHERE("value moved");
	NOWHERE("was paid");
	end();
}

static void g9_1_unchecked(void)
{
	/* lacks a cMIP named by fingerprint */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "in_force ~absent\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_UNCHECKED, 0, "not held");
	end();
	/* lacks an act named by fingerprint: the Terms named in `under` */
	begin(NULL);
	D(T_PQ("") STMT("G1", "@g", "none", "T", "T", "assent") "for @p\nunder ~absent-terms\n" E);
	V();
	IS(F(MOR_ABOUT_POWER, "G1", "@p"), MOR_UNCHECKED, 0, "not held");
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_UNCHECKED, 0, NULL);
	end();
	/* lacks the acts of an agreement named as a party, whose signing is asked */
	begin(NULL);
	D(TERMS("T", "@a", "none") "party signer @a\nparty agreement ~xroot\nauthor @a\nbody $w0 $plain\nin_force $count 2 @a ~xroot\n" E
	  ASSENT("A1", "@a", "$T", "T", "T") STMT("S", "@x", "none", "T", "T", "assent") "for ~xroot\nunder ~xterms\n" E);
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", NULL), MOR_UNCHECKED, 0, NULL);
	end();
	/* unchecked is not passed: a spot whose cMIP is lacking */
	begin(NULL);
	D(TERMS("T", "@a", "none") "party signer @a\nparty role r kind=done what=~thing cmip=~absent\nauthor @a\nbody $w0 $plain\nin_force $yes\nreceiver role:r\n" E);
	V();
	IS(F(MOR_ABOUT_FILLING, "T", "role:r"), MOR_UNCHECKED, 0, NULL);
	IS(F(MOR_ABOUT_SPOT_OPEN, "T", "role:r"), MOR_UNCHECKED, 0, NULL);
	IS(mor_ask_receiver(C, X("T"), NULL), MOR_UNCHECKED, 0, NULL);
	end();
}

static void g9_2_where_a_verifier_stops(void)
{
	mor_options o;
	memset(&o, 0, sizeof o);
	o.max_receiver_hops = 2;
	o.max_chain = 1;
	begin(&o);
	D(TERMS("R4", "@a", "none") "party signer @h\nauthor @h\nbody $w0 $plain\nin_force $yes\nreceiver @h\n" E);
	D(TERMS("R3", "@a", "none") "party signer @h\nauthor @h\nbody $w0 $plain\nin_force $yes\nreceiver $R4\n" E);
	D(TERMS("R2", "@a", "none") "party signer @h\nauthor @h\nbody $w0 $plain\nin_force $yes\nreceiver $R3\n" E);
	D(TERMS("R1", "@a", "none") "party signer @h\nauthor @h\nbody $w0 $plain\nin_force $yes\nreceiver $R2\n" E);
	V();
	const mor_finding *f = mor_ask_receiver(C, X("R1"), NULL);
	IS(f, MOR_UNCHECKED, 0, "stopped following receivers");
	CHECK(f && strstr(f->text, "after 2"), "says how far it followed");
	IS(mor_ask_receiver(C, X("R2"), NULL), MOR_SHOWN, 0, "@h");
	end();
	/* the versions of a long chain */
	begin(&o);
	D(TERMS("T", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\n" E ASSENT("A1", "@a", "$T", "T", "T")
	  TERMS("U", "@a", "$A1") UPD("T", "T") "party signer @a\nauthor @a\nbody $w0 $plain\n" E ASSENT("A2", "@a", "$U", "T", "U")
	  TERMS("U9", "@a", "$A2") UPD("U", "T") "party signer @a\nauthor @a\nbody $w0 $plain\n" E ASSENT("A3", "@a", "$U9", "T", "U9"));
	V();
	f = F(MOR_ABOUT_IN_FORCE, "T", NULL);
	IS(f, MOR_UNCHECKED, 0, "stopped following the versions");
	CHECK(f && strcmp(f->object, X("U")) == 0, "names the last version it established");
	end();
}

static void g9_2_counter_work(void)
{
	/* found by fuzzing: two signers who assent and cancel many times over, with no place and nothing naming
	 * anything, made the reading of each counter signature lean on all the others. The verifier now stops
	 * after a set number of readings: unchecked, and it says so; never not shown or invalid because it stopped. */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB E);
	char prev_a[16] = "$T", prev_b[16] = "none";
	for (int i = 0; i < 12; i++) {
		D("=A%d\nmor-act 1 statement\nsigner @a\nprevious %s\nagreement $T\nterms $T\nkind assent\n\n"
		  "=KA%d\nmor-act 1 statement\nsigner @a\nprevious $A%d\nagreement $T\nterms $T\nkind counter_signature\n\n"
		  "=B%d\nmor-act 1 statement\nsigner @b\nprevious %s\nagreement $T\nterms $T\nkind assent\n\n"
		  "=KB%d\nmor-act 1 statement\nsigner @b\nprevious $B%d\nagreement $T\nterms $T\nkind counter_signature\n\n",
		  i, prev_a, i, i, i, prev_b, i, i);
		snprintf(prev_a, sizeof prev_a, "$KA%d", i);
		snprintf(prev_b, sizeof prev_b, "$KB%d", i);
	}
	V();
	const mor_finding *f = F(MOR_ABOUT_IN_FORCE, "T", NULL);
	CHECK(f && (f->answer == MOR_UNCHECKED || f->answer == MOR_NOT_SHOWN), "in force is unchecked or not shown, and the verifier returns");
	if (f && f->answer == MOR_UNCHECKED) CHECK(strstr(f->text, "stopped"), "it says that it stopped");
	end();
}

/* ====================== G10 ====================== */

static void g10_1_role_fields(void)
{
	begin(NULL);
	D(TERMS("T1", "@a", "none") "party signer @a\nparty role r what=~thing cmip=$witness @w one\nauthor @a\nbody $w0 $plain\nin_force $yes\n" E
	  TERMS("T2", "@a", "none") "party signer @a\nparty role r kind=done cmip=$witness @w one\nauthor @a\nbody $w0 $plain\nin_force $yes\n" E
	  TERMS("T3", "@a", "none") "party signer @a\nparty role r kind=done what=~thing\nauthor @a\nbody $w0 $plain\nin_force $yes\n" E
	  TERMS("T4", "@a", "none") "party signer @a\nparty role r kind=owner what=~thing cmip=$witness @w one\nauthor @a\nbody $w0 $plain\nin_force $yes\n" E
	  TERMS("T5", "@a", "none") "party signer @a\nparty role r kind=future cmip=$witness @w one\nauthor @a\nbody $w0 $plain\nin_force $yes\n" E
	  TERMS("T6", "@a", "none") "party signer @a\nparty role r kind=fits what=the-rules.txt cmip=$witness @w one\nauthor @a\nbody $w0 $plain\nin_force $yes\n" E);
	V();
	IS(F(MOR_ABOUT_ACT, "T1", NULL), MOR_INVALID, 0, "kind");
	CHECK(F(MOR_ABOUT_ACT, "T1", NULL) && strcmp(F(MOR_ABOUT_ACT, "T1", NULL)->rule, "G10.1") == 0, "names G10.1");
	IS(F(MOR_ABOUT_ACT, "T2", NULL), MOR_INVALID, 0, "what");
	IS(F(MOR_ABOUT_ACT, "T3", NULL), MOR_INVALID, 0, "cMIP");
	IS(F(MOR_ABOUT_ACT, "T4", NULL), MOR_INVALID, 0, "kind");
	CHECK(F(MOR_ABOUT_ACT, "T5", NULL) == NULL, "a role of kind future names no `what`");
	IS(F(MOR_ABOUT_ACT, "T6", NULL), MOR_INVALID, 0, "what");
	end();
}

#define OFFER(extra, mode) TERMS("T", "@a", "none") "party signer @a\n" ROLE(mode) "author @a\nbody $w0 $plain\nin_force $count 1 @a\n" extra E \
	ASSENT("A1", "@a", "$T", "T", "T")

static void g10_2_fillings(void)
{
	begin(NULL);
	D(OFFER("", "many") FILL("W1", "none", "T", "T", "@p") FILL("W2", "$W1", "T", "T", "@q")
	  /* a count by anyone else is not counted */
	  STMT("Z", "@z", "none", "T", "T", "$kind") "content fills r @z\n" E);
	V();
	const mor_finding *f = FT(MOR_ABOUT_FILLING, "T", "role:r", "@p");
	IS(f, MOR_SHOWN, MOR_Q_AS_SHOWN, "confirmed by @w");
	char pf[13];
	snprintf(pf, sizeof pf, "%.12s", X("W1"));
	CHECK(f && strstr(f->text, pf), "each filling is answered with its proof");
	IS(FT(MOR_ABOUT_FILLING, "T", "role:r", "@q"), MOR_SHOWN, 0, "many fillings");
	CHECK(FT(MOR_ABOUT_FILLING, "T", "role:r", "filled by @z") == NULL, "a stranger's claim fills nothing");
	/* G10.3: a signer who fills a spot is bound only by their own assent */
	IS(mor_ask_bound(C, "@p", X("T")), MOR_NOT_SHOWN, 0, NULL);
	D(ASSENT("P1", "@p", "none", "T", "T"));
	V();
	IS(mor_ask_bound(C, "@p", X("T")), MOR_SHOWN, 0, NULL);
	IS(mor_ask_bound(C, "@q", X("T")), MOR_NOT_SHOWN, 0, NULL); /* filling a spot binds no other party */
	end();
}

static void g10_5_new_identities(void)
{
	begin(NULL);
	D(OFFER("", "many max 1"));
	V();
	IS(F(MOR_ABOUT_SPOT_OPEN, "T", "role:r"), MOR_YES, MOR_Q_AS_SHOWN, "cause");
	D(FILL("W1", "none", "T", "T", "@p"));
	V();
	const mor_finding *f = F(MOR_ABOUT_SPOT_OPEN, "T", "role:r");
	IS(f, MOR_NO, MOR_Q_AS_SHOWN, "a count the version in force states");
	/* whatever that answer is, a filling already earned stays a filling */
	IS(FT(MOR_ABOUT_FILLING, "T", "role:r", "@p"), MOR_SHOWN, 0, NULL);
	end();
}

static void g10_6_rights(void)
{
	/* (a) the holder's own assent to the later version */
	begin(NULL);
	D(OFFER("", "many") FILL("W1", "none", "T", "T", "@p")
	  TERMS("U", "@a", "$A1") UPD("T", "T") "party signer @a\n" ROLE("many") "author @a\nbody $w0 $plain\nin_force $count 1 @a\n" E
	  ASSENT("A2", "@a", "$U", "T", "U"));
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U"), MOR_SHOWN, 0, NULL);
	const mor_finding *f = FT(MOR_ABOUT_FILLING, "U", "role:r", "@p");
	char ev[13];
	snprintf(ev, sizeof ev, "%.12s", X("T"));
	IS(f, MOR_SHOWN, 0, "earned under the version");
	CHECK(f && strstr(f->text, ev), "names the version the filling was earned under");
	/* the version it was earned under names no update condition: only (a) changes its rights */
	IS(F(MOR_ABOUT_RIGHTS, "W1", "U"), MOR_NOT_SHOWN, 0, "names no update condition");
	D(ASSENT("P1", "@p", "none", "T", "U"));
	V();
	f = F(MOR_ABOUT_RIGHTS, "W1", "U");
	IS(f, MOR_SHOWN, 0, "assent signed by the holder");
	CHECK(f && strcmp(f->rule, "G10.6(a)") == 0, "what it rests on: the holder's own assent");
	NOWHERE("triggered");
	NOWHERE("no longer appl");
	end();
	/* (b) the version it was earned under names an update condition, and it answers valid */
	begin(NULL);
	D(OFFER("condition $yes\n", "many") FILL("W1", "none", "T", "T", "@p")
	  TERMS("U", "@a", "$A1") UPD("T", "T") "party signer @a\n" ROLE("many") "author @a\nbody $w0 $plain\nin_force $count 1 @a\n" E
	  ASSENT("A2", "@a", "$U", "T", "U"));
	V();
	f = F(MOR_ABOUT_RIGHTS, "W1", "U");
	IS(f, MOR_VALID, 0, "as far as its own words say");
	CHECK(f && strstr(f->text, X("yes")), "the condition is named, with its answer");
	/* nothing is undone: the filling stays a filling, shown with its proof; the verifier does not say of
	 * itself whether the holder is still a party in the spot (G10.7) */
	IS(FT(MOR_ABOUT_FILLING, "U", "role:r", "@p"), MOR_SHOWN, 0, "proof");
	NOWHERE("triggered");
	NOWHERE("no longer appl");
	NOWHERE("is no longer a party");
	end();
}

static void g10_6_not_under_another_condition(void)
{
	/* the filling's own version T names no condition; the next version U names one that answers valid to
	 * everything; a third version U3 is made under U's condition. U3 is not read as changing the filling's
	 * rights under a condition the filling's own version does not name. */
	begin(NULL);
	D(OFFER("", "many") FILL("W1", "none", "T", "T", "@p")
	  TERMS("U", "@a", "$A1") UPD("T", "T") "party signer @a\n" ROLE("many") "author @a\nbody $w0 $plain\nin_force $count 1 @a\ncondition $yes\n" E
	  ASSENT("A2", "@a", "$U", "T", "U")
	  TERMS("U3", "@a", "$A2") UPD("U", "T") "party signer @a\n" ROLE("many") "author @a\nbody $w0 $plain\nin_force $yes\n" E);
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "U3"), MOR_SHOWN, 0, NULL);
	IS(F(MOR_ABOUT_RIGHTS, "W1", "U3"), MOR_NOT_SHOWN, 0, "names no update condition");
	IS(F(MOR_ABOUT_RIGHTS, "W1", "U"), MOR_NOT_SHOWN, 0, NULL);
	end();
}

/* ====================== G11 ====================== */

/* a work whose owner @a receives for @a and @b: it holds the Control task with its socket empty */
#define SHARES "=sw\nmor-act 1 words\nline share @a 50\nline share @b 50\n" E
#define WORK(extra) SHARES TERMS("T", "@a", "none") "party signer @a\nparty signer @b\nauthor @a\nbody $sw $plain\nreceiver @a\n" extra E \
	ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
#define SETTLE(l, id) "=" l "\nmor-act 1 settlement\nsigner @bank\npayment " id "\n" E
#define REPORT(l, s, call) "=" l "\nmor-act 1 report\nsigner " s "\ncall $" call "\n"

static void g11_2_control_task(void)
{
	begin(NULL);
	D(WORK("call control both\ncall delivery doer\n"));
	V();
	IS(F(MOR_ABOUT_CONTROL, "T", NULL), MOR_SHOWN, 0, "holds for the others");
	IS(F(MOR_ABOUT_CALL, "T", "both"), MOR_SHOWN, 0, "the doer is @a");
	/* only a call attached to the Control task is read */
	IS(FT(MOR_ABOUT_UNREAD, "T", "delivery", NULL), MOR_NOTE, 0, NULL);
	end();
	/* no part for another party: the task is not held */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "receiver @a\ncall control both\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_CONTROL, "T", NULL), MOR_NOT_SHOWN, 0, NULL);
	end();
	/* the body cannot be read: unchecked */
	begin(NULL);
	D(TERMS("T", "@a", "none") "party signer @a\nparty signer @b\nauthor @a\nbody ~nowords $plain\nreceiver @a\n" E
	  ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_CONTROL, "T", NULL), MOR_UNCHECKED, 0, NULL);
	end();
	/* the cMIP in `control` is lacking: unchecked */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "receiver @a\ncontrol ~absent\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_CONTROL, "T", NULL), MOR_UNCHECKED, 0, NULL);
	end();
}

static void g11_3_readable_without_cmip(void)
{
	/* `control` names a cMIP the verifier holds for the call, but the body cMIP is lacking for the rest:
	 * here the Control cMIP itself answers nothing we can check, and the report is still read */
	begin(NULL);
	D(CM("ctl", "odd-behaviour") TERMS("T", "@a", "none") "party signer @a\nparty signer @b\nauthor @a\nbody ~nowords ~nocmip\nreceiver @a\n"
	  "control $ctl\ncall control both\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  SETTLE("S1", "p1") SETTLE("S2", "p2")
	  REPORT("RD", "@a", "T") "received 100 EUR $S1\npassed 50 EUR @b $S2\nown 45 EUR\nkept 5 EUR\nowed 0 EUR @b\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_REPORT, "RD", "T");
	IS(f, MOR_SHOWN, 0, "stated by its signer @a");
	CHECK(f && strstr(f->text, "received 100 EUR") && strstr(f->text, "passed on 50 EUR") && strstr(f->text, "kept 5 EUR"),
	      "what the report states is read without any cMIP");
	CHECK(f && strstr(f->text, "unchecked") && strstr(f->text, "does not make the leg report itself unchecked"),
	      "whether it is what the Terms require is unchecked; the report is not");
	/* G11.5: the doer's own part is its own, and is not read as kept */
	CHECK(f && strstr(f->text, "its own part 45 EUR (not a fee, and not read as kept)"), "own part is not a fee");
	end();
}

static void g11_4_ties(void)
{
	begin(NULL);
	D(WORK("call control both\n") SETTLE("S1", "p1") SETTLE("S2", "p2") SETTLE("S3", "p3")
	  REPORT("RD", "@a", "T") "received 100 EUR $S1\npassed 50 EUR @b $S2\npassed 7 EUR @c $S3\npassed 9 EUR @d ~unheld\nown 50 EUR\n" E
	  REPORT("RB", "@b", "T") "received 50 EUR $S2\n" E
	  REPORT("RC", "@c", "T") "received 6 EUR $S3\n" E
	  REPORT("RDD", "@d", "T") "received 9 EUR ~unheld\n" E
	  /* a report by anyone else is listed, as its signer's word, and ties nothing */
	  REPORT("RZ", "@z", "T") "received 50 EUR $S2\npassed 1 EUR @b $S2\n" E);
	V();
	IS(F(MOR_ABOUT_TIE, "S2", "T"), MOR_MATCHED, 0, "@b");
	const mor_finding *f = F(MOR_ABOUT_TIE, "S3", "T");
	IS(f, MOR_MISMATCHED, 0, "neither is chosen");
	CHECK(f && strstr(f->text, "7 EUR") && strstr(f->text, "6 EUR"), "both are shown, with the amount each states");
	/* the settlement two reports name is not held: the tie is unchecked; what each states is still shown */
	f = F(MOR_ABOUT_TIE, MU("~unheld"), "T");
	IS(f, MOR_UNCHECKED, 0, "still shown");
	IS(F(MOR_ABOUT_REPORT, "RZ", "T"), MOR_SHOWN, 0, "ties nothing");
	end();
	/* one side only */
	begin(NULL);
	D(WORK("call control both\n") SETTLE("S2", "p2") REPORT("RD", "@a", "T") "passed 50 EUR @b $S2\n" E);
	V();
	IS(F(MOR_ABOUT_TIE, "S2", "T"), MOR_ONE_SIDE_ONLY, 0, "the doer's word alone");
	end();
	begin(NULL);
	D(WORK("call control both\n") SETTLE("S2", "p2") REPORT("RB", "@b", "T") "received 50 EUR $S2\n" E);
	V();
	IS(F(MOR_ABOUT_TIE, "S2", "T"), MOR_ONE_SIDE_ONLY, 0, "@b");
	end();
	/* Money records one payment as two acts: the halves are tied by its answer to "the same payment" */
	begin(NULL);
	D(WORK("call control both\n") SETTLE("CLAIM", "p2") "=RECEIPT\nmor-act 1 settlement\nsigner @bank2\npayment p2\n" E
	  REPORT("RD", "@a", "T") "passed 50 EUR @b $CLAIM\n" E REPORT("RB", "@b", "T") "received 50 EUR $RECEIPT\n" E);
	V();
	IS(F(MOR_ABOUT_TIE, "CLAIM", "T"), MOR_MATCHED, 0, NULL);
	end();
}

static void g11_4_d_silence(void)
{
	/* a verifier told nothing reads as no side of any leg: it must not show the call as unanswered */
	begin(NULL);
	D(WORK("call control both\n"));
	V();
	const mor_finding *f = F(MOR_ABOUT_CALL, "T", "silence");
	IS(f, MOR_NOT_SHOWN, 0, "not shown as unanswered");
	CHECK(count_about(MOR_ABOUT_CALL, "T", MOR_UNANSWERED) == 0, "never unanswered for a reader beyond the leg");
	end();
	/* a verifier that reads as a side of that leg */
	mor_options o;
	memset(&o, 0, sizeof o);
	o.reader = "@b";
	begin(&o);
	D(WORK("call control both\n"));
	V();
	IS(F(MOR_ABOUT_CALL, "T", "silence"), MOR_UNANSWERED, 0, "read as nothing else");
	end();
	/* or along a path the people of that leg signed for (K8) */
	o.reader = "@partner";
	begin(&o);
	D(WORK("call control both\ndata $readers @partner\n"));
	V();
	IS(F(MOR_ABOUT_CALL, "T", "silence"), MOR_UNANSWERED, 0, NULL);
	end();
	/* Terms that hold the task and carry no call */
	begin(NULL);
	D(WORK(""));
	V();
	f = F(MOR_ABOUT_CALL, "T", NULL);
	IS(f, MOR_NO_REPORT_CALLED_FOR, 0, "not as kept, not as failed");
	end();
}

static void g11_6_call_reads(void)
{
	/* a call that tries to say more than its task and from whom: what is more is not read */
	begin(NULL);
	D(WORK("call control doer place:spy @reader\n")
	  "=sw2\nmor-act 1 words\nline share @a 50\nline share @b 50\ntext second\n" E
	  TERMS("O", "@a", "none") "party signer @a\nparty signer @b\nauthor @a\nbody $sw2 $plain\nreceiver @a\n" E
	  ASSENT("OA", "@a", "$O", "O", "O") ASSENT("OB", "@b", "none", "O", "O")
	  SETTLE("S2", "p2") REPORT("RD", "@a", "T") "passed 50 EUR @b $S2\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_CALL, "T", "doer");
	IS(f, MOR_SHOWN, 0, "names no place and no reader");
	IS(FT(MOR_ABOUT_UNREAD, "T", "call:more-than-task-and-from", NULL), MOR_NOTE, 0, "left unread");
	CHECK(f && !strstr(f->text, "place:spy") && !strstr(f->text, "@reader"), "the call is not read as naming a place or a reader");
	/* T's call is not read as expecting a report on the task held by O */
	IS(F(MOR_ABOUT_CALL, "O", NULL), MOR_NO_REPORT_CALLED_FOR, 0, NULL);
	CHECK(F(MOR_ABOUT_REPORT, "RD", "O") == NULL, "a report answers the call of its own leg only");
	end();
}

static void g11_6_data_path(void)
{
	begin(NULL);
	D(WORK("call control both\n") SETTLE("S2", "p2") REPORT("RD", "@a", "T") "passed 50 EUR @b $S2\n" E);
	V();
	/* `data` is empty: one leg. The people of the leg are not read as having agreed to more. */
	IS(mor_ask_data_path(C, "@partner", X("RD")), MOR_NO, 0, "empty");
	IS(mor_ask_data_path(C, "@a", X("RD")), MOR_YES, 0, "a side of the report's own leg");
	end();
	begin(NULL);
	D(WORK("call control both\ndata $readers @partner\n") SETTLE("S2", "p2") REPORT("RD", "@a", "T") "passed 50 EUR @b $S2\n" E);
	V();
	IS(mor_ask_data_path(C, "@partner", X("RD")), MOR_YES, 0, "lists this reader");
	IS(mor_ask_data_path(C, "@stranger", X("RD")), MOR_NO, 0, NULL);
	end();
	/* the report speaks of an agreement whose own Terms name no data cMIP: every one asked must say yes */
	begin(NULL);
	D(TERMS("S", "@s", "none") "party signer @s\nauthor @s\nbody $w0 $plain\nreceiver @s\n" E ASSENT("SA", "@s", "$S", "S", "S")
	  WORK("call control both\ndata $yes\n") SETTLE("S2", "p2") REPORT("RD", "@a", "T") "passed 50 EUR $S $S2\n" E);
	V();
	IS(mor_ask_data_path(C, "@partner", X("RD")), MOR_NO, 0, NULL);
	end();
	/* the cMIP is lacking */
	begin(NULL);
	D(WORK("call control both\ndata ~absent\n") SETTLE("S2", "p2") REPORT("RD", "@a", "T") "passed 50 EUR @b $S2\n" E);
	V();
	IS(mor_ask_data_path(C, "@partner", X("RD")), MOR_UNCHECKED, 0, NULL);
	end();
}

static void g11_6_b_majority(void)
{
	/* Terms in force by two of three, naming a data cMIP: the outvoted member is read as the rule says;
	 * the verifier does not answer, beyond it, whether that member agreed */
	begin(NULL);
	D("=sw3\nmor-act 1 words\nline share @a 40\nline share @b 30\nline share @c 30\n" E
	  TERMS("T", "@a", "none") "party signer @a\nparty signer @b\nparty signer @c\nauthor @a\nbody $sw3 $plain\nreceiver @a\n"
	  "in_force $count 2 @a @b @c\ncall control both\ndata $yes\n" E
	  ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T")
	  SETTLE("S2", "p2") REPORT("RD", "@a", "T") "passed 30 EUR @c $S2\n" E);
	V();
	IS(F(MOR_ABOUT_IN_FORCE, "T", "T"), MOR_SHOWN, 0, NULL);
	IS(mor_ask_data_path(C, "@partner", X("RD")), MOR_YES, 0, NULL);
	NOWHERE("@c agreed");
	NOWHERE("@c did not agree");
	end();
}

#define SHOP TERMS("T", "@a", "none") AB "receiver @b\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T") SETTLE("S1", "p1") \
	"=PAY\nmor-act 1 payment\nsigner @buyer\nto $T\nversion $T\nsettlement $S1\namount 10 EUR\n" E

static void g11_7_buyers_leg(void)
{
	begin(NULL);
	D(SHOP);
	V();
	IS(F(MOR_ABOUT_RECEIVER, "PAY", "T"), MOR_SHOWN, 0, "@b");
	const mor_finding *f = F(MOR_ABOUT_PAYMENT, "PAY", NULL);
	IS(f, MOR_ONE_SIDE_ONLY, 0, "not shown as received on the buyer's word alone");
	/* confirmed by the receiving side, naming the payment by the settlement both name */
	D(REPORT("CF", "@b", "S1") "received 10 EUR $S1\n" E);
	V();
	IS(F(MOR_ABOUT_PAYMENT, "PAY", NULL), MOR_MATCHED, 0, "@b confirms");
	/* by default the buyer is shown nothing further; a buyer signs no Terms, so no path beyond that leg is read */
	IS(mor_ask_data_path(C, "@partner", X("CF")), MOR_NO, 0, "buyer");
	IS(mor_ask_data_path(C, "@buyer", X("CF")), MOR_YES, 0, "a side");
	end();
	begin(NULL);
	D(SHOP REPORT("CF", "@b", "PAY") "received 9 EUR $S1\n" E);
	V();
	f = F(MOR_ABOUT_PAYMENT, "PAY", NULL);
	IS(f, MOR_MISMATCHED, 0, "neither is chosen");
	CHECK(f && strstr(f->text, "10 EUR") && strstr(f->text, "9 EUR"), "both amounts are shown");
	end();
	/* a confirmation by someone who is not the receiving side ties nothing */
	begin(NULL);
	D(SHOP REPORT("CF", "@a", "PAY") "received 10 EUR $S1\n" E);
	V();
	IS(F(MOR_ABOUT_PAYMENT, "PAY", NULL), MOR_ONE_SIDE_ONLY, 0, NULL);
	IS(F(MOR_ABOUT_REPORT, "CF", "PAY"), MOR_SHOWN, 0, "ties nothing");
	end();
}

static void g11_4_sequence(void)
{
	/* a buyer pays work T; T's receiver is its owner @a, who holds for @b and reports under T's call */
	begin(NULL);
	D(WORK("call control both\n") SETTLE("S1", "p1") SETTLE("S2", "p2")
	  "=PAY\nmor-act 1 payment\nsigner @buyer\nto $T\nversion $T\nsettlement $S1\namount 100 EUR\n" E
	  REPORT("CF", "@a", "PAY") "received 100 EUR $S1\n" E
	  REPORT("RD", "@a", "T") "received 100 EUR $S1\npassed 50 EUR @b $S2\nown 50 EUR\n" E
	  REPORT("RB", "@b", "T") "received 50 EUR $S2\n" E);
	V();
	const mor_finding *f = mor_ask_sequence(C, X("PAY"));
	IS(f, MOR_MATCHED, 0, "the buyer's leg");
	f = FT(MOR_ABOUT_SEQUENCE, "PAY", "S2", NULL);
	IS(f, MOR_MATCHED, 0, "signed by @a");
	end();
	/* a silent leg: nothing ties what lies beyond it; the legs beyond are not shown as part of the result */
	begin(NULL);
	D(WORK("call control both\n") SETTLE("S1", "p1")
	  "=PAY\nmor-act 1 payment\nsigner @buyer\nto $T\nversion $T\nsettlement $S1\namount 100 EUR\n" E
	  REPORT("CF", "@a", "PAY") "received 100 EUR $S1\n" E);
	V();
	mor_ask_sequence(C, X("PAY"));
	f = FT(MOR_ABOUT_SEQUENCE, "PAY", "T", NULL);
	IS(f, MOR_NOT_SHOWN, 0, "legs beyond are not shown");
	end();
}

static void g11_4_e_before_paying(void)
{
	begin(NULL);
	D(TERMS("S", "@s", "none") "party signer @s\nparty signer @a\nauthor @s\nbody $w0 $plain\nreceiver @s\ncontrol $yes\ncall control doer\ndata $yes\n" E
	  ASSENT("SA", "@s", "$S", "S", "S") ASSENT("SB", "@a", "none", "S", "S")
	  TERMS("T", "@a", "none") "party signer @a\nauthor @a\nbody $w0 $plain\nreceiver $S\n" E ASSENT("A1", "@a", "$T", "T", "T"));
	V();
	mor_ask_before_paying(C, X("T"));
	IS(FT(MOR_ABOUT_CALL, "T", "T", "do not hold"), MOR_NOT_SHOWN, MOR_Q_AS_SHOWN, NULL);
	IS(FT(MOR_ABOUT_DATA_PATH, "T", NULL, "no path beyond the leg"), MOR_NO, 0, NULL);
	IS(FT(MOR_ABOUT_CALL, "S", "S", "before any payment"), MOR_SHOWN, 0, "doer");
	const mor_finding *f = FT(MOR_ABOUT_DATA_PATH, "S", "yes", NULL);
	IS(f, MOR_SHOWN, 0, NULL);
	CHECK(f && strstr(f->text, X("yes")), "shown, with its fingerprint");
	end();
}

static void g11_8_contest(void)
{
	begin(NULL);
	D(TERMS("T", "@a", "none") "party signer @a\nparty signer @b\n" ROLE("many") "author @a\nbody $w0 $plain\nin_force $count 2 @a @b\nreceiver @b\n" E
	  ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T") FILL("W1", "none", "T", "T", "@p") SETTLE("S1", "p1")
	  "=PAY\nmor-act 1 payment\nsigner @buyer\nto $T\nversion $T\nsettlement $S1\namount 10 EUR\n" E
	  STMT("K", "@buyer", "none", "T", "T", "contest") "settlement $S1\ncontent not-delivered\n" E
	  STMT("K2", "@other", "none", "T", "T", "contest") "settlement $S1\n" E
	  STMT("K3", "@other", "$K2", "T", "A1", "contest") E
	  SETTLE("S2", "p2") REPORT("RD", "@b", "T") "passed 5 EUR @a $S2\n" E);
	V();
	const mor_finding *f = F(MOR_ABOUT_CONTEST, "K", "T");
	IS(f, MOR_SHOWN, 0, "@a, @b");
	/* it reaches the signers of the version, and no one else: a signer who fills a role is not counted */
	CHECK(f && !strstr(f->text, "@p") && strstr(f->text, "no one else"), "no one but the signers");
	CHECK(f && strstr(f->text, "names a party by role"), "whether the version names a party by role is shown beside it");
	IS(FT(MOR_ABOUT_CONTEST, "K", "buyer", NULL), MOR_SHOWN, 0, "buyer's word");
	/* no payment by the same signer held: whether it is a buyer's is unchecked */
	IS(FT(MOR_ABOUT_CONTEST, "K2", "buyer", NULL), MOR_UNCHECKED, 0, NULL);
	/* its `terms` names no Terms: rejected upstream or not shown */
	CHECK(F(MOR_ABOUT_CONTEST, "K3", NULL) == NULL || F(MOR_ABOUT_CONTEST, "K3", NULL)->answer != MOR_SHOWN, "a contest against no version is not shown");
	/* a contest gives its signer sight of no leg */
	CHECK(f && strstr(f->text, "sight of no leg"), "says so");
	IS(mor_ask_data_path(C, "@buyer", X("RD")), MOR_NO, 0, NULL);
	end();
}

/* ====================== G14 ====================== */

#define ENV(l, baker, work) "=" l "\nmor-act 1 envelope\nbaker " baker "\nwork ~" work "\n" E

static void g14_baker(void)
{
	/* (a) the baker is a party, bound */
	begin(NULL);
	D(ENV("EV", "@a", "song") TERMS("T", "@a", "none") AB "envelope $EV\nreceiver @a\n" E ASSENT("A1", "@a", "$T", "T", "T")
	  ASSENT("B1", "@b", "none", "T", "T") "=RP\nmor-act 1 report\nsigner @a\ncall $T\nown 1 EUR\n" E);
	V();
	IS(F(MOR_ABOUT_BAKER, "T", "EV"), MOR_SHOWN, 0, "the baker @a, as the envelope");
	IS(F(MOR_ABOUT_REPORT, "RP", "T"), MOR_SHOWN, 0, "as an agreement for the baked work"); /* with a leg report too */
	/* that answer is given with every answer about that work that rests on those Terms */
	const mor_finding *f = F(MOR_ABOUT_RECEIVER, "T", "T");
	IS(f, MOR_SHOWN, 0, "as an agreement for the baked work");
	end();
	/* the baker named, not (yet) bound: not shown */
	begin(NULL);
	D(ENV("EV", "@a", "song") TERMS("T", "@a", "none") AB "envelope $EV\n" E ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_BAKER, "T", "EV"), MOR_NOT_SHOWN, 0, NULL);
	end();
	/* neither the baker nor a succession, nothing lacking: invalid */
	begin(NULL);
	D(ENV("EV", "@maker", "song") TERMS("T", "@a", "none") AB "envelope $EV\n" E ASSENT("A1", "@a", "$T", "T", "T")
	  ASSENT("B1", "@b", "none", "T", "T"));
	V();
	f = F(MOR_ABOUT_BAKER, "T", "EV");
	IS(f, MOR_INVALID, 0, "cannot count as an agreement for that work");
	CHECK(f && strcmp(f->rule, "G14.3") == 0, "names G14.3");
	end();
	/* the envelope is lacking: unchecked */
	begin(NULL);
	D(TERMS("T", "@a", "none") AB "envelope ~noenv\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_BAKER, "T", NULL), MOR_UNCHECKED, 0, "baker is not known");
	end();
	/* something relies_on names is lacking: it might be the succession: unchecked, not invalid */
	begin(NULL);
	D(ENV("EV", "@maker", "song") TERMS("T", "@a", "none") AB "envelope $EV\nrelies_on ~lacking\n" E ASSENT("A1", "@a", "$T", "T", "T")
	  ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_BAKER, "T", "EV"), MOR_UNCHECKED, 0, "might be the succession");
	end();
	/* a party by role, which the baker might fill: not shown */
	begin(NULL);
	D(ENV("EV", "@maker", "song") TERMS("T", "@a", "none") "party signer @a\n" ROLE("one") "author @a\nbody $w0 $plain\nin_force $yes\nenvelope $EV\n" E);
	V();
	IS(F(MOR_ABOUT_BAKER, "T", "EV"), MOR_NOT_SHOWN, 0, "might fill");
	f = F(MOR_ABOUT_FILLING, "T", "role:r");
	CHECK(f && strstr(f->text, "as an agreement for the baked work"), "given with a filling answer too");
	end();
}

static void g14_succession(void)
{
	/* the baker @maker sells to @a (agreement M); @a then makes Terms T for the same envelope, naming M */
	begin(NULL);
	D(ENV("EV", "@maker", "song") "=pw\nmor-act 1 words\nline passes @a\n" E
	  TERMS("M", "@maker", "none") "party signer @maker\nparty signer @a\nauthor @maker\nbody $pw $plain\nenvelope $EV\n" E
	  ASSENT("M1", "@maker", "$M", "M", "M") ASSENT("M2", "@a", "none", "M", "M")
	  TERMS("T", "@a", "none") AB "envelope $EV\nrelies_on $M\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_BAKER, "M", "EV"), MOR_SHOWN, 0, "(a)");
	IS(F(MOR_ABOUT_BAKER, "T", "EV"), MOR_SHOWN, 0, "succession");
	end();
	/* the earlier agreement's words cannot be read: unchecked */
	begin(NULL);
	D(ENV("EV", "@maker", "song")
	  TERMS("M", "@maker", "none") "party signer @maker\nparty signer @a\nauthor @maker\nbody ~nowords $plain\nenvelope $EV\n" E
	  ASSENT("M1", "@maker", "$M", "M", "M") ASSENT("M2", "@a", "none", "M", "M")
	  TERMS("T", "@a", "none") AB "envelope $EV\nrelies_on $M\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_BAKER, "T", "EV"), MOR_UNCHECKED, 0, NULL);
	end();
	/* a succession is named, and the acts held do not establish it: not shown */
	begin(NULL);
	D(ENV("EV", "@maker", "song")
	  TERMS("M", "@maker", "none") "party signer @maker\nparty signer @a\nauthor @maker\nbody $w0 $plain\nenvelope $EV\n" E
	  ASSENT("M1", "@maker", "$M", "M", "M") ASSENT("M2", "@a", "none", "M", "M")
	  TERMS("T", "@a", "none") AB "envelope $EV\nrelies_on $M\n" E ASSENT("A1", "@a", "$T", "T", "T") ASSENT("B1", "@b", "none", "T", "T"));
	V();
	IS(F(MOR_ABOUT_BAKER, "T", "EV"), MOR_NOT_SHOWN, 0, NULL);
	end();
}

static void g14_5_same_work(void)
{
	begin(NULL);
	D(ENV("E1", "@maker", "song") ENV("E2", "@copier", "song") ENV("E3", "@other", "another-song")
	  TERMS("T", "@copier", "none") "party signer @copier\nauthor @copier\nbody $w0 $plain\nenvelope $E2\nreceiver @copier\n" E
	  ASSENT("C1", "@copier", "$T", "T", "T")
	  "=PL\nmor-act 1 place\nplace place:any\nentry 1 $E1\nentry 2 $E2\n" E);
	V();
	/* Terms that name the copy's envelope and its baker meet G14.2(a): not forbidden, made legible */
	const mor_finding *f = F(MOR_ABOUT_BAKER, "T", "E2");
	IS(f, MOR_SHOWN, MOR_Q_AS_SHOWN, "the same work is baked by others too");
	CHECK(f && strstr(f->text, "@maker"), "beside the answer of G14.3: each other envelope held and the baker it states");
	IS(F(MOR_ABOUT_SAME_WORK, "E1", "E2"), MOR_SHOWN, MOR_Q_AS_SHOWN, "@copier");
	IS(F(MOR_ABOUT_SAME_WORK, "E2", "E1"), MOR_SHOWN, MOR_Q_AS_SHOWN, "@maker");
	CHECK(F(MOR_ABOUT_SAME_WORK, "E3", NULL) == NULL, "another work is not shown beside");
	/* never picks: no answer says which is first, original, or who made the work */
	f = F(MOR_ABOUT_SAME_WORK, "E1", "E2");
	CHECK(f && strstr(f->text, "no bake is chosen") && strstr(f->text, "not who made it"), "the reasoning is given with the answer");
	NOWHERE("the original bake");
	NOWHERE("the first bake");
	NOWHERE("is the author");
	NOWHERE("made the work is @");
	end();
}

/* ====================== the decoder: rejected ====================== */

static void s3_rejected(void)
{
	static const char *const bad[] = {
		/* a Terms act missing a required field never reaches the grammar rules */
		"mor-act 1 terms\nsigner @a\nparty signer @a\nauthor @a\nbody ~w\n",                                /* no previous */
		"mor-act 1 terms\nsigner @a\nprevious none\nauthor @a\nbody ~w\n",                                    /* no parties */
		"mor-act 1 terms\nsigner @a\nprevious none\nparty signer @a\nbody ~w\n",                              /* no authors */
		"mor-act 1 terms\nsigner @a\nprevious none\nparty signer @a\nauthor @a\n",                            /* no body */
		"mor-act 1 terms\nsigner @a\nprevious none\nparty signer @a\nauthor @a\nbody ~w\nreplaces ~t\n",      /* replaces without agreement */
		"mor-act 1 terms\nsigner @a\nprevious none\nparty signer @a\nauthor @a\nbody ~w\nagreement ~t\n",     /* agreement without replaces */
		"mor-act 1 terms\nsigner @a\nprevious none\nparty signer @a\nauthor @a\nbody ~w\ncontinues ~t\n",     /* continues without resolves */
		"mor-act 1 statement\nsigner @a\nprevious none\nterms ~t\nkind assent\n",                              /* no agreement */
		"mor-act 1 statement\nsigner @a\nprevious none\nagreement ~t\nkind assent\n",                          /* no terms */
		"mor-act 1 statement\nsigner @a\nprevious none\nagreement ~t\nterms ~t\n",                             /* no kind */
		"mor-act 1 statement\nsigner @a\nagreement ~t\nterms ~t\nkind assent\n",                               /* no previous */
		"mor-act 1 statement\nsigner @a\nprevious none\nagreement ~t\nterms ~t\nkind assent\nfor @b\n",        /* for without under */
		"mor-act 1 statement\nsigner @a\nprevious none\nagreement ~t\nterms ~t\nkind assent\nunder ~w\n",      /* under without for */
		"mor-act 1 statement\nsigner @a\nprevious none\nagreement ~t\nterms ~t\nkind my-kind\n",               /* kind not named by fingerprint */
		"mor-act 1 report\nsigner @a\nreceived 1 EUR ~s\n",                                                      /* names no call: no report (R2) */
		"mor-act 1 terms\nsigner @a\nprevious none\nprevious none\nparty signer @a\nauthor @a\nbody ~w\n",    /* a field twice */
	};
	for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
		begin(NULL);
		size_t before = mor_finding_count(C);
		(void)before;
		CHECK(mor_draft(C, bad[i], strlen(bad[i])) == 1, "the draft assembles");
		V();
		CHECK(count_about(MOR_ABOUT_ACT, NULL, MOR_REJECTED) == 1, "case %zu is rejected", i);
		CHECK(count_about(MOR_ABOUT_IN_FORCE, NULL, MOR_SHOWN) + count_about(MOR_ABOUT_IN_FORCE, NULL, MOR_NOT_SHOWN) == 0,
		      "case %zu never reaches the grammar rules", i);
		end();
	}
	/* bytes that cannot be read as an act; a bad signature; a record cut off */
	begin(NULL);
	static const char junk[] = "hello\n\x01\x02\nmor-act 1 terms\nsigner @a\nprevious none\nparty signer @a\nauthor @a\nbody x\n"
	                           "sig 0000000000000000000000000000000000000000000000000000000000000000\nend\nmor-act 1 statement\nsigner @a\n";
	CHECK(mor_load(C, (const unsigned char *)junk, sizeof junk - 1) == 0, "nothing is taken");
	V();
	CHECK(count_about(MOR_ABOUT_ACT, NULL, MOR_REJECTED) == 3, "junk, a bad signature and a cut-off record are rejected");
	end();
	/* a resolving update needs no parties, authors or body; `resolves` without `continues` is an act that resolves nothing */
	begin(NULL);
	D(BASE TERMS("R", "@a", "$A1") UPD("T", "T") "resolves $T\n" E);
	V();
	CHECK(count_about(MOR_ABOUT_ACT, NULL, MOR_REJECTED) == 0, "not rejected");
	IS(F(MOR_ABOUT_REPLACES, "R", "T"), MOR_NOT_SHOWN, 0, NULL);
	end();
}

/* ====================== the table ====================== */

typedef struct {
	void (*fn)(void);
	const char *name, *rule, *sentence;
} tcase;
#define TC(fn, rule, sentence) { fn, #fn, rule, sentence }

static const tcase cases[] = {
	TC(g1_2_under_other_terms, "G1.2", "Where an act's `for` names an agreement, its `under` MUST name Terms of that agreement: an act made for an agreement under any other Terms is not that agreement's signature."),
	TC(g1_2_one_level, "G1.2", "these grammar rules MUST NOT be read as making an agreement's signing need the signature of anyone inside an agreement it names, unless its own Terms say so."),
	TC(g1_2_named_not_bound, "G1.2", "Naming an agreement as a party asks nothing of it: it need not sign to be named, [...] and where it has not signed it is not bound by any act of these Terms, and neither are its parties."),
	TC(g1_2_power_signature, "G1.2", "An act is signed by an agreement x when its `for` names x, by its root; its `under` names Terms of x that are in force (G4.6) and give a power for acts of that kind; and that power's rule, applied to the acts held, says yes."),
	TC(g1_4_no_version_qualified, "G1.4", "Where that act names no version, a verifier MUST take the receiver from that agreement's Terms in force, per that agreement's own place, qualified per place with that place named, or, where it names none, per the anchoring the reader accepts, or as shown (G4.6, G7, G13.4)."),
	TC(g1_4_fork_receiver, "G1.4", "while a fork of its Terms stands, the receiver is the one named by the Terms both branches replace, and the verifier MUST show the fork with that answer and MUST NOT choose a branch (G4.7)."),
	TC(g1_4_not_shown_cases, "G1.4", "If a version read names no receiver, or names as receiver a signer or a role that is not among its own parties, [...] no one receives, and the answer is not shown. [...] If the verifier lacks that agreement's acts, it is unchecked (G9)."),
	TC(g1_4_through_agreements, "G1.4", "Where it is another agreement (G1.3), [...] a verifier reads that agreement's receiver in the same way, and so on, until one of an agreement's own parties is reached."),
	TC(g1_4_role_receiver, "G1.4", "or the spot named is not filled, or is filled more than once (which of them receives is not said), [...] no one receives, and the answer is not shown."),
	TC(g2_1_fingerprints, "G2.1", "The `body`, every piece in `relies_on`, and every cMIP named anywhere in a Terms act MUST be named by fingerprint."),
	TC(g2_2_not_part, "G2.2", "Anything a Terms act refers to in another way is not part of those Terms. A verifier MUST NOT read it as part of them."),
	TC(g3_1_authors, "G3.1", "`authors` MUST name one or more of the parties named as signers or roles."),
	TC(g3_2_authors_input, "G3.2", "A cMIP that reads a judge's decision, in whichever socket it sits (section 5.3), MUST accept `authors` as an input."),
	TC(g4_1_default, "G4.1", "If `in_force` is absent, they are in force when there is an assent to them in the name of every party named as a signer"),
	TC(g4_2_role_needs_in_force, "G4.2", "Terms that name a party by role MUST carry `in_force`."),
	TC(g4_6_qualified, "G4.6", "A verifier MUST take it per the agreement's place, qualified per place (G7.2); or, where no place is named, per the anchoring the reader accepts, with it named, or else as shown (G7.3, G13.4). It MUST NOT give it as latest without saying what the answer stands on."),
	TC(g4_7_fork_of_terms, "G4.7", "A verifier MUST answer fork for which Terms replace them, show both tips, and MUST NOT choose between them, by position or in any other way."),
	TC(g4_7_answers_from_shared, "G4.7", "A verifier MUST answer from them every answer that rests on the Terms in force: among others, a receiver for an act (G1.4), who fills a spot (G10.2), and what a rule the Terms name produces (G5; tasks K2 to K4). It MUST show the fork with each such answer, per place or as shown."),
	TC(g4_8_default_update, "G4.8(c)", "Under the default, a Terms act that lacks one of those signatures replaces nothing: the Terms it names stay in force, and it is not a tip of a fork. A verifier answers not shown for it as a replacement"),
	TC(g4_8_e_unchecked_update, "G4.8(e)", "Where two Terms acts name the same original, and for one of them the verifier cannot tell whether it is an update [...], which Terms are in force is unchecked from there on [...]. The verifier names the last version it has established."),
	TC(g4_8_new_version_in_force, "G4.8", "Whether the replacing Terms are themselves in force is G4.1's, as draft 18 has it (G4.5; section 8.3)."),
	TC(g4_9_never_ended, "G4.9", "No act ends Terms, or an agreement, without replacing them, and a verifier MUST NOT answer that Terms or an agreement have ended."),
	TC(g4_10_counter_place, "G4.10", "Whether the Terms were in force before the counter signature is an answer about what is latest: per place, or as shown (G4.6). With a place, the positions it gives settle it."),
	TC(g4_10_counter_citing, "G4.10", "With none, only citing gives an order (G13.3): a counter signature that names the assents which complete the Terms came after them, and changes nothing; a completing assent that names the counter signature came after it, and the assent it cancels is not counted. Where neither names the other, whether those Terms are in force is not shown, and the verifier says why."),
	TC(g4_11_c_not_valid, "G4.11(c)", "A verifier MUST answer not valid for it, and MUST name the condition beside that answer, with the ground the cMIP gives: an act, reported by who signed it (G8), or a time, per the clock the agreement names (G7.4)"),
	TC(g4_11_c_time_ground, "G4.11(c)", "[the ground the cMIP gives:] or a time, per the clock the agreement names (G7.4), or, where it names none, per the anchoring named or as shown (G13.4)."),
	TC(g4_11_i_no_lock, "G4.11(i)", "A verifier MUST NOT answer, of itself, that an agreement is locked, or that it can never be updated."),
	TC(g4_11_h_lacks_cmip, "G4.11(h)", "Where the verifier lacks the cMIP, whether the update replaces is unchecked (G9), and so is every answer that rests on it (G4.8(e))."),
	TC(g5_1_b_power, "G5.1(b)", "the act's `for` names them [...]; its `under` names Terms by which they are bound and which are in force; those Terms give a power for acts of this kind; and that power's rule, applied to the acts held, says yes"),
	TC(g5_1_c_bound_by_update, "G5.1(c)", "the act is a Terms act that replaces Terms by which they are bound, and the update condition of those Terms answers valid for it (G5.2)."),
	TC(g5_powers_through_powers, "G9.2", "A verifier that stops following (powers through powers, G5; receivers from agreement to agreement, G1.4; the versions of a long chain) answers unchecked for what lies beyond where it stopped, and says how far it followed."),
	TC(g5_2_no_power_for_replacement, "G5.2", "No power makes a Terms act replace, and no power binds a party to one: `powers` has no kind `replaces`. An entry of `powers` of that kind gives no power; a verifier says that it left it unread (G2.2)."),
	TC(g5_3_own_name_only, "G5.3", "An act with no `for` and no `under` is made in its signer's own name only. It MUST NOT later be counted as made for anyone else."),
	TC(g5_4_replaced_power_place, "G5.4", "A verifier MUST answer from G7: per place if the Terms that gave the power name one, by the act's position against the position at which the replacing Terms came into force;"),
	TC(g5_4_replaced_power_citing, "G5.4", "otherwise as shown, by citing alone (G13.3), and not shown where neither the act nor the replacement names the other."),
	TC(g5_5_delegation, "G5.5", "A verifier MUST NOT answer that such a delegation can never be ended: whether the agent will end it is not shown, and the verifier shows the condition as the Terms the delegator signed state it."),
	TC(g5_6_powers_cannot_touch_judicial, "G5.6(d)", "A verifier MUST NOT read an act made under a power as naming or replacing a judge; as giving, widening, narrowing or removing a power to resolve; or as changing whom a contest reaches."),
	TC(g6_1_previous, "G6.1", "Every act within an agreement MUST name in `previous` its signer's previous act within the same agreement, or none if it is the signer's first. An act whose `previous` names an act of another signer, or of another agreement, is invalid (section 2)."),
	TC(g6_2_line_fork, "G6.2", "A verifier holding both MUST answer fork for that signer, and MUST NOT choose between the tips."),
	TC(g7_2_place, "G7.2", "A verifier MUST take those answers from the place, qualified per place."),
	TC(g7_3_as_shown, "G7.3", "Where no place is named, a verifier MUST qualify every such answer as shown, or per anchoring where the reader accepts one, with it named (G13.4). It MUST NOT give such an answer as latest or whole without saying what it stands on."),
	TC(g7_4_one_place_for_all, "G7.4 / G13.4", "a verifier MUST NOT give an answer of G7.2 for that agreement per another anchoring. || A verifier MUST NOT give, per a reader's own anchoring, an answer that the place or the clock the agreement names gives. || A verifier MUST give each such answer per anchoring, with the anchoring named (per place or per clock where it is the one the Terms name), or as shown."),
	TC(g8_who_said_it, "G8.1 / G8.3", "a verifier MUST report who stated it: the one who performed, the one who received, a named witness, the controller, or the place. || A verifier MUST NOT report value as moved on the strength of an Agreements act alone."),
	TC(g9_1_unchecked, "G9.1", "Where a verifier cannot apply a rule it needs, because it lacks a cMIP named by fingerprint, lacks an act named by fingerprint, lacks the acts of an agreement named as a party, or cannot get an answer from a named place, it MUST answer unchecked for every answer that depends on it."),
	TC(g9_2_where_a_verifier_stops, "G9.2", "A verifier that stops following [...] answers unchecked for what lies beyond where it stopped, and says how far it followed. It MUST NOT answer not shown or invalid because it stopped."),
	TC(g9_2_counter_work, "G9.2", "A verifier that stops following [...] answers unchecked for what lies beyond where it stopped, and says how far it followed. [a case found by fuzzing: counter signatures (G4.10) that lean on one another]"),
	TC(g10_1_role_fields, "G10.1", "A role in `parties` MUST name: its kind [...]; its `what` [...], by fingerprint (G2), except for kind future (G10.4); its spot socket (task K5)"),
	TC(g10_2_fillings, "G10.2 / G8.4", "A verifier MUST answer each filling with its proof, each act reported by who signed it (G8.4), qualified per place or as shown (G7). || A verifier MUST report them with whoever confirmed them."),
	TC(g10_5_new_identities, "G10.5", "A verifier MUST give that answer with its cause, as the cMIP gives it, per place, per clock, per the anchoring named or as shown (G7, G13.4)."),
	TC(g10_6_rights, "G10.6 / G10.7", "A verifier MUST name, with each filling, the version it was earned under, and each later version that changes its rights, with what that rests on: the holder's own assent, or the condition named and its answer. || these grammar rules have no answer triggered, and none that a filling's rights no longer apply, and a verifier MUST NOT give either. || a verifier reads them from there (G2) and MUST NOT answer them of itself."),
	TC(g10_6_not_under_another_condition, "G10.6", "A verifier MUST NOT read a later version as changing a filling's rights under an update condition which that filling's own version does not name."),
	TC(g11_2_control_task, "G11.2", "Terms hold the Control task (K6) where their field `control` names a cMIP (G2); or, with that socket empty, where they name as `receiver` one of their own parties (G1.3) and give a part to any other party"),
	TC(g11_3_readable_without_cmip, "G11.3 / G11.5", "What K6's row asks of a leg report MUST be readable without any cMIP those Terms name: a verifier that lacks them still reads the leg reports, and reports what they state as stated by their signers. || Where the doer is itself one to whom those Terms give a part, its report states that part as its own, and a verifier MUST NOT read it as kept (K6's row)."),
	TC(g11_4_ties, "G11.4(c)", "Where they do not, the verifier MUST show both, with the amount each states, and MUST NOT choose between them."),
	TC(g11_4_d_silence, "G11.4(d)", "A verifier that does not, because that leg's reports travel no further than the leg (G11.6), MUST NOT show the call as unanswered: for it, that leg's reports are not shown."),
	TC(g11_4_sequence, "G11.4", "A verifier asked about a result that depends on a sequence MUST read it leg by leg, from the reports it holds (G11.6), and give each leg's part with who signed it (G8.4)."),
	TC(g11_4_e_before_paying, "G11.4(e)", "Asked which agreements of a sequence carry a call for reports, a verifier answers from their Terms alone (G11.2), one agreement at a time, per place or as shown (G4.6), and unchecked where it lacks them."),
	TC(g11_6_call_reads, "G11.6", "A call MUST NOT be read as naming a place, or a reader. A call MUST NOT be read as expecting a report on a task held by other Terms than the ones that carry it."),
	TC(g11_6_data_path, "G11.6(a) / G11.6(b)", "no call, no field and no answer of this text carries a report beyond its leg, and a verifier MUST NOT read any as doing so. || A verifier MUST NOT read the people of a leg as having agreed to be read from beyond their leg unless Terms they signed say so (G5.1): by naming such a data cMIP in `data` (K8), or by a rule in them under which others decide it."),
	TC(g11_6_b_majority, "G11.6(b)", "Where those Terms say that a majority decides, the majority decides (G5.1(b)): a verifier reads a member who was outvoted as that rule says, and MUST NOT answer, beyond it, whether that member agreed."),
	TC(g11_7_buyers_leg, "G11.7", "a verifier shows a payment so confirmed as matched, one whose confirmation states another amount as mismatched, and one the receiving side has not confirmed as one side only; it MUST NOT show a payment as received on the buyer's word alone (G8.3)."),
	TC(g11_8_contest, "G11.8", "A verifier MUST NOT read a contest as contesting anyone but the signers of what it is made against: not a service those signers picked, not a deal of theirs, not any other agreement of the sequence. It MUST NOT read a contest as giving its signer sight of any leg (G11.6, G11.7)."),
	TC(g12_2_built_on_tip, "G12.2", "A verifier MUST show both tips, and what it holds that is built on them, with the fork; MUST answer fork where G4.7 and G6.2 say; and MUST NOT choose between the tips, by position or in any other way."),
	TC(g12_3_line_resolved, "G12.3", "An act that names `resolves` and no `continues` resolves nothing. [...] After a fork in a signer's line is resolved, that signer's next act MAY name as `previous` either the resolving update or the last act of the continued branch"),
	TC(g12_3_must_name_every_tip, "G12.3", "A resolving update MUST name every tip, and MAY name more acts than the tips."),
	TC(g12_3_terms_resolved, "G12.3", "It resolves the fork only when it is signed as an update of the shared act must be: [...] for Terms, so that the update condition of the shared Terms answers valid for it, which says who must sign it; by default, everyone who signed the shared version"),
	TC(g12_4_power_to_resolve, "G12.4", "A verifier MUST report such a resolving update with the power it was signed under and whoever it rests on (G5.1, G8)."),
	TC(g12_5_disputed, "G12.5", "A verifier MUST show that act, and every answer resting on it, as disputed."),
	TC(g12_7_rival_resolvers, "G12.7", "Two resolving updates of the same fork, each signed as G12.3 asks, that continue the same tip resolve it, and are no fork. Two that continue different tips are themselves a fork"),
	TC(g12_7_rival_resolvers_terms, "G12.7", "Neither resolves the first fork while the other stands: every tip stays stopped (G12.2), and the shared act stays the last act of the line that counts. That fork is resolved the same way: by a resolving update that names the two rival ones in `resolves` and one of them in `continues`"),
	TC(g1_4_loop, "G1.4", "or an agreement is met a second time on the way, no one receives, and the answer is not shown."),
	TC(g12_8_client, "G12.8", "A client that holds both tips of a fork MUST NOT show either tip, or anything built on either, as a resolved act: not as the next version, not as in force, not as that signer's latest act. [a duty of clients; no verifier can check it: tested only as far as the verifier's own output goes]"),
	TC(g13_order, "G13.3", "An act that names another, directly or through acts that name it, came after it [...]. Between two acts of which neither so names the other, no order is shown from the acts alone."),
	TC(g13_1_read_against_named, "G13.2", "A verifier MUST NOT read an act as having relied on a version or act it does not name."),
	TC(g14_baker, "G14.3", "A verifier asked whether Terms count as an agreement for a baked work MUST read the envelope, the baker it states, and, under (b), every agreement of the succession. || A verifier MUST give that answer with every answer about that work that rests on those Terms (a receiver, a filling, a leg report)."),
	TC(g14_succession, "G14.2(b)", "one of their parties, bound by them, holds the baker's rights through a succession: agreements for the same envelope, of which the first meets (a); each passes the baker's rights to a party of the next [...]; and the last is these Terms."),
	TC(g14_5_same_work, "G14.5(a) / G14.5(b)", "it MUST show, beside each of them and beside every answer of G14.3 about Terms for one of them, that the same work is baked by others too, with each other envelope it holds and the baker each states (G8). || A verifier MUST NOT choose between bakes that carry the same work: not the first, by position, by time or in any other way, and it MUST NOT answer which baker made the work."),
	TC(s3_rejected, "section 2 / 3", "An act that lacks a required field (section 3), or bytes that cannot be read as an act, never reach these grammar rules. A verifier shows them as rejected, which is no answer of this text."),
};

int main(int argc, char **argv)
{
	size_t n = sizeof cases / sizeof cases[0];
	if (argc > 1 && strcmp(argv[1], "--list") == 0) {
		printf("| Grammar rule | Test case (tests/test_mor.c) | The sentence it tests |\n|---|---|---|\n");
		for (size_t i = 0; i < n; i++) printf("| %s | `%s` | %s |\n", cases[i].rule, cases[i].name, cases[i].sentence);
		return 0;
	}
	for (size_t i = 0; i < n; i++) {
		int before = g_fails;
		if (argc > 1 && strcmp(argv[1], cases[i].name) != 0) continue;
		g_case = cases[i].name;
		cases[i].fn();
		printf("%s %s (%s)\n", g_fails == before ? "ok  " : "FAIL", cases[i].name, cases[i].rule);
	}
	printf("%zu cases, %d checks, %d failures\n", n, g_checks, g_fails);
	return g_fails ? 1 : 0;
}
