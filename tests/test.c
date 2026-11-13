/* test.c - test cases written from the text alone. Every test cites the sentence it tests;
 * tests/must_coverage.py checks that every sentence with MUST or MUST NOT has one. */
#include "kit.h"

#include <stdio.h>

static int g_fail, g_checks;
#define CHECK(c)                                                                                                              \
    do {                                                                                                                      \
        g_checks++;                                                                                                           \
        if (!(c)) {                                                                                                           \
            g_fail++;                                                                                                         \
            printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);                                                           \
        }                                                                                                                     \
    } while (0)

static bool has(const char *s, const char *sub)
{
    return s && strstr(s, sub) != NULL;
}

static bool eq(const char *a, const char *b)
{
    return a && b && strcmp(a, b) == 0;
}

static mor_cmip g_stubs[32];
static mor_plugins g_plug;
static mor_options g_opt;
static mor_set *g_sets[32];
static int g_nsets;
static kit K;
static char g_text[1 << 18];

static void begin(void)
{
    mor_stub_fill(g_stubs);
    g_plug.cmips = g_stubs;
    g_plug.n = mor_stub_count();
    memset(&g_opt, 0, sizeof g_opt);
    g_opt.plugins = &g_plug;
    K.n = 0;
}

static void without(const char *name)
{
    for (int i = 0; i < g_plug.n; i++)
        if (strcmp(g_stubs[i].name, name) == 0)
            g_stubs[i].absent = true;
}

static mor_set *run(void)
{
    mor_set *s = mor_set_new(&g_opt);
    mor_set_load(s, (const unsigned char *)K.buf, K.n);
    mor_evaluate(s);
    CHECK(!mor_unsettled(s));
    if (g_nsets >= 32)
        abort();
    g_sets[g_nsets++] = s;
    return s;
}

static const mor_act *A(mor_set *s, mor_fp fp)
{
    const mor_act *a = mor_find(s, &fp);
    if (!a) {
        printf("    act not in set\n");
        abort();
    }
    return a;
}

static const mor_agreement *AG(mor_set *s, mor_fp root)
{
    const mor_agreement *a = mor_agreement_find(s, &root);
    if (!a)
        abort();
    return a;
}

static const char *show(mor_set *s)
{
    FILE *f = tmpfile();
    if (!f)
        abort();
    mor_show_all(s, f);
    for (int i = 0; i < mor_n_acts(s); i++)
        if (mor_act_at(s, i)->type == MOR_T_TERMS && !mor_act_at(s, i)->has_replaces)
            mor_show_before(s, &mor_act_at(s, i)->fp, f);
    long n = ftell(f);
    rewind(f);
    if (n < 0 || (size_t)n >= sizeof g_text)
        n = (long)sizeof g_text - 1;
    size_t got = fread(g_text, 1, (size_t)n, f);
    g_text[got] = 0;
    fclose(f);
    return g_text;
}

#define P2 "party signer alice|party signer bob|author signer alice"

/* alice states Terms for alice and bob; both assent. Returns the root. */
static mor_fp two(const char *extra, const char *words, mor_fp *alast, mor_fp *blast)
{
    mor_fp t = terms(&K, "alice", "none", F("%s%s%s", P2, extra && *extra ? "|" : "", extra ? extra : ""), words);
    mor_fp a = assent(&K, "alice", t, t, hx(t));
    mor_fp b = assent(&K, "bob", t, t, "none");
    if (alast)
        *alast = a;
    if (blast)
        *blast = b;
    return t;
}

/* ============================ section 3 and the decoder ============================ */

static void t_required_fields(void)
{
    rec(&K, "terms", "alice", "party signer alice|author signer alice"); /* no previous, no body */
    rec(&K, "statement", "alice", F("agreement %s|kind assent|previous none", stub("x")));
    rec(&K, "report", "sam", "side doer"); /* names no call */
    mor_set *s = run();
    CHECK(mor_n_acts(s) == 0);
    CHECK(mor_n_rejects(s) == 3);
    CHECK(has(mor_reject_at(s, 2)->reason, "names no call") != false);
}

static void t_signature_standin(void)
{
    mor_fp t = terms(&K, "alice", "none", "party signer alice|author signer alice", NULL);
    (void)t;
    size_t n = K.n;
    K.buf[n - 3] = K.buf[n - 3] == '0' ? '1' : '0'; /* damage the last sig */
    mor_set *s = run();
    CHECK(mor_n_acts(s) == 1); /* the piece only */
    CHECK(mor_n_rejects(s) == 1);
}

/* ============================ G1 ============================ */

static void t_g1_2_one_level(void)
{
    /* y: dave's agreement. x names carol as signer and y as a party. w asks x to sign. */
    mor_fp y = terms(&K, "dave", "none", "party signer dave|author signer dave", NULL);
    assent(&K, "dave", y, y, hx(y));
    mor_fp x = terms(&K, "carol", "none", F("party signer carol|party agreement %s|author signer carol", hx(y)), NULL);
    assent(&K, "carol", x, x, hx(x));
    mor_fp w = terms(&K, "alice", "none",
                     F("party signer alice|party agreement %s|author signer alice|in_force %s 2 signer alice agreement %s", hx(x),
                       stub("k4.threshold"), hx(x)),
                     NULL);
    assent(&K, "alice", w, w, hx(w));
    mor_set *s = run();
    CHECK(AG(s, w)->in_force == MOR_NOT_SHOWN); /* x has not signed */
    /* x signs by every one of its signers: carol alone. Nobody inside y signs anything. */
    stmt(&K, "carol", w, w, "assent", "none", F("for agreement %s|under %s", hx(x), hx(x)));
    s = run();
    CHECK(AG(s, w)->in_force == MOR_SHOWN);
}

static void t_g1_2_unchecked_without_its_terms(void)
{
    mor_fp ghost = mor_stub_fp("an agreement whose acts are not held");
    mor_fp w = terms(&K, "alice", "none",
                     F("party signer alice|party agreement %s|author signer alice|in_force %s 2 signer alice agreement %s", hx(ghost),
                       stub("k4.threshold"), hx(ghost)),
                     NULL);
    assent(&K, "alice", w, w, hx(w));
    mor_set *s = run();
    CHECK(AG(s, w)->in_force == MOR_UNCHECKED);
}

static void t_g1_2_agreement_signature(void)
{
    /* x gives carol a power to assent in its name */
    mor_fp x = terms(&K, "carol", "none",
                     F("party signer carol|party signer dave|author signer carol|power assent %s carol", stub("k4.delegate")), NULL);
    assent(&K, "carol", x, x, hx(x));
    assent(&K, "dave", x, x, "none");
    mor_fp w = terms(&K, "alice", "none",
                     F("party signer alice|party agreement %s|author signer alice|in_force %s 2 signer alice agreement %s", hx(x),
                       stub("k4.threshold"), hx(x)),
                     NULL);
    assent(&K, "alice", w, w, hx(w));
    mor_fp sg = stmt(&K, "carol", w, w, "assent", "none", F("for agreement %s|under %s", hx(x), hx(x)));
    /* erin has no power */
    mor_fp bad = stmt(&K, "erin", w, w, "assent", "none", F("for agreement %s|under %s", hx(x), hx(x)));
    mor_set *s = run();
    mor_answer a;
    mor_agreement_signature(s, A(s, sg), &a);
    CHECK(a.ans == MOR_SHOWN);
    mor_agreement_signature(s, A(s, bad), &a);
    CHECK(a.ans == MOR_NOT_SHOWN);
    CHECK(AG(s, w)->in_force == MOR_SHOWN);
    /* it binds every party bound by x's Terms, and no one else */
    mor_bound_by_act(s, "dave", A(s, sg), &a);
    CHECK(a.ans == MOR_SHOWN);
    mor_bound_by_act(s, "erin", A(s, sg), &a);
    CHECK(a.ans == MOR_NOT_SHOWN);
    mor_bound_by_terms(s, "dave", A(s, w), &a);
    CHECK(a.ans == MOR_SHOWN);
}

static void t_g1_4_no_version(void)
{
    mor_fp t = two("receiver signer bob", NULL, NULL, NULL);
    mor_set *s = run();
    mor_receiver r;
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.ans.ans == MOR_SHOWN && eq(r.who, "bob"));
    CHECK(r.ans.qual.kind == MOR_Q_AS_SHOWN);
}

static void t_g1_4_per_place(void)
{
    mor_fp t = terms(&K, "alice", "none", "party signer alice|author signer alice|receiver signer alice|place plc1", NULL);
    mor_fp a = assent(&K, "alice", t, t, hx(t));
    rec(&K, "place", "plc1", F("entry %s|entry %s", hx(t), hx(a)));
    mor_set *s = run();
    mor_receiver r;
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.ans.ans == MOR_SHOWN);
    CHECK(r.ans.qual.kind == MOR_Q_PER_PLACE && eq(r.ans.qual.name, "plc1"));
}

static mor_fp g_tip1, g_tip2;
/* alice and bob; two replacing Terms, each signed by both: a fork of Terms */
static mor_fp fork_of_terms(const char *extra_root)
{
    mor_fp al, bl;
    mor_fp t = two(extra_root, NULL, &al, &bl);
    g_tip1 = terms(&K, "alice", hx(al), F("replaces %s|agreement %s|%s|receiver signer alice", hx(t), hx(t), P2), NULL);
    mor_fp a1 = assent(&K, "alice", t, g_tip1, hx(g_tip1));
    mor_fp b1 = assent(&K, "bob", t, g_tip1, hx(bl));
    g_tip2 = terms(&K, "bob", hx(b1), F("replaces %s|agreement %s|%s|receiver signer bob", hx(t), hx(t), P2), NULL);
    assent(&K, "alice", t, g_tip2, hx(a1));
    assent(&K, "bob", t, g_tip2, hx(g_tip2));
    return t;
}

static void t_g1_4_fork(void)
{
    mor_fp t = fork_of_terms("receiver signer bob");
    mor_set *s = run();
    mor_receiver r;
    mor_receiver_for(s, &t, NULL, &r);
    /* the receiver is the one named by the Terms both branches replace; the fork is shown with it */
    CHECK(r.ans.ans == MOR_SHOWN && eq(r.who, "bob") && r.read[0] == A(s, t));
    CHECK(r.ans.fork);
}

static void t_g1_4_stops(void)
{
    /* no receiver: not shown. An agreement met twice: not shown. Acts lacking: unchecked. */
    mor_fp none = two(NULL, NULL, NULL, NULL);
    mor_fp ghost = mor_stub_fp("not held");
    mor_fp lack = terms(&K, "carol", "none", F("party signer carol|author signer carol|receiver agreement %s", hx(ghost)), NULL);
    assent(&K, "carol", lack, lack, hx(lack));
    mor_fp self = terms(&K, "dave", "none", F("party signer dave|author signer dave|receiver agreement %s", hx(lack)), NULL);
    assent(&K, "dave", self, self, hx(self));
    mor_set *s = run();
    mor_receiver r;
    mor_receiver_for(s, &none, NULL, &r);
    CHECK(r.ans.ans == MOR_NOT_SHOWN);
    mor_receiver_for(s, &lack, NULL, &r);
    CHECK(r.ans.ans == MOR_UNCHECKED);
    mor_receiver_for(s, &self, NULL, &r);
    CHECK(r.ans.ans == MOR_UNCHECKED);
}

static void t_g1_4_through_an_agreement(void)
{
    mor_fp d = terms(&K, "sam", "none", "party signer sam|author signer sam|receiver signer sam", NULL);
    assent(&K, "sam", d, d, hx(d));
    mor_fp w = terms(&K, "alice", "none", F("party signer alice|author signer alice|receiver agreement %s", hx(d)), NULL);
    assent(&K, "alice", w, w, hx(w));
    mor_set *s = run();
    mor_receiver r;
    mor_receiver_for(s, &w, NULL, &r);
    CHECK(r.ans.ans == MOR_SHOWN && eq(r.who, "sam") && r.n_read == 2);
}

/* ============================ G2, G3 ============================ */

static void t_g2_1(void)
{
    mor_fp bad = terms(&K, "alice", "none", "party signer alice|author signer alice|body the-words-we-agreed plain-text", NULL);
    mor_fp bad2 = terms(&K, "bob", "none", "party signer bob|author signer bob|in_force the-usual-rule", NULL);
    mor_set *s = run();
    CHECK(A(s, bad)->ev.invalid != NULL);
    CHECK(A(s, bad2)->ev.invalid != NULL);
    CHECK(AG(s, bad)->in_force == MOR_INVALID);
}

