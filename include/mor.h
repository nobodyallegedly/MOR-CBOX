/*
 * mor.h - a verifier for the Agreements layer of MOR (draft 17), built from
 * spec/agreements-draft-17.md and spec/report-block-2.md alone.
 *
 * MEMORY DISCIPLINE (one, kept throughout):
 *   Every object the library makes lives in the arena of one mor_set and dies
 *   with it, in mor_set_free(). Nothing is freed one by one, nothing is
 *   reallocated, and the library keeps no pointer into bytes the caller gave
 *   it: input is copied in. All counts are bounded by the limits below; a set
 *   that passes a limit gets a rejected record or the answer "unchecked",
 *   never a silent cut. Running out of memory is the one failure that is not
 *   an answer: the arena calls abort().
 */
#ifndef MOR_H
#define MOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* ---- limits ---------------------------------------------------------- */
#define MOR_MAX_INPUT (4u * 1024u * 1024u) /* bytes per load */
#define MOR_MAX_ACTS 1024                  /* records per set */
#define MOR_MAX_LINE 600                   /* bytes per line of a record */
#define MOR_MAX_LINES 200                  /* lines per record */
#define MOR_MAX_LIST 64                    /* entries of any repeated field */
#define MOR_MAX_ENTRIES 1024               /* entries of one place answer */
#define MOR_MAX_DEPTH 8                    /* powers through powers, receivers, legs (P2, P8) */
#define MOR_MAX_CHAIN 256                  /* versions of one agreement */
#define MOR_MAX_LEGS 64

/* ---- fingerprint (stand-in, see STANDINS.md) -------------------------- */
#define MOR_FP_LEN 32
typedef struct {
    unsigned char b[MOR_FP_LEN];
} mor_fp;

void mor_fp_of(const unsigned char *bytes, size_t len, mor_fp *out);
void mor_fp_hex(const mor_fp *fp, char out[65]);
bool mor_fp_parse(const char *hex, size_t len, mor_fp *out);
bool mor_fp_eq(const mor_fp *a, const mor_fp *b);

/* ---- signature (stand-in, see STANDINS.md) ---------------------------- */
void mor_standin_sign(const char *signer, const mor_fp *act, char out[65]);
bool mor_standin_verify(const char *signer, const mor_fp *act, const char *sig_hex);

/* ---- answers (section 2, "Answers") ----------------------------------- */
typedef enum {
    MOR_SHOWN,
    MOR_NOT_SHOWN,
    MOR_FORK,
    MOR_VALID,
    MOR_NOT_VALID,
    MOR_INVALID,
    MOR_UNCHECKED,
    /* the report block's words, shown of reports (G11.4) */
    MOR_MATCHED,
    MOR_MISMATCHED,
    MOR_ONE_SIDE_ONLY,
    MOR_UNANSWERED,
    MOR_NO_REPORT_CALLED_FOR
} mor_ans;

typedef enum { MOR_Q_NONE, MOR_Q_AS_SHOWN, MOR_Q_PER_PLACE, MOR_Q_PER_CLOCK, MOR_Q_PER_ANCHORING } mor_qkind;

typedef struct {
    mor_qkind kind;
    const char *name; /* the place, clock or anchoring named */
} mor_qual;

typedef struct {
    mor_ans ans;
    mor_qual qual;
    bool fork;       /* a fork of Terms stands and is shown with this answer (G4.7) */
    bool disputed;   /* rests on the branch of a tip set aside (G12.5) */
    const char *why; /* plain words, with the grammar rule */
} mor_answer;

/* a yes / no / could-not-apply, used inside the grammar and by plug-ins */
typedef enum { MOR_NO = 0, MOR_YES = 1, MOR_CANNOT = 2 } mor_tri;

const char *mor_ans_name(mor_ans a);
const char *mor_qual_str(const mor_qual *q, char *buf, size_t n);

/* ---- acts (stand-in format, see STANDINS.md) --------------------------- */
typedef enum {
    MOR_T_TERMS,
    MOR_T_STATEMENT,
    MOR_T_PAYMENT,    /* Money layer stand-in */
    MOR_T_SETTLEMENT, /* Money layer stand-in */
    MOR_T_REPORT,     /* shared language stand-in */
    MOR_T_ENVELOPE,   /* core stand-in */
    MOR_T_PLACE,      /* Order layer stand-in: one place's answer */
    MOR_T_CLOCK,      /* clock stand-in: one clock's answer */
    MOR_T_PIECE       /* fixed bytes named by fingerprint (a body's words) */
} mor_type;

