/* decode.c - the decoder of untrusted bytes: the stand-in act format (STANDINS.md, S1).
 *
 * A record is lines of printable ASCII, each ended by '\n':
 *     mor-act 1
 *     type <type>
 *     signer <identifier>
 *     <field> <value...>          (any number, in any order)
 *     sig <64 hex>
 * Its fingerprint is SHA-256 of every byte from "mor-act 1" up to, not including, the "sig" line.
 * Between records, blank lines and lines starting with '#' are skipped.
 * A record that is malformed, or lacks a field section 3 marks required, is rejected with a
 * reason ("not an Agreements act"); it never reaches the grammar.
 */
#include "internal.h"

#include <string.h>

typedef struct {
    const char *p;
    size_t n;
} tok;

#define MAXTOK 8

static bool tok_is(const tok *t, const char *s)
{
    size_t n = strlen(s);
    return t->n == n && memcmp(t->p, s, n) == 0;
}

/* split into at most max tokens; the last one takes the rest of the line */
static int split(const char *p, size_t n, tok *t, int max)
{
    int k = 0;
    size_t i = 0;
    while (i < n && k < max) {
        size_t j = i;
        if (k == max - 1)
            j = n;
        else
            while (j < n && p[j] != ' ')
                j++;
        t[k].p = p + i;
        t[k].n = j - i;
        k++;
        i = j + 1;
    }
    return k;
}

static bool line_ok(const char *p, size_t n)
{
    if (n == 0 || n > MOR_MAX_LINE || p[0] == ' ' || p[n - 1] == ' ')
        return false;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)p[i];
        if (c < 0x20 || c > 0x7e)
            return false;
        if (c == ' ' && i + 1 < n && p[i + 1] == ' ')
            return false;
    }
    return true;
}

static bool id_ok(const tok *t)
{
    if (t->n == 0 || t->n > 48 || tok_is(t, "none"))
        return false;
    for (size_t i = 0; i < t->n; i++) {
        char c = t->p[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!ok)
            return false;
    }
    return true;
}

static bool get_fp(const tok *t, mor_fp *fp)
{
    return mor_fp_parse(t->p, t->n, fp);
}

static bool get_u64(const tok *t, uint64_t *out)
{
    if (t->n == 0 || t->n > 18 || (t->n > 1 && t->p[0] == '0'))
        return false;
    uint64_t v = 0;
    for (size_t i = 0; i < t->n; i++) {
        if (t->p[i] < '0' || t->p[i] > '9')
            return false;
        v = v * 10 + (uint64_t)(t->p[i] - '0');
    }
    *out = v;
    return true;
}

