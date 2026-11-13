/* grammar.c - G2 to G7, G9, G12, G13: which acts count, who is bound, which Terms are in force. */
#include "internal.h"

#include <stdlib.h>
#include <string.h>

#define STOPS (MOR_M_TIP | MOR_M_BUILT | MOR_M_SET_ASIDE)

static mor_eval *EV(const mor_act *a)
{
    return &((mor_act *)(uintptr_t)a)->ev;
}

unsigned mor_restmark(const mor_set *s, const mor_act *a)
{
    return s->evaluated ? a->ev.rest : a->ev.rest_prev;
}

static bool counts_now(const mor_set *s, const mor_act *a)
{
    unsigned r = mor_restmark(s, a);
    /* an act made within Terms of a stopped branch still counts toward whether those Terms were signed;
     * nothing in them counts, because they are not in force */
    return !a->ev.invalid && !(a->ev.line & STOPS) && (!(r & (MOR_M_BUILT | MOR_M_SET_ASIDE)) || (r & MOR_M_WITHIN));
}

bool mor_unsettled(const mor_set *s)
{
    return s->unsettled;
}

bool mor_counts(const mor_act *a)
{
    return !a->ev.invalid && !(a->ev.line & STOPS) && !(a->ev.rest & (MOR_M_BUILT | MOR_M_SET_ASIDE));
}

static bool in_a_line(const mor_act *a)
{
    return a->type == MOR_T_TERMS || a->type == MOR_T_STATEMENT;
}

/* every fingerprint an act names (G13.1) */
int mor_named(const mor_act *a, const mor_fp **out, int max)
{
    int n = 0;
#define PUT(cond, f)                                                                                                          \
    do {                                                                                                                      \
        if ((cond) && n < max)                                                                                                \
            out[n++] = &(f);                                                                                                  \
    } while (0)
    PUT(a->has_previous, a->previous);
    PUT(a->has_replaces, a->replaces);
    PUT(a->has_agreement, a->agreement);
    PUT(a->has_terms, a->terms);
    PUT(a->has_under, a->under);
    PUT(a->has_settlement, a->settlement);
    PUT(a->has_version, a->version);
    PUT(a->has_continues, a->continues);
    PUT(a->type == MOR_T_REPORT, a->call);
    for (int i = 0; i < a->n_answers; i++)
        PUT(true, a->answers[i]);
    for (int i = 0; i < a->n_resolves; i++)
        PUT(true, a->resolves[i]);
    for (int i = 0; i < a->n_relies_on; i++)
        PUT(true, a->relies_on[i]);
    for (int i = 0; i < a->n_lines; i++)
        PUT(a->lines[i].has_via, a->lines[i].via);
#undef PUT
    return n;
}

#define MAXNAMED (16 + 4 * MOR_MAX_LIST)

bool mor_names(const mor_act *a, const mor_fp *b)
{
    const mor_fp *f[MAXNAMED];
    int n = mor_named(a, f, MAXNAMED);
    for (int i = 0; i < n; i++)
        if (mor_fp_eq(f[i], b))
            return true;
    return false;
}

/* does x name y, directly or through acts that name it (G13.3) */
static bool cites(mor_set *s, const mor_act *x, const mor_act *y)
{
    int sp = 0;
    s->epoch++;
    s->stack[sp++] = x;
    s->seen[x->index] = s->epoch;
    while (sp > 0) {
        const mor_act *c = s->stack[--sp];
        const mor_fp *f[MAXNAMED];
        int n = mor_named(c, f, MAXNAMED);
        for (int i = 0; i < n; i++) {
            const mor_act *t = mor_find(s, f[i]);
            if (!t || s->seen[t->index] == s->epoch)
                continue;
            if (t == y)
                return true;
            s->seen[t->index] = s->epoch;
            s->stack[sp++] = t;
        }
    }
    return false;
}

mor_order mor_order_by_citing(mor_set *s, const mor_act *a, const mor_act *b)
{
    if (a == b)
        return MOR_ORDER_NONE;
    if (cites(s, b, a))
        return MOR_ORDER_A_FIRST;
    if (cites(s, a, b))
        return MOR_ORDER_B_FIRST;
    return MOR_ORDER_NONE;
}

/* ---- the place: G7 ---------------------------------------------------------- */

static const mor_act *find_answer(const mor_set *s, mor_type ty, const char *id, int *count)
{
    const mor_act *r = NULL;
    *count = 0;
    for (int i = 0; i < s->n_acts; i++)
        if (s->acts[i]->type == ty && strcmp(s->acts[i]->signer, id) == 0) {
            r = s->acts[i];
            (*count)++;
        }
    return r;
}

mor_view mor_view_for(mor_set *s, const mor_act *terms)
{
    mor_view v;
    memset(&v, 0, sizeof v);
    const char *id = terms->place;
    v.qual.kind = MOR_Q_PER_PLACE;
    if (!id) {
        id = s->opt.anchoring_place;
        v.qual.kind = MOR_Q_PER_ANCHORING;
    }
    if (!id) {
        v.qual.kind = MOR_Q_AS_SHOWN; /* G7.3 */
        return v;
    }
    v.qual.name = id;
    int c;
    const mor_act *p = find_answer(s, MOR_T_PLACE, id, &c);
    if (c == 1)
        v.place = p;
    else
        v.cannot = true; /* no answer from the named place, or two that cannot both be its answer: G9 */
    return v;
}

int mor_pos(const mor_view *v, const mor_act *a)
{
    if (!v->place)
        return -1;
    mor_eval *e = EV(a);
    if (e->pos_place != v->place) {
        e->pos_place = v->place;
        e->pos = -1;
        for (int i = 0; i < v->place->n_entries; i++)
            if (mor_fp_eq(&v->place->entries[i], &a->fp)) {
                e->pos = i;
                break;
            }
    }
    return e->pos;
}

static bool visible(const mor_view *v, const mor_act *a)
{
    return !v->place || mor_pos(v, a) >= 0;
}

bool mor_usable(const mor_set *s, const mor_act *a, const mor_view *v)
{
    return counts_now(s, a) && !a->ev.mask && visible(v, a);
}

int mor_order3(mor_set *s, const mor_view *v, const mor_act *a, const mor_act *b)
{
    if (v && v->place) {
        int pa = mor_pos(v, a), pb = mor_pos(v, b);
        if (pa >= 0 && pb >= 0 && pa != pb)
            return pa < pb ? -1 : 1;
        return 0;
    }
    mor_order o = mor_order_by_citing(s, a, b);
    return o == MOR_ORDER_A_FIRST ? -1 : o == MOR_ORDER_B_FIRST ? 1 : 0;
}

mor_tri mor_time_for(mor_set *s, const mor_act *terms, uint64_t *t, mor_qual *q)
{
    const char *id = terms ? terms->clock : NULL;
    q->kind = MOR_Q_PER_CLOCK;
    if (!id) {
        id = s->opt.anchoring_clock;
        q->kind = MOR_Q_PER_ANCHORING;
    }
    q->name = id;
    if (!id)
        return MOR_CANNOT;
    int c;
    const mor_act *k = find_answer(s, MOR_T_CLOCK, id, &c);
    if (c != 1)
        return MOR_CANNOT;
    *t = k->now;
    return MOR_YES;
}

/* ---- plug-ins ------------------------------------------------------------------ */

const mor_cmip *mor_cmip_find(const mor_set *s, const mor_fp *fp, int task)
{
    const mor_plugins *p = s->opt.plugins;
    if (!p)
        return NULL;
    for (int i = 0; i < p->n; i++)
        if (!p->cmips[i].absent && (task == 0 || p->cmips[i].task == task) && mor_fp_eq(&p->cmips[i].fp, fp))
            return &p->cmips[i];
    return NULL;
}

mor_tri mor_k4_call(mor_set *s, const mor_rule *rule, mor_k4_q q, const mor_act *terms, const mor_act *subject, const mor_view *v,
                    mor_k4_out *out)
{
    memset(out, 0, sizeof *out);
    const mor_cmip *c = rule->bad ? NULL : mor_cmip_find(s, &rule->cmip, 4);
    if (!c || !c->k4) {
        out->ans = MOR_CANNOT;
        out->ground = "the cMIP named is not held (G9.1)";
        return MOR_CANNOT;
    }
    mor_k4_in in;
    memset(&in, 0, sizeof in);
    in.q = q;
    in.rule = rule;
    in.terms = terms;
    in.subject = subject;
    in.set = s;
    in.has_time = mor_time_for(s, terms, &in.time, &in.time_qual) == MOR_YES;
    mor_view save = s->view;
    s->view = *v;
    c->k4(&in, out);
    s->view = save;
    return out->ans;
}

mor_tri mor_k1_read(mor_set *s, const mor_act *terms, mor_k1_out *out)
{
    memset(out, 0, sizeof *out);
    out->read = MOR_CANNOT;
    if (!terms->has_body || terms->body_bad)
        return MOR_CANNOT;
    const mor_cmip *c = mor_cmip_find(s, &terms->body_cmip, 1);
    const mor_act *w = mor_find(s, &terms->body_words);
    if (!c || !c->k1 || !w || w->type != MOR_T_PIECE)
        return MOR_CANNOT;
    c->k1(s, w, out);
    return out->read;
}