typedef enum { MOR_REF_NONE, MOR_REF_SIGNER, MOR_REF_ROLE, MOR_REF_AGREEMENT } mor_ref_kind;

typedef struct {
    mor_ref_kind kind;
    const char *name; /* signer identifier, or role name */
    mor_fp root;      /* agreement root */
} mor_ref;

typedef struct {
    bool present;
    bool bad; /* not named by fingerprint (G2.1) */
    mor_fp cmip;
    const char *params; /* "" where none */
} mor_rule;

typedef enum { MOR_ROLE_DONE, MOR_ROLE_RECEIVED, MOR_ROLE_HOLDING, MOR_ROLE_FITS, MOR_ROLE_FUTURE, MOR_ROLE_BAD } mor_role_kind;

typedef struct {
    mor_ref ref;
    mor_role_kind role_kind;
    bool has_what;
    bool what_bad;
    mor_fp what;
    mor_rule k12;
} mor_party;

typedef struct {
    const char *kind; /* assent, counter_signature, contest, replaces, resolves, or a K11 fingerprint in hex */
    mor_rule rule;
} mor_power;

typedef enum { MOR_FROM_DOER, MOR_FROM_RECEIVING, MOR_FROM_BOTH } mor_from;

typedef struct {
    const char *task; /* "K8", ... */
    mor_from from;
    int unread; /* tokens after "from whom": never read (G11.6) */
} mor_call;

typedef struct {
    uint64_t n;
    const char *unit;
} mor_amount;

typedef enum { MOR_L_RECEIVED, MOR_L_PASSED, MOR_L_OWN, MOR_L_KEPT, MOR_L_OWED } mor_line_kind;

typedef struct {
    mor_line_kind kind;
    mor_amount amount;
    mor_ref to;      /* passed, owed */
    bool has_via;    /* received, passed */
    mor_fp via;      /* the settlement it came or went by */
} mor_rline;

typedef enum { MOR_SIDE_DOER, MOR_SIDE_RECEIVING } mor_side;

/* marks a fork leaves on an act (G12) */
enum {
    MOR_M_TIP = 1,       /* a tip of a fork that stands */
    MOR_M_BUILT = 2,     /* built on a tip of a fork that stands: does not count (G12.2) */
    MOR_M_SET_ASIDE = 4, /* a tip set aside, or what its own signers built on it (G12.3) */
    MOR_M_DISPUTED = 8,  /* stands, shown as disputed (G12.5) */
    MOR_M_RESOLVER = 16, /* a resolving update that resolved a fork */
    MOR_M_WITHIN = 32    /* with BUILT or SET_ASIDE: only by being made within Terms of the branch; such an act still
                            counts toward whether those Terms were signed, which is what makes them a tip at all */
};

/* what a Terms act is in its agreement's chain */
typedef enum {
    MOR_C_UNREAD,       /* not Terms, or its agreement could not be read */
    MOR_C_VERSION,      /* a version: the root in force, or an update that replaced */
    MOR_C_ROOT_WAITING, /* the root, not shown in force */
    MOR_C_NOT_SHOWN,    /* not shown as a replacement (G4.8) */
    MOR_C_NOT_VALID,    /* the update condition answers not valid (G4.11) */
    MOR_C_UNCHECKED,    /* G9 */
    MOR_C_TIP,          /* a tip of a fork of Terms that stands (G4.7) */
    MOR_C_SET_ASIDE,    /* a tip set aside, or a version following one (G12.3) */
    MOR_C_RESOLVER,     /* a resolving update that resolved */
    MOR_C_RESOLVES_NOTHING,
    MOR_C_UNREACHED,    /* replaces Terms that are not shown as a version */
    MOR_C_STOPPED       /* built on a tip of a fork that stands (G12.2) */
} mor_chain;

typedef struct mor_act mor_act;