static void t_g2_2(void)
{
    mor_fp t = terms(&K, "alice", "none", "party signer alice|author signer alice|annex see-the-side-letter|relies_on our-usual-conditions", NULL);
    assent(&K, "alice", t, t, hx(t));
    mor_set *s = run();
    const mor_act *a = A(s, t);
    CHECK(a->ev.invalid == NULL);
    CHECK(a->unread == 1 && a->n_relies_bad == 1 && a->n_relies_on == 0); /* not part of those Terms */
    CHECK(AG(s, t)->in_force == MOR_SHOWN);
    CHECK(has(show(s), "see-the-side-letter") == false && has(g_text, "our-usual-conditions") == false);
}

static void t_g3_1(void)
{
    mor_fp bad = terms(&K, "alice", "none", "party signer alice|author signer mallory", NULL);
    mor_fp ok = terms(&K, "bob", "none", "party signer bob|author signer bob", NULL);
    mor_set *s = run();
    CHECK(A(s, bad)->ev.invalid && has(A(s, bad)->ev.invalid, "G3.1"));
    CHECK(A(s, ok)->ev.invalid == NULL);
}

static void t_g3_2(void)
{
    mor_fp t = terms(&K, "alice", "none", "party signer alice|party signer bob|author signer alice|author signer bob", NULL);
    mor_set *s = run();
    mor_k10_out o;
    mor_fp k10 = mor_stub_fp("k10.judge");
    CHECK(mor_judge(s, &k10, A(s, t), &o) == MOR_YES);
    CHECK(o.authors_seen == 2);
    without("k10.judge");
    CHECK(mor_judge(s, &k10, A(s, t), &o) == MOR_CANNOT);
}

/* ============================ G4 ============================ */

static void t_g4_1_default(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    assent(&K, "alice", t, t, hx(t));
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_NOT_SHOWN);
    assent(&K, "bob", t, t, "none");
    s = run();
    CHECK(AG(s, t)->in_force == MOR_SHOWN);
    mor_answer a;
    mor_bound_by_terms(s, "bob", A(s, t), &a);
    CHECK(a.ans == MOR_SHOWN);
    mor_bound_by_terms(s, "carol", A(s, t), &a);
    CHECK(a.ans == MOR_NOT_SHOWN);
}

static void t_g4_2(void)
{
    mor_fp bad = terms(&K, "alice", "none",
                       F("party signer alice|party role buyer received %s %s|author signer alice", stub("thing"), stub("k12.claim")), NULL);
    mor_fp ok = terms(&K, "alice", "none",
                      F("party signer alice|party role buyer received %s %s|author signer alice|in_force %s 1 signer alice", stub("thing"),
                        stub("k12.claim"), stub("k4.threshold")),
                      NULL);
    mor_set *s = run();
    CHECK(A(s, bad)->ev.invalid && has(A(s, bad)->ev.invalid, "G4.2"));
    CHECK(A(s, ok)->ev.invalid == NULL);
}

static void t_g4_3(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a1 = assent(&K, "alice", t, t, hx(t));
    mor_set *s = run();
    mor_answer a;
    mor_bound_by_act(s, "alice", A(s, a1), &a);
    CHECK(a.ans == MOR_NOT_SHOWN); /* not in force: the assent binds no one yet */
}

static void t_g4_6_qualifiers(void)
{
    mor_fp t = two(NULL, NULL, NULL, NULL);
    mor_set *s = run();
    mor_answer a;
    mor_terms_in_force(s, A(s, t), &a);
    CHECK(a.ans == MOR_SHOWN && a.qual.kind == MOR_Q_AS_SHOWN);
    /* the reader accepts an anchoring: named */
    mor_fp al = {{0}};
    (void)al;
    rec(&K, "place", "myplace", F("entry %s", hx(t)));
    g_opt.anchoring_place = "myplace";
    s = run();
    mor_terms_in_force(s, A(s, t), &a);
    CHECK(a.qual.kind == MOR_Q_PER_ANCHORING && eq(a.qual.name, "myplace"));
    CHECK(a.ans == MOR_NOT_SHOWN); /* that anchoring does not show the assents */
    /* every answer about which Terms are in force says what it stands on */
    for (int i = 0; i < mor_n_agreements(s); i++)
        CHECK(mor_agreement_at(s, i)->qual.kind != MOR_Q_NONE);
}

static void t_g4_7_fork(void)
{
    mor_fp t = fork_of_terms(NULL);
    mor_set *s = run();
    const mor_agreement *ag = AG(s, t);
    CHECK(ag->in_force == MOR_FORK);
    CHECK(ag->n_tips == 2);
    CHECK(ag->cur == A(s, t)); /* the last shared version stays in force */
    mor_answer a;
    mor_terms_in_force(s, A(s, g_tip1), &a);
    CHECK(a.ans == MOR_FORK);
    mor_terms_in_force(s, A(s, g_tip2), &a);
    CHECK(a.ans == MOR_FORK);
    mor_terms_in_force(s, A(s, t), &a);
    CHECK(a.ans == MOR_SHOWN && a.fork);
    /* a place that gives positions does not choose either */
    K.n = 0;
    t = fork_of_terms("place plc1");
    char list[8192] = "";
    mor_set *tmp = run();
    for (int i = 0; i < mor_n_acts(tmp); i++)
        snprintf(list + strlen(list), sizeof list - strlen(list), "%sentry %s", i ? "|" : "", hx(mor_act_at(tmp, i)->fp));
    rec(&K, "place", "plc1", list);
    s = run();
    CHECK(AG(s, t)->in_force == MOR_FORK && AG(s, t)->qual.kind == MOR_Q_PER_PLACE);
    CHECK(A(s, g_tip1)->ev.chain == MOR_C_TIP && A(s, g_tip2)->ev.chain == MOR_C_TIP);
}

static void t_g4_7_answers_from_shared(void)
{
    /* the shared version has a spot and a receiver; each tip changes them */
    mor_fp t = terms(&K, "alice", "none",
                     F("party signer alice|party role fan done %s %s many|author signer alice|in_force %s 1 signer alice|receiver signer alice|"
                       "power replaces %s alice",
                       stub("thing"), stub("k12.claim"), stub("k4.threshold"), stub("k4.delegate")),
                     NULL);
    mor_fp a0 = assent(&K, "alice", t, t, hx(t));
    stmt(&K, "zed", t, t, stub("k11.claim"), "none", "content fan");
    const char *rest = F("replaces %s|agreement %s|party signer alice|author signer alice|in_force %s", hx(t), hx(t), stub("k4.yes"));
    mor_fp x = terms(&K, "alice", hx(a0), rest, NULL);
    mor_fp y = terms(&K, "alice", hx(x), F("%s|receiver signer alice", rest), NULL);
    (void)y;
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_FORK);
    mor_spot sp;
    mor_spot_read(s, &t, "fan", &sp);
    CHECK(sp.ans.ans == MOR_SHOWN && sp.n == 1 && sp.terms == A(s, t));
    CHECK(sp.ans.fork); /* the fork is shown with each such answer */
    mor_receiver r;
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.ans.fork && r.read[0] == A(s, t));
    mor_k8 k;
    mor_k8_read(s, AG(s, t)->cur, &k);
    CHECK(k.in_force.fork);
    CHECK(k.in_force.qual.kind == MOR_Q_AS_SHOWN);
}

static void t_g4_8_not_shown(void)
{
    mor_fp al, bl;
    mor_fp t = two(NULL, NULL, &al, &bl);
    mor_fp t2 = terms(&K, "alice", hx(al), F("replaces %s|agreement %s|%s", hx(t), hx(t), P2), NULL);
    assent(&K, "alice", t, t2, hx(t2));
    mor_set *s = run();
    CHECK(A(s, t2)->ev.chain == MOR_C_NOT_SHOWN); /* not invalid: bob's assent may exist */
    CHECK(AG(s, t)->cur == A(s, t) && AG(s, t)->in_force == MOR_SHOWN);
    assent(&K, "bob", t, t2, hx(bl));
    s = run();
    CHECK(A(s, t2)->ev.chain == MOR_C_VERSION && AG(s, t)->cur == A(s, t2));
}

static void t_g4_9_never_ended(void)
{
    mor_fp al, bl;
    mor_fp t = two(NULL, NULL, &al, &bl);
    mor_fp t2 = terms(&K, "alice", hx(al), F("replaces %s|agreement %s|%s", hx(t), hx(t), P2), "this agreement is over");
    assent(&K, "alice", t, t2, hx(t2));
    assent(&K, "bob", t, t2, hx(bl));
    mor_set *s = run();
    mor_answer a;
    mor_terms_in_force(s, A(s, t), &a);
    CHECK(a.ans == MOR_NOT_SHOWN && has(a.why, "G4.5")); /* replaced, not ended */
    CHECK(AG(s, t)->in_force == MOR_SHOWN && AG(s, t)->cur == A(s, t2)); /* the agreement goes on as its last version */
    CHECK(has(show(s), "ended") == false);
    for (int i = 0; i <= MOR_NO_REPORT_CALLED_FOR; i++)
        CHECK(has(mor_ans_name((mor_ans)i), "end") == false);
}

static void t_g4_10_counter_signature(void)
{
    /* alice assents, then cancels before bob signs: her assent is not counted */
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a1 = assent(&K, "alice", t, t, hx(t));
    mor_fp cs = stmt(&K, "alice", t, t, "counter_signature", hx(a1), NULL);
    mor_fp b1 = stmt(&K, "bob", t, t, "assent", "none", F("relies_on %s", hx(cs))); /* bob's assent cites it: it came after */
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_NOT_SHOWN);
    CHECK(A(s, a1)->ev.cancelled);
    CHECK(!A(s, b1)->ev.cancelled); /* it cancels that assent, and no one else's */
    /* a later assent by the same signer is counted as any assent is */
    assent(&K, "alice", t, t, hx(cs));
    s = run();
    CHECK(AG(s, t)->in_force == MOR_SHOWN);
}

static void t_g4_10_after_in_force(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a1 = assent(&K, "alice", t, t, hx(t));
    mor_fp b1 = assent(&K, "bob", t, t, "none");
    /* made for Terms that are in force (it cites bob's assent): changes nothing */
    stmt(&K, "alice", t, t, "counter_signature", hx(a1), F("relies_on %s", hx(b1)));
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_SHOWN);
    CHECK(!A(s, a1)->ev.cancelled);
}

static void t_g4_10_race(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a1 = assent(&K, "alice", t, t, hx(t));
    mor_fp b1 = assent(&K, "bob", t, t, "none");
    mor_fp cs = stmt(&K, "alice", t, t, "counter_signature", hx(a1), NULL); /* no order shown against bob's assent */
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_NOT_SHOWN);
    CHECK(has(AG(s, t)->why, "which came first is not shown"));
    /* a place settles the race */
    K.n = 0;
    t = terms(&K, "alice", "none", P2 "|place plc1", NULL);
    a1 = assent(&K, "alice", t, t, hx(t));
    b1 = assent(&K, "bob", t, t, "none");
    cs = stmt(&K, "alice", t, t, "counter_signature", hx(a1), NULL);
    size_t keep = K.n;
    rec(&K, "place", "plc1", F("entry %s|entry %s|entry %s|entry %s", hx(t), hx(a1), hx(b1), hx(cs)));
    s = run();
    CHECK(AG(s, t)->in_force == MOR_SHOWN && AG(s, t)->qual.kind == MOR_Q_PER_PLACE);
    K.n = keep;
    rec(&K, "place", "plc1", F("entry %s|entry %s|entry %s|entry %s", hx(t), hx(a1), hx(cs), hx(b1)));
    s = run();
    CHECK(AG(s, t)->in_force == MOR_NOT_SHOWN);
}

static void t_g4_11_not_valid(void)
{
    mor_fp al, bl;
    mor_fp t = two(F("condition %s", stub("k4.no")), NULL, &al, &bl);
    mor_fp t2 = terms(&K, "alice", hx(al), F("replaces %s|agreement %s|%s", hx(t), hx(t), P2), NULL);
    assent(&K, "alice", t, t2, hx(t2));
    assent(&K, "bob", t, t2, hx(bl)); /* everyone signed */
    mor_set *s = run();
    const mor_act *u = A(s, t2);
    CHECK(u->ev.chain == MOR_C_NOT_VALID); /* replaces nothing, whoever signed it */
    CHECK(u->ev.cond_asked && u->ev.cond == MOR_NO);
    mor_fp want = mor_stub_fp("k4.no");
    CHECK(u->ev.cond_terms == A(s, t) && mor_fp_eq(&u->ev.cond_terms->condition.cmip, &want)); /* the condition named beside it */
    CHECK(u->ev.cond_ground != NULL);
    CHECK(AG(s, t)->cur == A(s, t) && AG(s, t)->in_force == MOR_SHOWN && AG(s, t)->n_tips == 0);
    mor_answer a;
    mor_terms_in_force(s, u, &a);
    CHECK(a.ans == MOR_NOT_VALID);
    CHECK(has(show(s), "not valid") != false);
}