mor_tri mor_plug_assent_in_name(mor_set *s, const mor_act *terms, const mor_ref *name)
{
    return mor_assent_in_name(s, terms, name, &s->view);
}

bool mor_plug_counts(mor_set *s, const mor_act *a)
{
    return mor_usable(s, a, &s->view);
}

int mor_plug_position(mor_set *s, const mor_act *a)
{
    return mor_pos(&s->view, a);
}

/* ---- agreements ------------------------------------------------------------------ */

mor_agreement *mor_agr(mor_set *s, const mor_fp *root)
{
    for (int i = 0; i < s->n_agr; i++)
        if (mor_fp_eq(&s->agr[i]->root, root))
            return s->agr[i];
    if (s->evaluated || s->n_agr >= MOR_MAX_AGR)
        return NULL;
    mor_agreement *a = mor_alloc(s, sizeof *a);
    a->root = *root;
    const mor_act *r = mor_find(s, root);
    if (r && r->type == MOR_T_TERMS && !r->has_replaces)
        a->root_act = r;
    a->in_force = MOR_UNCHECKED;
    s->agr[s->n_agr++] = a;
    return a;
}

int mor_n_agreements(const mor_set *s)
{
    return s->n_agr;
}

const mor_agreement *mor_agreement_at(const mor_set *s, int i)
{
    return i >= 0 && i < s->n_agr ? s->agr[i] : NULL;
}

const mor_agreement *mor_agreement_find(const mor_set *s, const mor_fp *root)
{
    for (int i = 0; i < s->n_agr; i++)
        if (mor_fp_eq(&s->agr[i]->root, root))
            return s->agr[i];
    return NULL;
}

bool mor_is_version(const mor_agreement *a, const mor_act *t)
{
    for (int i = 0; i < a->n_versions; i++)
        if (a->versions[i] == t)
            return true;
    return false;
}

const mor_party *mor_party_named(const mor_act *t, mor_ref_kind k, const char *name)
{
    for (int i = 0; i < t->n_parties; i++)
        if (t->parties[i].ref.kind == k && strcmp(t->parties[i].ref.name, name) == 0)
            return &t->parties[i];
    return NULL;
}