typedef struct {
    const char *invalid;   /* why the act is invalid, with the grammar rule; NULL if not */
    const char *unchecked; /* an act it names is not held (G9); NULL if none */
    unsigned line;         /* MOR_M_* from a fork in a signer's own line */
    unsigned rest;         /* MOR_M_* from a fork of Terms */
    mor_chain chain;
    const char *chain_why;
    bool cond_asked; /* an update condition was asked about this proposed update */
    mor_tri cond;    /* MOR_YES valid, MOR_NO not valid, MOR_CANNOT unchecked */
    const mor_act *cond_terms; /* the version whose condition was asked */
    const char *cond_ground;
    mor_qual cond_qual;
    bool by_power;   /* replaced under a power for replacement (G5.2) */
    const char *signed_how;
    bool cancelled;  /* an assent cancelled by a counter signature (G4.10) */
    /* internal scratch */
    bool mask, tmp_d, tmp_b, power_resolve_prev;
    unsigned rest_prev;
    const mor_act *pos_place;
    int pos;
} mor_eval;

struct mor_act {
    mor_type type;
    mor_fp fp;
    const char *signer;
    int index;   /* order in which the set was loaded; never read as an order of acts */
    int unread;  /* fields this text does not give: not part of the act as read (G2.2) */

    /* Terms and Statement */
    bool has_previous;
    mor_fp previous;
    bool has_agreement;
    mor_fp agreement;
    mor_fp root; /* computed: the agreement the act is within */
    mor_fp *relies_on;
    int n_relies_on;
    int n_relies_bad; /* named some other way: not part (G2.2) */
    mor_fp *resolves;
    int n_resolves;
    bool has_continues;
    mor_fp continues;

    /* Terms */
    bool has_replaces;
    mor_fp replaces;
    mor_party *parties;
    int n_parties;
    mor_ref *authors;
    int n_authors;
    bool has_body, body_bad;
    mor_fp body_words, body_cmip;
    mor_rule in_force;
    mor_power *powers;
    int n_powers;
    const char *place;
    const char *clock;
    mor_ref receiver;
    bool has_envelope;
    mor_fp envelope;
    mor_rule condition;
    mor_call *calls;
    int n_calls;

    /* Statement */
    bool has_terms;
    mor_fp terms;
    const char *kind;
    bool kind_is_fp;
    mor_fp kind_fp;
    const char *content;
    mor_ref for_;
    bool has_under;
    mor_fp under;
    mor_fp *answers;
    int n_answers;
    bool has_settlement;
    mor_fp settlement;

    /* payment: agreement (the one paid), version, settlement, amount */
    bool has_version;
    mor_fp version;
    bool has_amount;
    mor_amount amount;

    /* report: call, side, lines; relied acts are in relies_on */
    mor_fp call;
    mor_side side;
    mor_rline *lines;
    int n_lines;

    /* envelope */
    const char *baker;

    /* place: entries in order; clock: now */
    mor_fp *entries;
    int n_entries;
    uint64_t now;

    /* piece */
    const char **data;
    int n_data;

    mor_eval ev;
};

/* ---- plug-ins: the tasks K1 to K13 (section 5) ------------------------- */
typedef struct mor_set mor_set;

typedef struct {
    mor_ref party;
    uint64_t share;
} mor_share;

typedef enum { MOR_END_DEFAULT, MOR_END_AGENT_ONLY, MOR_END_OPAQUE } mor_end_mode;

typedef struct {
    mor_tri read; /* MOR_CANNOT: the words could not be read */
    mor_share shares[MOR_MAX_LIST];
    int n_shares;
    bool delegation;
    mor_ref delegator, agent;
    mor_end_mode end_mode;
    const char *end_words; /* the condition as the Terms state it */
} mor_k1_out;

typedef enum { MOR_K4_IN_FORCE, MOR_K4_POWER, MOR_K4_REPLACE, MOR_K4_RESOLVE, MOR_K4_CONDITION } mor_k4_q;

typedef struct {
    mor_k4_q q;
    const mor_rule *rule;
    const mor_act *terms;   /* the Terms that name the rule; for the condition, the version in force (or earned under) */
    const mor_act *subject; /* the act asked about: the Terms to put in force, the act made under the power, the proposed update */
    mor_set *set;
    bool has_time; /* an answer of the clock the agreement names, or of the anchoring named */
    uint64_t time;
    mor_qual time_qual;
} mor_k4_in;