static void t_g4_11_time_and_bytes(void)
{
    /* a time, per the clock the agreement names */
    mor_fp al, bl;
    mor_fp t = two(F("clock ck1|condition %s 100", stub("k4.before")), NULL, &al, &bl);
    mor_fp t2 = terms(&K, "alice", hx(al), F("replaces %s|agreement %s|%s|clock ck1", hx(t), hx(t), P2), NULL);
    assent(&K, "alice", t, t2, hx(t2));
    assent(&K, "bob", t, t2, hx(bl));
    size_t keep = K.n;
    rec(&K, "clock", "ck1", "now 150");
    rec(&K, "clock", "other", "now 5");
    g_opt.anchoring_clock = "other"; /* the reader's own clock is not used: the agreement names one (G13.4) */
    mor_set *s = run();
    const mor_act *u = A(s, t2);
    CHECK(u->ev.chain == MOR_C_NOT_VALID);
    CHECK(u->ev.cond_qual.kind == MOR_Q_PER_CLOCK && eq(u->ev.cond_qual.name, "ck1"));
    K.n = keep;
    rec(&K, "clock", "ck1", "now 50");
    s = run();
    CHECK(A(s, t2)->ev.cond == MOR_YES && A(s, t2)->ev.chain == MOR_C_VERSION);
    /* no answer from the clock: unchecked */
    K.n = keep;
    s = run();
    CHECK(A(s, t2)->ev.chain == MOR_C_UNCHECKED && AG(s, t)->in_force == MOR_UNCHECKED);
}

static void t_g4_11_valid_needs_signatures(void)
{
    mor_fp al, bl;
    mor_fp t = two(F("condition %s", stub("k4.same-parties")), NULL, &al, &bl);
    /* valid, but only alice signed: valid is about the update, never about who may sign it */
    mor_fp t2 = terms(&K, "alice", hx(al), F("replaces %s|agreement %s|%s", hx(t), hx(t), P2), NULL);
    mor_fp a2 = assent(&K, "alice", t, t2, hx(t2));
    /* an update that changes the parties: not valid from the bytes alone, even only proposed */
    mor_fp t3 = terms(&K, "alice", hx(a2), F("replaces %s|agreement %s|party signer alice|author signer alice", hx(t), hx(t)), NULL);
    mor_set *s = run();
    CHECK(A(s, t2)->ev.cond == MOR_YES && A(s, t2)->ev.chain == MOR_C_NOT_SHOWN);
    CHECK(A(s, t3)->ev.cond == MOR_NO && A(s, t3)->ev.chain == MOR_C_NOT_VALID);
    /* (h): without the cMIP, whether the update replaces is unchecked, and so is what rests on it */
    without("k4.same-parties");
    s = run();
    CHECK(A(s, t2)->ev.chain == MOR_C_UNCHECKED && AG(s, t)->in_force == MOR_UNCHECKED);
}

/* ============================ G5 ============================ */

static void t_g5_3(void)
{
    /* bob assents with no for: it is never counted as made for alice */
    mor_fp d = terms(&K, "alice", "none", F("%s|power assent %s bob", P2, stub("k4.delegate")), "delegation signer alice signer bob default");
    assent(&K, "alice", d, d, hx(d));
    assent(&K, "bob", d, d, "none");
    mor_fp w = terms(&K, "carol", "none", "party signer alice|party signer carol|author signer carol", NULL);
    assent(&K, "carol", w, w, hx(w));
    mor_fp own = assent(&K, "bob", w, w, "none");
    mor_set *s = run();
    CHECK(AG(s, w)->in_force == MOR_NOT_SHOWN);
    mor_answer a;
    mor_bound_by_act(s, "alice", A(s, own), &a);
    CHECK(a.ans == MOR_NOT_SHOWN);
    /* the same act made for alice, under the delegation, does count */
    mor_fp forher = stmt(&K, "bob", w, w, "assent", hx(own), F("for signer alice|under %s", hx(d)));
    s = run();
    CHECK(AG(s, w)->in_force == MOR_SHOWN);
    mor_bound_by_act(s, "alice", A(s, forher), &a);
    CHECK(a.ans == MOR_SHOWN);
    mor_bound_by_act(s, "alice", A(s, own), &a);
    CHECK(a.ans == MOR_NOT_SHOWN); /* still not: it MUST NOT later be counted as made for anyone else */
}

/* a delegation from alice to bob, then ended by alice alone; returns the first version */
static mor_fp g_d2;
static mor_fp delegation(const char *mode, const char *extra, mor_fp *alast)
{
    mor_fp d = terms(&K, "alice", "none", F("%s|power assent %s bob%s%s", P2, stub("k4.delegate"), extra ? "|" : "", extra ? extra : ""),
                     F("delegation signer alice signer bob %s", mode));
    mor_fp a = assent(&K, "alice", d, d, hx(d));
    assent(&K, "bob", d, d, "none");
    g_d2 = terms(&K, "alice", hx(a), F("replaces %s|agreement %s|%s%s%s", hx(d), hx(d), P2, extra ? "|" : "", extra ? extra : ""),
                 "the power is removed");
    mor_fp a2 = assent(&K, "alice", d, g_d2, hx(g_d2));
    if (alast)
        *alast = a2;
    return d;
}

static void t_g5_4(void)
{
    mor_fp al;
    mor_fp d = delegation("default", NULL, &al);
    mor_fp w = terms(&K, "carol", "none", "party signer alice|party signer carol|author signer carol", NULL);
    assent(&K, "carol", w, w, hx(w));
    mor_fp x = stmt(&K, "bob", w, w, "assent", "none", F("for signer alice|under %s", hx(d)));
    mor_set *s = run();
    CHECK(AG(s, d)->cur == A(s, g_d2)); /* ended by the delegator alone */
    mor_answer a;
    mor_bound_by_act(s, "alice", A(s, x), &a);
    CHECK(a.ans == MOR_NOT_SHOWN && a.qual.kind == MOR_Q_AS_SHOWN); /* no order shown: as shown */
    /* with a place named by the delegation, the place answers: the act came first */
    K.n = 0;
    d = delegation("default", "place plc1", &al);
    w = terms(&K, "carol", "none", "party signer alice|party signer carol|author signer carol", NULL);
    assent(&K, "carol", w, w, hx(w));
    x = stmt(&K, "bob", w, w, "assent", "none", F("for signer alice|under %s", hx(d)));
    mor_set *tmp = run();
    char list[8192] = "";
    snprintf(list, sizeof list, "entry %s", hx(x));
    for (int i = 0; i < mor_n_acts(tmp); i++)
        if (mor_act_at(tmp, i) != A(tmp, x))
            snprintf(list + strlen(list), sizeof list - strlen(list), "|entry %s", hx(mor_act_at(tmp, i)->fp));
    /* x is listed first; the delegation's first version must be seen before x for x to be read: put d first */
    rec(&K, "place", "plc1", F("entry %s|%s", hx(d), list));
    s = run();
    CHECK(AG(s, d)->cur == A(s, g_d2));
    mor_bound_by_act(s, "alice", A(s, x), &a);
    CHECK(a.ans == MOR_SHOWN && a.qual.kind == MOR_Q_PER_PLACE);
}

static void t_g5_5_default(void)
{
    mor_fp d = delegation("default", NULL, NULL);
    mor_set *s = run();
    CHECK(A(s, g_d2)->ev.chain == MOR_C_VERSION);
    CHECK(has(A(s, g_d2)->ev.signed_how, "delegator"));
    (void)d;
}

static void t_g5_5_agent_only(void)
{
    mor_fp d = delegation("agent", NULL, NULL);
    mor_set *s = run();
    /* a version carrying the delegator's part alone replaces nothing */
    CHECK(A(s, g_d2)->ev.chain == MOR_C_NOT_SHOWN);
    CHECK(AG(s, d)->cur == A(s, d));
    const char *txt = show(s);
    CHECK(has(txt, "never") == false);                 /* no answer that it can never be ended */
    CHECK(has(txt, "whether it will be ended: not shown")); /* not shown */
    CHECK(has(txt, "delegation signer alice signer bob agent")); /* the condition as the Terms state it */
    /* where the words cannot be read, the answer is unchecked */
    without("k1.lines");
    s = run();
    CHECK(A(s, g_d2)->ev.chain == MOR_C_UNCHECKED);
}

static void t_g5_2_power_for_replacement(void)
{
    mor_fp t = terms(&K, "alice", "none",
                     F("party signer alice|party signer bob|party signer carol|author signer alice|"
                       "power replaces %s 2 signer alice signer bob signer carol",
                       stub("k4.threshold")),
                     NULL);
    mor_fp a = assent(&K, "alice", t, t, hx(t));
    mor_fp b = assent(&K, "bob", t, t, "none");
    assent(&K, "carol", t, t, "none");
    mor_fp t2 = terms(&K, "alice", hx(a),
                      F("replaces %s|agreement %s|party signer alice|party signer bob|party signer carol|author signer alice|in_force %s", hx(t),
                        hx(t), stub("k4.yes")),
                      NULL);
    assent(&K, "alice", t, t2, hx(t2));
    assent(&K, "bob", t, t2, hx(b));
    mor_set *s = run();
    CHECK(A(s, t2)->ev.chain == MOR_C_VERSION && A(s, t2)->ev.by_power);
    /* G11.6(b): the majority decides; carol, outvoted, is read as that rule says: bound through the power she signed for */
    mor_answer ans;
    mor_bound_by_terms(s, "carol", A(s, t2), &ans);
    CHECK(ans.ans == MOR_SHOWN && has(ans.why, "power"));
    /* and nothing says, beyond the rule, that she agreed: no assent in her name is shown */
    mor_bound_by_act(s, "carol", A(s, mor_act_at(s, mor_n_acts(s) - 1)->fp), &ans);
    CHECK(ans.ans == MOR_NOT_SHOWN);
}

/* ============================ G6, G12 ============================ */

static void t_g6_1(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a = assent(&K, "alice", t, t, hx(t));
    mor_fp bad = assent(&K, "bob", t, t, hx(a)); /* names another signer's act as previous */
    rec(&K, "statement", "bob", F("agreement %s|terms %s|kind assent", hx(t), hx(t))); /* names no previous at all */
    mor_set *s = run();
    CHECK(A(s, bad)->ev.invalid && has(A(s, bad)->ev.invalid, "G6.1"));
    CHECK(mor_n_rejects(s) == 1 && has(mor_reject_at(s, 0)->reason, "previous"));
    CHECK(AG(s, t)->in_force == MOR_NOT_SHOWN);
}

static void t_g6_2(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a1 = assent(&K, "alice", t, t, hx(t));
    mor_fp a2 = stmt(&K, "alice", t, t, "contest", hx(t), NULL); /* same previous */
    assent(&K, "bob", t, t, "none");
    mor_set *s = run();
    CHECK(mor_n_line_forks(s) == 1);
    const mor_line_fork *f = mor_line_fork_at(s, 0);
    CHECK(eq(f->signer, "alice") && f->n_tips == 2 && !f->resolved && f->continued == NULL);
    CHECK((A(s, a1)->ev.line & MOR_M_TIP) && (A(s, a2)->ev.line & MOR_M_TIP)); /* both tips stop */
    CHECK(AG(s, t)->in_force == MOR_NOT_SHOWN); /* her assent is a tip: not counted */
    /* two acts that both name none */
    K.n = 0;
    t = terms(&K, "alice", "none", P2, NULL);
    assent(&K, "bob", t, t, "none");
    stmt(&K, "bob", t, t, "contest", "none", NULL);
    s = run();
    CHECK(mor_n_line_forks(s) == 1 && eq(mor_line_fork_at(s, 0)->signer, "bob"));
}

static void t_g12_2_built_on(void)
{
    mor_fp t = fork_of_terms(NULL);
    mor_fp on_tip = stmt(&K, "zed", t, g_tip1, "contest", "none", NULL);
    mor_fp pay = rec(&K, "payment", "zed", F("agreement %s|version %s|settlement %s|amount 5 EUR", hx(t), hx(g_tip2), stub("s")));
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_FORK);
    CHECK(AG(s, t)->tips[0] != AG(s, t)->tips[1]);
    CHECK(A(s, on_tip)->ev.rest & MOR_M_BUILT); /* shown, with the fork, and not counted */
    CHECK(A(s, pay)->ev.rest & MOR_M_BUILT);
    CHECK(!mor_counts(A(s, on_tip)));
    const char *txt = show(s);
    CHECK(has(txt, "FORK of Terms") && has(txt, "built on a tip"));
    /* G12.8: neither tip, nor anything built on it, is shown as resolved: not as the next version, not as in force */
    CHECK(has(txt, "resolved by") == false);
    mor_answer a;
    mor_terms_in_force(s, A(s, g_tip1), &a);
    CHECK(a.ans == MOR_FORK);
    for (int i = 0; i < AG(s, t)->n_versions; i++)
        CHECK(AG(s, t)->versions[i] != A(s, g_tip1) && AG(s, t)->versions[i] != A(s, g_tip2));
}

