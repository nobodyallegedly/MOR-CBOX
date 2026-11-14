/* internal.h: shared types of the verifier. See mor.h for the memory discipline. */
#ifndef MOR_INTERNAL_H
#define MOR_INTERNAL_H

#include "mor.h"
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef const char *sym; /* interned, arena-owned, compared by pointer */

#define VEC(T) struct { T *v; size_t n, cap; }
typedef VEC(sym) symvec;
typedef VEC(char) charvec;

/* limits on untrusted input */
#define MOR_MAX_LINE     4096u
#define MOR_MAX_RECORDS  20000u
#define MOR_MAX_REJECTS  2000u
#define MOR_MAX_ARENA    ((size_t)512 << 20)
#define MOR_MAX_FINDINGS 400000u
#define MOR_MAX_COUNTER_WORK 64u /* nested readings of counter signatures before a verifier stops (G9.2) */
#define MOR_MAX_COUNTER_TOTAL 100000u /* the same, over one whole run */

typedef struct {
	bool present;  /* the socket holds something */
	bool is_fp;    /* and it is named by fingerprint */
	sym cmip;
	symvec params;
} msock;

enum { PARTY_SIGNER, PARTY_ROLE, PARTY_AGREEMENT };
typedef struct {
	int how;
	sym id;        /* signer id, role label, or root */
	sym kind;      /* role: NULL when lacking */
	sym what;      /* role: NULL when lacking */
	msock spot;    /* role: K5 */
} mparty;

typedef struct { sym kind; msock rule; } mpower;
typedef struct { sym task; sym from; } mcall;

enum { LN_RECEIVED, LN_PASSED, LN_OWN, LN_KEPT, LN_OWED };
typedef struct { int type; uint64_t amount; sym unit; sym to; sym settlement; } mline;
typedef struct { uint64_t pos; sym fp; } mentry;
typedef struct { symvec t; } mwline;

enum rtype { R_TERMS, R_STATEMENT, R_REPORT, R_PAYMENT, R_SETTLEMENT, R_ENVELOPE, R_PLACE, R_CLOCK, R_CMIP, R_WORDS, R_NTYPES };

enum { L_OK, L_STANDING, L_SET_ASIDE };                 /* state in a line or chain */
enum { V_NONE, V_VERSION, V_TIP, V_SET_ASIDE, V_UNCHECKED };
enum { CH_TODO, CH_BUSY, CH_DONE };
enum { CR_NONE, CR_IN_FORCE, CR_NOT_IN_FORCE, CR_FORK, CR_UNCHECKED };
enum { T_NO, T_YES, T_UNK };                            /* not shown / shown / unchecked */

typedef struct mrec mrec;
typedef VEC(mrec *) recvec;
struct mrec {
	int type;
	sym fp;
	sym signer;
	size_t index;
	/* Agreements fields (section 3) */
	bool has_previous;
	sym previous;  /* NULL: none */
	sym agreement, replaces, terms, kind, for_, under, settlement, continues;
	sym place, clock, receiver, envelope, body;
	bool has_content;
	symvec relies_on, answers, resolves, authors, content, unread;
	VEC(mparty) parties;
	VEC(mpower) powers;
	VEC(mcall) calls;
	msock body_cmip, in_force, condition, control, data;
	/* stand-in records */
	sym call;                 /* report */
	VEC(mline) lines;         /* report */
	sym to, version, unit;    /* payment */
	uint64_t amount;
	bool has_amount;
	sym payment_id;           /* settlement */
	sym baker, work;          /* envelope */
	VEC(mentry) entries;      /* place */
	uint64_t time;            /* clock */
	bool has_time;
	sym stub;                 /* cmip */
	VEC(mwline) wlines;       /* words */
	/* analysis */
	const char *invalid_rule, *invalid_why;
	sym root;                 /* the agreement this act is within */
	int lstate;               /* signer's line */
	int cstate;               /* Terms: chain */
	int vstate;               /* Terms: as a version */
	mrec *next;               /* Terms: the version that replaced it */
	mrec *shared;             /* Terms tip/set aside: the shared version */
	sym judged_place;         /* Terms: place its coming into force was judged by */
	bool line_open_before;    /* previous names an act not held */
	bool walked, evald;
	signed char ceff;         /* counter signature: 0 not read yet, else 1 + its plain reading */
	const char *ceff_why;
	VEC(mrec *) lkids;        /* acts naming this one in `previous` */
	VEC(mrec *) ckids;        /* Terms naming this one in `replaces` */
	VEC(mrec *) stmts;        /* Statements naming this one in `terms` */
	uint32_t mark;
	/* root Terms only */
	int ch, cresult;
	mrec *clast;              /* last version established */
	mrec *ccur;               /* during the walk */
	int ext_ok, ext_ok_prev;  /* Statement: resolving update under a power (G12.4) */
};