int mor_st_range(const mor_set *s, const mor_fp *terms, int *end)
{
    int lo = 0, hi = s->n_st;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (mor_fp_cmp(&s->st_by_terms[mid]->terms, terms) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    int e = lo;
    while (e < s->n_st && mor_fp_eq(&s->st_by_terms[e]->terms, terms))
        e++;
    *end = e;
    return lo;
}

static int tr_range(const mor_set *s, const mor_fp *replaced, int *end)
{
    int lo = 0, hi = s->n_tr;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (mor_fp_cmp(&s->tr_by_repl[mid]->replaces, replaced) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    int e = lo;
    while (e < s->n_tr && mor_fp_eq(&s->tr_by_repl[e]->replaces, replaced))
        e++;
    *end = e;
    return lo;
}

static bool is_kind(const mor_act *x, const char *kind)
{
    return x->type == MOR_T_STATEMENT && x->kind && strcmp(x->kind, kind) == 0;
}

static const mor_power *power_for(const mor_act *u, const char *kind)
{
    for (int i = 0; i < u->n_powers; i++)
        if (strcmp(u->powers[i].kind, kind) == 0)
            return &u->powers[i];
    return NULL;
}

/* ---- G5: own signature ------------------------------------------------------------ */

/* Is the power an act claims (for, under) one its Terms give, in force, with its rule saying yes? (G5.1(b), G1.2) */
mor_tri mor_check_power(mor_set *s, const mor_act *x, const char *kind, const char **why)
{
    const char *dummy;
    if (!why)
        why = &dummy;
    *why = NULL;
    if (x->for_.kind == MOR_REF_NONE || !x->has_under) {
        *why = "the act names no for and no under (G5.3)";
        return MOR_NO;
    }
    if (++s->depth > MOR_MAX_DEPTH) {
        s->depth--;
        *why = "powers through powers go deeper than this verifier follows (P2): unchecked";
        return MOR_CANNOT;
    }
    mor_tri r = MOR_NO;
    const mor_act *u = mor_find(s, &x->under);
    const mor_power *p;
    if (!u) {
        *why = "the Terms named in under are not held (G9.1)";
        r = MOR_CANNOT;
        goto done;
    }
    if (u->type != MOR_T_TERMS || u->ev.invalid) {
        *why = "under does not name Terms that count";
        goto done;
    }
    p = power_for(u, kind);
    if (!p) {
        *why = "those Terms give no power for acts of this kind (G5.1(b))";
        goto done;
    }
    mor_tri bound = MOR_YES;
    if (x->for_.kind == MOR_REF_SIGNER) {
        bound = mor_bound_signer(s, x->for_.name, u);
        if (bound == MOR_NO) {
            *why = "the one named in for is not bound by the Terms named in under (G5.1(b))";
            goto done;
        }
    } else if (!mor_fp_eq(&u->root, &x->for_.root)) {
        *why = "for names an agreement and under names Terms that are not its own (section 3.2; P17)";
        goto done;
    }
    mor_agreement *ag = mor_agr(s, &u->root);
    if (!ag) {
        r = MOR_CANNOT;
        goto done;
    }
    mor_agr_eval(s, ag);
    mor_view v = mor_view_for(s, ag->cur ? ag->cur : u);
    bool inforce = false;
    if (ag->cur == u) {
        if (ag->state == 2 || (ag->state == 3 && (ag->in_force == MOR_SHOWN || ag->in_force == MOR_FORK))) {
            inforce = true;
        } else if (ag->state == 3 && ag->in_force == MOR_UNCHECKED) {
            *why = "whether the Terms named in under are in force is unchecked (G9.1)";
            r = MOR_CANNOT;
            goto done;
        }
    } else if (mor_is_version(ag, u)) {
        /* G5.4: bound only through Terms that have been replaced: answered from G7 */
        const mor_act *next = NULL;
        for (int i = 0; i + 1 < ag->n_versions; i++)
            if (ag->versions[i] == u)
                next = ag->versions[i + 1];
        int o = next ? mor_order3(s, &v, x, next) : 0;
        if (o < 0) {
            inforce = true;
        } else {
            *why = o > 0 ? "made after the Terms named in under were replaced (G5.4)"
                         : "the Terms named in under have been replaced, and it is not shown that the act came before (G5.4, G13.3)";
            goto done;
        }
    }
    if (!inforce) {
        *why = "the Terms named in under are not in force (G5.1(b))";
        goto done;
    }
    mor_k4_out ko;
    r = mor_k4_call(s, &p->rule, strcmp(kind, "resolves") == 0 ? MOR_K4_RESOLVE : MOR_K4_POWER, u, x, &v, &ko);
    if (r == MOR_NO)
        *why = "the power's rule says no (G5.1(b))";
    else if (r == MOR_CANNOT)
        *why = ko.ground ? ko.ground : "the power's rule could not be applied (G9.1)";
    else if (bound == MOR_CANNOT) {
        r = MOR_CANNOT;
        *why = "whether the one named in for is bound by the Terms named in under is unchecked (G9.1)";
    }
done:
    s->depth--;
    return r;
}

/* Is there an assent to Terms t in this name? (G4.1, G4.4, G1.2) */
mor_tri mor_assent_in_name(mor_set *s, const mor_act *t, const mor_ref *name, const mor_view *v)
{
    if (name->kind != MOR_REF_SIGNER && name->kind != MOR_REF_AGREEMENT)
        return MOR_NO;
    if (++s->depth > MOR_MAX_DEPTH) {
        s->depth--;
        return MOR_CANNOT;
    }
    mor_tri best = MOR_NO;
    int end, i = mor_st_range(s, &t->fp, &end);
    for (int k = i; k < end && best != MOR_YES; k++) {
        const mor_act *x = s->st_by_terms[k];
        if (!is_kind(x, "assent") || x->ev.cancelled || !mor_usable(s, x, v))
            continue;
        if (x->for_.kind == MOR_REF_NONE) {
            if (name->kind == MOR_REF_SIGNER && strcmp(x->signer, name->name) == 0)
                best = MOR_YES;
        } else if (mor_ref_eq(&x->for_, name)) {
            mor_tri r = mor_check_power(s, x, "assent", NULL);
            if (r == MOR_YES)
                best = MOR_YES;
            else if (r == MOR_CANNOT)
                best = MOR_CANNOT;
        }
    }
    if (best != MOR_YES && name->kind == MOR_REF_AGREEMENT) {
        /* G1.2: where x's Terms give no power, x signs by every one of its signers (one level).
         * By which acts is not said (P17); read as P17's least reading: an assent by each of them,
         * its for naming the agreement and its under its Terms in force. */
        mor_agreement *xa = mor_agr(s, &name->root);
        if (!xa || !xa->root_act) {
            best = MOR_CANNOT; /* lacks the acts of an agreement named as a party: G9.1 */
        } else {
            mor_agr_eval(s, xa);
            bool live = xa->state == 2 || (xa->state == 3 && (xa->in_force == MOR_SHOWN || xa->in_force == MOR_FORK));
            if (xa->state == 3 && xa->in_force == MOR_UNCHECKED)
                best = MOR_CANNOT;
            else if (live && xa->cur && !power_for(xa->cur, "assent")) {
                int need = 0, have = 0;
                for (int p = 0; p < xa->cur->n_parties; p++) {
                    if (xa->cur->parties[p].ref.kind != MOR_REF_SIGNER)
                        continue;
                    need++;
                    for (int k = i; k < end; k++) {
                        const mor_act *x = s->st_by_terms[k];
                        if (is_kind(x, "assent") && !x->ev.cancelled && mor_usable(s, x, v) && mor_ref_eq(&x->for_, name) &&
                            x->has_under && mor_fp_eq(&x->under, &xa->cur->fp) &&
                            strcmp(x->signer, xa->cur->parties[p].ref.name) == 0) {
                            have++;
                            break;
                        }
                    }
                }
                if (need > 0 && have == need)
                    best = MOR_YES;
            }
        }
    }
    s->depth--;
    return best;
}

/* Is signer p bound by Terms u, which are or were a version? (G5.1, G5.2, G1.2) */
mor_tri mor_bound_signer(mor_set *s, const char *p, const mor_act *u)
{
    if (u->type != MOR_T_TERMS)
        return MOR_NO;
    if (++s->depth > MOR_MAX_DEPTH) {
        s->depth--;
        return MOR_CANNOT;
    }
    mor_tri r = MOR_NO;
    mor_agreement *ag = mor_agr(s, &u->root);
    if (!ag) {
        r = MOR_CANNOT;
        goto done;
    }
    mor_agr_eval(s, ag);
    if (!mor_is_version(ag, u)) {
        if (ag->state == 3 && ag->in_force == MOR_UNCHECKED)
            r = MOR_CANNOT;
        goto done;
    }
    mor_view v = mor_view_for(s, u);
    mor_ref me = {MOR_REF_SIGNER, p, {{0}}};
    r = mor_assent_in_name(s, u, &me, &v);
    if (r == MOR_YES)
        goto done;
    if (u->ev.by_power && u->has_replaces) {
        /* G5.2: bound by a power for replacement in the Terms it replaces */
        const mor_act *prev = mor_find(s, &u->replaces);
        mor_tri q = prev ? mor_bound_signer(s, p, prev) : MOR_NO;
        if (q == MOR_YES) {
            r = MOR_YES;
            goto done;
        }
        if (q == MOR_CANNOT)
            r = MOR_CANNOT;
    }
    for (int i = 0; i < u->n_parties; i++) {
        /* G1.2: an agreement's signature binds every party bound by its Terms */
        if (u->parties[i].ref.kind != MOR_REF_AGREEMENT)
            continue;
        mor_tri q = mor_assent_in_name(s, u, &u->parties[i].ref, &v);
        if (q == MOR_YES) {
            mor_agreement *xa = mor_agr(s, &u->parties[i].ref.root);
            q = MOR_NO;
            if (xa && xa->cur && (xa->state == 2 || xa->in_force == MOR_SHOWN || xa->in_force == MOR_FORK))
                q = mor_bound_signer(s, p, xa->cur);
        }
        if (q == MOR_YES) {
            r = MOR_YES;
            goto done;
        }
        if (q == MOR_CANNOT)
            r = MOR_CANNOT;
    }
done:
    s->depth--;
    return r;
}

/* ---- G4: in force --------------------------------------------------------------------- */

static mor_tri in_force_eval(mor_set *s, const mor_act *t, const mor_view *v)
{
    if (t->in_force.present) {
        mor_k4_out ko;
        return mor_k4_call(s, &t->in_force, MOR_K4_IN_FORCE, t, t, v, &ko);
    }
    /* G4.1 default: an assent in the name of every party named as a signer (agreements are not counted, G4.4) */
    mor_tri r = MOR_YES;
    for (int i = 0; i < t->n_parties; i++) {
        if (t->parties[i].ref.kind != MOR_REF_SIGNER)
            continue;
        mor_tri q = mor_assent_in_name(s, t, &t->parties[i].ref, v);
        if (q == MOR_NO)
            return MOR_NO;
        if (q == MOR_CANNOT)
            r = MOR_CANNOT;
    }
    return r;
}

static bool follows_in_line(const mor_set *s, const mor_act *later, const mor_act *earlier)
{
    const mor_act *c = later;
    for (int n = 0; n < MOR_MAX_ACTS && c && c->has_previous; n++) {
        c = mor_find(s, &c->previous);
        if (c == earlier)
            return true;
    }
    return false;
}

static int cmp_pos(const void *a, const void *b)
{
    const mor_act *x = *(const mor_act *const *)a, *y = *(const mor_act *const *)b;
    return x->ev.pos - y->ev.pos;
}

static void cs_cancel(const mor_set *s, const mor_act *cs, const mor_act **as, int na, bool on)
{
    for (int i = 0; i < na; i++)
        if (strcmp(as[i]->signer, cs->signer) == 0 && mor_ref_eq(&as[i]->for_, &cs->for_) && follows_in_line(s, cs, as[i]))
            EV(as[i])->cancelled = on;
}

/* G4.1 with G4.10: are Terms t in force, counting counter signatures made before they were? */
static mor_tri in_force_cs(mor_set *s, const mor_act *t, const mor_view *v, const char **why)
{
    const mor_act *as[MOR_MAX_LIST * 4], *cs[MOR_MAX_LIST];
    int na = 0, nc = 0, end, i = mor_st_range(s, &t->fp, &end);
    *why = NULL;
    for (int k = i; k < end; k++) {
        const mor_act *x = s->st_by_terms[k];
        if (!mor_usable(s, x, v))
            continue;
        if (is_kind(x, "assent") && na < MOR_MAX_LIST * 4)
            as[na++] = x;
        else if (is_kind(x, "counter_signature") && nc < MOR_MAX_LIST)
            cs[nc++] = x;
    }
    /* keep only counter signatures that follow an assent of their own signer to these Terms */
    int m = 0;
    for (int c = 0; c < nc; c++) {
        bool hit = false;
        for (int a = 0; a < na && !hit; a++)
            hit = strcmp(as[a]->signer, cs[c]->signer) == 0 && mor_ref_eq(&as[a]->for_, &cs[c]->for_) &&
                  follows_in_line(s, cs[c], as[a]);
        if (hit)
            cs[m++] = cs[c];
    }
    nc = m;
    if (nc == 0)
        return in_force_eval(s, t, v);

    if (v->place) {
        /* the place gives positions: replay them */
        const mor_act *ev[MOR_MAX_LIST * 5];
        int ne = 0;
        for (int a = 0; a < na; a++) {
            EV(as[a])->mask = true;
            ev[ne++] = as[a];
        }
        for (int c = 0; c < nc; c++)
            ev[ne++] = cs[c];
        qsort(ev, (size_t)ne, sizeof ev[0], cmp_pos);
        mor_tri r = MOR_NO;
        int k = 0;
        for (; k < ne; k++) {
            if (is_kind(ev[k], "assent"))
                EV(ev[k])->mask = false;
            else
                cs_cancel(s, ev[k], as, na, true);
            r = in_force_eval(s, t, v);
            if (r == MOR_YES)
                break;
        }
        for (int a = 0; a < na; a++)
            EV(as[a])->mask = false; /* later assents count as any assent does; later counter signatures change nothing */
        if (r == MOR_NO && nc > 0)
            *why = "an assent was cancelled by a counter signature before the Terms were in force (G4.10)";
        return r;
    }

    /* no positions: order only from citing (G13.3) */
    if (nc == 1) {
        for (int a = 0; a < na; a++)
            EV(as[a])->mask = mor_order3(s, v, as[a], cs[0]) >= 0; /* keep those shown before it */
        mor_tri before = in_force_eval(s, t, v);
        for (int a = 0; a < na; a++)
            EV(as[a])->mask = mor_order3(s, v, as[a], cs[0]) > 0; /* drop only those shown after it */
        mor_tri maybe = in_force_eval(s, t, v);
        for (int a = 0; a < na; a++)
            EV(as[a])->mask = false;
        if (before == MOR_YES)
            return in_force_eval(s, t, v); /* in force before it: it changes nothing */
        if (maybe != MOR_YES) {
            cs_cancel(s, cs[0], as, na, true);
            mor_tri r = in_force_eval(s, t, v);
            if (r == MOR_NO)
                *why = "an assent was cancelled by a counter signature before the Terms were in force (G4.10)";
            return r;
        }
    }
    mor_tri with = in_force_eval(s, t, v);
    for (int c = 0; c < nc; c++)
        cs_cancel(s, cs[c], as, na, true);
    mor_tri without = in_force_eval(s, t, v);
    if (with == without) {
        if (with == MOR_YES)
            for (int c = 0; c < nc; c++)
                cs_cancel(s, cs[c], as, na, false);
        return with;
    }
    *why = "a counter signature and the assents that would put the Terms in force: which came first is not shown (G4.10, G13.3)";
    return MOR_NO;
}

static bool same_rule(const mor_rule *a, const mor_rule *b)
{
    return a->present == b->present && a->bad == b->bad && mor_fp_eq(&a->cmip, &b->cmip) && strcmp(a->params, b->params) == 0;
}

/* G5.5: does c remove or narrow the powers of cur? Read as: its powers are a strict subset. */
static bool removes_power(const mor_act *cur, const mor_act *c)
{
    if (c->n_powers >= cur->n_powers)
        return false;
    for (int i = 0; i < c->n_powers; i++) {
        bool found = false;
        for (int j = 0; j < cur->n_powers && !found; j++)
            found = strcmp(c->powers[i].kind, cur->powers[j].kind) == 0 && same_rule(&c->powers[i].rule, &cur->powers[j].rule);
        if (!found)
            return false;
    }
    return true;
}

/* the parties bound by a version: those in whose name an assent to it counts, and those a power for replacement carried over */
static mor_tri bound_names(mor_set *s, const mor_act *t, const mor_view *v, mor_ref *out, int *n, int depth)
{
    mor_tri r = MOR_YES;
    for (int i = 0; i < t->n_parties; i++) {
        const mor_party *p = &t->parties[i];
        if (p->ref.kind == MOR_REF_ROLE) {
            const char *who[MOR_MAX_LIST];
            int nw = 0;
            if (mor_spot_fillers(s, t, p, v, who, &nw) == MOR_CANNOT)
                r = MOR_CANNOT;
            for (int f = 0; f < nw && *n < MOR_MAX_LIST; f++) {
                mor_ref me = {MOR_REF_SIGNER, who[f], {{0}}};
                if (mor_assent_in_name(s, t, &me, v) == MOR_YES)
                    out[(*n)++] = me;
            }
            continue;
        }
        mor_tri q = mor_assent_in_name(s, t, &p->ref, v);
        if (q == MOR_YES && *n < MOR_MAX_LIST)
            out[(*n)++] = p->ref;
        else if (q == MOR_CANNOT)
            r = MOR_CANNOT;
    }
    if (t->ev.by_power && t->has_replaces && depth < MOR_MAX_DEPTH) {
        const mor_act *prev = mor_find(s, &t->replaces);
        if (prev && bound_names(s, prev, v, out, n, depth + 1) == MOR_CANNOT)
            r = MOR_CANNOT;
    }
    return r;
}

/* G4.8: does c carry the signatures cur requires for its replacement? */
static mor_tri sigs_ok(mor_set *s, const mor_act *cur, const mor_act *c, const mor_view *v, bool resolver, bool *by_power,
                       int *deleg, const char **how)
{
    mor_ref names[MOR_MAX_LIST];
    int n = 0;
    mor_tri cannot = MOR_NO;
    *by_power = false;
    *deleg = 0; /* 1: replaced by one part alone, as G5.5 says; 2: whether G5.5 applies could not be read */
    mor_tri bn = bound_names(s, cur, v, names, &n, 0);
    mor_tri all = MOR_YES;
    for (int i = 0; i < n && all != MOR_NO; i++) {
        mor_tri q = mor_assent_in_name(s, c, &names[i], v);
        if (q == MOR_NO)
            all = MOR_NO;
        else if (q == MOR_CANNOT)
            all = MOR_CANNOT;
    }
    if (all == MOR_YES && bn == MOR_YES) {
        *how = "an assent in the name of every party bound by the Terms it replaces (G4.8)";
        return MOR_YES;
    }
    if (all != MOR_NO)
        cannot = MOR_CANNOT;
    for (int i = 0; i < cur->n_powers; i++) {
        bool repl = strcmp(cur->powers[i].kind, "replaces") == 0;
        bool res = resolver && strcmp(cur->powers[i].kind, "resolves") == 0;
        if (!repl && !res)
            continue;
        mor_k4_out ko;
        mor_tri q = mor_k4_call(s, &cur->powers[i].rule, res ? MOR_K4_RESOLVE : MOR_K4_REPLACE, cur, c, v, &ko);
        if (q == MOR_YES) {
            *by_power = true;
            *how = res ? "a power to resolve in the Terms both tips replace (G12.4)" : "a power for replacement in the Terms it replaces (G5.2)";
            return MOR_YES;
        }
        if (q == MOR_CANNOT)
            cannot = MOR_CANNOT;
    }
    if (cur->n_powers > 0 && !resolver) {
        /* G5.5: a delegation is ended by its delegator alone, by default */
        mor_k1_out k1;
        if (mor_k1_read(s, cur, &k1) != MOR_YES) {
            cannot = MOR_CANNOT; /* cannot read whether these Terms are a delegation, or its conditions */
            *deleg = 2;
        } else if (k1.delegation && removes_power(cur, c)) {
            if (k1.end_mode == MOR_END_OPAQUE) {
                cannot = MOR_CANNOT;
                *deleg = 2;
            } else {
                const mor_ref *who = k1.end_mode == MOR_END_DEFAULT ? &k1.delegator : &k1.agent;
                mor_tri q = mor_assent_in_name(s, c, who, v);
                if (q == MOR_YES) {
                    *deleg = 1;
                    *how = k1.end_mode == MOR_END_DEFAULT ? "the delegator's part alone (G5.5)"
                                                          : "the agent's part, as the delegation's signed condition says (G5.5)";
                    return MOR_YES;
                }
                if (q == MOR_CANNOT)
                    cannot = MOR_CANNOT;
            }
        }
    }
    *how = cannot == MOR_CANNOT ? "whether it carries the signatures the Terms require could not be checked (G9.1)"
                                : "it does not carry the signatures the Terms it names require; an assent not held may exist (G4.8)";
    return cannot;
}

/* ---- G12: forks -------------------------------------------------------------------------- */

static bool names_all(const mor_act *r, const mor_act *const *u, int nu)
{
    for (int i = 0; i < nu; i++) {
        bool f = false;
        for (int k = 0; k < r->n_resolves && !f; k++)
            f = mor_fp_eq(&r->resolves[k], &u[i]->fp);
        if (!f)
            return false;
    }
    return true;
}

static const mor_act *member(const mor_act *const *u, int nu, const mor_fp *fp)
{
    for (int i = 0; i < nu; i++)
        if (mor_fp_eq(&u[i]->fp, fp))
            return u[i];
    return NULL;
}

/* G12.3, G12.7: which of the updates u continues, if a resolving update among r says so; NULL while the fork stands */
static const mor_act *resolve_group(const mor_act *const *u, int nu, const mor_act *const *r, int nr, bool *used, int depth,
                                    const mor_act **resolver)
{
    if (nu < 2 || depth > MOR_MAX_DEPTH)
        return NULL;
    const mor_act *lvl[MOR_MAX_LIST];
    int nl = 0;
    for (int i = 0; i < nr; i++)
        if (!used[i] && r[i]->has_continues && names_all(r[i], u, nu) && member(u, nu, &r[i]->continues)) {
            used[i] = true;
            lvl[nl++] = r[i];
        }
    if (nl == 0)
        return NULL;
    bool same = true;
    for (int i = 1; i < nl; i++)
        same = same && mor_fp_eq(&lvl[i]->continues, &lvl[0]->continues);
    if (same) {
        *resolver = lvl[0];
        return member(u, nu, &lvl[0]->continues);
    }
    /* rival resolving updates are themselves a fork (G12.7) */
    const mor_act *w = resolve_group(lvl, nl, r, nr, used, depth + 1, resolver);
    return w ? member(u, nu, &w->continues) : NULL;
}

/* mark what follows the seeds in their line, and what rests on that (G12.2, G12.3, G12.5) */
static void mark_closure(mor_set *s, const mor_act *const *seeds, int n, bool terms_fork, bool standing)
{
    for (int i = 0; i < s->n_acts; i++)
        s->acts[i]->ev.tmp_d = s->acts[i]->ev.tmp_b = false;
    for (int i = 0; i < n; i++)
        EV(seeds[i])->tmp_d = true;
    for (int i = 0; i < s->n_topo; i++) {
        mor_act *x = s->topo[i];
        if (x->ev.tmp_d || !in_a_line(x))
            continue;
        const mor_act *y = NULL;
        if (terms_fork)
            y = x->type == MOR_T_TERMS && x->has_replaces ? mor_find(s, &x->replaces) : NULL;
        else
            y = x->has_previous ? mor_find(s, &x->previous) : NULL;
        if (y && y->ev.tmp_d)
            x->ev.tmp_d = true;
    }
    for (int i = 0; i < s->n_topo; i++) {
        mor_act *x = s->topo[i];
        if (x->ev.tmp_d)
            continue;
        const mor_fp *f[4 + 2 * MOR_MAX_LIST];
        int k = 0;
        if (x->has_terms)
            f[k++] = &x->terms;
        if (x->has_under)
            f[k++] = &x->under;
        if (x->has_replaces)
            f[k++] = &x->replaces;
        if (x->has_version)
            f[k++] = &x->version;
        for (int q = 0; q < x->n_answers; q++)
            f[k++] = &x->answers[q];
        if (x->type != MOR_T_PLACE)
            for (int q = 0; q < x->n_relies_on; q++)
                f[k++] = &x->relies_on[q];
        bool strong = false;
        for (int q = 0; q < k; q++) {
            const mor_act *y = mor_find(s, f[q]);
            if (y && (y->ev.tmp_d || y->ev.tmp_b)) {
                x->ev.tmp_b = true;
                if (!(x->has_terms && f[q] == &x->terms))
                    strong = true;
            }
        }
        x->ev.mask = x->ev.tmp_b && !strong; /* scratch: rests on the branch only by being made within its Terms */
    }
    for (int i = 0; i < s->n_acts; i++) {
        mor_act *x = s->acts[i];
        if (x->ev.tmp_d) {
            bool seed = false;
            for (int q = 0; q < n && !seed; q++)
                seed = seeds[q] == x;
            if (terms_fork) {
                x->ev.chain = standing ? (seed ? MOR_C_TIP : MOR_C_STOPPED) : MOR_C_SET_ASIDE;
            } else {
                x->ev.line |= standing ? (seed ? MOR_M_TIP : MOR_M_BUILT) : MOR_M_SET_ASIDE;
            }
        } else if (x->ev.tmp_b) {
            unsigned m = MOR_M_BUILT;
            if (!standing) {
                /* G12.5: an act signed by someone who signed none of the acts of the branch stands, disputed */
                bool theirs = false;
                for (int q = 0; q < s->n_acts && !theirs; q++)
                    theirs = s->acts[q]->ev.tmp_d && strcmp(s->acts[q]->signer, x->signer) == 0;
                m = theirs ? MOR_M_SET_ASIDE : MOR_M_DISPUTED;
            }
            if (terms_fork)
                x->ev.rest |= m | (x->ev.mask && m != MOR_M_DISPUTED ? MOR_M_WITHIN : 0u);
            else
                x->ev.line |= m;
        }
        x->ev.mask = false;
    }
}

static int cmp_line(const void *a, const void *b)
{
    const mor_act *x = *(const mor_act *const *)a, *y = *(const mor_act *const *)b;
    int c = mor_fp_cmp(&x->root, &y->root);
    if (c)
        return c;
    c = strcmp(x->signer, y->signer);
    if (c)
        return c;
    if (x->has_previous != y->has_previous)
        return x->has_previous ? 1 : -1;
    if (x->has_previous && (c = mor_fp_cmp(&x->previous, &y->previous)) != 0)
        return c;
    return x->index - y->index;
}

static bool same_line_key(const mor_act *x, const mor_act *y)
{
    return mor_fp_eq(&x->root, &y->root) && strcmp(x->signer, y->signer) == 0 && x->has_previous == y->has_previous &&
           (!x->has_previous || mor_fp_eq(&x->previous, &y->previous));
}

/* G6.2: acts by one signer within one agreement that name the same previous (or all name none) */
static void line_group(mor_set *s, const mor_act **m, int k)
{
    if (k > MOR_MAX_LIST) {
        for (int i = MOR_MAX_LIST; i < k; i++)
            EV(m[i])->line |= MOR_M_TIP; /* over the limit: still tips, never resolved here */
        k = MOR_MAX_LIST;
    }
    bool ord[MOR_MAX_LIST], used[2 * MOR_MAX_LIST];
    const mor_act *u[MOR_MAX_LIST], *r[2 * MOR_MAX_LIST];
    int nu = 0, nr = 0, nsib = 0;
    const mor_act *winner = NULL, *resolver = NULL;
    for (int i = 0; i < k; i++)
        ord[i] = !(m[i]->n_resolves > 0 && m[i]->has_continues);
    for (int iter = 0; iter <= k; iter++) {
        nu = nr = 0;
        for (int i = 0; i < k; i++)
            if (ord[i])
                u[nu++] = m[i];
            else
                r[nr++] = m[i];
        nsib = nr;
        /* G12.4: a resolving update made for this signer under a power given in advance */
        for (int i = 0; i < s->n_acts && nr < 2 * MOR_MAX_LIST; i++) {
            const mor_act *x = s->acts[i];
            if (x->ev.power_resolve_prev && x->for_.kind == MOR_REF_SIGNER && strcmp(x->for_.name, m[0]->signer) == 0 &&
                mor_fp_eq(&x->root, &m[0]->root) && counts_now(s, x))
                r[nr++] = x;
        }
        memset(used, 0, sizeof used);
        resolver = NULL;
        winner = resolve_group(u, nu, r, nr, used, 0, &resolver);
        bool changed = false;
        for (int i = 0, q = 0; i < k; i++) {
            if (ord[i])
                continue;
            if (!used[q]) { /* names resolves, resolves nothing here: an act like any other, and so a tip */
                ord[i] = true;
                changed = true;
            }
            q++;
        }
        if (!changed)
            break;
    }
    if (nu < 2)
        return;
    if (s->n_lforks < MOR_MAX_ACTS) {
        mor_line_fork *lf = &s->lforks[s->n_lforks++];
        memset(lf, 0, sizeof *lf);
        lf->root = m[0]->root;
        lf->signer = m[0]->signer;
        for (int i = 0; i < nu; i++)
            lf->tips[lf->n_tips++] = u[i];
        lf->resolved = winner != NULL;
        lf->continued = winner;
        lf->resolver = winner ? resolver : NULL;
        if (winner && resolver && resolver->for_.kind != MOR_REF_NONE)
            lf->resolver_power = mor_sprintf(s, "signed by %s for %s under a power to resolve in Terms %s (G12.4)", resolver->signer,
                                             m[0]->signer, mor_short(s, &resolver->under));
    }
    const mor_act *seeds[3 * MOR_MAX_LIST];
    int ns = 0;
    if (!winner) {
        for (int i = 0; i < nu; i++)
            seeds[ns++] = u[i];
        for (int i = 0; i < nsib; i++)
            if (used[i])
                seeds[ns++] = r[i]; /* rival resolving updates: themselves a fork (G12.7) */
        mark_closure(s, seeds, ns, false, true);
        return;
    }
    for (int i = 0; i < nu; i++)
        if (u[i] != winner)
            seeds[ns++] = u[i];
    for (int i = 0; i < nr; i++)
        if (used[i]) {
            if (i < nsib && !mor_fp_eq(&r[i]->continues, &winner->fp) && member(u, nu, &r[i]->continues))
                seeds[ns++] = r[i]; /* a rival that lost */
            else
                EV(r[i])->line |= MOR_M_RESOLVER;
        }
    mark_closure(s, seeds, ns, false, false);
}

static void line_forks(mor_set *s)
{
    const mor_act **v = mor_alloc(s, sizeof *v * (size_t)(s->n_acts + 1));
    int n = 0;
    for (int i = 0; i < s->n_acts; i++)
        if (in_a_line(s->acts[i]) && !s->acts[i]->ev.invalid)
            v[n++] = s->acts[i];
    qsort(v, (size_t)n, sizeof v[0], cmp_line);
    for (int i = 0; i < n;) {
        int j = i + 1;
        while (j < n && same_line_key(v[i], v[j]))
            j++;
        if (j - i >= 2)
            line_group(s, v + i, j - i);
        i = j;
    }
}

/* ---- the chain of one agreement: G4.5 to G4.11, G12 ------------------------------------------ */

static void finish(mor_set *s, mor_agreement *a)
{
    for (int i = 0; i < s->n_tr; i++) {
        mor_act *x = s->tr_by_repl[i];
        if (mor_fp_eq(&x->root, &a->root) && x->ev.chain == MOR_C_UNREAD && !x->ev.invalid) {
            x->ev.chain = MOR_C_UNREACHED;
            x->ev.chain_why = "it replaces Terms that are not shown as a version of the agreement";
        }
    }
    a->state = 3;
}

void mor_agr_eval(mor_set *s, mor_agreement *a)
{
    if (a->state != 0)
        return;
    a->state = 1;
    const mor_act *root = a->root_act;
    a->qual.kind = MOR_Q_AS_SHOWN;
    if (!root) {
        a->in_force = MOR_UNCHECKED;
        a->why = "the first Terms act of the agreement is not held (G9.1)";
        finish(s, a);
        return;
    }
    a->cur = root;
    mor_view v = mor_view_for(s, root);
    a->qual = v.qual;
    if (root->ev.invalid) {
        a->in_force = MOR_INVALID;
        a->why = root->ev.invalid;
        finish(s, a);
        return;
    }
    if (!counts_now(s, root)) {
        a->in_force = MOR_NOT_SHOWN;
        a->why = "the first Terms act is a tip of a fork, or built on one: it does not count (G6.2, G12.2)";
        EV(root)->chain = MOR_C_STOPPED;
        finish(s, a);
        return;
    }
    if (v.cannot) {
        a->in_force = MOR_UNCHECKED;
        a->why = "the place the Terms name gives no answer (G9.1, G7.2)";
        EV(root)->chain = MOR_C_UNCHECKED;
        finish(s, a);
        return;
    }
    const char *why = NULL;
    mor_tri r = visible(&v, root) ? in_force_cs(s, root, &v, &why) : MOR_NO;
    if (r != MOR_YES) {
        a->in_force = r == MOR_NO ? MOR_NOT_SHOWN : MOR_UNCHECKED;
        a->why = !visible(&v, root) ? "the place does not show the Terms (G7.2)"
                 : why              ? why
                 : r == MOR_NO      ? "the assents the Terms require are not all held (G4.1)"
                                    : "the in_force rule, or an agreement named as a party, could not be read (G9.1)";
        EV(root)->chain = r == MOR_NO ? MOR_C_ROOT_WAITING : MOR_C_UNCHECKED;
        EV(root)->chain_why = a->why;
        finish(s, a);
        return;
    }
    EV(root)->chain = MOR_C_VERSION;
    a->versions[a->n_versions++] = root;
    a->state = 2;
    a->in_force = MOR_SHOWN;

    for (;;) {
        const mor_act *cur = a->cur;
        if (a->n_versions >= MOR_MAX_CHAIN) {
            a->in_force = MOR_UNCHECKED;
            a->why = "the chain is longer than this verifier reads: unchecked";
            break;
        }
        v = mor_view_for(s, cur);
        a->qual = v.qual;
        if (v.cannot) {
            a->in_force = MOR_UNCHECKED;
            a->why = "the place the Terms in force name gives no answer (G9.1, G7.2)";
            break;
        }
        const mor_act *ups[MOR_MAX_LIST], *res[MOR_MAX_LIST];
        int nups = 0, nres = 0, end, i = tr_range(s, &cur->fp, &end);
        bool cannot = false;
        for (int k = i; k < end; k++) {
            mor_act *c = s->tr_by_repl[k];
            if (c->ev.invalid)
                continue;
            if (!counts_now(s, c)) {
                c->ev.chain = MOR_C_STOPPED;
                c->ev.chain_why = "it is a tip of a fork, set aside, or built on one: it does not count (G12.2, G12.3)";
                continue;
            }
            if (!visible(&v, c)) {
                c->ev.chain = MOR_C_NOT_SHOWN;
                c->ev.chain_why = "the place does not show it: it does not count yet (G7.2)";
                continue;
            }
            if (nups >= MOR_MAX_LIST || nres >= MOR_MAX_LIST) {
                cannot = true;
                continue;
            }
            if (c->n_resolves > 0) {
                if (c->has_continues) {
                    res[nres++] = c;
                } else {
                    c->ev.chain = MOR_C_RESOLVES_NOTHING;
                    c->ev.chain_why = "an act that names resolves and no continues resolves nothing (G12.3)";
                }
                continue;
            }
            if (cur->condition.present) {
                /* G4.11: the condition of the version in force, asked of the proposed update */
                mor_k4_out ko;
                mor_tri q = mor_k4_call(s, &cur->condition, MOR_K4_CONDITION, cur, c, &v, &ko);
                c->ev.cond_asked = true;
                c->ev.cond = q;
                c->ev.cond_terms = cur;
                c->ev.cond_ground = ko.ground;
                if (ko.ground_act)
                    c->ev.cond_ground = mor_sprintf(s, "%s: an act signed by %s (G8)", ko.ground ? ko.ground : "", ko.ground_act->signer);
                if (ko.ground_is_time)
                    mor_time_for(s, cur, &(uint64_t){0}, &c->ev.cond_qual);
                else
                    c->ev.cond_qual = v.qual;
                if (q == MOR_NO) {
                    c->ev.chain = MOR_C_NOT_VALID;
                    c->ev.chain_why = "the update condition answers not valid: it replaces nothing, whoever signed it (G4.11(c))";
                    continue;
                }
                if (q == MOR_CANNOT) {
                    c->ev.chain = MOR_C_UNCHECKED;
                    c->ev.chain_why = "the update condition's cMIP is not held: whether it replaces is unchecked (G4.11(h))";
                    cannot = true;
                    continue;
                }
            }
            const char *iwhy = NULL, *how = NULL;
            bool bp = false;
            int deleg = 0;
            mor_tri inf = in_force_cs(s, c, &v, &iwhy);
            mor_tri sg = sigs_ok(s, cur, c, &v, false, &bp, &deleg, &how);
            if (deleg == 1)
                inf = MOR_YES; /* G5.5: it replaces when it carries that part alone */
            if (sg == MOR_YES && inf == MOR_YES) {
                c->ev.by_power = bp;
                c->ev.signed_how = how;
                ups[nups++] = c;
            } else if (deleg != 2 && (sg == MOR_NO || inf == MOR_NO)) {
                c->ev.chain = MOR_C_NOT_SHOWN;
                c->ev.chain_why = sg == MOR_NO ? how
                                  : iwhy       ? iwhy
                                               : "the assents its own in_force requires are not all held; one not held may exist (G4.1, G4.7)";
            } else {
                c->ev.chain = MOR_C_UNCHECKED;
                c->ev.chain_why = sg == MOR_CANNOT ? how : "its own in_force rule could not be applied (G9.1)";
                cannot = true;
            }
        }
        if (cannot) {
            a->in_force = MOR_UNCHECKED;
            a->why = "whether an update replaced the last version shown could not be checked: which Terms are in force is unchecked from here (G9.1)";
            for (int k = 0; k < nups; k++)
                EV(ups[k])->chain = MOR_C_UNCHECKED;
            break;
        }
        const mor_act *next = NULL;
        if (nups == 0) {
            for (int k = 0; k < nres; k++) {
                EV(res[k])->chain = MOR_C_RESOLVES_NOTHING;
                EV(res[k])->chain_why = "there is no fork of these Terms for it to resolve";
            }
            a->in_force = MOR_SHOWN;
            break;
        }
        if (nups == 1) {
            next = ups[0];
        } else {
            /* G4.7: a fork. Look for a resolving update, signed as an update of the shared Terms must be (G12.3) */
            const mor_act *ok[MOR_MAX_LIST];
            bool used[MOR_MAX_LIST];
            int nok = 0;
            for (int k = 0; k < nres; k++) {
                const char *how = NULL;
                bool bp = false;
                int deleg = 0;
                mor_tri sg = sigs_ok(s, cur, res[k], &v, true, &bp, &deleg, &how);
                if (sg == MOR_YES) {
                    EV(res[k])->by_power = bp;
                    EV(res[k])->signed_how = how;
                    ok[nok++] = res[k];
                } else if (sg == MOR_CANNOT) {
                    cannot = true;
                    EV(res[k])->chain = MOR_C_UNCHECKED;
                    EV(res[k])->chain_why = how;
                } else {
                    EV(res[k])->chain = MOR_C_NOT_SHOWN;
                    EV(res[k])->chain_why = how;
                }
            }
            memset(used, 0, sizeof used);
            const mor_act *resolver = NULL;
            const mor_act *w = resolve_group(ups, nups, ok, nok, used, 0, &resolver);
            if (!w && cannot) {
                a->in_force = MOR_UNCHECKED;
                a->why = "a fork of Terms, and whether a resolving update resolves it could not be checked (G9.1)";
                for (int k = 0; k < nups; k++)
                    EV(ups[k])->chain = MOR_C_UNCHECKED;
                break;
            }
            if (!w) {
                const mor_act *seeds[2 * MOR_MAX_LIST];
                int ns = 0;
                for (int k = 0; k < nups; k++) {
                    seeds[ns++] = ups[k];
                    if (a->n_tips < MOR_MAX_LIST)
                        a->tips[a->n_tips++] = ups[k];
                }
                mark_closure(s, seeds, ns, true, true);
                for (int k = 0; k < nok; k++) {
                    EV(ok[k])->chain = used[k] ? MOR_C_TIP : MOR_C_RESOLVES_NOTHING;
                    EV(ok[k])->chain_why = used[k] ? "rival resolving updates are themselves a fork (G12.7)"
                                                   : "it does not name every tip, or names no tip to continue (G12.3)";
                }
                a->in_force = MOR_FORK;
                a->why = "two Terms acts each replace the same Terms: both tips stop, and the Terms both replace stay in force (G4.7, G12.2)";
                break;
            }
            const mor_act *seeds[MOR_MAX_LIST];
            int ns = 0;
            for (int k = 0; k < nups; k++)
                if (ups[k] != w)
                    seeds[ns++] = ups[k];
            mark_closure(s, seeds, ns, true, false);
            for (int k = 0; k < nok; k++) {
                bool lost = used[k] && !mor_fp_eq(&ok[k]->continues, &w->fp) && member(ups, nups, &ok[k]->continues);
                EV(ok[k])->chain = !used[k] ? MOR_C_RESOLVES_NOTHING : lost ? MOR_C_SET_ASIDE : MOR_C_RESOLVER;
            }
            next = w;
        }
        EV(next)->chain = MOR_C_VERSION;
        a->versions[a->n_versions++] = next;
        a->cur = next;
    }
    finish(s, a);
}

/* ---- well-formedness: what an act itself shows ---------------------------------------------- */

static void invalid(mor_act *a, const char *why)
{
    if (!a->ev.invalid)
        a->ev.invalid = why;
}

static void wf(mor_set *s)
{
    for (int i = 0; i < s->n_acts; i++) {
        mor_act *a = s->acts[i];
        if (a->type == MOR_T_TERMS)
            a->root = a->has_replaces ? a->agreement : a->fp;
        else if (a->type == MOR_T_STATEMENT)
            a->root = a->agreement;
    }
    for (int i = 0; i < s->n_acts; i++) {
        mor_act *a = s->acts[i];
        if (!in_a_line(a))
            continue;
        if (a->type == MOR_T_TERMS) {
            bool has_role = false;
            if (a->body_bad)
                invalid(a, "invalid: the body and its cMIP MUST be named by fingerprint (G2.1)");
            if (a->in_force.bad || a->condition.bad)
                invalid(a, "invalid: every cMIP named anywhere in a Terms act MUST be named by fingerprint (G2.1)");
            for (int k = 0; k < a->n_powers; k++)
                if (a->powers[k].rule.bad || !a->powers[k].rule.present)
                    invalid(a, "invalid: a power's rule is a cMIP, which MUST be named by fingerprint (G2.1)");
            for (int k = 0; k < a->n_parties; k++) {
                const mor_party *p = &a->parties[k];
                if (p->ref.kind != MOR_REF_ROLE)
                    continue;
                has_role = true;
                if (p->role_kind == MOR_ROLE_BAD)
                    invalid(a, "invalid: a role MUST name its kind: done, received, holding, fits or future (G10.1)");
                else if (p->role_kind != MOR_ROLE_FUTURE && (!p->has_what || p->what_bad))
                    invalid(a, "invalid: a role MUST name its what, by fingerprint, except for kind future (G10.1)");
                if (!p->k12.present || p->k12.bad)
                    invalid(a, "invalid: a role MUST name the cMIP answering task K12, by fingerprint (G10.1)");
            }
            if (has_role && !a->in_force.present)
                invalid(a, "invalid: Terms that name a party by role MUST carry in_force (G4.2)");
            for (int k = 0; k < a->n_authors; k++) {
                const mor_ref *au = &a->authors[k];
                if (au->kind == MOR_REF_AGREEMENT || !mor_party_named(a, au->kind, au->name))
                    invalid(a, "invalid: authors MUST name one or more of the parties named as signers or roles (G3.1)");
            }
            if (a->has_replaces) {
                const mor_act *r = mor_find(s, &a->replaces);
                if (!r)
                    a->ev.unchecked = "the Terms it replaces are not held (G9.1)";
                else if (r->type != MOR_T_TERMS)
                    invalid(a, "invalid: replaces names an act that is not Terms (section 3.1)");
                else if (!mor_fp_eq(&r->root, &a->agreement))
                    invalid(a, "invalid: agreement does not name the root of the Terms it replaces (section 3.1)");
            }
        } else {
            const mor_act *t = mor_find(s, &a->terms);
            if (!t)
                a->ev.unchecked = "the Terms it is made within are not held (G9.1)";
            else if (t->type != MOR_T_TERMS || !mor_fp_eq(&t->root, &a->agreement))
                invalid(a, "invalid: terms does not name a Terms act of the agreement it names (section 3.2)");
            if ((is_kind(a, "assent") || is_kind(a, "counter_signature")) && a->content)
                invalid(a, "invalid: the content of an assent or of a counter signature is empty (section 3.2)");
        }
        if (a->has_continues) {
            bool in = false;
            for (int k = 0; k < a->n_resolves && !in; k++)
                in = mor_fp_eq(&a->resolves[k], &a->continues);
            if (!in)
                invalid(a, "invalid: continues is not one of the fingerprints in resolves (section 3)");
        }
        if (a->has_previous) {
            const mor_act *p = mor_find(s, &a->previous);
            if (!p) {
                if (!a->ev.unchecked)
                    a->ev.unchecked = "the act it names in previous is not held: its signer's line is unchecked before it (G9.1)";
            } else if (!in_a_line(p) || strcmp(p->signer, a->signer) != 0 || !mor_fp_eq(&p->root, &a->root)) {
                invalid(a, "invalid: previous MUST name its signer's previous act within the same agreement (G6.1)");
            }
        }
    }
}

static int cmp_terms(const void *a, const void *b)
{
    const mor_act *x = *(const mor_act *const *)a, *y = *(const mor_act *const *)b;
    int c = mor_fp_cmp(&x->terms, &y->terms);
    return c ? c : x->index - y->index;
}

static int cmp_repl(const void *a, const void *b)
{
    const mor_act *x = *(const mor_act *const *)a, *y = *(const mor_act *const *)b;
    int c = mor_fp_cmp(&x->replaces, &y->replaces);
    return c ? c : x->index - y->index;
}

/* every act after the acts it names; a fingerprint cannot name what did not yet exist */
static void topo(mor_set *s)
{
    unsigned char *st = mor_alloc(s, (size_t)s->n_acts + 1);
    int *it = mor_alloc(s, sizeof(int) * (size_t)(s->n_acts + 1));
    s->n_topo = 0;
    for (int i = 0; i < s->n_acts; i++) {
        if (st[i])
            continue;
        int sp = 0;
        s->stack[sp++] = s->acts[i];
        st[i] = 1;
        it[i] = 0;
        while (sp > 0) {
            const mor_act *c = s->stack[sp - 1];
            const mor_fp *f[MAXNAMED];
            int n = mor_named(c, f, MAXNAMED);
            bool pushed = false;
            while (it[c->index] < n && !pushed) {
                const mor_act *t = mor_find(s, f[it[c->index]++]);
                if (t && !st[t->index]) {
                    st[t->index] = 1;
                    it[t->index] = 0;
                    s->stack[sp++] = t;
                    pushed = true;
                }
            }
            if (!pushed) {
                st[c->index] = 2;
                s->topo[s->n_topo++] = s->acts[c->index];
                sp--;
            }
        }
    }
}

void mor_evaluate(mor_set *s)
{
    s->evaluated = false;
    size_t n = (size_t)s->n_acts + 1;
    s->st_by_terms = mor_alloc(s, sizeof(mor_act *) * n);
    s->tr_by_repl = mor_alloc(s, sizeof(mor_act *) * n);
    s->topo = mor_alloc(s, sizeof(mor_act *) * n);
    s->stack = mor_alloc(s, sizeof(mor_act *) * n);
    s->agr = mor_alloc(s, sizeof(mor_agreement *) * MOR_MAX_AGR);
    s->lforks = mor_alloc(s, sizeof(mor_line_fork) * n);
    s->n_agr = s->n_st = s->n_tr = 0;
    for (int i = 0; i < s->n_acts; i++)
        memset(&s->acts[i]->ev, 0, sizeof s->acts[i]->ev);
    wf(s);
    for (int i = 0; i < s->n_acts; i++) {
        mor_act *a = s->acts[i];
        if (a->type == MOR_T_STATEMENT)
            s->st_by_terms[s->n_st++] = a;
        if (a->type == MOR_T_TERMS && a->has_replaces)
            s->tr_by_repl[s->n_tr++] = a;
    }
    qsort(s->st_by_terms, (size_t)s->n_st, sizeof(mor_act *), cmp_terms);
    qsort(s->tr_by_repl, (size_t)s->n_tr, sizeof(mor_act *), cmp_repl);
    topo(s);
    for (int i = 0; i < s->n_acts; i++) {
        const mor_act *a = s->acts[i];
        if (in_a_line(a))
            mor_agr(s, &a->root);
        if (a->type == MOR_T_PAYMENT)
            mor_agr(s, &a->agreement);
        for (int k = 0; k < a->n_parties; k++)
            if (a->parties[k].ref.kind == MOR_REF_AGREEMENT)
                mor_agr(s, &a->parties[k].ref.root);
        if (a->receiver.kind == MOR_REF_AGREEMENT)
            mor_agr(s, &a->receiver.root);
        if (a->for_.kind == MOR_REF_AGREEMENT)
            mor_agr(s, &a->for_.root);
        for (int k = 0; k < a->n_lines; k++)
            if (a->lines[k].to.kind == MOR_REF_AGREEMENT)
                mor_agr(s, &a->lines[k].to.root);
    }
    /* Forks in lines, forks of Terms and powers to resolve lean on one another: three rounds, each
     * reading the marks of the one before. */
    unsigned long long sig_prev = 0;
    s->unsettled = true;
    for (int round = 0; round < 6 && s->unsettled; round++) {
        for (int i = 0; i < s->n_acts; i++) {
            mor_eval *e = &s->acts[i]->ev;
            e->rest_prev = e->rest;
            e->rest = 0;
            e->line = 0;
            e->chain = MOR_C_UNREAD;
            e->chain_why = NULL;
            e->cond_asked = e->by_power = e->cancelled = e->mask = false;
            e->signed_how = NULL;
        }
        for (int i = 0; i < s->n_agr; i++) {
            mor_agreement *a = s->agr[i];
            a->state = 0;
            a->n_versions = a->n_tips = 0;
            a->cur = NULL;
            a->why = NULL;
            a->in_force = MOR_UNCHECKED;
        }
        s->n_lforks = 0;
        s->depth = 0;
        line_forks(s);
        for (int i = 0; i < s->n_agr; i++)
            mor_agr_eval(s, s->agr[i]);
        for (int i = 0; i < s->n_acts; i++) {
            mor_act *a = s->acts[i];
            a->ev.power_resolve_prev = in_a_line(a) && a->n_resolves > 0 && a->has_continues && a->for_.kind == MOR_REF_SIGNER &&
                                       !a->ev.invalid && mor_check_power(s, a, "resolves", NULL) == MOR_YES;
        }
        /* settled when two rounds in a row read the same */
        unsigned long long sig = 1469598103934665603ull;
        for (int i = 0; i < s->n_acts; i++) {
            const mor_eval *e = &s->acts[i]->ev;
            sig = (sig ^ (e->rest * 64u + e->line + (unsigned)e->chain * 4096u + (e->power_resolve_prev ? 1u << 20 : 0u) +
                          (e->cancelled ? 1u << 21 : 0u))) * 1099511628211ull;
        }
        for (int i = 0; i < s->n_agr; i++)
            sig = (sig ^ ((unsigned)s->agr[i]->in_force + 16u * (unsigned)(s->agr[i]->cur ? s->agr[i]->cur->index + 1 : 0))) * 1099511628211ull;
        s->unsettled = round == 0 || sig != sig_prev;
        sig_prev = sig;
    }
    s->evaluated = true;
}

/* ---- questions --------------------------------------------------------------------------------- */

int mor_n_line_forks(const mor_set *s)
{
    return s->n_lforks;
}

const mor_line_fork *mor_line_fork_at(const mor_set *s, int i)
{
    return i >= 0 && i < s->n_lforks ? &s->lforks[i] : NULL;
}

void mor_terms_in_force(mor_set *s, const mor_act *t, mor_answer *out)
{
    memset(out, 0, sizeof *out);
    out->ans = MOR_NOT_SHOWN;
    if (t->type != MOR_T_TERMS) {
        out->why = "in force is said of Terms only (section 2)";
        return;
    }
    if (t->ev.invalid) {
        out->ans = MOR_INVALID;
        out->why = t->ev.invalid;
        return;
    }
    const mor_agreement *a = mor_agreement_find(s, &t->root);
    if (!a) {
        out->ans = MOR_UNCHECKED;
        out->why = "the agreement could not be read";
        return;
    }
    out->qual = a->qual; /* G4.6: never given as latest without what it stands on */
    if (t == a->cur) {
        out->ans = a->in_force == MOR_FORK ? MOR_SHOWN : a->in_force;
        out->fork = a->in_force == MOR_FORK;
        out->why = a->why ? a->why : "it is the last version shown (G4.6)";
        return;
    }
    if (mor_is_version(a, t)) {
        out->why = "Terms that replace them are in force (G4.5)";
        return;
    }
    switch (t->ev.chain) {
    case MOR_C_TIP:
        out->ans = MOR_FORK;
        out->why = "a tip of a fork of Terms: neither tip is in force until the fork is resolved (G4.7)";
        break;
    case MOR_C_NOT_VALID:
        out->ans = MOR_NOT_VALID;
        out->why = t->ev.chain_why;
        break;
    case MOR_C_UNCHECKED:
        out->ans = MOR_UNCHECKED;
        out->why = t->ev.chain_why;
        break;
    default:
        if (a->in_force == MOR_UNCHECKED && t->ev.chain == MOR_C_UNREACHED)
            out->ans = MOR_UNCHECKED;
        out->why = t->ev.chain_why ? t->ev.chain_why : a->why;
    }
}

void mor_bound_by_terms(mor_set *s, const char *signer, const mor_act *t, mor_answer *out)
{
    mor_terms_in_force(s, t, out);
    if (out->ans != MOR_SHOWN) {
        if (out->ans == MOR_NOT_SHOWN)
            out->why = "an assent binds its signer only while the Terms it names are in force (G4.3)";
        return;
    }
    s->depth = 0;
    mor_tri r = mor_bound_signer(s, signer, t);
    out->ans = r == MOR_YES ? MOR_SHOWN : r == MOR_NO ? MOR_NOT_SHOWN : MOR_UNCHECKED;
    out->why = r == MOR_YES  ? "bound through their own assent, or a power they signed for (G5.1, G5.2)"
               : r == MOR_NO ? "no assent in their name and no power they are bound by is held (G5.1)"
                             : "a rule or an act needed could not be read (G9.1)";
}

void mor_agreement_signature(mor_set *s, const mor_act *act, mor_answer *out)
{
    memset(out, 0, sizeof *out);
    out->ans = MOR_NOT_SHOWN;
    out->qual.kind = MOR_Q_AS_SHOWN;
    if (act->type != MOR_T_STATEMENT || act->for_.kind != MOR_REF_AGREEMENT) {
        out->why = "the act's for names no agreement (G1.2)";
        return;
    }
    if (!mor_counts(act)) {
        out->ans = act->ev.invalid ? MOR_INVALID : MOR_NOT_SHOWN;
        out->why = act->ev.invalid ? act->ev.invalid : "the act does not count while a fork stands, or was set aside (G12.2)";
        return;
    }
    s->depth = 0;
    const char *why = NULL;
    mor_tri r = mor_check_power(s, act, act->kind, &why);
    const mor_agreement *xa = mor_agreement_find(s, &act->for_.root);
    if (xa)
        out->qual = xa->qual;
    out->ans = r == MOR_YES ? MOR_SHOWN : r == MOR_NO ? MOR_NOT_SHOWN : MOR_UNCHECKED;
    out->why = r == MOR_YES ? "made in the agreement's name, under a power its own Terms in force give, whose rule says yes (G1.2)" : why;
}

void mor_bound_by_act(mor_set *s, const char *signer, const mor_act *act, mor_answer *out)
{
    memset(out, 0, sizeof *out);
    out->ans = MOR_NOT_SHOWN;
    out->qual.kind = MOR_Q_AS_SHOWN;
    if (act->type == MOR_T_TERMS) {
        mor_bound_by_terms(s, signer, act, out);
        return;
    }
    if (act->type != MOR_T_STATEMENT) {
        out->why = "G5 reads acts within an agreement";
        return;
    }
    if (act->ev.invalid) {
        out->ans = MOR_INVALID;
        out->why = act->ev.invalid;
        return;
    }
    out->disputed = ((act->ev.line | act->ev.rest) & MOR_M_DISPUTED) != 0;
    if (!mor_counts(act)) {
        out->why = "the act is a tip of a fork that stands, is built on one, or was set aside: it does not count (G12.2, G12.3)";
        return;
    }
    const mor_act *t = mor_find(s, &act->terms);
    const mor_agreement *a = mor_agreement_find(s, &act->root);
    if (a)
        out->qual = a->qual;
    if (is_kind(act, "assent")) {
        /* G4.3: an assent binds its signer only while the Terms it names are in force */
        if (!t) {
            out->ans = MOR_UNCHECKED;
            out->why = "the Terms the assent names are not held (G9.1)";
            return;
        }
        mor_answer inf;
        mor_terms_in_force(s, t, &inf);
        if (inf.ans != MOR_SHOWN) {
            out->ans = inf.ans == MOR_UNCHECKED ? MOR_UNCHECKED : MOR_NOT_SHOWN;
            out->why = "an assent binds its signer only while the Terms it names are in force (G4.3)";
            return;
        }
        if (act->ev.cancelled) {
            out->why = "the assent was cancelled by a counter signature (G4.10)";
            return;
        }
    }
    if (act->for_.kind == MOR_REF_NONE) {
        /* G5.1(a), G5.3: made in its signer's own name only, and never counted as made for anyone else */
        if (strcmp(act->signer, signer) == 0) {
            out->ans = MOR_SHOWN;
            out->why = "they signed it in their own name (G5.1(a))";
        } else {
            out->why = "the act names no for and no under: it is made in its signer's own name only (G5.3)";
        }
        return;
    }
    s->depth = 0;
    const char *why = NULL;
    mor_tri r = mor_check_power(s, act, act->kind, &why);
    /* G5.4: answered from G7, per the place the Terms of the power name, otherwise as shown */
    const mor_act *u = mor_find(s, &act->under);
    const mor_agreement *ua = u ? mor_agreement_find(s, &u->root) : NULL;
    if (ua)
        out->qual = ua->qual;
    if (r == MOR_YES) {
        if (act->for_.kind == MOR_REF_SIGNER) {
            r = strcmp(act->for_.name, signer) == 0 ? MOR_YES : MOR_NO;
            why = "the act's for names someone else (G5.1(b))";
        } else {
            const mor_agreement *xa = mor_agreement_find(s, &act->for_.root);
            r = xa && xa->cur ? mor_bound_signer(s, signer, xa->cur) : MOR_CANNOT;
            why = "they are not shown bound by the Terms of the agreement the act is made for (G1.2, G5.1(b))";
        }
    }
    out->ans = r == MOR_YES ? MOR_SHOWN : r == MOR_NO ? MOR_NOT_SHOWN : MOR_UNCHECKED;
    out->why = r == MOR_YES ? "made for them under a power in Terms they are bound by, whose rule says yes (G5.1(b))" : why;
}