static void t_g12_3_resolve_terms(void)
{
    mor_fp t = fork_of_terms(NULL);
    mor_fp stranger = stmt(&K, "zed", t, g_tip2, "contest", "none", NULL);
    mor_fp pay = rec(&K, "payment", "zed", F("agreement %s|version %s|settlement %s|amount 5 EUR", hx(t), hx(g_tip2), stub("s")));
    /* a resolving update of the shared Terms, signed as an update of them must be: by alice and bob */
    mor_fp r = terms(&K, "carol", "none", F("replaces %s|agreement %s|resolves %s|resolves %s|continues %s", hx(t), hx(t), hx(g_tip1), hx(g_tip2), hx(g_tip1)), NULL);
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_FORK); /* not signed yet */
    mor_set *prev = s;
    /* find alice's and bob's last acts to keep their lines whole */
    mor_fp al = {{0}}, bl = {{0}};
    for (int i = 0; i < mor_n_acts(prev); i++) {
        const mor_act *x = mor_act_at(prev, i);
        bool last = x->type == MOR_T_STATEMENT || x->type == MOR_T_TERMS;
        for (int k = 0; last && k < mor_n_acts(prev); k++)
            last = !(mor_act_at(prev, k)->has_previous && mor_fp_eq(&mor_act_at(prev, k)->previous, &x->fp));
        if (last && eq(x->signer, "alice"))
            al = x->fp;
        if (last && eq(x->signer, "bob"))
            bl = x->fp;
    }
    assent(&K, "alice", t, r, hx(al));
    assent(&K, "bob", t, r, hx(bl));
    s = run();
    CHECK(AG(s, t)->in_force == MOR_SHOWN && AG(s, t)->cur == A(s, g_tip1));
    CHECK(A(s, g_tip2)->ev.chain == MOR_C_SET_ASIDE && A(s, r)->ev.chain == MOR_C_RESOLVER);
    /* G12.5: what a stranger relied on in the tip set aside stands, shown as disputed */
    CHECK(A(s, stranger)->ev.rest & MOR_M_DISPUTED);
    CHECK(mor_counts(A(s, stranger)));
    mor_payment p;
    mor_payment_read(s, A(s, pay), &p);
    CHECK(p.disputed && p.receiver.ans.disputed && p.receiver.who && eq(p.receiver.who, "bob"));
    mor_contest c;
    mor_contest_read(s, A(s, stranger), &c);
    CHECK(c.ans.disputed);
    /* an act that names resolves and no continues resolves nothing */
    K.n = 0;
    t = fork_of_terms(NULL);
    r = terms(&K, "carol", "none", F("replaces %s|agreement %s|resolves %s|resolves %s", hx(t), hx(t), hx(g_tip1), hx(g_tip2)), NULL);
    s = run();
    CHECK(AG(s, t)->in_force == MOR_FORK && A(s, r)->ev.chain == MOR_C_RESOLVES_NOTHING);
}

static void t_g12_4_power_to_resolve(void)
{
    mor_fp t = fork_of_terms(F("power resolves %s judy", stub("k4.delegate")));
    mor_fp r = terms(&K, "judy", "none", F("replaces %s|agreement %s|resolves %s|resolves %s|continues %s", hx(t), hx(t), hx(g_tip1), hx(g_tip2), hx(g_tip2)), NULL);
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_SHOWN && AG(s, t)->cur == A(s, g_tip2));
    /* reported with the power it was signed under and whoever it rests on */
    CHECK(A(s, r)->ev.by_power && has(A(s, r)->ev.signed_how, "power to resolve"));
    CHECK(eq(A(s, r)->signer, "judy"));
    /* in one signer's line: a power to make that act for the signer */
    K.n = 0;
    mor_fp d = terms(&K, "alice", "none", F("%s|power resolves %s judy", P2, stub("k4.delegate")), NULL);
    mor_fp a1 = assent(&K, "alice", d, d, hx(d));
    assent(&K, "bob", d, d, "none");
    mor_fp x1 = stmt(&K, "alice", d, d, "contest", hx(a1), "content one");
    mor_fp x2 = stmt(&K, "alice", d, d, "contest", hx(a1), "content two");
    stmt(&K, "judy", d, d, "contest", "none", F("for signer alice|under %s|resolves %s|resolves %s|continues %s", hx(d), hx(x1), hx(x2), hx(x1)));
    s = run();
    CHECK(mor_n_line_forks(s) == 1);
    const mor_line_fork *f = mor_line_fork_at(s, 0);
    CHECK(f->resolved && f->continued == A(s, x1));
    CHECK(f->resolver_power && has(f->resolver_power, "judy") && has(f->resolver_power, "power to resolve"));
    CHECK(A(s, x2)->ev.line & MOR_M_SET_ASIDE);
    CHECK(mor_counts(A(s, x1)) && !mor_counts(A(s, x2)));
}

static void t_g12_3_resolve_line(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a1 = assent(&K, "alice", t, t, hx(t));
    mor_fp a2 = stmt(&K, "alice", t, t, "contest", hx(t), NULL);
    assent(&K, "bob", t, t, "none");
    mor_fp carol = stmt(&K, "carol", t, t, "contest", "none", F("answers %s", hx(a2)));
    stmt(&K, "alice", t, t, "contest", hx(t), F("resolves %s|resolves %s|continues %s", hx(a1), hx(a2), hx(a1)));
    mor_set *s = run();
    CHECK(mor_n_line_forks(s) == 1 && mor_line_fork_at(s, 0)->resolved);
    CHECK(AG(s, t)->in_force == MOR_SHOWN); /* her assent continues, and counts */
    CHECK(A(s, a2)->ev.line & MOR_M_SET_ASIDE);
    CHECK(A(s, carol)->ev.line & MOR_M_DISPUTED); /* carol signed none of that branch: her act stands, disputed */
    mor_answer a;
    mor_bound_by_act(s, "carol", A(s, carol), &a);
    CHECK(a.ans == MOR_SHOWN && a.disputed);
}

static void t_g12_7_rivals(void)
{
    mor_fp t = fork_of_terms(F("power resolves %s judy kim lee", stub("k4.delegate")));
    const char *head = F("replaces %s|agreement %s|resolves %s|resolves %s", hx(t), hx(t), hx(g_tip1), hx(g_tip2));
    mor_fp r1 = terms(&K, "judy", "none", F("%s|continues %s", head, hx(g_tip1)), NULL);
    mor_fp r2 = terms(&K, "kim", "none", F("%s|continues %s", head, hx(g_tip2)), NULL);
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_FORK); /* neither resolves while the other stands */
    CHECK(A(s, r1)->ev.chain == MOR_C_TIP && A(s, r2)->ev.chain == MOR_C_TIP);
    terms(&K, "lee", "none", F("replaces %s|agreement %s|resolves %s|resolves %s|continues %s", hx(t), hx(t), hx(r1), hx(r2), hx(r2)), NULL);
    s = run();
    CHECK(AG(s, t)->in_force == MOR_SHOWN && AG(s, t)->cur == A(s, g_tip2));
}

/* ============================ G7, G13 ============================ */

static void t_g7_2(void)
{
    mor_fp t = terms(&K, "alice", "none", P2 "|place plc1", NULL);
    mor_fp a = assent(&K, "alice", t, t, hx(t));
    mor_fp b = assent(&K, "bob", t, t, "none");
    size_t keep = K.n;
    rec(&K, "place", "plc1", F("entry %s|entry %s", hx(t), hx(a))); /* bob's assent is genuine, and not in the place */
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_NOT_SHOWN && AG(s, t)->qual.kind == MOR_Q_PER_PLACE);
    K.n = keep;
    rec(&K, "place", "plc1", F("entry %s|entry %s|entry %s", hx(t), hx(a), hx(b)));
    s = run();
    CHECK(AG(s, t)->in_force == MOR_SHOWN && AG(s, t)->qual.kind == MOR_Q_PER_PLACE && eq(AG(s, t)->qual.name, "plc1"));
}

static void t_g7_3(void)
{
    mor_fp t = two(NULL, NULL, NULL, NULL);
    mor_set *s = run();
    mor_answer a;
    mor_terms_in_force(s, A(s, t), &a);
    CHECK(a.qual.kind == MOR_Q_AS_SHOWN);
    mor_receiver r;
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.ans.qual.kind == MOR_Q_AS_SHOWN);
    mor_spot sp;
    mor_spot_read(s, &t, "nobody", &sp);
    CHECK(has(show(s), "[as shown]") != false);
    CHECK(has(g_text, "in force: shown -") == false); /* never given without what it stands on */
}

static void t_g7_4(void)
{
    mor_fp t = terms(&K, "alice", "none", P2 "|place plc1", NULL);
    mor_fp a = assent(&K, "alice", t, t, hx(t));
    mor_fp b = assent(&K, "bob", t, t, "none");
    rec(&K, "place", "plc1", F("entry %s|entry %s", hx(t), hx(a)));
    rec(&K, "place", "mine", F("entry %s|entry %s|entry %s", hx(t), hx(a), hx(b)));
    g_opt.anchoring_place = "mine"; /* the reader's own anchoring would say "in force" */
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_NOT_SHOWN);
    CHECK(AG(s, t)->qual.kind == MOR_Q_PER_PLACE && eq(AG(s, t)->qual.name, "plc1"));
}

static void t_g13_2(void)
{
    mor_fp al, bl;
    mor_fp t = two("receiver signer alice", NULL, &al, &bl);
    mor_fp t2 = terms(&K, "alice", hx(al), F("replaces %s|agreement %s|%s|receiver signer bob", hx(t), hx(t), P2), NULL);
    assent(&K, "alice", t, t2, hx(t2));
    assent(&K, "bob", t, t2, hx(bl));
    mor_fp st = rec(&K, "settlement", "bank", "amount 5 EUR");
    mor_fp named = rec(&K, "payment", "zed", F("agreement %s|version %s|settlement %s|amount 5 EUR", hx(t), hx(t), hx(st)));
    mor_fp unnamed = rec(&K, "payment", "yan", F("agreement %s|settlement %s|amount 5 EUR", hx(t), hx(st)));
    mor_set *s = run();
    CHECK(mor_names(A(s, named), &t) && !mor_names(A(s, named), &t2));
    CHECK(!mor_names(A(s, unnamed), &t2)); /* never read as having relied on a version it does not name */
    mor_payment p;
    mor_payment_read(s, A(s, named), &p);
    CHECK(eq(p.receiver.who, "alice")); /* G13.1: a later version does not change what the act did */
    mor_payment_read(s, A(s, unnamed), &p);
    CHECK(eq(p.receiver.who, "bob") && p.receiver.ans.qual.kind == MOR_Q_AS_SHOWN);
}

static void t_g13_3(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a = assent(&K, "alice", t, t, hx(t));
    mor_fp b = assent(&K, "bob", t, t, "none");
    mor_fp c = stmt(&K, "alice", t, t, "contest", hx(a), NULL);
    mor_set *s = run();
    CHECK(mor_order_by_citing(s, A(s, t), A(s, c)) == MOR_ORDER_A_FIRST); /* through acts that name it */
    CHECK(mor_order_by_citing(s, A(s, c), A(s, t)) == MOR_ORDER_B_FIRST);
    CHECK(mor_order_by_citing(s, A(s, a), A(s, b)) == MOR_ORDER_NONE);
}

static void t_g13_4(void)
{
    /* where the agreement names none, the reader's anchoring is used, and named */
    mor_fp t = two(F("condition %s 100", stub("k4.before")), NULL, NULL, NULL);
    mor_fp al = {{0}};
    (void)al;
    mor_set *s0 = run();
    mor_fp last = {{0}};
    for (int i = 0; i < mor_n_acts(s0); i++)
        if (eq(mor_act_at(s0, i)->signer, "alice") && mor_act_at(s0, i)->type == MOR_T_STATEMENT)
            last = mor_act_at(s0, i)->fp;
    mor_fp t2 = terms(&K, "alice", hx(last), F("replaces %s|agreement %s|%s", hx(t), hx(t), P2), NULL);
    rec(&K, "clock", "myclock", "now 7");
    g_opt.anchoring_clock = "myclock";
    mor_set *s = run();
    CHECK(A(s, t2)->ev.cond == MOR_YES);
    CHECK(A(s, t2)->ev.cond_qual.kind == MOR_Q_PER_ANCHORING && eq(A(s, t2)->ev.cond_qual.name, "myclock"));
    uint64_t now;
    mor_qual q;
    CHECK(mor_time_for(s, A(s, t), &now, &q) == MOR_YES && now == 7 && q.kind == MOR_Q_PER_ANCHORING);
}