struct ient { mrec *rec; uint32_t hash; char s[]; };

struct placeinfo { sym id; int status; VEC(mentry) entries; };
enum { PL_OK, PL_NO_ANSWER, PL_CONTRADICT };

struct chunk;
struct mor_ctx {
	mor_options opt;
	jmp_buf jb;
	bool oom, verified;
	struct chunk *chunks;
	unsigned char *cur;
	size_t left, total;
	struct ient **itab;
	size_t icap, icount;
	VEC(mrec *) recs;
	VEC(mor_finding) finds;
	VEC(mor_finding) rejects;   /* kept across verify passes */
	VEC(struct placeinfo) places;
	VEC(mrec *) stack;          /* scratch for graph walks */
	VEC(mrec *) exts;           /* Statements shaped as resolving updates made for someone */
	size_t base_finds;
	uint32_t epoch;
	unsigned cnest, cwork;      /* counter signatures being read, and the work spent on the outermost */
	unsigned ctotal;            /* and over the whole run */
	bool unsettled;
	sym s_assent, s_counter, s_contest, s_resolves, s_replaces, s_none, s_control;
	sym s_doer, s_receiving, s_both;
	sym reader, anch_place, anch_clock;
	char why[512];              /* reason of the last tri-state answer */
	VEC(struct mlabel { sym name; sym fp; }) labels; /* draft.c */
	charvec sealed;           /* draft.c: the sealed bytes of what was drafted */
};

typedef struct { unsigned bits; sym anchor; } mqual;

/* a view: which acts count for answers about what is latest (G7.2, G4.10) */
typedef struct {
	int pstat;         /* 0 none named, 1 a place answers, 2 no answer: unchecked */
	sym place;
	bool anchoring;    /* the place is the reader's anchoring, not the Terms' */
	bool has_cut;
	uint64_t cut;
	mrec *cited_by;    /* only acts this act cites */
	const mrec *skip;  /* a counter signature left out of the reading */
} mview;

/* core.c */
void *ar_alloc(mor_ctx *c, size_t n);
void mor_fail(mor_ctx *c);
sym intern(mor_ctx *c, const char *s, size_t n);
sym intern0(mor_ctx *c, const char *s);
sym lookup_sym(const mor_ctx *c, const char *s);
void vec_grow(mor_ctx *c, void *pv, size_t *cap, size_t n, size_t es);
#define vec_push(c, vec, x) \
	(vec_grow((c), &(vec).v, &(vec).cap, (vec).n, sizeof *(vec).v), (vec).v[(vec).n++] = (x))
#define SYM_ENT(x_) ((struct ient *)(void *)((char *)(uintptr_t)(x_) - offsetof(struct ient, s)))
static inline mrec *rec_of(sym s) { return s ? SYM_ENT(s)->rec : NULL; }
bool is_fp(sym s);
bool sym_in(const symvec *v, sym s);
#if defined(__GNUC__)
#define PRINTF_LIKE(a, b) __attribute__((format(printf, a, b)))
#else
#define PRINTF_LIKE(a, b)
#endif
const mor_finding *emit(mor_ctx *c, mor_about about, sym subj, sym obj, mor_answer a, mqual q,
                        const char *rule, const char *fmt, ...) PRINTF_LIKE(8, 9);
void why(mor_ctx *c, const char *fmt, ...) PRINTF_LIKE(2, 3);
char *ar_printf(mor_ctx *c, const char *fmt, ...) PRINTF_LIKE(2, 3);

/* decode.c */
int decode_all(mor_ctx *c, const unsigned char *bytes, size_t n);