static bool get_amount(mor_set *s, const tok *num, const tok *unit, mor_amount *a)
{
    if (!get_u64(num, &a->n) || unit->n == 0 || unit->n > 8)
        return false;
    for (size_t i = 0; i < unit->n; i++) {
        char c = unit->p[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
            return false;
    }
    a->unit = mor_strdup_n(s, unit->p, unit->n);
    return true;
}

/* "signer <id>" | "role <name>" | "agreement <fp>" */
static bool get_ref(mor_set *s, const tok *k, const tok *v, mor_ref *r, bool allow_role)
{
    memset(r, 0, sizeof *r);
    if (tok_is(k, "signer") && id_ok(v)) {
        r->kind = MOR_REF_SIGNER;
        r->name = mor_strdup_n(s, v->p, v->n);
        return true;
    }
    if (allow_role && tok_is(k, "role") && id_ok(v)) {
        r->kind = MOR_REF_ROLE;
        r->name = mor_strdup_n(s, v->p, v->n);
        return true;
    }
    if (tok_is(k, "agreement") && get_fp(v, &r->root)) {
        r->kind = MOR_REF_AGREEMENT;
        return true;
    }
    return false;
}

/* a cMIP by fingerprint, with its parameters: "<fp> [params...]" */
static void get_rule(mor_set *s, const tok *t, int n, mor_rule *r)
{
    r->present = true;
    r->params = "";
    if (n < 1 || !get_fp(&t[0], &r->cmip)) {
        r->bad = true; /* named, but not by fingerprint: G2.1 */
        return;
    }
    if (n >= 2)
        r->params = mor_strdup_n(s, t[1].p, t[1].n);
}

typedef struct {
    mor_party parties[MOR_MAX_LIST];
    mor_ref authors[MOR_MAX_LIST];
    mor_fp relies[MOR_MAX_LIST], resolves[MOR_MAX_LIST], answers[MOR_MAX_LIST];
    mor_power powers[MOR_MAX_LIST];
    mor_call calls[MOR_MAX_LIST];
    mor_rline lines[MOR_MAX_LIST];
    const char *data[MOR_MAX_LIST];
    mor_fp entries[MOR_MAX_ENTRIES];
    bool seen_previous, seen_kind, seen_content, seen_call, seen_side, seen_now, seen_place, seen_clock;
} scratch;

#define FAIL(msg)                                                                                                             \
    do {                                                                                                                      \
        *why = (msg);                                                                                                         \
        return false;                                                                                                         \
    } while (0)
#define ONCE(flag, msg)                                                                                                       \
    do {                                                                                                                      \
        if (flag)                                                                                                             \
            FAIL(msg);                                                                                                        \
        (flag) = true;                                                                                                        \
    } while (0)

static bool field_common(mor_act *a, scratch *sc, const tok *t, int n, const char **why, bool *taken)
{
    *taken = true;
    if (tok_is(&t[0], "previous")) {
        ONCE(sc->seen_previous, "previous given twice");
        if (n != 2)
            FAIL("previous: one value");
        if (tok_is(&t[1], "none"))
            a->has_previous = false;
        else if (get_fp(&t[1], &a->previous))
            a->has_previous = true;
        else
            FAIL("previous: a fingerprint or none");
    } else if (tok_is(&t[0], "agreement")) {
        ONCE(a->has_agreement, "agreement given twice");
        if (n != 2 || !get_fp(&t[1], &a->agreement))
            FAIL("agreement: a fingerprint");
    } else if (tok_is(&t[0], "continues")) {
        ONCE(a->has_continues, "continues given twice");
        if (n != 2 || !get_fp(&t[1], &a->continues))
            FAIL("continues: a fingerprint");
    } else {
        *taken = false;
    }
    return true;
}

static bool field_terms(mor_set *s, mor_act *a, scratch *sc, const char *lp, size_t ln, const char **why)
{
    tok t[MAXTOK];
    int n = split(lp, ln, t, 3);
    bool taken;
    /* resolves is repeated: keep its own slot handling simple */
    if (tok_is(&t[0], "resolves")) {
        mor_fp f;
        if (n != 2 || !get_fp(&t[1], &f) || a->n_resolves >= MOR_MAX_LIST)
            FAIL("resolves: a fingerprint");
        sc->resolves[a->n_resolves++] = f;
        return true;
    }
    if (!field_common(a, sc, t, n, why, &taken))
        return false;
    if (taken)
        return true;
    if (tok_is(&t[0], "replaces")) {
        ONCE(a->has_replaces, "replaces given twice");
        if (n != 2 || !get_fp(&t[1], &a->replaces))
            FAIL("replaces: a fingerprint");
    } else if (tok_is(&t[0], "party")) {
        tok u[MAXTOK];
        int m = split(lp, ln, u, 7);
        if (a->n_parties >= MOR_MAX_LIST)
            FAIL("too many parties");
        mor_party *p = &sc->parties[a->n_parties];
        memset(p, 0, sizeof *p);
        if (m == 3 && !tok_is(&u[1], "role")) {
            if (!get_ref(s, &u[1], &u[2], &p->ref, false))
                FAIL("party: signer <id> or agreement <root>");
        } else if (m >= 3 && tok_is(&u[1], "role")) {
            /* party role <name> <kind> <what|-> <K12 cMIP> [params]; what is missing is read by G10.1, not here */
            if (!id_ok(&u[2]))
                FAIL("party role: a name");
            p->ref.kind = MOR_REF_ROLE;
            p->ref.name = mor_strdup_n(s, u[2].p, u[2].n);
            p->role_kind = MOR_ROLE_BAD;
            if (m >= 4) {
                static const char *const kinds[] = {"done", "received", "holding", "fits", "future"};
                for (int i = 0; i < 5; i++)
                    if (tok_is(&u[3], kinds[i]))
                        p->role_kind = (mor_role_kind)i;
            }
            if (m >= 5 && !tok_is(&u[4], "-")) {
                p->has_what = true;
                p->what_bad = !get_fp(&u[4], &p->what);
            }
            if (m >= 6)
                get_rule(s, &u[5], m - 5, &p->k12);
        } else {
            FAIL("party: signer <id>, role <name> ..., or agreement <root>");
        }
        a->n_parties++;
    } else if (tok_is(&t[0], "author")) {
        if (n != 3 || a->n_authors >= MOR_MAX_LIST || !get_ref(s, &t[1], &t[2], &sc->authors[a->n_authors], true))
            FAIL("author: signer <id>, role <name> or agreement <root>");
        a->n_authors++;
    } else if (tok_is(&t[0], "body")) {
        ONCE(a->has_body, "body given twice");
        if (n != 3)
            FAIL("body: the words and the K1 cMIP");
        tok u[MAXTOK];
        int m = split(lp, ln, u, 4);
        a->body_bad = (m != 3) || !get_fp(&u[1], &a->body_words) || !get_fp(&u[2], &a->body_cmip);
    } else if (tok_is(&t[0], "relies_on")) {
        if (n != 2)
            FAIL("relies_on: one value");
        mor_fp f;
        if (!get_fp(&t[1], &f))
            a->n_relies_bad++; /* referred to in another way: not part (G2.2) */
        else if (a->n_relies_on >= MOR_MAX_LIST)
            FAIL("too many relies_on");
        else
            sc->relies[a->n_relies_on++] = f;
    } else if (tok_is(&t[0], "in_force")) {
        ONCE(a->in_force.present, "in_force given twice");
        get_rule(s, &t[1], n - 1, &a->in_force);
    } else if (tok_is(&t[0], "condition")) {
        ONCE(a->condition.present, "condition given twice");
        get_rule(s, &t[1], n - 1, &a->condition);
    } else if (tok_is(&t[0], "power")) {
        tok u[MAXTOK];
        int m = split(lp, ln, u, 4);
        if (m < 2 || a->n_powers >= MOR_MAX_LIST)
            FAIL("power: a kind and a rule");
        mor_fp f;
        bool ok = tok_is(&u[1], "assent") || tok_is(&u[1], "counter_signature") || tok_is(&u[1], "contest") ||
                  tok_is(&u[1], "replaces") || tok_is(&u[1], "resolves") || get_fp(&u[1], &f);
        if (!ok)
            FAIL("power: unknown kind");
        mor_power *p = &sc->powers[a->n_powers++];
        memset(p, 0, sizeof *p);
        p->kind = mor_strdup_n(s, u[1].p, u[1].n);
        get_rule(s, &u[2], m - 2, &p->rule);
    } else if (tok_is(&t[0], "place")) {
        ONCE(sc->seen_place, "place given twice");
        if (n != 2 || !id_ok(&t[1]))
            FAIL("place: an identifier");
        a->place = mor_strdup_n(s, t[1].p, t[1].n);
    } else if (tok_is(&t[0], "clock")) {
        ONCE(sc->seen_clock, "clock given twice");
        if (n != 2 || !id_ok(&t[1]))
            FAIL("clock: an identifier");
        a->clock = mor_strdup_n(s, t[1].p, t[1].n);
    } else if (tok_is(&t[0], "receiver")) {
        if (a->receiver.kind != MOR_REF_NONE)
            FAIL("receiver given twice");
        if (n != 3 || !get_ref(s, &t[1], &t[2], &a->receiver, true))
            FAIL("receiver: signer <id>, role <name> or agreement <root>");
    } else if (tok_is(&t[0], "envelope")) {
        ONCE(a->has_envelope, "envelope given twice");
        if (n != 2 || !get_fp(&t[1], &a->envelope))
            FAIL("envelope: a fingerprint");
    } else if (tok_is(&t[0], "call")) {
        tok u[MAXTOK];
        int m = split(lp, ln, u, 4);
        if (m < 3 || a->n_calls >= MOR_MAX_LIST || !id_ok(&u[1]))
            FAIL("call: a task and from whom");
        mor_call *c = &sc->calls[a->n_calls];
        memset(c, 0, sizeof *c);
        c->task = mor_strdup_n(s, u[1].p, u[1].n);
        if (tok_is(&u[2], "doer"))
            c->from = MOR_FROM_DOER;
        else if (tok_is(&u[2], "receiving"))
            c->from = MOR_FROM_RECEIVING;
        else if (tok_is(&u[2], "both"))
            c->from = MOR_FROM_BOTH;
        else
            FAIL("call: from doer, receiving or both");
        c->unread = m > 3 ? 1 : 0; /* a call names no place and no reader: never read (G11.6) */
        a->n_calls++;
    } else {
        a->unread++; /* G2.2: not part of these Terms */
    }
    return true;
}

static bool field_statement(mor_set *s, mor_act *a, scratch *sc, const char *lp, size_t ln, const char **why)
{
    tok t[MAXTOK];
    int n = split(lp, ln, t, 3);
    bool taken;
    if (tok_is(&t[0], "resolves")) {
        mor_fp f;
        if (n != 2 || !get_fp(&t[1], &f) || a->n_resolves >= MOR_MAX_LIST)
            FAIL("resolves: a fingerprint");
        sc->resolves[a->n_resolves++] = f;
        return true;
    }
    if (!field_common(a, sc, t, n, why, &taken))
        return false;
    if (taken)
        return true;
    if (tok_is(&t[0], "terms")) {
        ONCE(a->has_terms, "terms given twice");
        if (n != 2 || !get_fp(&t[1], &a->terms))
            FAIL("terms: a fingerprint");
    } else if (tok_is(&t[0], "kind")) {
        ONCE(sc->seen_kind, "kind given twice");
        if (n != 2)
            FAIL("kind: one value");
        a->kind_is_fp = get_fp(&t[1], &a->kind_fp);
        if (!a->kind_is_fp && !tok_is(&t[1], "assent") && !tok_is(&t[1], "counter_signature") && !tok_is(&t[1], "contest"))
            FAIL("kind: assent, counter_signature, contest or a K11 cMIP by fingerprint");
        a->kind = mor_strdup_n(s, t[1].p, t[1].n);
    } else if (tok_is(&t[0], "content")) {
        ONCE(sc->seen_content, "content given twice");
        tok u[MAXTOK];
        int m = split(lp, ln, u, 2);
        if (m != 2)
            FAIL("content: a value");
        a->content = mor_strdup_n(s, u[1].p, u[1].n);
    } else if (tok_is(&t[0], "for")) {
        if (a->for_.kind != MOR_REF_NONE)
            FAIL("for given twice");
        if (n != 3 || !get_ref(s, &t[1], &t[2], &a->for_, false))
            FAIL("for: signer <id> or agreement <root>");
    } else if (tok_is(&t[0], "under")) {
        ONCE(a->has_under, "under given twice");
        if (n != 2 || !get_fp(&t[1], &a->under))
            FAIL("under: a fingerprint");
    } else if (tok_is(&t[0], "answers")) {
        if (n != 2 || a->n_answers >= MOR_MAX_LIST || !get_fp(&t[1], &sc->answers[a->n_answers]))
            FAIL("answers: a fingerprint");
        a->n_answers++;
    } else if (tok_is(&t[0], "relies_on")) {
        if (n != 2 || a->n_relies_on >= MOR_MAX_LIST || !get_fp(&t[1], &sc->relies[a->n_relies_on]))
            FAIL("relies_on: a fingerprint");
        a->n_relies_on++;
    } else if (tok_is(&t[0], "settlement")) {
        ONCE(a->has_settlement, "settlement given twice");
        if (n != 2 || !get_fp(&t[1], &a->settlement))
            FAIL("settlement: a fingerprint");
    } else {
        a->unread++;
    }
    return true;
}

static bool field_other(mor_set *s, mor_act *a, scratch *sc, const char *lp, size_t ln, const char **why)
{
    tok t[MAXTOK];
    int n = split(lp, ln, t, 7);
    switch (a->type) {
    case MOR_T_PAYMENT:
    case MOR_T_SETTLEMENT:
        if (tok_is(&t[0], "agreement") && a->type == MOR_T_PAYMENT) {
            ONCE(a->has_agreement, "agreement given twice");
            if (n != 2 || !get_fp(&t[1], &a->agreement))
                FAIL("agreement: a fingerprint");
        } else if (tok_is(&t[0], "version") && a->type == MOR_T_PAYMENT) {
            ONCE(a->has_version, "version given twice");
            if (n != 2 || !get_fp(&t[1], &a->version))
                FAIL("version: a fingerprint");
        } else if (tok_is(&t[0], "settlement") && a->type == MOR_T_PAYMENT) {
            ONCE(a->has_settlement, "settlement given twice");
            if (n != 2 || !get_fp(&t[1], &a->settlement))
                FAIL("settlement: a fingerprint");
        } else if (tok_is(&t[0], "amount")) {
            ONCE(a->has_amount, "amount given twice");
            if (n != 3 || !get_amount(s, &t[1], &t[2], &a->amount))
                FAIL("amount: a whole number and a unit");
        } else {
            a->unread++;
        }
        break;
    case MOR_T_REPORT: {
        if (tok_is(&t[0], "call")) {
            ONCE(sc->seen_call, "call given twice");
            if (n != 2 || !get_fp(&t[1], &a->call))
                FAIL("call: a fingerprint");
        } else if (tok_is(&t[0], "side")) {
            ONCE(sc->seen_side, "side given twice");
            if (n == 2 && tok_is(&t[1], "doer"))
                a->side = MOR_SIDE_DOER;
            else if (n == 2 && tok_is(&t[1], "receiving"))
                a->side = MOR_SIDE_RECEIVING;
            else
                FAIL("side: doer or receiving");
        } else if (tok_is(&t[0], "relied")) {
            if (n != 2 || a->n_relies_on >= MOR_MAX_LIST || !get_fp(&t[1], &sc->relies[a->n_relies_on]))
                FAIL("relied: a fingerprint");
            a->n_relies_on++;
        } else {
            static const char *const kinds[] = {"received", "passed", "own", "kept", "owed"};
            int k = -1;
            for (int i = 0; i < 5; i++)
                if (tok_is(&t[0], kinds[i]))
                    k = i;
            if (k < 0) {
                a->unread++;
                break;
            }
            if (a->n_lines >= MOR_MAX_LIST)
                FAIL("too many lines");
            mor_rline *l = &sc->lines[a->n_lines];
            memset(l, 0, sizeof *l);
            l->kind = (mor_line_kind)k;
            if (n < 3 || !get_amount(s, &t[1], &t[2], &l->amount))
                FAIL("a report line starts with an amount: a whole number and a unit");
            bool ok = false;
            switch (l->kind) {
            case MOR_L_RECEIVED:
                ok = n == 4 && get_fp(&t[3], &l->via);
                l->has_via = true;
                break;
            case MOR_L_PASSED:
                ok = n == 6 && get_ref(s, &t[3], &t[4], &l->to, false) && get_fp(&t[5], &l->via);
                l->has_via = true;
                break;
            case MOR_L_OWED:
                ok = n == 5 && get_ref(s, &t[3], &t[4], &l->to, false);
                break;
            default:
                ok = n == 3;
            }
            if (!ok)
                FAIL("report line: wrong shape");
            a->n_lines++;
        }
        break;
    }
    case MOR_T_ENVELOPE:
        if (tok_is(&t[0], "baker")) {
            if (a->baker || n != 2 || !id_ok(&t[1]))
                FAIL("baker: one identifier");
            a->baker = mor_strdup_n(s, t[1].p, t[1].n);
        } else {
            a->unread++;
        }
        break;
    case MOR_T_PLACE:
        if (tok_is(&t[0], "entry")) {
            if (n != 2 || a->n_entries >= MOR_MAX_ENTRIES || !get_fp(&t[1], &sc->entries[a->n_entries]))
                FAIL("entry: a fingerprint");
            a->n_entries++;
        } else {
            a->unread++;
        }
        break;
    case MOR_T_CLOCK:
        if (tok_is(&t[0], "now")) {
            ONCE(sc->seen_now, "now given twice");
            if (n != 2 || !get_u64(&t[1], &a->now))
                FAIL("now: a whole number");
        } else {
            a->unread++;
        }
        break;
    case MOR_T_PIECE:
        if (tok_is(&t[0], "data")) {
            tok u[2];
            int m = split(lp, ln, u, 2);
            if (m != 2 || a->n_data >= MOR_MAX_LIST)
                FAIL("data: a value");
            sc->data[a->n_data++] = mor_strdup_n(s, u[1].p, u[1].n);
        } else {
            a->unread++;
        }
        break;
    default:
        break;
    }
    return true;
}

#define COPY(dst, src, cnt)                                                                                                   \
    do {                                                                                                                      \
        if ((cnt) > 0) {                                                                                                      \
            (dst) = mor_alloc(s, sizeof *(dst) * (size_t)(cnt));                                                              \
            memcpy((dst), (src), sizeof *(dst) * (size_t)(cnt));                                                              \
        }                                                                                                                     \
    } while (0)

/* section 3: "An act missing a field marked required is not an Agreements act." */
static bool required(mor_act *a, const scratch *sc, const char **why)
{
    switch (a->type) {
    case MOR_T_TERMS:
        if (!sc->seen_previous)
            FAIL("Terms: previous is required (section 3.1; G6.1)");
        if (a->has_replaces != a->has_agreement)
            FAIL("Terms: agreement is required exactly when replaces is present (section 3.1)");
        if (a->has_continues && a->n_resolves == 0)
            FAIL("Terms: continues is present exactly when resolves is (section 3.1)");
        if (a->n_resolves > 0 && !a->has_replaces)
            FAIL("Terms: a resolving update names the shared Terms in replaces (G12.3)");
        if (a->n_resolves == 0) {
            if (a->n_parties == 0)
                FAIL("Terms: parties is required (section 3.1)");
            if (a->n_authors == 0)
                FAIL("Terms: authors is required (section 3.1)");
            if (!a->has_body)
                FAIL("Terms: body is required (section 3.1)");
        }
        break;
    case MOR_T_STATEMENT:
        if (!a->has_agreement)
            FAIL("Statement: agreement is required (section 3.2)");
        if (!a->has_terms)
            FAIL("Statement: terms is required (section 3.2)");
        if (!sc->seen_kind)
            FAIL("Statement: kind is required (section 3.2)");
        if (!sc->seen_previous)
            FAIL("Statement: previous is required (section 3.2; G6.1)");
        if ((a->for_.kind != MOR_REF_NONE) != a->has_under)
            FAIL("Statement: for and under come together (section 3.2)");
        if (a->has_continues && a->n_resolves == 0)
            FAIL("Statement: continues is present exactly when resolves is (section 3.2)");
        break;
    case MOR_T_PAYMENT:
        if (!a->has_agreement || !a->has_settlement || !a->has_amount)
            FAIL("payment (stand-in): agreement, settlement and amount are required");
        break;
    case MOR_T_REPORT:
        if (!sc->seen_call)
            FAIL("report: a report that names no call is no report (report block R2)");
        if (!sc->seen_side)
            FAIL("report (stand-in): side is required");
        break;
    case MOR_T_ENVELOPE:
        if (!a->baker)
            FAIL("envelope (stand-in): baker is required");
        break;
    case MOR_T_CLOCK:
        if (!sc->seen_now)
            FAIL("clock (stand-in): now is required");
        break;
    default:
        break;
    }
    return true;
}

static bool parse_record(mor_set *s, scratch *sc, const char *rec, size_t len, mor_act *a, const char **why)
{
    static const char *const types[] = {"terms", "statement", "payment", "settlement", "report", "envelope", "place", "clock", "piece"};
    memset(sc, 0, sizeof *sc);
    size_t i = 0;
    int ln = 0;
    while (i < len) {
        size_t j = i;
        while (j < len && rec[j] != '\n')
            j++;
        const char *lp = rec + i;
        size_t n = j - i;
        i = j + 1;
        ln++;
        if (ln == 1)
            continue; /* the header, checked by the caller */
        if (ln > MOR_MAX_LINES)
            FAIL("too many lines in one record");
        if (!line_ok(lp, n))
            FAIL("a line is empty, too long, not printable ASCII, or badly spaced");
        tok t[MAXTOK];
        int k = split(lp, n, t, 3);
        if (ln == 2) {
            int ty = -1;
            for (int q = 0; q < 9; q++)
                if (k == 2 && tok_is(&t[0], "type") && tok_is(&t[1], types[q]))
                    ty = q;
            if (ty < 0)
                FAIL("second line must be: type <known type>");
            a->type = (mor_type)ty;
            continue;
        }
        if (ln == 3) {
            if (k != 2 || !tok_is(&t[0], "signer") || !id_ok(&t[1]))
                FAIL("third line must be: signer <identifier>");
            a->signer = mor_strdup_n(s, t[1].p, t[1].n);
            continue;
        }
        if (k < 2)
            FAIL("a field needs a value");
        bool ok;
        if (a->type == MOR_T_TERMS)
            ok = field_terms(s, a, sc, lp, n, why);
        else if (a->type == MOR_T_STATEMENT)
            ok = field_statement(s, a, sc, lp, n, why);
        else
            ok = field_other(s, a, sc, lp, n, why);
        if (!ok)
            return false;
    }
    if (ln < 3)
        FAIL("a record needs type and signer");
    if (!required(a, sc, why))
        return false;
    COPY(a->parties, sc->parties, a->n_parties);
    COPY(a->authors, sc->authors, a->n_authors);
    COPY(a->relies_on, sc->relies, a->n_relies_on);
    COPY(a->resolves, sc->resolves, a->n_resolves);
    COPY(a->answers, sc->answers, a->n_answers);
    COPY(a->powers, sc->powers, a->n_powers);
    COPY(a->calls, sc->calls, a->n_calls);
    COPY(a->lines, sc->lines, a->n_lines);
    COPY(a->data, sc->data, a->n_data);
    COPY(a->entries, sc->entries, a->n_entries);
    return true;
}

static void reject(mor_set *s, size_t off, const char *reason)
{
    if (s->n_rejects < MOR_MAX_ACTS) {
        s->rejects[s->n_rejects].offset = off;
        s->rejects[s->n_rejects].reason = reason;
        s->n_rejects++;
    }
}

static bool is_header(const char *p, size_t n)
{
    return n == 9 && memcmp(p, "mor-act 1", 9) == 0;
}

int mor_decode(mor_set *s, const unsigned char *bytes, size_t len)
{
    const char *b = (const char *)bytes;
    size_t i = 0;
    int taken = 0;
    bool stray = false;
    scratch *sc = mor_alloc(s, sizeof *sc);
    while (i < len) {
        size_t j = i;
        while (j < len && b[j] != '\n')
            j++;
        size_t n = j - i;
        if (!is_header(b + i, n) || j >= len) {
            if (n != 0 && b[i] != '#' && !stray) {
                reject(s, i, "bytes outside any record");
                stray = true;
            }
            i = j + 1;
            continue;
        }
        stray = false;
        /* a record: from the header to the sig line */
        size_t start = i, k = j + 1, sig_at = 0, sig_len = 0;
        bool closed = false, broken = false;
        while (k < len) {
            size_t e = k;
            while (e < len && b[e] != '\n')
                e++;
            if (e >= len) { /* last line not ended by a newline */
                broken = true;
                k = len;
                break;
            }
            size_t ln = e - k;
            if (ln == 0 || is_header(b + k, ln)) {
                broken = true;
                break;
            }
            if (ln > 4 && memcmp(b + k, "sig ", 4) == 0) {
                sig_at = k;
                sig_len = ln;
                closed = true;
                k = e + 1;
                break;
            }
            k = e + 1;
        }
        if (!closed) {
            reject(s, start, "record not closed by a sig line");
            i = broken ? k : len;
            continue;
        }
        i = k;
        if (s->n_acts >= MOR_MAX_ACTS) {
            reject(s, start, "the set is over the limit of records");
            continue;
        }
        mor_act *a = mor_alloc(s, sizeof *a);
        const char *why = NULL;
        mor_fp_of(bytes + start, sig_at - start, &a->fp);
        if (!parse_record(s, sc, b + start, sig_at - start, a, &why)) {
            reject(s, start, why);
            continue;
        }
        if (sig_len != 4 + 64) {
            reject(s, start, "sig: 64 hex digits");
            continue;
        }
        if (!s->opt.standin_skip_signature) {
            char sig[65];
            memcpy(sig, b + sig_at + 4, 64);
            sig[64] = 0;
            if (!mor_standin_verify(a->signer, &a->fp, sig)) {
                reject(s, start, "the signature does not verify (stand-in)");
                continue;
            }
        }
        bool dup = false;
        for (int q = 0; q < s->n_acts && !dup; q++)
            dup = mor_fp_eq(&s->acts[q]->fp, &a->fp);
        if (dup)
            continue; /* the same act twice is one act */
        a->index = s->n_acts;
        s->acts[s->n_acts++] = a;
        taken++;
    }
    return taken;
}