/* ============================ G8, G9 ============================ */

static void t_g9_1(void)
{
    /* lacks a cMIP named by fingerprint */
    mor_fp t = terms(&K, "alice", "none", F("party signer alice|author signer alice|in_force %s 1 signer alice", stub("k4.threshold")), NULL);
    assent(&K, "alice", t, t, hx(t));
    without("k4.threshold");
    mor_set *s = run();
    CHECK(AG(s, t)->in_force == MOR_UNCHECKED);
    mor_receiver r;
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.ans.ans == MOR_UNCHECKED); /* every answer that depends on it */
    mor_answer a;
    mor_bound_by_terms(s, "alice", A(s, t), &a);
    CHECK(a.ans == MOR_UNCHECKED);
    /* lacks an act named by fingerprint */
    mor_fp pay = rec(&K, "payment", "zed", F("agreement %s|version %s|settlement %s|amount 5 EUR", hx(t), stub("gone"), stub("s")));
    mor_fp orphan = stmt(&K, "zed", t, mor_stub_fp("gone"), "assent", "none", NULL);
    s = run();
    mor_payment p;
    mor_payment_read(s, A(s, pay), &p);
    CHECK(p.status == MOR_UNCHECKED);
    CHECK(A(s, orphan)->ev.unchecked != NULL);
    /* cannot get an answer from a named place */
    K.n = 0;
    begin();
    t = terms(&K, "alice", "none", "party signer alice|author signer alice|place silent", NULL);
    assent(&K, "alice", t, t, hx(t));
    s = run();
    CHECK(AG(s, t)->in_force == MOR_UNCHECKED);
    /* with no plug-in at all, nothing that needs a rule passes */
    g_opt.plugins = NULL;
    K.n = 0;
    t = terms(&K, "alice", "none", F("party signer alice|author signer alice|in_force %s", stub("k4.yes")), NULL);
    s = run();
    CHECK(AG(s, t)->in_force == MOR_UNCHECKED);
}

/* ============================ G10 ============================ */

static mor_fp offer(const char *k12params, const char *extra)
{
    mor_fp t = terms(&K, "alice", "none",
                     F("party signer alice|party role buyer received %s %s %s|author signer alice|in_force %s 1 signer alice|"
                       "power replaces %s alice%s%s",
                       stub("thing"), stub("k12.claim"), k12params, stub("k4.threshold"), stub("k4.delegate"), extra ? "|" : "",
                       extra ? extra : ""),
                     NULL);
    assent(&K, "alice", t, t, hx(t));
    return t;
}

static void t_g10_1(void)
{
    const char *tail = F("author signer alice|in_force %s", stub("k4.yes"));
    mor_fp nokind = terms(&K, "alice", "none", F("party signer alice|party role r|%s", tail), NULL);
    mor_fp badkind = terms(&K, "alice", "none", F("party signer alice|party role r winner %s %s|%s", stub("w"), stub("k12.claim"), tail), NULL);
    mor_fp nowhat = terms(&K, "alice", "none", F("party signer alice|party role r done - %s|%s", stub("k12.claim"), tail), NULL);
    mor_fp nocmip = terms(&K, "alice", "none", F("party signer alice|party role r done %s|%s", stub("w"), tail), NULL);
    mor_fp future = terms(&K, "alice", "none", F("party signer alice|party role r future - %s|%s", stub("k12.claim"), tail), NULL);
    mor_set *s = run();
    CHECK(A(s, nokind)->ev.invalid && has(A(s, nokind)->ev.invalid, "G10.1"));
    CHECK(A(s, badkind)->ev.invalid && A(s, nowhat)->ev.invalid && A(s, nocmip)->ev.invalid);
    CHECK(A(s, future)->ev.invalid == NULL); /* kind future names no what (G10.4) */
}

static void t_g10_2(void)
{
    mor_fp t = offer("many confirm=wit", NULL);
    mor_fp c1 = stmt(&K, "zed", t, t, stub("k11.claim"), "none", "content buyer");
    stmt(&K, "yan", t, t, stub("k11.claim"), "none", "content buyer"); /* not confirmed */
    mor_fp cf = stmt(&K, "wit", t, t, stub("k11.confirm"), "none", F("answers %s", hx(c1)));
    mor_set *s = run();
    mor_spot sp;
    mor_spot_read(s, &t, "buyer", &sp);
    CHECK(sp.ans.ans == MOR_SHOWN && sp.n == 1 && sp.many);
    CHECK(eq(sp.fill[0].signer, "zed"));
    /* each filling with its proof, each act reported by who signed it (G8.4) */
    CHECK(sp.fill[0].n_proof == 2 && sp.fill[0].proof[0] == A(s, c1) && sp.fill[0].proof[1] == A(s, cf));
    CHECK(eq(sp.fill[0].proof[1]->signer, "wit"));
    CHECK(sp.ans.qual.kind == MOR_Q_AS_SHOWN);
    CHECK(has(show(s), "signed by wit") != false);
    /* G10.3: filling a spot binds no one: zed is bound only by his own assent */
    mor_answer a;
    mor_bound_by_terms(s, "zed", A(s, t), &a);
    CHECK(a.ans == MOR_NOT_SHOWN);
}

static void t_g10_5(void)
{
    mor_fp t = offer("many max=1", NULL);
    mor_set *s = run();
    mor_spot sp;
    mor_spot_read(s, &t, "buyer", &sp);
    CHECK(sp.takes_new.ans == MOR_SHOWN && sp.takes && sp.takes_new.why != NULL);
    stmt(&K, "zed", t, t, stub("k11.claim"), "none", "content buyer");
    s = run();
    mor_spot_read(s, &t, "buyer", &sp);
    CHECK(sp.takes_new.ans == MOR_SHOWN && !sp.takes && has(sp.takes_new.why, "count")); /* with its cause */
    CHECK(sp.takes_new.qual.kind == MOR_Q_AS_SHOWN);
    CHECK(sp.n == 1); /* a filling already earned stays a filling */
    /* a date the version states in advance, per the clock the agreement names: no update is needed */
    K.n = 0;
    t = offer("many until=100", "clock ck1");
    rec(&K, "clock", "ck1", "now 150");
    s = run();
    mor_spot_read(s, &t, "buyer", &sp);
    CHECK(!sp.takes && sp.takes_new.qual.kind == MOR_Q_PER_CLOCK && eq(sp.takes_new.qual.name, "ck1"));
    /* where the verifier lacks the cMIP, the answer is unchecked */
    without("k12.claim");
    s = run();
    mor_spot_read(s, &t, "buyer", &sp);
    CHECK(sp.takes_new.ans == MOR_UNCHECKED && sp.ans.ans == MOR_UNCHECKED);
}

static void t_g10_6(void)
{
    /* v1 names no update condition; zed earns under it; v2 (with a condition of its own) and v3 follow */
    mor_fp t = offer("many", NULL);
    mor_fp claim = stmt(&K, "zed", t, t, stub("k11.claim"), "none", "content buyer");
    const char *body = F("party signer alice|party role buyer received %s %s many|author signer alice|in_force %s|power replaces %s alice",
                         stub("thing"), stub("k12.claim"), stub("k4.yes"), stub("k4.delegate"));
    mor_set *s0 = run();
    mor_fp al = {{0}};
    for (int i = 0; i < mor_n_acts(s0); i++)
        if (eq(mor_act_at(s0, i)->signer, "alice") && mor_act_at(s0, i)->type == MOR_T_STATEMENT)
            al = mor_act_at(s0, i)->fp;
    mor_fp v2 = terms(&K, "alice", hx(al), F("replaces %s|agreement %s|%s|condition %s", hx(t), hx(t), body, stub("k4.yes")), NULL);
    mor_fp v3 = terms(&K, "alice", hx(v2), F("replaces %s|agreement %s|%s", hx(v2), hx(t), body), NULL);
    mor_set *s = run();
    CHECK(AG(s, t)->cur == A(s, v3));
    mor_spot sp;
    mor_spot_read(s, &t, "buyer", &sp);
    CHECK(sp.n == 1 && sp.fill[0].earned_under == A(s, t)); /* the version it was earned under, named */
    /* v2's condition is not the filling's own version's: no later version changes its rights */
    CHECK(sp.fill[0].n_changes == 0);
    /* (a): the holder's own assent to a later version */
    assent(&K, "zed", t, v3, hx(claim));
    s = run();
    mor_spot_read(s, &t, "buyer", &sp);
    CHECK(sp.fill[0].n_changes == 1 && sp.fill[0].changes[0].version == A(s, v3) && sp.fill[0].changes[0].ans == MOR_SHOWN);
    CHECK(has(sp.fill[0].changes[0].rests_on, "own assent"));
}

static void t_g10_6b_and_7(void)
{
    /* the version the filling was earned under names an update condition, which answers valid */
    mor_fp t = offer("many", F("condition %s", stub("k4.yes")));
    stmt(&K, "zed", t, t, stub("k11.claim"), "none", "content buyer");
    mor_set *s0 = run();
    mor_fp al = {{0}};
    for (int i = 0; i < mor_n_acts(s0); i++)
        if (eq(mor_act_at(s0, i)->signer, "alice") && mor_act_at(s0, i)->type == MOR_T_STATEMENT)
            al = mor_act_at(s0, i)->fp;
    mor_fp v2 = terms(&K, "alice", hx(al),
                      F("replaces %s|agreement %s|party signer alice|party role buyer received %s %s many|author signer alice|in_force %s",
                        hx(t), hx(t), stub("thing"), stub("k12.claim"), stub("k4.yes")),
                      "the licence stops");
    mor_set *s = run();
    mor_spot sp;
    mor_spot_read(s, &t, "buyer", &sp);
    CHECK(sp.n == 1); /* a filling whose rights an update changed stays a filling: nothing is undone */
    CHECK(sp.fill[0].n_changes == 1 && sp.fill[0].changes[0].version == A(s, v2) && sp.fill[0].changes[0].ans == MOR_VALID);
    CHECK(has(sp.fill[0].changes[0].rests_on, "update condition") && has(sp.fill[0].changes[0].rests_on, "valid"));
    /* no answer "triggered", none that rights "no longer apply"; what the update says of them is not answered */
    const char *txt = show(s);
    CHECK(has(txt, "triggered") == false && has(txt, "no longer appl") == false);
    CHECK(has(txt, "still a party") == false && has(txt, "licence stops") == false);
    for (int i = 0; i <= MOR_NO_REPORT_CALLED_FOR; i++)
        CHECK(strcmp(mor_ans_name((mor_ans)i), "triggered") != 0 && has(mor_ans_name((mor_ans)i), "appl") == false);
    /* without the cMIP: unchecked */
    without("k4.yes");
    s = run();
    CHECK(AG(s, t)->in_force == MOR_UNCHECKED);
}

/* ============================ G11 ============================ */

static mor_fp g_w, g_d, g_pay, g_s1, g_s2, g_s3, g_doer;
/* a work (alice, bob) whose receiver is its deal with a service, sam; a buyer pays the work */
static void click(const char *deal_call, bool confirm, bool doer_report, const char *bob_amount)
{
    g_w = terms(&K, "alice", "none", F("%s|power assent %s alice", P2, stub("k4.delegate")), "share signer alice 50|share signer bob 50");
    mor_fp wa = assent(&K, "alice", g_w, g_w, hx(g_w));
    assent(&K, "bob", g_w, g_w, "none");
    g_d = terms(&K, "sam", "none",
                F("party signer sam|party agreement %s|author signer sam|receiver signer sam|in_force %s 2 signer sam agreement %s%s%s", hx(g_w),
                  stub("k4.threshold"), hx(g_w), deal_call ? "|" : "", deal_call ? deal_call : ""),
                F("share agreement %s 90|share signer sam 10", hx(g_w)));
    assent(&K, "sam", g_d, g_d, hx(g_d));
    /* the work names the deal as its receiver, in a new version; under it alice signs the deal in the work's name */
    mor_fp w2 = terms(&K, "alice", hx(wa),
                      F("replaces %s|agreement %s|%s|receiver agreement %s|in_force %s|power assent %s alice", hx(g_w), hx(g_w), P2, hx(g_d),
                        stub("k4.yes"), stub("k4.delegate")),
                      "share signer alice 50|share signer bob 50");
    stmt(&K, "alice", g_d, g_d, "assent", "none", F("for agreement %s|under %s", hx(g_w), hx(w2)));
    mor_fp a2 = assent(&K, "alice", g_w, w2, hx(w2));
    (void)a2;
    mor_set *tmp = run();
    mor_fp bl = {{0}};
    for (int i = 0; i < mor_n_acts(tmp); i++)
        if (eq(mor_act_at(tmp, i)->signer, "bob"))
            bl = mor_act_at(tmp, i)->fp;
    assent(&K, "bob", g_w, w2, hx(bl));
    g_s1 = rec(&K, "settlement", "bank", "amount 100 EUR");
    g_s2 = rec(&K, "settlement", "bank", "amount 45 EUR");
    g_s3 = rec(&K, "settlement", "bank", "amount 45 EUR|note second");
    g_pay = rec(&K, "payment", "zed", F("agreement %s|version %s|settlement %s|amount 100 EUR", hx(g_w), hx(w2), hx(g_s1)));
    if (confirm)
        rec(&K, "report", "sam", F("call %s|side receiving|received 100 EUR %s", hx(g_s1), hx(g_s1)));
    if (doer_report)
        g_doer = rec(&K, "report", "sam",
                     F("call %s|side doer|relied %s|received 100 EUR %s|passed 45 EUR signer alice %s|passed 45 EUR signer bob %s|kept 10 EUR",
                       hx(g_d), hx(g_d), hx(g_s1), hx(g_s2), hx(g_s3)));
    if (doer_report)
        rec(&K, "report", "alice", F("call %s|side receiving|received 45 EUR %s", hx(g_d), hx(g_s2)));
    if (bob_amount)
        rec(&K, "report", "bob", F("call %s|side receiving|received %s EUR %s", hx(g_d), bob_amount, hx(g_s3)));
}