typedef struct {
    mor_tri ans; /* yes / no; for the condition valid / not valid */
    const char *ground;
    const mor_act *ground_act; /* an act the ground rests on: reported by who signed it (G8) */
    bool ground_is_time;
} mor_k4_out;

typedef struct {
    const mor_act *terms; /* G3.2: a K10 cMIP MUST accept authors: they are in terms->authors */
    const mor_ref *authors;
    int n_authors;
    const mor_act *const *dispute;
    int n_dispute;
} mor_k10_in;

typedef struct {
    mor_tri ans;
    const mor_act *decision; /* the judge's signed decision */
    int authors_seen;
} mor_k10_out;

typedef struct {
    mor_set *set;
    const mor_act *terms; /* the version the role is read from */
    const mor_party *role;
    bool has_time;
    uint64_t time;
    mor_qual time_qual;
} mor_k12_in;

typedef struct {
    const char *signer;
    const mor_act *proof[4];
    int n_proof;
    const mor_act *earned_under; /* the version the proof names (G10.6) */
} mor_k12_filling;

typedef struct {
    mor_tri ans; /* MOR_CANNOT: could not be applied */
    mor_k12_filling fill[MOR_MAX_LIST];
    int n;
    bool many;
    mor_tri takes_new;
    const char *cause;
    bool cause_is_time;
} mor_k12_out;

typedef struct {
    const char *signer;
    int forks_standing, forks_resolved, contests_made, assents, spots_filled;
} mor_k13_out;

typedef struct mor_cmip {
    const char *name; /* a label for people; the cMIP is named by its fingerprint */
    int task;         /* 1..13 */
    mor_fp fp;
    bool absent;      /* true: the verifier does not hold this cMIP */
    void (*k1)(mor_set *, const mor_act *words, mor_k1_out *);
    void (*k4)(const mor_k4_in *, mor_k4_out *);
    void (*k10)(const mor_k10_in *, mor_k10_out *);
    const char *k11_meaning;
    void (*k12)(const mor_k12_in *, mor_k12_out *);
    void (*k13)(mor_set *, const char *signer, mor_k13_out *);
} mor_cmip;

typedef struct {
    mor_cmip *cmips;
    int n;
} mor_plugins;

/* the stub cMIPs shipped with this library; the caller owns the array and may set .absent */
int mor_stub_count(void);
void mor_stub_fill(mor_cmip *out); /* out has room for mor_stub_count() */
mor_fp mor_stub_fp(const char *name);

/* helpers a plug-in may call while it is being asked */
mor_tri mor_plug_assent_in_name(mor_set *, const mor_act *terms, const mor_ref *name);
bool mor_plug_counts(mor_set *, const mor_act *act); /* counts, and is shown by the place where one applies */
int mor_plug_position(mor_set *, const mor_act *act); /* position in the place in use, or -1 */
const mor_cmip *mor_cmip_find(const mor_set *, const mor_fp *fp, int task);

/* ---- the set ------------------------------------------------------------ */
typedef struct {
    const mor_plugins *plugins;  /* NULL: no plug-in is present */
    const char *reader;          /* the signer on whose behalf the verifier reads, or NULL (G11.4(d)) */
    const char *anchoring_place; /* an anchoring the reader accepts where an agreement names no place (G13.4) */
    const char *anchoring_clock; /* likewise for time */
    bool standin_skip_signature; /* for fuzzing the grammar only */
} mor_options;

typedef struct {
    size_t offset;
    const char *reason;
} mor_reject;

mor_set *mor_set_new(const mor_options *opt);
void mor_set_free(mor_set *);
/* decode untrusted bytes; returns the number of records taken, or -1 if the input is over the limit */
int mor_set_load(mor_set *, const unsigned char *bytes, size_t len);
/* apply the grammar rules; call once after loading, before any question */
void mor_evaluate(mor_set *);
/* true if forks and powers leaned on one another so that the rounds of reading did not settle:
 * every answer about the acts concerned is then to be taken as unchecked */
bool mor_unsettled(const mor_set *);

