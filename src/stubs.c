/* stubs.c - stub cMIPs, one or more for each task the grammar needs answered.
 * Each is a stand-in for a document "named by fingerprint" (STANDINS.md, S4): its fingerprint is
 * SHA-256 of "mor-stub-cmip-1:" and its name. None of them is part of MOR. */
#include "internal.h"

#include <stdlib.h>
#include <string.h>

mor_fp mor_stub_fp(const char *name)
{
    char buf[96];
    int n = snprintf(buf, sizeof buf, "mor-stub-cmip-1:%s", name);
    mor_fp fp;
    mor_fp_of((const unsigned char *)buf, (size_t)(n < 0 ? 0 : n), &fp);
    return fp;
}

/* split a parameter string in place */
static int words(char *buf, char **w, int max)
{
    int n = 0;
    char *p = buf;
    while (*p && n < max) {
        while (*p == ' ')
            p++;
        if (!*p)
            break;
        w[n++] = p;
        while (*p && *p != ' ')
            p++;
        if (*p)
            *p++ = 0;
    }
    return n;
}

static bool to_ref(const char *kind, const char *val, mor_ref *r)
{
    memset(r, 0, sizeof *r);
    if (strcmp(kind, "signer") == 0) {
        r->kind = MOR_REF_SIGNER;
        r->name = val;
        return true;
    }
    if (strcmp(kind, "role") == 0) {
        r->kind = MOR_REF_ROLE;
        r->name = val;
        return true;
    }
    if (strcmp(kind, "agreement") == 0 && mor_fp_parse(val, strlen(val), &r->root)) {
        r->kind = MOR_REF_AGREEMENT;
        return true;
    }
    return false;
}

/* K1 "k1.lines": the words are lines; the lines "share <party> <n>" and
 * "delegation <delegator> <agent> default|agent|opaque" are the parts for machines. */
static void k1_lines(mor_set *s, const mor_act *wd, mor_k1_out *out)
{
    out->read = MOR_YES;
    for (int i = 0; i < wd->n_data; i++) {
        char buf[MOR_MAX_LINE + 1];
        char *w[8];
        snprintf(buf, sizeof buf, "%s", wd->data[i]);
        int n = words(buf, w, 8);
        mor_ref r;
        if (n == 4 && strcmp(w[0], "share") == 0 && to_ref(w[1], w[2], &r) && out->n_shares < MOR_MAX_LIST) {
            if (r.name)
                r.name = mor_sprintf(s, "%s", r.name);
            out->shares[out->n_shares].party = r;
            out->shares[out->n_shares].share = strtoull(w[3], NULL, 10);
            out->n_shares++;
        } else if (n == 6 && strcmp(w[0], "delegation") == 0 && to_ref(w[1], w[2], &out->delegator) && to_ref(w[3], w[4], &out->agent)) {
            if (out->delegator.name)
                out->delegator.name = mor_sprintf(s, "%s", out->delegator.name);
            if (out->agent.name)
                out->agent.name = mor_sprintf(s, "%s", out->agent.name);
            out->delegation = true;
            out->end_mode = strcmp(w[5], "agent") == 0 ? MOR_END_AGENT_ONLY : strcmp(w[5], "default") == 0 ? MOR_END_DEFAULT : MOR_END_OPAQUE;
            out->end_words = wd->data[i];
        }
    }
}

static void k4_yes(const mor_k4_in *in, mor_k4_out *out)
{
    (void)in;
    out->ans = MOR_YES;
    out->ground = "stub k4.yes: yes to everything";
}

static void k4_no(const mor_k4_in *in, mor_k4_out *out)
{
    (void)in;
    out->ans = MOR_NO;
    out->ground = "stub k4.no: no to everything";
}

/* "k4.threshold N <party> <party> ...": yes when N of the parties listed have signed.
 * For a power over a Statement: yes when its signer is one of the signers listed. */
static void k4_threshold(const mor_k4_in *in, mor_k4_out *out)
{
    char buf[MOR_MAX_LINE + 1];
    char *w[2 * MOR_MAX_LIST + 1];
    snprintf(buf, sizeof buf, "%s", in->rule->params);
    int n = words(buf, w, 2 * MOR_MAX_LIST + 1);
    out->ans = MOR_CANNOT;
    out->ground = "stub k4.threshold: bad parameters";
    if (n < 1 || (n - 1) % 2 != 0)
        return;
    long need = strtol(w[0], NULL, 10);
    long have = 0, maybe = 0;
    if (in->q == MOR_K4_CONDITION)
        return;
    for (int i = 1; i + 1 < n; i += 2) {
        mor_ref r;
        if (!to_ref(w[i], w[i + 1], &r))
            return;
        if (in->q == MOR_K4_POWER) {
            if (r.kind == MOR_REF_SIGNER && strcmp(r.name, in->subject->signer) == 0)
                have = need;
            continue;
        }
        mor_tri t = mor_plug_assent_in_name(in->set, in->subject, &r);
        if (t == MOR_YES)
            have++;
        else if (t == MOR_CANNOT)
            maybe++;
    }
    out->ans = have >= need ? MOR_YES : have + maybe >= need ? MOR_CANNOT : MOR_NO;
    out->ground = "stub k4.threshold: counted the assents of the parties it lists";
}