static void t_g11_7_payment(void)
{
    click("call K8 both", false, false, NULL);
    mor_set *s = run();
    mor_payment p;
    mor_payment_read(s, A(s, g_pay), &p);
    CHECK(p.receiver.ans.ans == MOR_SHOWN && eq(p.receiver.who, "sam"));
    CHECK(p.status == MOR_ONE_SIDE_ONLY); /* never shown as received on the buyer's word alone */
    CHECK(has(show(s), "received") == false || has(g_text, "payment received") == false);
    /* confirmed with the same amount: matched */
    rec(&K, "report", "sam", F("call %s|side receiving|received 100 EUR %s", hx(g_s1), hx(g_s1)));
    s = run();
    mor_payment_read(s, A(s, g_pay), &p);
    CHECK(p.status == MOR_MATCHED && p.confirmation && eq(p.confirmation->signer, "sam"));
    /* a confirmation that states another amount: mismatched */
    rec(&K, "report", "sam", F("call %s|side receiving|received 90 EUR %s", hx(g_s1), hx(g_s1)));
    s = run();
    mor_payment_read(s, A(s, g_pay), &p);
    CHECK(p.status == MOR_MISMATCHED);
    /* a "confirmation" by someone who is not the receiving side confirms nothing */
    K.n = 0;
    click("call K8 both", false, false, NULL);
    rec(&K, "report", "zed", F("call %s|side receiving|received 100 EUR %s", hx(g_s1), hx(g_s1)));
    s = run();
    mor_payment_read(s, A(s, g_pay), &p);
    CHECK(p.status == MOR_ONE_SIDE_ONLY);
}

static void t_g8_3(void)
{
    /* a Statement tied to a settlement moves no value: it is reported on the word of whoever gave the settlement */
    mor_fp t = two(NULL, NULL, NULL, NULL);
    mor_fp st = rec(&K, "settlement", "bank", "amount 5 EUR");
    stmt(&K, "zed", t, t, "contest", "none", F("settlement %s|content i-paid-you", hx(st)));
    mor_fp pay = rec(&K, "payment", "zed", F("agreement %s|version %s|settlement %s|amount 5 EUR", hx(t), hx(t), hx(st)));
    mor_set *s = run();
    const char *txt = show(s);
    CHECK(has(txt, "on the word of bank") != false);
    CHECK(has(txt, "value moved") == false && has(txt, " paid:") == false);
    mor_payment p;
    mor_payment_read(s, A(s, pay), &p);
    CHECK(p.status != MOR_MATCHED && p.status != MOR_SHOWN); /* no receiver named: not shown; never "moved" */
}

static void t_g11_legs(void)
{
    click("call K8 both", true, true, "45");
    mor_set *s = run();
    mor_k8 k;
    mor_k8_read(s, A(s, g_d), &k);
    CHECK(k.holds == MOR_YES && k.has_call && k.from == MOR_FROM_BOTH);
    CHECK(k.doer && eq(k.doer, "sam"));
    CHECK(k.call_status == MOR_SHOWN && k.n_reports == 3 && k.n_ties == 2);
    CHECK(k.ties[0].status == MOR_MATCHED && k.ties[1].status == MOR_MATCHED);
    /* G8.4: each part with whoever confirmed it */
    CHECK(eq(k.ties[0].doer_report->signer, "sam") && eq(k.ties[0].recv_report->signer, "alice"));
    /* G11.4: a result that depends on a sequence, read leg by leg */
    mor_trace tr;
    mor_trace_payment(s, A(s, g_pay), &tr);
    CHECK(tr.n == 3);
    CHECK(eq(tr.legs[0].doer, "zed") && eq(tr.legs[0].receiving, "sam") && tr.legs[0].status == MOR_MATCHED);
    CHECK(eq(tr.legs[1].doer, "sam") && eq(tr.legs[1].receiving, "alice") && tr.legs[1].status == MOR_MATCHED);
    CHECK(tr.legs[1].doer_report && tr.legs[1].recv_report && eq(tr.legs[2].recv_report->signer, "bob"));
    /* G11.5: a fee kept is a line of its own; the doer here states no part of its own */
    const mor_act *dr = A(s, g_doer);
    CHECK(dr->lines[3].kind == MOR_L_KEPT);
}

static void t_g11_4c_mismatch(void)
{
    click("call K8 both", true, true, "40");
    mor_set *s = run();
    mor_k8 k;
    mor_k8_read(s, A(s, g_d), &k);
    CHECK(k.ties[1].status == MOR_MISMATCHED);
    CHECK(k.ties[1].doer_line->amount.n == 45 && k.ties[1].recv_line->amount.n == 40); /* both shown, neither chosen */
    const char *txt = show(s);
    CHECK(has(txt, "mismatched; doer sam states 45 EUR; receiving side bob states 40 EUR"));
    /* one side only, where bob says nothing */
    K.n = 0;
    click("call K8 both", true, true, NULL);
    s = run();
    mor_k8_read(s, A(s, g_d), &k);
    CHECK(k.ties[1].status == MOR_ONE_SIDE_ONLY);
}

static void t_g11_3_without_cmips(void)
{
    click("call K8 both", true, true, "45");
    g_opt.plugins = NULL; /* the verifier lacks every cMIP those Terms name */
    mor_set *s = run();
    mor_k8 k;
    mor_k8_read(s, A(s, g_d), &k);
    CHECK(k.n_reports == 3); /* still reads the leg reports */
    CHECK(k.reports[0]->n_lines > 0 && k.reports[0]->signer != NULL);
    CHECK(k.conforms == MOR_UNCHECKED); /* whether what was passed on is what the Terms require */
    CHECK(k.n_ties == 2 && k.ties[0].status == MOR_MATCHED); /* that does not make the leg report itself unchecked */
    CHECK(has(show(s), "stated by sam as doer") != false);
}

static void t_g11_4d_silence(void)
{
    click("call K8 both", true, false, NULL);
    mor_set *s = run(); /* a reader who is no side of that leg */
    mor_k8 k;
    mor_k8_read(s, A(s, g_d), &k);
    CHECK(k.call_status == MOR_NOT_SHOWN);
    g_opt.reader = "zed"; /* the buyer: still no side of the service's legs */
    s = run();
    mor_k8_read(s, A(s, g_d), &k);
    CHECK(k.call_status == MOR_NOT_SHOWN);
    mor_trace tr;
    mor_trace_payment(s, A(s, g_pay), &tr);
    CHECK(tr.n == 2 && tr.legs[1].status == MOR_NOT_SHOWN);
    g_opt.reader = "sam"; /* a side of that leg */
    s = run();
    mor_k8_read(s, A(s, g_d), &k);
    CHECK(k.call_status == MOR_UNANSWERED);
    mor_trace_payment(s, A(s, g_pay), &tr);
    CHECK(tr.n == 2 && tr.legs[1].status == MOR_UNANSWERED);
    /* Terms that hold the task and carry no call: no report called for */
    K.n = 0;
    click(NULL, true, false, NULL);
    s = run();
    mor_k8_read(s, A(s, g_d), &k);
    CHECK(k.holds == MOR_YES && !k.has_call && k.call_status == MOR_NO_REPORT_CALLED_FOR);
    mor_trace_payment(s, A(s, g_pay), &tr);
    CHECK(tr.n == 2 && tr.legs[1].status == MOR_NO_REPORT_CALLED_FOR);
}

static void t_g11_5_own_part(void)
{
    /* alice receives for herself and bob: she holds the task, and her own share is not a fee */
    mor_fp t = two("receiver signer alice|call K8 doer", "share signer alice 50|share signer bob 50", NULL, NULL);
    mor_fp s1 = rec(&K, "settlement", "bank", "amount 10 EUR");
    mor_fp s2 = rec(&K, "settlement", "bank", "amount 5 EUR");
    mor_fp r = rec(&K, "report", "alice", F("call %s|side doer|received 10 EUR %s|passed 5 EUR signer bob %s|own 5 EUR", hx(t), hx(s1), hx(s2)));
    mor_set *s = run();
    mor_k8 k;
    mor_k8_read(s, A(s, t), &k);
    CHECK(k.holds == MOR_YES && eq(k.doer, "alice"));
    const mor_act *rp = A(s, r);
    int own = 0, kept = 0;
    for (int i = 0; i < rp->n_lines; i++) {
        own += rp->lines[i].kind == MOR_L_OWN;
        kept += rp->lines[i].kind == MOR_L_KEPT;
    }
    CHECK(own == 1 && kept == 0);
    const char *txt = show(s);
    CHECK(has(txt, "the doer's own part (not a fee) 5 EUR") != false && has(txt, "kept (a fee)") == false);
}

static void t_g11_6_call(void)
{
    /* a call that tries to name a reader and a place: never read */
    click("call K8 doer to zed at somewhere", true, true, "45");
    mor_set *s = run();
    const mor_act *d = A(s, g_d);
    CHECK(d->n_calls == 1 && d->calls[0].unread == 1 && d->calls[0].from == MOR_FROM_DOER);
    mor_k8 k;
    mor_k8_read(s, d, &k);
    CHECK(k.has_call && k.from == MOR_FROM_DOER);
    const char *txt = show(s);
    CHECK(has(txt, "somewhere") == false && has(txt, "to zed") == false);
    /* a call is not read as expecting a report on a task held by other Terms than the ones that carry it */
    const mor_agreement *w = AG(s, g_w);
    mor_k8 kw;
    mor_k8_read(s, w->cur, &kw);
    CHECK(!kw.has_call && kw.n_reports == 0); /* the deal's call, and its reports, are not the work's */
    /* no call, field or answer carries a report beyond its leg; no one is read as having agreed to that */
    CHECK(k.path_beyond != MOR_SHOWN && kw.path_beyond != MOR_SHOWN);
    CHECK(has(txt, "path beyond the leg agreed by the signers (a data cMIP): unchecked") != false);
}

static void t_g11_4e_before_paying(void)
{
    click("call K8 both", false, false, NULL);
    mor_set *s = run();
    mor_before b;
    mor_before_paying(s, &g_w, &b);
    CHECK(b.n == 2);
    CHECK(b.e[0].terms == MOR_SHOWN && !b.e[0].k8.has_call);
    CHECK(b.e[1].terms == MOR_SHOWN && b.e[1].k8.has_call && b.e[1].k8.from == MOR_FROM_BOTH);
    CHECK(b.e[1].k8.in_force.qual.kind == MOR_Q_AS_SHOWN);
}

static void t_g11_8_contest(void)
{
    click("call K8 both", true, false, NULL);
    mor_set *s0 = run();
    const mor_act *w2 = AG(s0, g_w)->cur;
    mor_fp w2fp = w2->fp;
    mor_fp c = stmt(&K, "zed", g_w, w2fp, "contest", "none", F("settlement %s|content nothing-was-delivered", hx(g_s1)));
    g_opt.reader = "zed";
    mor_set *s = run();
    mor_contest ct;
    mor_contest_read(s, A(s, c), &ct);
    CHECK(ct.ans.ans == MOR_SHOWN);
    CHECK(eq(ct.said_by, "zed")); /* its signer's word (G8.1) */
    /* it reaches the signers of what it is made against, and no one else: not the service, not the deal */
    CHECK(ct.n_reaches == 2 && eq(ct.reaches[0], "alice") && eq(ct.reaches[1], "bob"));
    for (int i = 0; i < ct.n_reaches; i++)
        CHECK(!eq(ct.reaches[i], "sam"));
    CHECK(ct.buyers == MOR_SHOWN);
    /* it gives its signer no sight of any leg: the service's leg stays not shown to the buyer */
    mor_k8 k;
    mor_k8_read(s, A(s, g_d), &k);
    CHECK(k.call_status == MOR_NOT_SHOWN);
    /* a contest that names another version than the payment read is not a buyer's contest */
    mor_fp c2 = stmt(&K, "zed", g_w, g_w, "contest", hx(c), F("settlement %s", hx(g_s1)));
    s = run();
    mor_contest_read(s, A(s, c2), &ct);
    CHECK(ct.ans.ans == MOR_SHOWN && ct.buyers == MOR_NOT_SHOWN);
    CHECK(has(show(s), "said by zed") != false);
}

