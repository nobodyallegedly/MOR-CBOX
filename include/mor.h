/* mor.h: public interface of the Agreements verifier (draft 19).
 *
 * Memory discipline: one arena per mor_ctx. Every byte the library keeps is
 * copied into that arena; nothing is freed one by one; mor_free() releases all
 * of it at once. Pointers handed out (findings, strings) live until mor_free().
 * The library keeps no global mutable state. */
#ifndef MOR_H
#define MOR_H

#include <stddef.h>
#include <stdio.h>

#define MOR_FP_LEN 64 /* a fingerprint is 64 lower-case hex characters */

typedef struct mor_ctx mor_ctx;

/* The answers of section 2, the block's words (G11.4), and "rejected". */
typedef enum {
	MOR_SHOWN,
	MOR_NOT_SHOWN,
	MOR_FORK,
	MOR_VALID,
	MOR_NOT_VALID,
	MOR_INVALID,
	MOR_UNCHECKED,
	MOR_REJECTED,          /* no answer of the text: not an Agreements act */
	MOR_MATCHED,           /* the report block's words */
	MOR_MISMATCHED,
	MOR_ONE_SIDE_ONLY,
	MOR_UNANSWERED,
	MOR_NO_REPORT_CALLED_FOR,
	MOR_YES,               /* a task's own output, passed on (K8, K5) */
	MOR_NO,
	MOR_NOTE               /* something said beside an answer (left unread, ...) */
} mor_answer;

/* Qualifiers (section 2). */
#define MOR_Q_AS_SHOWN      0x001u
#define MOR_Q_PER_PLACE     0x002u
#define MOR_Q_PER_CLOCK     0x004u
#define MOR_Q_PER_ANCHORING 0x008u
#define MOR_Q_DISPUTED      0x010u
#define MOR_Q_WITH_FORK     0x020u /* the fork is shown with this answer */
#define MOR_Q_BY_DEFAULT    0x040u /* "by default" beside valid */
#define MOR_Q_BUILT_ON_TIP  0x080u

/* What a finding is about. */
typedef enum {
	MOR_ABOUT_ACT,        /* an act as such: rejected, invalid, counts */
	MOR_ABOUT_UNREAD,     /* a field or entry left unread (G2.2, G5.2) */
	MOR_ABOUT_LINE,       /* one signer's line in one agreement (G6, G12) */
	MOR_ABOUT_IN_FORCE,   /* which Terms of an agreement are in force (G4.6) */
	MOR_ABOUT_VERSION,    /* whether given Terms are in force (G4.9) */
	MOR_ABOUT_REPLACES,   /* a proposed update, as a replacement (G4.8, G4.11) */
	MOR_ABOUT_REPLACED_BY,/* which Terms replace given Terms (G4.7) */
	MOR_ABOUT_ASSENT,     /* an assent: counted, binding (G4.3) */
	MOR_ABOUT_COUNTER,    /* a counter signature (G4.10) */
	MOR_ABOUT_POWER,      /* an act made for someone under a power (G5.1(b)) */
	MOR_ABOUT_BOUND,      /* is a person bound by an act (G5.1) */
	MOR_ABOUT_RECEIVER,   /* receiver for an act (G1.4) */
	MOR_ABOUT_FILLING,    /* who fills a spot (G10.2) */
	MOR_ABOUT_SPOT_OPEN,  /* does a spot still take new identities (G10.5) */
	MOR_ABOUT_RIGHTS,     /* a later version and a filling's rights (G10.6) */
	MOR_ABOUT_CONTROL,    /* do Terms hold the Control task (G11.2) */
	MOR_ABOUT_CALL,       /* a call for reports (G11.2, G11.4) */
	MOR_ABOUT_REPORT,     /* a report, as its signer's word (G11.3) */
	MOR_ABOUT_TIE,        /* two halves about one settlement (G11.4) */
	MOR_ABOUT_PAYMENT,    /* a payment and its confirmation (G11.7) */
	MOR_ABOUT_CONTEST,    /* a contest (G11.8) */
	MOR_ABOUT_BAKER,      /* Terms as an agreement for a baked work (G14.3) */
	MOR_ABOUT_SAME_WORK,  /* another bake of the same work (G14.5) */
	MOR_ABOUT_DATA_PATH,  /* a path beyond one leg (G11.6, K8) */
	MOR_ABOUT_ORDER,      /* order from citing (G13.3) */
	MOR_ABOUT_SEQUENCE,   /* a result read leg by leg (G11.4) */
	MOR_ABOUT_SETTLING    /* the reading of forks did not settle (G9.2) */
} mor_about;