/* "k4.delegate <signer> ...": yes when the act asked about is signed by one of them */
static void k4_delegate(const mor_k4_in *in, mor_k4_out *out)
{
    char buf[MOR_MAX_LINE + 1];
    char *w[MOR_MAX_LIST];
    snprintf(buf, sizeof buf, "%s", in->rule->params);
    int n = words(buf, w, MOR_MAX_LIST);
    out->ans = MOR_NO;
    out->ground = "stub k4.delegate: the act is not signed by a delegate it names";
    for (int i = 0; i < n; i++)
        if (strcmp(w[i], in->subject->signer) == 0) {
            out->ans = MOR_YES;
            out->ground = "stub k4.delegate: signed by a delegate it names";
            out->ground_act = in->subject;
        }
}

/* "k4.before T": yes (valid) while the time is before T */
static void k4_before(const mor_k4_in *in, mor_k4_out *out)
{
    out->ground_is_time = true;
    if (!in->has_time) {
        out->ans = MOR_CANNOT;
        out->ground = "stub k4.before: no clock or anchoring gives the time (G9.1)";
        return;
    }
    unsigned long long t = strtoull(in->rule->params, NULL, 10);
    out->ans = in->time < t ? MOR_YES : MOR_NO;
    out->ground = in->time < t ? "stub k4.before: the time it names is not yet reached" : "stub k4.before: the time it names is passed";
}

/* "k4.same-parties": an update condition, from the bytes of the two versions alone:
 * valid only where the update names the same parties. */
static void k4_same_parties(const mor_k4_in *in, mor_k4_out *out)
{
    const mor_act *a = in->terms, *b = in->subject;
    bool same = a->n_parties == b->n_parties;
    for (int i = 0; same && i < a->n_parties; i++)
        same = mor_ref_eq(&a->parties[i].ref, &b->parties[i].ref);
    out->ans = same ? MOR_YES : MOR_NO;
    out->ground = same ? "stub k4.same-parties: the update names the same parties" : "stub k4.same-parties: the update changes the parties";
}

/* K10 "k10.judge": hands back what it was given; shows that it is given the authors (G3.2) */
static void k10_judge(const mor_k10_in *in, mor_k10_out *out)
{
    out->ans = MOR_YES;
    out->authors_seen = in->n_authors;
    out->decision = in->n_dispute > 0 ? in->dispute[0] : NULL;
}

/* K12 "k12.claim [one|many] [max=N] [until=T] [confirm=<signer>]":
 * a filling is earned by a Statement of kind k11.claim whose content is the role's name,
 * and, where confirm= is given, a Statement of kind k11.confirm by that signer that answers it. */