/* ============================ G14 ============================ */

static void t_g14(void)
{
    mor_fp env = rec(&K, "envelope", "sealer", "baker alice");
    /* (a) the baker is a party, bound */
    mor_fp t = two(F("envelope %s|receiver signer alice", hx(env)), NULL, NULL, NULL);
    /* a stranger's Terms for the same envelope: neither the baker nor a succession */
    mor_fp thief = terms(&K, "mallory", "none", F("party signer mallory|author signer mallory|receiver signer mallory|envelope %s", hx(env)), NULL);
    assent(&K, "mallory", thief, thief, hx(thief));
    /* names the baker, who has not signed */
    mor_fp waiting = terms(&K, "carol", "none", F("party signer carol|party signer alice|author signer carol|envelope %s|in_force %s", hx(env), stub("k4.yes")), NULL);
    /* the envelope is not held */
    mor_fp lost = terms(&K, "dave", "none", F("party signer dave|author signer dave|envelope %s", stub("no such envelope")), NULL);
    mor_set *s = run();
    mor_answer a;
    const char *baker = NULL;
    mor_work_read(s, A(s, t), &a, &baker);
    CHECK(a.ans == MOR_SHOWN && baker && eq(baker, "alice")); /* the baker as the envelope states it */
    mor_work_read(s, A(s, thief), &a, &baker);
    CHECK(a.ans == MOR_INVALID);
    mor_work_read(s, A(s, waiting), &a, &baker);
    CHECK(a.ans == MOR_NOT_SHOWN);
    mor_work_read(s, A(s, lost), &a, &baker);
    CHECK(a.ans == MOR_UNCHECKED);
    /* given with every answer about that work that rests on those Terms: a receiver */
    mor_receiver r;
    mor_receiver_for(s, &thief, NULL, &r);
    CHECK(r.ans.ans == MOR_SHOWN && r.has_work && r.work.ans == MOR_INVALID);
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.has_work && r.work.ans == MOR_SHOWN);
    /* ... a leg report */
    mor_k8 k;
    mor_k8_read(s, A(s, thief), &k);
    CHECK(k.has_work && k.work.ans == MOR_INVALID);
    CHECK(has(show(s), "they cannot count as an agreement for that work") != false);
}

static void t_g14_succession_and_spot(void)
{
    mor_fp env = rec(&K, "envelope", "sealer", "baker alice");
    mor_fp al;
    mor_fp t = two(F("envelope %s", hx(env)), NULL, &al, NULL);
    /* alice passes the baker's rights to carol (stub kind: P18 does not say what counts as passing) */
    stmt(&K, "alice", t, t, stub("k11.pass-rights"), hx(al), "content carol");
    mor_fp next = terms(&K, "carol", "none",
                        F("party signer carol|party role buyer received %s %s many|author signer carol|envelope %s|relies_on %s|in_force %s 1 signer carol",
                          stub("thing"), stub("k12.claim"), hx(env), hx(t), stub("k4.threshold")),
                        NULL);
    assent(&K, "carol", next, next, hx(next));
    stmt(&K, "zed", next, next, stub("k11.claim"), "none", "content buyer");
    mor_set *s = run();
    mor_answer a;
    mor_work_read(s, A(s, next), &a, NULL);
    CHECK(a.ans == MOR_SHOWN && has(a.why, "succession"));
    /* ... given with a filling */
    mor_spot sp;
    mor_spot_read(s, &next, "buyer", &sp);
    CHECK(sp.n == 1 && sp.has_work && sp.work.ans == MOR_SHOWN);
    /* without the K11 cMIP that says what passes them: unchecked */
    without("k11.pass-rights");
    s = run();
    mor_work_read(s, A(s, next), &a, NULL);
    CHECK(a.ans == MOR_UNCHECKED);
    /* a succession named, whose first agreement is not held: unchecked */
    K.n = 0;
    begin();
    env = rec(&K, "envelope", "sealer", "baker alice");
    next = terms(&K, "carol", "none", F("party signer carol|author signer carol|envelope %s|relies_on %s", hx(env), stub("gone")), NULL);
    s = run();
    mor_work_read(s, A(s, next), &a, NULL);
    CHECK(a.ans == MOR_UNCHECKED);
}

/* ============================ tasks ============================ */

static void t_k13(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    assent(&K, "alice", t, t, hx(t));
    stmt(&K, "alice", t, t, "contest", hx(t), NULL);
    mor_set *s = run();
    mor_k13_out k;
    CHECK(mor_standing(s, "alice", &k) == MOR_YES && k.forks_standing == 1);
    without("k13.count");
    CHECK(mor_standing(s, "alice", &k) == MOR_CANNOT);
}


/* ============================ readings chosen (QUESTIONS.md) ============================ */

static void t_q10_no_hiding_a_fork(void)
{
    /* an act that names resolves, and resolves nothing here, is an act like any other: a tip */
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a1 = assent(&K, "alice", t, t, hx(t));
    mor_fp a2 = stmt(&K, "alice", t, t, "contest", hx(t), F("resolves %s|resolves %s|continues %s", stub("x"), stub("y"), stub("x")));
    mor_set *s = run();
    CHECK(mor_n_line_forks(s) == 1 && !mor_line_fork_at(s, 0)->resolved);
    CHECK((A(s, a1)->ev.line & MOR_M_TIP) && (A(s, a2)->ev.line & MOR_M_TIP));
}

static void t_q31_three_tips(void)
{
    mor_fp t = terms(&K, "alice", "none", P2, NULL);
    mor_fp a1 = assent(&K, "alice", t, t, hx(t));
    mor_fp a2 = stmt(&K, "alice", t, t, "contest", hx(t), "content two");
    mor_fp a3 = stmt(&K, "alice", t, t, "contest", hx(t), "content three");
    /* a resolving update that names only two of the three resolves nothing */
    mor_fp r = stmt(&K, "alice", t, t, "contest", hx(t), F("resolves %s|resolves %s|continues %s", hx(a1), hx(a2), hx(a1)));
    mor_set *s = run();
    CHECK(mor_n_line_forks(s) == 1 && mor_line_fork_at(s, 0)->n_tips == 4 && !mor_line_fork_at(s, 0)->resolved);
    (void)r;
    K.n = 0;
    t = terms(&K, "alice", "none", P2, NULL);
    a1 = assent(&K, "alice", t, t, hx(t));
    a2 = stmt(&K, "alice", t, t, "contest", hx(t), "content two");
    a3 = stmt(&K, "alice", t, t, "contest", hx(t), "content three");
    stmt(&K, "alice", t, t, "contest", hx(t), F("resolves %s|resolves %s|resolves %s|continues %s", hx(a1), hx(a2), hx(a3), hx(a1)));
    s = run();
    CHECK(mor_line_fork_at(s, 0)->n_tips == 3 && mor_line_fork_at(s, 0)->resolved && mor_line_fork_at(s, 0)->continued == A(s, a1));
    CHECK((A(s, a2)->ev.line & MOR_M_SET_ASIDE) && (A(s, a3)->ev.line & MOR_M_SET_ASIDE));
}

static void t_q16_receiver_is_a_spot(void)
{
    mor_fp t = offer("many", "receiver role buyer");
    mor_set *s = run();
    mor_receiver r;
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.ans.ans == MOR_NOT_SHOWN); /* the spot named is not filled */
    stmt(&K, "zed", t, t, stub("k11.claim"), "none", "content buyer");
    s = run();
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.ans.ans == MOR_SHOWN && eq(r.who, "zed"));
    stmt(&K, "yan", t, t, stub("k11.claim"), "none", "content buyer");
    s = run();
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.ans.ans == MOR_NOT_SHOWN && has(r.ans.why, "several"));
    without("k12.claim");
    s = run();
    mor_receiver_for(s, &t, NULL, &r);
    CHECK(r.ans.ans == MOR_UNCHECKED);
}

static void t_q13_settlement_not_held(void)
{
    mor_fp t = two("receiver signer alice", NULL, NULL, NULL);
    mor_fp gone = mor_stub_fp("a settlement not held");
    mor_fp pay = rec(&K, "payment", "zed", F("agreement %s|version %s|settlement %s|amount 5 EUR", hx(t), hx(t), hx(gone)));
    rec(&K, "report", "alice", F("call %s|side receiving|received 5 EUR %s", hx(gone), hx(gone)));
    mor_set *s = run();
    mor_payment p;
    mor_payment_read(s, A(s, pay), &p);
    CHECK(p.status == MOR_UNCHECKED && p.confirmation != NULL); /* what each side states is still held and shown */
}

/* ============================ the table ============================ */