typedef struct {
	mor_about about;
	const char *subject;   /* fingerprint or identifier; never NULL */
	const char *object;    /* second fingerprint or identifier, or "" */
	mor_answer answer;
	unsigned quals;        /* MOR_Q_* */
	const char *anchoring; /* the place, clock or anchoring named, or "" */
	const char *rule;      /* the grammar rule named with the answer */
	const char *text;      /* who said it, the ground, the reason */
} mor_finding;

/* Stand-in interfaces (STANDINS.md). NULL selects the built-in stand-in. */
typedef struct {
	/* fingerprint of fixed bytes: writes 64 hex chars and a NUL; 0 on success */
	int (*fingerprint)(const unsigned char *bytes, size_t n, char out[MOR_FP_LEN + 1]);
	/* is `sig` the signature of `signer` over `bytes`: 1 yes, 0 no */
	int (*sig_check)(const char *signer, const unsigned char *bytes, size_t n, const char *sig);
	/* who is reading (G11.4(d)): a signer identifier, or NULL for no side */
	const char *reader;
	/* the anchoring the reader accepts where an agreement names none (G13.4) */
	const char *anchoring_place;
	const char *anchoring_clock;
	/* where the verifier stops (G9.2); 0 selects the default */
	unsigned max_power_depth;    /* default 8 */
	unsigned max_receiver_hops;  /* default 8 */
	unsigned max_chain;          /* default 1024 */
} mor_options;

mor_ctx *mor_new(const mor_options *opt);
void mor_free(mor_ctx *c);

/* Decode records from untrusted bytes. Returns the number of records taken,
 * or -1 when memory ran out. Bytes that are no act are recorded as rejected. */
int mor_load(mor_ctx *c, const unsigned char *bytes, size_t n);

/* Apply the grammar rules to everything loaded. 0 on success, -1 on memory. */
int mor_verify(mor_ctx *c);

size_t mor_finding_count(const mor_ctx *c);
const mor_finding *mor_finding_at(const mor_ctx *c, size_t i);
/* first finding with this about/subject (object may be NULL for any) */
const mor_finding *mor_find(const mor_ctx *c, mor_about about, const char *subject, const char *object);

/* Questions asked after mor_verify(). Each appends one or more findings and
 * returns the first of them (NULL on memory failure). */
const mor_finding *mor_ask_receiver(mor_ctx *c, const char *agreement_root, const char *version_or_null);
const mor_finding *mor_ask_bound(mor_ctx *c, const char *person, const char *act);
const mor_finding *mor_ask_in_force(mor_ctx *c, const char *terms);
const mor_finding *mor_ask_order(mor_ctx *c, const char *later, const char *earlier);
const mor_finding *mor_ask_data_path(mor_ctx *c, const char *reader, const char *report);
const mor_finding *mor_ask_before_paying(mor_ctx *c, const char *agreement_root);
const mor_finding *mor_ask_sequence(mor_ctx *c, const char *payment);

const char *mor_answer_name(mor_answer a);
const char *mor_about_name(mor_about a);
void mor_print_finding(const mor_finding *f, FILE *out, int full);
void mor_print(const mor_ctx *c, FILE *out, int full);

/* Drafts (tests, tools): records written without `sig` and `end`, separated by
 * blank lines. A line `=name` labels the next record; `$name` in a later record
 * is replaced by that record's fingerprint; `~name` by a made-up fingerprint of
 * something not held. Each record is sealed and loaded. Returns the number of
 * records sealed, or -1 on a malformed draft or memory failure. */
int mor_draft(mor_ctx *c, const char *text, size_t n);
const char *mor_label(const mor_ctx *c, const char *name);   /* fingerprint, or NULL */
const char *mor_sealed(const mor_ctx *c, size_t *n);         /* sealed bytes so far */

/* Stand-in helpers for writing acts (tests, tools).
 * mor_seal: `body` is a record without its `sig` and `end` lines. Writes the
 * sealed record to out (NUL-terminated) and its fingerprint to fp.
 * Returns the sealed length, or 0 if it does not fit or is malformed. */
size_t mor_seal(const mor_options *opt, const char *body, char *out, size_t cap, char fp[MOR_FP_LEN + 1]);
int mor_standin_fingerprint(const unsigned char *bytes, size_t n, char out[MOR_FP_LEN + 1]);
int mor_standin_sig_check(const char *signer, const unsigned char *bytes, size_t n, const char *sig);
void mor_standin_sig_make(const unsigned char *bytes, size_t n, char out[MOR_FP_LEN + 1]);

#endif