static void k12_claim(const mor_k12_in *in, mor_k12_out *out)
{
    char buf[MOR_MAX_LINE + 1];
    char *w[8];
    snprintf(buf, sizeof buf, "%s", in->role->k12.params);
    int n = words(buf, w, 8);
    bool one = false, has_max = false, has_until = false;
    unsigned long long max = 0, until = 0;
    const char *confirm = NULL;
    for (int i = 0; i < n; i++) {
        if (strcmp(w[i], "one") == 0)
            one = true;
        else if (strncmp(w[i], "max=", 4) == 0) {
            has_max = true;
            max = strtoull(w[i] + 4, NULL, 10);
        } else if (strncmp(w[i], "until=", 6) == 0) {
            has_until = true;
            until = strtoull(w[i] + 6, NULL, 10);
        } else if (strncmp(w[i], "confirm=", 8) == 0)
            confirm = mor_sprintf(in->set, "%s", w[i] + 8);
    }
    mor_fp kclaim = mor_stub_fp("k11.claim"), kconf = mor_stub_fp("k11.confirm");
    mor_set *s = in->set;
    out->ans = MOR_YES;
    out->many = !one;
    int total = mor_n_acts(s), best = -1, bestpos = -1;
    for (int i = 0; i < total; i++) {
        const mor_act *x = mor_act_at(s, i);
        if (x->type != MOR_T_STATEMENT || !x->kind_is_fp || !mor_fp_eq(&x->kind_fp, &kclaim) || !mor_fp_eq(&x->root, &in->terms->root) ||
            !x->content || strcmp(x->content, in->role->ref.name) != 0 || !mor_plug_counts(s, x))
            continue;
        const mor_act *under = mor_find(s, &x->terms);
        if (!under || !mor_party_named(under, MOR_REF_ROLE, in->role->ref.name))
            continue;
        const mor_act *cf = NULL;
        for (int k = 0; confirm && k < total && !cf; k++) {
            const mor_act *y = mor_act_at(s, k);
            if (y->type == MOR_T_STATEMENT && y->kind_is_fp && mor_fp_eq(&y->kind_fp, &kconf) && strcmp(y->signer, confirm) == 0 &&
                mor_plug_counts(s, y))
                for (int q = 0; q < y->n_answers; q++)
                    if (mor_fp_eq(&y->answers[q], &x->fp))
                        cf = y;
        }
        if (confirm && !cf)
            continue;
        if (out->n >= MOR_MAX_LIST)
            break;
        mor_k12_filling *f = &out->fill[out->n];
        f->signer = x->signer;
        f->proof[0] = x;
        f->n_proof = 1;
        if (cf)
            f->proof[f->n_proof++] = cf;
        f->earned_under = under; /* the version its proof names (G10.6) */
        int pos = mor_plug_position(s, x);
        if (pos >= 0 && (bestpos < 0 || pos < bestpos)) {
            bestpos = pos;
            best = out->n;
        }
        out->n++;
    }
    if (one && out->n > 1) {
        if (best >= 0) {
            out->fill[0] = out->fill[best];
            out->n = 1;
        } else {
            out->n = 0;
            out->cause = "stub k12.claim: the spot holds one filling and which claim came first is not shown";
        }
    }
    out->takes_new = MOR_YES;
    if (!out->cause)
        out->cause = "stub k12.claim: the version in force states nothing that closes the spot";
    if (one && out->n >= 1) {
        out->takes_new = MOR_NO;
        out->cause = "stub k12.claim: the spot holds one filling and it is filled";
    }
    if (has_max && (unsigned long long)out->n >= max) {
        out->takes_new = MOR_NO;
        out->cause = "stub k12.claim: the count the version in force states is reached";
    }
    if (has_until) {
        if (!in->has_time) {
            out->takes_new = MOR_CANNOT;
            out->cause = "stub k12.claim: the version in force states a date, and no clock or anchoring gives the time (G9.1)";
        } else if (in->time >= until) {
            out->takes_new = MOR_NO;
            out->cause = "stub k12.claim: the date the version in force states is passed";
            out->cause_is_time = true;
        }
    }
}

/* K13 "k13.count": plain counts from signed acts only */
static void k13_count(mor_set *s, const char *signer, mor_k13_out *out)
{
    for (int i = 0; i < mor_n_line_forks(s); i++) {
        const mor_line_fork *f = mor_line_fork_at(s, i);
        if (strcmp(f->signer, signer) != 0)
            continue;
        if (f->resolved)
            out->forks_resolved++;
        else
            out->forks_standing++;
    }
    for (int i = 0; i < mor_n_acts(s); i++) {
        const mor_act *x = mor_act_at(s, i);
        if (x->type != MOR_T_STATEMENT || strcmp(x->signer, signer) != 0 || x->ev.invalid)
            continue;
        if (strcmp(x->kind, "contest") == 0)
            out->contests_made++;
        if (strcmp(x->kind, "assent") == 0)
            out->assents++;
    }
}

static const struct {
    const char *name;
    int task;
} names[] = {
    {"k1.lines", 1},        {"k4.yes", 4},     {"k4.no", 4},      {"k4.threshold", 4},   {"k4.delegate", 4}, {"k4.before", 4},
    {"k4.same-parties", 4}, {"k8.hand", 8},    {"k10.judge", 10}, {"k11.claim", 11},     {"k11.confirm", 11}, {"k11.pass-rights", 11},
    {"k12.claim", 12},      {"k13.count", 13},
};

int mor_stub_count(void)
{
    return (int)(sizeof names / sizeof names[0]);
}

void mor_stub_fill(mor_cmip *out)
{
    for (int i = 0; i < mor_stub_count(); i++) {
        mor_cmip *c = &out[i];
        memset(c, 0, sizeof *c);
        c->name = names[i].name;
        c->task = names[i].task;
        c->fp = mor_stub_fp(c->name);
    }
    out[0].k1 = k1_lines;
    out[1].k4 = k4_yes;
    out[2].k4 = k4_no;
    out[3].k4 = k4_threshold;
    out[4].k4 = k4_delegate;
    out[5].k4 = k4_before;
    out[6].k4 = k4_same_parties;
    out[8].k10 = k10_judge;
    out[9].k11_meaning = "claim";
    out[10].k11_meaning = "confirm";
    out[11].k11_meaning = "pass-rights";
    out[12].k12 = k12_claim;
    out[13].k13 = k13_count;
}