static const struct {
    void (*fn)(void);
    const char *name;
    const char *cite; /* sentences of the text, quoted; several are separated by " || " */
} tests[] = {
    {t_required_fields, "s3.required", "An act missing a field marked required is not an Agreements act. || A report that names no call is no report."},
    {t_signature_standin, "s2.act", "**Act**: a signed statement in the core's sense."},
    {t_g1_2_one_level, "G1.2.one-level",
     "these grammar rules MUST NOT be read as making an agreement's signing need the signature of anyone inside an agreement it names, unless its own Terms say so."},
    {t_g1_2_unchecked_without_its_terms, "G1.2.unchecked",
     "A verifier reads what they require from that agreement's Terms in force (G4.6); where it lacks them, the answer is **unchecked** (G9)."},
    {t_g1_2_agreement_signature, "G1.2.signature",
     "An act is signed by an agreement x when its `for` names x, by its root; its `under` names Terms of x that are in force (G4.6) and give a power for acts of that kind; and that power's rule, applied to the acts held, says yes."},
    {t_g1_4_no_version, "G1.4.no-version",
     "Where that act names no version, a verifier MUST take the receiver from that agreement's Terms in force, per that agreement's own place, qualified **per place** with that place named, or, where it names none, per the anchoring the reader accepts, or **as shown** (G4.6, G7, G13.4)."},
    {t_g1_4_per_place, "G1.4.per-place",
     "Where that act names no version, a verifier MUST take the receiver from that agreement's Terms in force, per that agreement's own place, qualified **per place** with that place named"},
    {t_g1_4_fork, "G1.4.fork",
     "while a fork of its Terms stands, the receiver is the one named by the Terms both branches replace, and the verifier MUST show the fork with that answer and MUST NOT choose a branch (G4.7)."},
    {t_g1_4_stops, "G1.4.stops",
     "If a version read names no receiver, or the spot named is not filled, or an agreement is met a second time on the way, no one receives, and the answer is **not shown**. || If the verifier lacks that agreement's acts, it is **unchecked** (G9)."},
    {t_g1_4_through_an_agreement, "G1.4.through",
     "a verifier reads that agreement's receiver in the same way, and so on, until one of an agreement's own parties is reached."},
    {t_g2_1, "G2.1", "The `body`, every piece in `relies_on`, and every cMIP named anywhere in a Terms act MUST be named by fingerprint."},
    {t_g2_2, "G2.2", "A verifier MUST NOT read it as part of them."},
    {t_g3_1, "G3.1", "`authors` MUST name one or more of the parties named as signers or roles."},
    {t_g3_2, "G3.2", "A cMIP answering task K10 (Judge) MUST accept `authors` as an input."},
    {t_g4_1_default, "G4.1", "If `in_force` is absent, they are in force when there is an assent to them in the name of every party named as a signer."},
    {t_g4_2, "G4.2", "Terms that name a party by role MUST carry `in_force`."},
    {t_g4_3, "G4.3", "An assent binds its signer only while the Terms it names are in force."},
    {t_g4_6_qualifiers, "G4.6",
     "A verifier MUST take it per the agreement's place, qualified **per place** (G7.2); or, where no place is named, per the anchoring the reader accepts, with it named, or else **as shown** (G7.3, G13.4). || It MUST NOT give it as latest without saying what the answer stands on."},
    {t_g4_7_fork, "G4.7.fork", "A verifier MUST answer **fork** for which Terms replace them, show both tips, and MUST NOT choose between them, by position or in any other way."},
    {t_g4_7_answers_from_shared, "G4.7.shared",
     "A verifier MUST answer from them every answer that rests on the Terms in force: among others, a receiver for an act (G1.4), who fills a spot (G10.2), and what a rule the Terms name produces (G5, task K4). || It MUST show the fork with each such answer, per place or as shown."},
    {t_g4_8_not_shown, "G4.8", "A verifier answers **not shown** for it as a replacement, per place or as shown: an assent it does not hold may exist"},
    {t_g4_9_never_ended, "G4.9", "No act ends Terms, or an agreement, without replacing them, and a verifier MUST NOT answer that Terms or an agreement have ended."},
    {t_g4_10_counter_signature, "G4.10.cancel", "It cancels that assent, and no one else's: an assent so cancelled is not counted by G4.1. || A later assent by the same signer to the same Terms is counted as any assent is."},
    {t_g4_10_after_in_force, "G4.10.after", "One made for Terms that are in force changes nothing: Terms in force change only as G4.5 and G4.9 say."},
    {t_g4_10_race, "G4.10.race", "Whether the Terms were in force before the counter signature is an answer about what is latest: per place, or as shown (G4.6)."},
    {t_g4_11_not_valid, "G4.11.c",
     "A verifier MUST answer **not valid** for it, and MUST name the condition beside that answer, with the ground the cMIP gives: an act, reported by who signed it (G8), or a time, per the clock the agreement names (G7.4), or, where it names none, per the anchoring named or as shown (G13.4)."},
    {t_g4_11_time_and_bytes, "G4.11.time",
     "A verifier MUST answer **not valid** for it, and MUST name the condition beside that answer, with the ground the cMIP gives || A verifier MUST NOT give, per a reader's own anchoring, an answer that the place or the clock the agreement names gives."},
    {t_g4_11_valid_needs_signatures, "G4.11.d",
     "The update then replaces the version in force only if it also carries the signatures G4.8 asks for. || Where the verifier lacks the cMIP, whether the update replaces is **unchecked** (G9), and so is every answer that rests on it."},
    {t_g5_3, "G5.3", "It MUST NOT later be counted as made for anyone else."},
    {t_g5_4, "G5.4",
     "Where whoever is bound through `for` is bound only through Terms that have been replaced, a verifier MUST answer from G7: per place if the Terms name one, otherwise as shown."},
    {t_g5_5_default, "G5.5.default", "By default the delegator can end it at any time: a version of those Terms that removes or narrows the power replaces them (G4.8) when it carries the delegator's part alone, without the agent's."},
    {t_g5_5_agent_only, "G5.5.agent",
     "A verifier MUST NOT answer that such a delegation can never be ended: whether the agent will end it is **not shown**, and the verifier shows the condition as the Terms the delegator signed state it."},
    {t_g5_2_power_for_replacement, "G5.2",
     "A replacing Terms act (one with `replaces`) binds a party only through G5.1: by that party's own assent to it, or by a power for replacement in the Terms it replaces. || a verifier reads a member who was outvoted as that rule says, and MUST NOT answer, beyond it, whether that member agreed."},
    {t_g6_1, "G6.1", "Every act within an agreement MUST name in `previous` its signer's previous act within the same agreement, or none if it is the signer's first."},
    {t_g6_2, "G6.2", "A verifier holding both MUST answer **fork** for that signer, and MUST NOT choose between the tips."},
    {t_g12_2_built_on, "G12.2",
     "A verifier MUST show both tips, and what it holds that is built on them, with the fork; || MUST answer **fork** where G4.7 and G6.2 say; and MUST NOT choose between the tips, by position or in any other way. || A client that holds both tips of a fork MUST NOT show either tip, or anything built on either, as a resolved act: not as the next version, not as in force, not as that signer's latest act."},
    {t_g12_3_resolve_terms, "G12.3/G12.5", "A verifier MUST show that act, and every answer resting on it, as **disputed**. || An act that names `resolves` and no `continues` resolves nothing."},
    {t_g12_4_power_to_resolve, "G12.4", "A verifier MUST report such a resolving update with the power it was signed under and whoever it rests on (G5.1, G8)."},
    {t_g12_3_resolve_line, "G12.3.line", "It resolves the fork only when it is signed as an update of the shared act must be: for one signer's line, by that signer (G6)"},
    {t_g12_7_rivals, "G12.7", "Neither resolves the first fork while the other stands: every tip stays stopped (G12.2), and the shared act stays the last act of the line that counts."},
    {t_g7_2, "G7.2", "A verifier MUST take those answers from the place, qualified **per place**."},
    {t_g7_3, "G7.3",
     "Where no place is named, a verifier MUST qualify every such answer **as shown**, or **per anchoring** where the reader accepts one, with it named (G13.4). || It MUST NOT give such an answer as latest or whole without saying what it stands on."},
    {t_g7_4, "G7.4", "a verifier MUST NOT give an answer of G7.2 for that agreement per another anchoring."},
    {t_g13_2, "G13.2", "A verifier MUST NOT read an act as having relied on a version or act it does not name."},
    {t_g13_3, "G13.3", "An act that names another, directly or through acts that name it, came after it."},
    {t_g13_4, "G13.4", "A verifier MUST give each such answer **per anchoring**, with the anchoring named (**per place** or **per clock** where it is the one the Terms name), or **as shown**."},
    {t_g9_1, "G9.1",
     "Where a verifier cannot apply a rule it needs, because it lacks a cMIP named by fingerprint, lacks an act named by fingerprint, lacks the acts of an agreement named as a party, or cannot get an answer from a named place, it MUST answer **unchecked** for every answer that depends on it."},
    {t_g10_1, "G10.1", "A role in `parties` MUST name:"},
    {t_g10_2, "G10.2/G8.4", "A verifier MUST answer each filling with its proof, each act reported by who signed it (G8.4), qualified per place or as shown (G7). || A verifier MUST report them with whoever confirmed them."},
    {t_g10_5, "G10.5", "A verifier MUST give that answer with its cause, as the cMIP gives it, per place, per clock, per the anchoring named or as shown (G7, G13.4)."},
    {t_g10_6, "G10.6",
     "A verifier MUST NOT read a later version as changing a filling's rights under an update condition which that filling's own version does not name. || A verifier MUST name, with each filling, the version it was earned under, and each later version that changes its rights, with what that rests on: the holder's own assent, or the condition named and its answer."},
    {t_g10_6b_and_7, "G10.6b/G10.7",
     "No condition triggers: these grammar rules have no answer **triggered**, and none that a filling's rights **no longer apply**, and a verifier MUST NOT give either. || a verifier reads them from there (G2) and MUST NOT answer them of itself."},
    {t_g11_7_payment, "G11.7",
     "As for any two halves, a verifier shows a payment so confirmed as **matched**, one whose confirmation states another amount as **mismatched**, and one the receiving side has not confirmed as **one side only**; it MUST NOT show a payment as received on the buyer's word alone (G8.3)."},
    {t_g8_3, "G8.3/G8.1",
     "A verifier MUST NOT report value as moved on the strength of an Agreements act alone. || a verifier MUST report who stated it: the one who performed, the one who received, a named witness, the controller, or the place."},
    {t_g11_legs, "G11.4",
     "A verifier asked about a result that depends on a sequence MUST read it leg by leg, from the reports it holds (G11.6), and give each leg's part with who signed it (G8.4)."},
    {t_g11_4c_mismatch, "G11.4c", "Where they do not, the verifier MUST show both, with the amount each states, and MUST NOT choose between them."},
    {t_g11_3_without_cmips, "G11.3",
     "What K8's row asks of a leg report MUST be readable without any cMIP those Terms name: a verifier that lacks them still reads the leg reports, and reports what they state as stated by their signers."},
    {t_g11_4d_silence, "G11.4d",
     "A verifier that does not, because that leg's reports travel no further than the leg (G11.6), MUST NOT show the call as unanswered: for it, that leg's reports are **not shown**. || As the block asks, the verifier shows a call with no report as **unanswered**, and Terms that hold the Control task and carry no call as **no report called for**."},
    {t_g11_5_own_part, "G11.5", "Where the doer is itself one to whom those Terms give a part, its report states that part as its own, and a verifier MUST NOT read it as kept (K8's row)."},
    {t_g11_6_call, "G11.6",
     "A call MUST NOT be read as naming a place, or a reader. || A call MUST NOT be read as expecting a report on a task held by other Terms than the ones that carry it. || no call, no field and no answer of this text carries a report beyond its leg, and a verifier MUST NOT read any as doing so. || A verifier MUST NOT read the people of a leg as having agreed to be read from beyond their leg unless Terms they signed say so (G5.1)"},
    {t_g11_4e_before_paying, "G11.4e", "Asked which agreements of a sequence carry a call for reports, a verifier answers from their Terms alone (G11.2), one agreement at a time, per place or as shown (G4.6), and **unchecked** where it lacks them."},
    {t_g11_8_contest, "G11.8",
     "A verifier MUST NOT read a contest as contesting anyone but the signers of what it is made against: not a service those signers picked, not a deal of theirs, not any other agreement of the sequence. || It MUST NOT read a contest as giving its signer sight of any leg (G11.6, G11.7)."},
    {t_g14, "G14.3",
     "A verifier asked whether Terms count as an agreement for a baked work MUST read the envelope, the baker it states, and, under (b), every agreement of the succession. || A verifier MUST give that answer with every answer about that work that rests on those Terms (a receiver, a filling, a leg report)."},
    {t_g14_succession_and_spot, "G14.2b", "one of their parties, bound by them, holds the baker's rights through a **succession**"},
    {t_q10_no_hiding_a_fork, "Q10.hidden", "Two acts by one signer within one agreement that name the same `previous` (or that both name none) are a fork."},
    {t_q31_three_tips, "Q31.tips", "which names every tip in `resolves`; and which names one of them in `continues`."},
    {t_q16_receiver_is_a_spot, "Q16.spot", "If a version read names no receiver, or the spot named is not filled, or an agreement is met a second time on the way, no one receives, and the answer is **not shown**."},
    {t_q13_settlement_not_held, "Q13.settlement", "Where the verifier lacks an act a report names, or the Terms that hold a leg's task, that part is **unchecked** (G9)."},
    {t_k13, "K13", "whatever measures the cMIP defines (proven forks, acknowledgments given and received, spots filled), computed from signed acts only, qualified per place or as shown"},
};

int main(int argc, char **argv)
{
    int n = (int)(sizeof tests / sizeof tests[0]);
    if (argc > 1 && eq(argv[1], "--cites")) {
        for (int i = 0; i < n; i++)
            printf("%s\t%s\n", tests[i].name, tests[i].cite);
        return 0;
    }
    const char *dump = argc > 2 && eq(argv[1], "--dump") ? argv[2] : NULL;
    kit_init(&K);
    if (argc > 2 && eq(argv[1], "--examples")) {
        /* the sets kept under examples/, for people and for tests/cli.sh */
        static const char *const names[] = {"click", "fork", "delegation", "offer"};
        for (int i = 0; i < 4; i++) {
            char path[512];
            begin();
            if (i == 0)
                click("call K8 both", true, true, "40");
            else if (i == 1) {
                mor_fp t = fork_of_terms(NULL);
                stmt(&K, "zed", t, g_tip1, "contest", "none", NULL);
            } else if (i == 2)
                delegation("agent", NULL, NULL);
            else {
                mor_fp t = offer("many max=2", F("condition %s", stub("k4.same-parties")));
                stmt(&K, "zed", t, t, stub("k11.claim"), "none", "content buyer");
            }
            snprintf(path, sizeof path, "%s/%s.mor", argv[2], names[i]);
            FILE *f = fopen(path, "wb");
            if (f) {
                fprintf(f, "# MOR Agreements draft 17: an example set in the stand-in act format (STANDINS.md).\n");
                fprintf(f, "# Made by build/test-san --examples; read it with: mor-agreements verify examples/%s.mor\n\n", names[i]);
                fwrite(K.buf, 1, K.n, f);
                fclose(f);
            }
            for (int k = 0; k < g_nsets; k++)
                mor_set_free(g_sets[k]);
            g_nsets = 0;
        }
        kit_free(&K);
        return 0;
    }
    for (int i = 0; i < n; i++) {
        int before = g_fail;
        begin();
        tests[i].fn();
        printf("%s %s\n", g_fail == before ? "ok  " : "FAIL", tests[i].name);
        if (dump) {
            char path[512];
            snprintf(path, sizeof path, "%s/%02d.mor", dump, i);
            FILE *f = fopen(path, "wb");
            if (f) {
                fwrite(K.buf, 1, K.n, f);
                fclose(f);
            }
        }
        for (int k = 0; k < g_nsets; k++)
            mor_set_free(g_sets[k]);
        g_nsets = 0;
    }
    kit_free(&K);
    printf("%d tests, %d checks, %d failed\n", n, g_checks, g_fail);
    return g_fail ? 1 : 0;
}