int mor_n_acts(const mor_set *);
const mor_act *mor_act_at(const mor_set *, int i);
const mor_act *mor_find(const mor_set *, const mor_fp *fp);
int mor_n_rejects(const mor_set *);
const mor_reject *mor_reject_at(const mor_set *, int i);
void *mor_alloc(mor_set *, size_t n);
const char *mor_sprintf(mor_set *, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/* ---- questions ------------------------------------------------------------ */
typedef struct mor_agreement {
    mor_fp root;
    const mor_act *root_act; /* NULL: not held */
    mor_ans in_force;        /* shown: cur is in force; not shown; unchecked; fork: cur is the last shared version */
    const mor_act *cur;
    mor_qual qual;
    const char *why;
    const mor_act *versions[MOR_MAX_CHAIN];
    int n_versions;
    const mor_act *tips[MOR_MAX_LIST];
    int n_tips;
    int state;
} mor_agreement;

int mor_n_agreements(const mor_set *);
const mor_agreement *mor_agreement_at(const mor_set *, int i);
const mor_agreement *mor_agreement_find(const mor_set *, const mor_fp *root);
bool mor_counts(const mor_act *a);

/* G4: are these Terms in force? */
void mor_terms_in_force(mor_set *, const mor_act *terms, mor_answer *out);
/* G5: is this signer bound by these Terms / by this act? */
void mor_bound_by_terms(mor_set *, const char *signer, const mor_act *terms, mor_answer *out);
void mor_bound_by_act(mor_set *, const char *signer, const mor_act *act, mor_answer *out);
/* G1.2: is this act the signature of the agreement its `for` names? */
void mor_agreement_signature(mor_set *, const mor_act *act, mor_answer *out);

/* G6, G12: forks in signers' own lines */
typedef struct {
    mor_fp root;
    const char *signer;
    const mor_act *tips[MOR_MAX_LIST];
    int n_tips;
    bool resolved;
    const mor_act *continued;
    const mor_act *resolver;
    const char *resolver_power; /* the power it was signed under and whom it rests on (G12.4), or NULL */
} mor_line_fork;
int mor_n_line_forks(const mor_set *);
const mor_line_fork *mor_line_fork_at(const mor_set *, int i);

/* G10: a spot */
typedef struct {
    const mor_act *version;
    const char *rests_on; /* the holder's own assent, or the condition named and its answer */
    mor_ans ans;          /* shown; valid (with the condition); unchecked */
} mor_rights_change;

typedef struct {
    const char *signer;
    const mor_act *proof[4];
    int n_proof;
    const mor_act *earned_under;
    bool disputed;
    mor_rights_change *changes; /* NULL while n_changes is 0 */
    int n_changes;
} mor_filling;

typedef struct {
    mor_answer ans; /* shown with fillings; not shown: not filled; unchecked */
    const mor_act *terms;
    const mor_party *role;
    mor_filling *fill;
    int n;
    bool many;
    mor_answer takes_new; /* shown / unchecked; the yes or no is in takes */
    bool takes;
    mor_answer work; /* G14.3, where the Terms name an envelope; else ans MOR_SHOWN and why NULL */
    bool has_work;
} mor_spot;
/* reads the role from the Terms in force of the agreement (G10.2) */
void mor_spot_read(mor_set *, const mor_fp *root, const char *role, mor_spot *out);
void mor_spot_read_version(mor_set *, const mor_act *terms, const char *role, mor_spot *out);

/* G1.4: receiver for an act */
typedef struct {
    mor_answer ans;
    const char *who;
    const mor_act *read[MOR_MAX_DEPTH + 1]; /* the versions read on the way */
    int n_read;
    mor_answer work;
    bool has_work;
} mor_receiver;
/* version NULL: the act names no version, so the Terms in force are read */
void mor_receiver_for(mor_set *, const mor_fp *root, const mor_act *version, mor_receiver *out);

/* G11: legs */
typedef struct {
    mor_fp settlement;
    mor_ans status; /* matched, mismatched, one side only, unchecked */
    const mor_act *doer_report;
    const mor_rline *doer_line;
    const mor_act *recv_report;
    const mor_rline *recv_line;
    const char *receiving_side; /* as read by G1.4, or NULL */
    const char *why;
} mor_tie;

typedef struct {
    const mor_act *terms;
    mor_tri holds; /* do these Terms hold the Control task (G11.2) */
    const char *holds_why;
    bool has_call;
    mor_from from;
    mor_ans call_status; /* no report called for; unanswered; not shown; shown (reports held) */
    const char *doer;    /* the controller, where shown */
    const mor_act *reports[MOR_MAX_LIST];
    int n_reports;
    mor_tie ties[MOR_MAX_LIST];
    int n_ties;
    mor_ans conforms;    /* whether what was passed on is what the Terms and cMIPs require: unchecked without them (G11.3) */
    mor_ans path_beyond; /* whether the signers agreed to a path beyond the leg (G11.6): never shown by this text */
    mor_answer in_force; /* G11.4(e): per place or as shown */
    mor_answer work;
    bool has_work;
} mor_k8;
void mor_k8_read(mor_set *, const mor_act *terms, mor_k8 *out);

typedef struct {
    const mor_act *payment;
    mor_receiver receiver; /* the receiving side of the buyer's leg (G11.7) */
    mor_ans status;        /* matched, mismatched, one side only; or not shown / unchecked */
    const mor_act *confirmation;
    const mor_rline *confirmation_line;
    const mor_act *settlement; /* NULL: not held */
    bool disputed, fork;
    const char *why;
} mor_payment;
void mor_payment_read(mor_set *, const mor_act *payment, mor_payment *out);

typedef struct {
    int depth;
    const char *doer;
    const char *receiving; /* NULL where not shown */
    mor_ref to;
    bool has_settlement;
    mor_fp settlement;
    mor_ans status;
    const mor_act *under; /* the payment, or the Terms that hold the leg's task */
    const mor_act *doer_report, *recv_report;
    const char *note;
} mor_leg;

typedef struct {
    mor_leg legs[MOR_MAX_LEGS];
    int n;
} mor_trace;
/* G11.4: a result that depends on a sequence, read leg by leg from the reports held */
void mor_trace_payment(mor_set *, const mor_act *payment, mor_trace *out);

/* G11.4(e): before paying, one agreement at a time */
typedef struct {
    mor_fp root;
    mor_k8 k8;
    mor_ans terms; /* shown / fork / not shown / unchecked */
} mor_before_entry;
typedef struct {
    mor_before_entry e[MOR_MAX_DEPTH + 1];
    int n;
} mor_before;
void mor_before_paying(mor_set *, const mor_fp *root, mor_before *out);

/* G11.8: a contest */
typedef struct {
    mor_answer ans;     /* shown: a contest made against that version; else why not */
    const char *said_by; /* its signer: a contest is its signer's word (G8.1) */
    const char *reaches[MOR_MAX_LIST]; /* the signers of what it is made against, one level, and no one else */
    int n_reaches;
    bool names_role;     /* the Terms name a party by role: an offer as section 2 reads it */
    mor_ans buyers;      /* shown: a buyer's contest; not shown; unchecked */
    const char *buyers_why;
} mor_contest;
void mor_contest_read(mor_set *, const mor_act *statement, mor_contest *out);

/* G13.3: order from citing */
typedef enum { MOR_ORDER_NONE, MOR_ORDER_A_FIRST, MOR_ORDER_B_FIRST } mor_order;
mor_order mor_order_by_citing(mor_set *, const mor_act *a, const mor_act *b);
/* G13.2: did act a rely on b (does it name it)? */
bool mor_names(const mor_act *a, const mor_fp *b);

/* G14: do these Terms count as an agreement for the baked work they name? */
void mor_work_read(mor_set *, const mor_act *terms, mor_answer *out, const char **baker);

/* K13, through the stub: what one signer's acts show */
mor_tri mor_standing(mor_set *, const char *signer, mor_k13_out *out);
/* K10, through the stub cMIP given: the interface carries the authors (G3.2) */
mor_tri mor_judge(mor_set *, const mor_fp *k10_cmip, const mor_act *terms, mor_k10_out *out);
/* G7.4, G8.2: what time it was for the agreement of these Terms */
mor_tri mor_time_for(mor_set *, const mor_act *terms, uint64_t *t, mor_qual *q);

/* ---- showing --------------------------------------------------------------- */
void mor_show_all(mor_set *, FILE *out);
void mor_show_trace(mor_set *, const mor_act *payment, FILE *out);
void mor_show_before(mor_set *, const mor_fp *root, FILE *out);

#endif