/* stubs.c: the sockets K1 to K8 */
typedef struct {
	int status;            /* T_YES read, T_UNK cannot read */
	bool is_delegation;
	sym delegator, agent;
	int end;               /* 0 none stated, 1 agent only, 2 other conditions */
	VEC(struct mshare { sym who; uint64_t n; }) shares;
	symvec passes;         /* to whom these Terms pass the baker's rights */
} k1_reading;
typedef struct { sym signer; mrec *proof; sym earned; bool disputed; } mfilling;
typedef struct {
	int status;            /* T_YES answered, T_UNK cannot apply */
	VEC(mfilling) fills;
	bool many;
	int open;              /* T_YES takes new identities, T_NO not */
	const char *cause;
	sym witness;
} k5_answer;

void k1_read(mor_ctx *c, mrec *terms, k1_reading *out);
int k2_in_force(mor_ctx *c, mrec *terms, const mview *v, int depth);
int k3_power(mor_ctx *c, mrec *terms, const mpower *p, mrec *act);
int k4_condition(mor_ctx *c, mrec *original, mrec *update, const mview *v, char *ground, size_t gcap, mqual *gq);
void k5_spot(mor_ctx *c, mrec *version, const mparty *role, k5_answer *out);
int k6_control(mor_ctx *c, mrec *terms);
int k7_kind(mor_ctx *c, sym kind);
int k8_data(mor_ctx *c, mrec *terms, sym reader, mrec *report, const char **ground);

/* engine.c */
void engine_run(mor_ctx *c);
void chain_eval(mor_ctx *c, mrec *root);
mview view_of(mor_ctx *c, sym place);
mqual qual_of(mor_ctx *c, const mrec *terms);
bool visible(mor_ctx *c, const mview *v, const mrec *r);
bool place_pos(mor_ctx *c, sym place, sym fp, uint64_t *pos);
bool cites(mor_ctx *c, mrec *a, const mrec *b);
bool act_counts(mor_ctx *c, const mrec *r, const mrec *assent_to);
int assent_in_name(mor_ctx *c, mrec *terms, sym name, const mview *v, int depth);
int assent_by_agreement(mor_ctx *c, mrec *terms, sym root, const mview *v, int depth);
int in_force_signed(mor_ctx *c, mrec *terms, const mview *v, int depth);
int agreement_terms(mor_ctx *c, sym root, mrec **out);
int power_ok(mor_ctx *c, mrec *act, sym kind, int depth);
int bound_by_terms(mor_ctx *c, sym person, mrec *terms, int depth);
bool is_signer_party(const mrec *terms, sym who);
const mparty *find_role(const mrec *terms, sym label);
mview view_for_version(mor_ctx *c, const mrec *terms);
typedef struct {
	mor_answer ans;
	sym party;
	mqual q;
	mrec *version;     /* the version the receiver was read from */
	int hops;
	const char *text;
} mreceiver;
void receiver_for(mor_ctx *c, sym root, sym version, mreceiver *out);
bool clock_time(mor_ctx *c, const mrec *terms, uint64_t *t, mqual *q, int *status);
bool branch_signed_by(mor_ctx *c, const mrec *x, sym signer);
bool rests_on_set_aside(mor_ctx *c, const mrec *r);
bool is_resolver_shaped(const mrec *r);
mqual qual_of_view(const mview *v);
int terms_in_force_for_act(mor_ctx *c, mrec *w, mrec *act);
bool follows_in_line(const mor_ctx *c, const mrec *x, const mrec *a);
size_t line_distance(const mor_ctx *c, const mrec *x, const mrec *a);

/* report.c: G10, G11, G14 and the questions */
void report_run(mor_ctx *c);
mor_answer baker_answer(mor_ctx *c, mrec *terms, const char **text);
const char *baker_note(mor_ctx *c, mrec *terms);

static inline int tri_and(int a, int b) {
	if (a == T_NO || b == T_NO) return T_NO;
	if (a == T_UNK || b == T_UNK) return T_UNK;
	return T_YES;
}
static inline int tri_or(int a, int b) {
	if (a == T_YES || b == T_YES) return T_YES;
	if (a == T_UNK || b == T_UNK) return T_UNK;
	return T_NO;
}
static inline int tri_not(int a) { return a == T_UNK ? T_UNK : (a == T_YES ? T_NO : T_YES); }
static inline mqual mq(unsigned bits, sym anchor) { mqual q = { bits, anchor }; return q; }

#endif
