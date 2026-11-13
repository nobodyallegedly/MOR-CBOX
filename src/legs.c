/* legs.c - G1.4 (receiver for an act), G8, G10 (spots), G11 (legs, reports, contest), G14 (the baker). */
#include "internal.h"

#include <string.h>

static mor_ans tri_ans(mor_tri t)
{
    return t == MOR_YES ? MOR_SHOWN : t == MOR_NO ? MOR_NOT_SHOWN : MOR_UNCHECKED;
}

static bool live(const mor_agreement *a)
{
    return a && a->cur && (a->in_force == MOR_SHOWN || a->in_force == MOR_FORK);
}

/* ---- G10: spots --------------------------------------------------------------------- */

void mor_spot_eval(mor_set *s, const mor_act *terms, const mor_party *role, const mor_view *v, mor_spot *out)
{
    memset(out, 0, sizeof *out);
    out->terms = terms;
    out->role = role;
    out->ans.qual = v->qual;
    out->takes_new.qual = v->qual;
    const mor_agreement *ag = mor_agreement_find(s, &terms->root);
    out->ans.fork = ag && ag->in_force == MOR_FORK;
    if (terms->has_envelope) {
        out->has_work = true;
        mor_work_read(s, terms, &out->work, NULL);
    }
    const mor_cmip *c = role->k12.bad ? NULL : mor_cmip_find(s, &role->k12.cmip, 12);
    if (!c || !c->k12 || v->cannot) {
        out->ans.ans = MOR_UNCHECKED;
        out->ans.why = v->cannot ? "the place named gives no answer (G9.1)" : "the K12 cMIP the role names is not held (G9.1, G10.5)";
        out->takes_new.ans = MOR_UNCHECKED;
        out->takes_new.why = out->ans.why;
        return;
    }
    mor_k12_in in;
    memset(&in, 0, sizeof in);
    in.set = s;
    in.terms = terms;
    in.role = role;
    in.has_time = mor_time_for(s, terms, &in.time, &in.time_qual) == MOR_YES;
    mor_k12_out kout, *ko = &kout;
    memset(ko, 0, sizeof *ko);
    mor_view save = s->view;
    s->view = *v;
    c->k12(&in, ko);
    s->view = save;
    if (ko->ans == MOR_CANNOT) {
        out->ans.ans = MOR_UNCHECKED;
        out->ans.why = ko->cause ? ko->cause : "the K12 cMIP could not be applied (G9.1)";
        out->takes_new.ans = MOR_UNCHECKED;
        return;
    }
    out->many = ko->many;
    out->n = ko->n;
    out->fill = mor_alloc(s, sizeof(mor_filling) * (size_t)(ko->n + 1));
    for (int i = 0; i < ko->n; i++) {
        mor_filling *f = &out->fill[i];
        f->signer = ko->fill[i].signer;
        f->n_proof = ko->fill[i].n_proof;
        for (int k = 0; k < f->n_proof; k++) {
            f->proof[k] = ko->fill[i].proof[k];
            if ((f->proof[k]->ev.line | mor_restmark(s, f->proof[k])) & MOR_M_DISPUTED)
                f->disputed = true;
        }
        f->earned_under = ko->fill[i].earned_under;
        if (f->disputed)
            out->ans.disputed = true;
        /* G10.6: which later versions change this filling's rights, and what that rests on */
        const mor_act *e = f->earned_under;
        if (!e || !ag)
            continue;
        int at = -1;
        for (int k = 0; k < ag->n_versions; k++)
            if (ag->versions[k] == e)
                at = k;
        if (at < 0)
            continue;
        for (int k = at + 1; k < ag->n_versions && f->n_changes < MOR_MAX_LIST; k++) {
            const mor_act *later = ag->versions[k];
            if (!f->changes)
                f->changes = mor_alloc(s, sizeof(mor_rights_change) * MOR_MAX_LIST);
            mor_rights_change *ch = &f->changes[f->n_changes];
            bool own = false;
            int end, st = mor_st_range(s, &later->fp, &end);
            for (int q = st; q < end && !own; q++) {
                const mor_act *x = s->st_by_terms[q];
                own = x->kind && strcmp(x->kind, "assent") == 0 && x->for_.kind == MOR_REF_NONE && strcmp(x->signer, f->signer) == 0 &&
                      !x->ev.cancelled && mor_usable(s, x, v);
            }
            if (own) {
                ch->version = later;
                ch->ans = MOR_SHOWN;
                ch->rests_on = "the holder's own assent to it (G10.6(a))";
                f->n_changes++;
            } else if (e->condition.present) {
                mor_k4_out k4;
                mor_tri q = mor_k4_call(s, &e->condition, MOR_K4_CONDITION, e, later, v, &k4);
                if (q == MOR_NO)
                    continue; /* that version does not reach this filling's rights */
                ch->version = later;
                ch->ans = q == MOR_YES ? MOR_VALID : MOR_UNCHECKED;
                ch->rests_on = mor_sprintf(s, "the update condition %s of the version it was earned under answers %s%s%s (G10.6(b))",
                                           mor_short(s, &e->condition.cmip), q == MOR_YES ? "valid" : "nothing: its cMIP is not held",
                                           k4.ground ? ": " : "", k4.ground ? k4.ground : "");
                f->n_changes++;
            }
            /* no condition named by the filling's own version: only (a) changes its rights */
        }
    }
    out->ans.ans = ko->n > 0 ? MOR_SHOWN : MOR_NOT_SHOWN;
    out->ans.why = ko->n > 0 ? "each filling is shown with its proof, each act reported by who signed it (G10.2, G8.4)"
                             : "no proof held earns the spot (G10.2)";
    out->takes_new.ans = tri_ans(ko->takes_new == MOR_CANNOT ? MOR_CANNOT : MOR_YES);
    out->takes = ko->takes_new == MOR_YES;
    out->takes_new.why = ko->cause;
    if (ko->cause_is_time)
        out->takes_new.qual = in.time_qual;
}

/* who fills a role, and nothing more: for the grammar's own use, with no allocation */
mor_tri mor_spot_fillers(mor_set *s, const mor_act *terms, const mor_party *role, const mor_view *v, const char **who, int *n)
{
    *n = 0;
    const mor_cmip *c = role->k12.bad ? NULL : mor_cmip_find(s, &role->k12.cmip, 12);
    if (!c || !c->k12 || v->cannot)
        return MOR_CANNOT;
    mor_k12_in in;
    mor_k12_out ko;
    memset(&in, 0, sizeof in);
    memset(&ko, 0, sizeof ko);
    in.set = s;
    in.terms = terms;
    in.role = role;
    in.has_time = mor_time_for(s, terms, &in.time, &in.time_qual) == MOR_YES;
    mor_view save = s->view;
    s->view = *v;
    c->k12(&in, &ko);
    s->view = save;
    if (ko.ans == MOR_CANNOT)
        return MOR_CANNOT;
    for (int i = 0; i < ko.n && i < MOR_MAX_LIST; i++)
        who[(*n)++] = ko.fill[i].signer;
    return MOR_YES;
}

void mor_spot_read_version(mor_set *s, const mor_act *terms, const char *role, mor_spot *out)
{
    memset(out, 0, sizeof *out);
    const mor_party *p = terms->type == MOR_T_TERMS ? mor_party_named(terms, MOR_REF_ROLE, role) : NULL;
    if (!p) {
        out->ans.ans = MOR_NOT_SHOWN;
        out->ans.why = "the Terms name no such role";
        return;
    }
    mor_view v = mor_view_for(s, terms);
    s->depth = 0;
    mor_spot_eval(s, terms, p, &v, out);
}

void mor_spot_read(mor_set *s, const mor_fp *root, const char *role, mor_spot *out)
{
    memset(out, 0, sizeof *out);
    const mor_agreement *ag = mor_agreement_find(s, root);
    if (!ag || !ag->root_act || ag->in_force == MOR_UNCHECKED) {
        out->ans.ans = MOR_UNCHECKED;
        out->ans.why = "which Terms of the agreement are in force is unchecked (G9.1)";
        return;
    }
    if (!live(ag)) {
        out->ans.ans = MOR_NOT_SHOWN;
        out->ans.why = "no Terms of the agreement are shown in force (G10.2)";
        return;
    }
    /* G10.2: read from the Terms in force; while a fork stands, from the Terms both branches replace */
    mor_spot_read_version(s, ag->cur, role, out);
}

/* ---- G14: the baker --------------------------------------------------------------------- */

static mor_ans work_rec(mor_set *s, const mor_act *t, int depth, const char **holder, const char **baker, const char **why)
{
    *holder = NULL;
    if (depth > MOR_MAX_DEPTH) {
        *why = "the succession is longer than this verifier follows: unchecked";
        return MOR_UNCHECKED;
    }
    const mor_act *env = mor_find(s, &t->envelope);
    if (!env || env->type != MOR_T_ENVELOPE) {
        *why = "the envelope is not held (G14.3, G9.1)";
        return MOR_UNCHECKED;
    }
    *baker = env->baker; /* G14.1: reported as the envelope states it */
    bool cannot = false, named = false;
    s->depth = 0;
    if (mor_party_named(t, MOR_REF_SIGNER, env->baker)) {
        named = true;
        mor_tri b = mor_bound_signer(s, env->baker, t);
        if (b == MOR_YES) {
            *holder = env->baker;
            *why = "the baker is one of their parties, bound by them (G14.2(a))";
            return MOR_SHOWN;
        }
        if (b == MOR_CANNOT)
            cannot = true;
    }
    bool has_role = false, lacking = false;
    for (int i = 0; i < t->n_parties; i++)
        has_role = has_role || t->parties[i].ref.kind == MOR_REF_ROLE;
    /* G14.2(b): a succession; each agreement names the one before it (G13.2) */
    for (int i = 0; i < t->n_relies_on; i++) {
        const mor_act *p = mor_find(s, &t->relies_on[i]);
        if (!p) {
            if (!mor_cmip_find(s, &t->relies_on[i], 0))
                lacking = true;
            continue;
        }
        if (p->type != MOR_T_TERMS || !p->has_envelope || !mor_fp_eq(&p->envelope, &t->envelope) || mor_fp_eq(&p->root, &t->root))
            continue;
        named = true;
        const char *h = NULL, *b2 = NULL, *w2 = NULL;
        mor_ans pa = work_rec(s, p, depth + 1, &h, &b2, &w2);
        if (pa == MOR_UNCHECKED)
            cannot = true;
        if (pa != MOR_SHOWN || !h)
            continue;
        /* the baker's rights pass by a Statement of the stub kind "pass-rights", made by whoever holds them (P18) */
        for (int hop = 0; hop < MOR_MAX_DEPTH; hop++) {
            const char *nh = NULL;
            for (int q = 0; q < s->n_st; q++) {
                const mor_act *x = s->st_by_terms[q];
                if (!mor_fp_eq(&x->root, &p->root) || !x->kind_is_fp || strcmp(x->signer, h) != 0 || !mor_counts(x) || !x->content)
                    continue;
                const mor_cmip *c = mor_cmip_find(s, &x->kind_fp, 11);
                if (!c) {
                    cannot = true; /* a kind this verifier cannot read might pass them */
                    continue;
                }
                if (c->k11_meaning && strcmp(c->k11_meaning, "pass-rights") == 0 && strcmp(x->content, h) != 0)
                    nh = x->content;
            }
            if (!nh)
                break;
            h = nh;
        }
        if (mor_party_named(t, MOR_REF_SIGNER, h)) {
            mor_tri b = mor_bound_signer(s, h, t);
            if (b == MOR_YES) {
                *holder = h;
                *why = "one of their parties, bound by them, holds the baker's rights through a succession (G14.2(b))";
                return MOR_SHOWN;
            }
            if (b == MOR_CANNOT)
                cannot = true;
        }
    }
    if (cannot) {
        *why = "an act, a cMIP or an agreement of the succession is not held, or could not be read (G14.3, G9.1)";
        return MOR_UNCHECKED;
    }
    if (named) {
        *why = "they name the baker or a succession, and the acts held do not establish it (G14.3)";
        return MOR_NOT_SHOWN;
    }
    if (lacking) {
        *why = "an act named in relies_on is not held; it may be an agreement of a succession (G14.3, G9.1)";
        return MOR_UNCHECKED;
    }
    if (has_role) {
        *why = "a party is named by role, which the baker might fill: not established by the acts held (G14.3)";
        return MOR_NOT_SHOWN;
    }
    *why = "the Terms name neither the baker among their parties nor a succession: they cannot count as an agreement for that work (G14.3)";
    return MOR_INVALID;
}

void mor_work_read(mor_set *s, const mor_act *terms, mor_answer *out, const char **baker)
{
    memset(out, 0, sizeof *out);
    const char *b = NULL, *h = NULL;
    if (baker)
        *baker = NULL;
    if (terms->type != MOR_T_TERMS || !terms->has_envelope) {
        out->ans = MOR_NOT_SHOWN;
        out->why = "the Terms name no envelope: G14 reads only Terms that name one (P18)";
        return;
    }
    int save = s->depth;
    out->ans = work_rec(s, terms, 0, &h, &b, &out->why);
    s->depth = save;
    const mor_agreement *ag = mor_agreement_find(s, &terms->root);
    out->qual.kind = MOR_Q_AS_SHOWN;
    if (ag)
        out->qual = ag->qual;
    if (out->ans == MOR_INVALID)
        out->qual.kind = MOR_Q_NONE;
    if (baker)
        *baker = b;
}

/* ---- G1.4: receiver for an act --------------------------------------------------------------- */

void mor_receiver_for(mor_set *s, const mor_fp *root, const mor_act *version, mor_receiver *out)
{
    memset(out, 0, sizeof *out);
    out->ans.ans = MOR_NOT_SHOWN;
    const mor_fp *r = root;
    const mor_act *named = version;
    mor_fp seen[MOR_MAX_DEPTH + 1];
    for (int depth = 0; depth <= MOR_MAX_DEPTH; depth++) {
        for (int i = 0; i < depth; i++)
            if (mor_fp_eq(&seen[i], r)) {
                out->ans.ans = MOR_NOT_SHOWN;
                out->ans.why = "an agreement is met a second time on the way: no one receives (G1.4)";
                return;
            }
        seen[depth] = *r;
        const mor_agreement *ag = mor_agreement_find(s, r);
        if (!ag || !ag->root_act) {
            out->ans.ans = MOR_UNCHECKED;
            out->ans.why = "the verifier lacks that agreement's acts (G1.4, G9.1)";
            return;
        }
        const mor_act *v = NULL;
        if (named) {
            /* the version the act names (G13.1): read as it was, whatever replaced it since */
            if (named->type != MOR_T_TERMS || !mor_fp_eq(&named->root, r)) {
                out->ans.why = "the act names, as the version it read, an act that is not Terms of that agreement";
                return;
            }
            if (mor_is_version(ag, named)) {
                v = named;
            } else if (named->ev.chain == MOR_C_TIP || named->ev.chain == MOR_C_STOPPED) {
                /* built on a tip while the fork stands: answered from the Terms both branches replace (G4.7, G12.2) */
                v = ag->cur;
                out->ans.fork = true;
                out->ans.qual = ag->qual;
            } else if (named->ev.chain == MOR_C_SET_ASIDE) {
                v = named; /* G12.5: as it would have been read had that tip stood alone, disputed */
                out->ans.disputed = true;
            } else if (ag->in_force == MOR_UNCHECKED || named->ev.chain == MOR_C_UNCHECKED) {
                out->ans.ans = MOR_UNCHECKED;
                out->ans.why = "whether the Terms the act names are a version of that agreement is unchecked (G9.1)";
                return;
            } else {
                out->ans.why = "the Terms the act names are not shown as a version of that agreement (G4.8)";
                return;
            }
        } else {
            /* the act names no version: the Terms in force, per place or as shown (G1.4, G4.6) */
            if (out->ans.qual.kind == MOR_Q_NONE)
                out->ans.qual = ag->qual;
            if (ag->in_force == MOR_UNCHECKED) {
                out->ans.ans = MOR_UNCHECKED;
                out->ans.why = ag->why;
                return;
            }
            if (!live(ag)) {
                out->ans.why = "no Terms of that agreement are shown in force (G1.4, G4.6)";
                return;
            }
            if (ag->in_force == MOR_FORK)
                out->ans.fork = true; /* shown with the answer; no branch is chosen (G4.7) */
            v = ag->cur;
        }
        if (out->n_read <= MOR_MAX_DEPTH)
            out->read[out->n_read++] = v;
        if (v->has_envelope && !out->has_work) {
            out->has_work = true;
            mor_work_read(s, v, &out->work, NULL); /* G14.3: given with every answer that rests on those Terms */
        }
        switch (v->receiver.kind) {
        case MOR_REF_NONE:
            out->ans.why = "a version read names no receiver: no one receives (G1.4)";
            return;
        case MOR_REF_SIGNER:
            if (!mor_party_named(v, MOR_REF_SIGNER, v->receiver.name)) {
                out->ans.why = "the receiver named is not one of the Terms' own parties (G1.3)";
                return;
            }
            out->who = v->receiver.name;
            out->ans.ans = MOR_SHOWN;
            out->ans.why = "the receiver named by the version read (G1.4)";
            return;
        case MOR_REF_ROLE: {
            mor_spot sp;
            mor_spot_read_version(s, v, v->receiver.name, &sp);
            if (out->ans.qual.kind == MOR_Q_NONE)
                out->ans.qual = sp.ans.qual;
            if (sp.ans.ans == MOR_UNCHECKED) {
                out->ans.ans = MOR_UNCHECKED;
                out->ans.why = sp.ans.why;
            } else if (sp.n == 0) {
                out->ans.why = "the spot named as receiver is not filled: no one receives (G1.4)";
            } else if (sp.n > 1) {
                out->ans.why = "the spot named as receiver has several fillings; which of them receives is not said (G1.4)";
            } else {
                out->who = sp.fill[0].signer;
                out->ans.ans = MOR_SHOWN;
                out->ans.disputed = out->ans.disputed || sp.fill[0].disputed;
                out->ans.why = "the signer who fills the spot named as receiver (G1.4, G10.2)";
            }
            return;
        }
        default:
            r = &v->receiver.root; /* another agreement: read its receiver in the same way */
            named = NULL;
        }
    }
    out->ans.ans = MOR_UNCHECKED;
    out->ans.why = "the receiver points through more agreements than this verifier follows (P8): unchecked";
}

/* ---- G11: legs ------------------------------------------------------------------------------- */

static bool amount_eq(const mor_amount *a, const mor_amount *b)
{
    return a->n == b->n && strcmp(a->unit, b->unit) == 0;
}

static const char *line_receiver(mor_set *s, const mor_act *report, const mor_rline *l)
{
    if (l->to.kind == MOR_REF_SIGNER)
        return l->to.name;
    if (l->to.kind != MOR_REF_AGREEMENT)
        return NULL;
    /* passed on to an agreement: its receiver for the act (G1.4); the version is one the report names, if any */
    const mor_act *ver = NULL;
    for (int i = 0; i < report->n_relies_on && !ver; i++) {
        const mor_act *t = mor_find(s, &report->relies_on[i]);
        if (t && t->type == MOR_T_TERMS && mor_fp_eq(&t->root, &l->to.root))
            ver = t;
    }
    mor_receiver rc;
    mor_receiver_for(s, &l->to.root, ver, &rc);
    return rc.ans.ans == MOR_SHOWN ? rc.who : NULL;
}

void mor_k8_read(mor_set *s, const mor_act *t, mor_k8 *out)
{
    memset(out, 0, sizeof *out);
    out->terms = t;
    out->conforms = MOR_UNCHECKED;
    out->path_beyond = MOR_UNCHECKED;
    out->call_status = MOR_NOT_SHOWN;
    if (t->type != MOR_T_TERMS)
        return;
    mor_terms_in_force(s, t, &out->in_force);
    if (t->has_envelope) {
        out->has_work = true;
        mor_work_read(s, t, &out->work, NULL);
    }
    /* G11.2: do the Terms hold the Control task? */
    bool cannot = false;
    mor_k1_out *k1 = mor_alloc(s, sizeof *k1);
    const mor_ref *rcv = &t->receiver;
    bool own = (rcv->kind == MOR_REF_SIGNER || rcv->kind == MOR_REF_ROLE) && mor_party_named(t, rcv->kind, rcv->name);
    mor_tri k1r = MOR_NO;
    if (own) {
        k1r = mor_k1_read(s, t, k1);
        if (k1r != MOR_YES)
            cannot = true;
        for (int i = 0; k1r == MOR_YES && i < k1->n_shares; i++)
            if (k1->shares[i].share > 0 && !mor_ref_eq(&k1->shares[i].party, rcv)) {
                out->holds = MOR_YES;
                out->holds_why = "they name one of their own parties as receiver and give a part to another party (G11.2)";
            }
    }
    for (int i = 0; i < t->n_relies_on && out->holds != MOR_YES; i++) {
        if (mor_cmip_find(s, &t->relies_on[i], 8)) {
            out->holds = MOR_YES;
            out->holds_why = "they name a cMIP that answers the Control task (G11.2)";
        } else if (!mor_find(s, &t->relies_on[i]) && !mor_cmip_find(s, &t->relies_on[i], 0)) {
            cannot = true; /* a piece named and not held might be a K8 cMIP */
        }
    }
    if (out->holds != MOR_YES) {
        out->holds = cannot ? MOR_CANNOT : MOR_NO;
        out->holds_why = cannot ? "the words, or a piece the Terms name, could not be read: whether they hold the Control task is unchecked (G9.1)"
                                : "they name no K8 cMIP, and no receiver among their parties that holds a part for another (G11.2)";
    }
    for (int i = 0; i < t->n_calls && !out->has_call; i++)
        if (strcmp(t->calls[i].task, "K8") == 0) {
            out->has_call = true;
            out->from = t->calls[i].from;
        }
    /* the doer: the controller, whoever the Terms name as receiver among their own parties */
    if (own && rcv->kind == MOR_REF_SIGNER) {
        out->doer = rcv->name;
    } else if (own) {
        mor_spot sp;
        mor_spot_read_version(s, t, rcv->name, &sp);
        if (sp.n == 1)
            out->doer = sp.fill[0].signer;
    }
    if (out->holds == MOR_CANNOT) {
        out->call_status = MOR_UNCHECKED;
    } else if (out->holds == MOR_YES && !out->has_call) {
        out->call_status = MOR_NO_REPORT_CALLED_FOR; /* G11.4(d) */
    }
    if (!out->has_call)
        return;
    for (int i = 0; i < s->n_acts && out->n_reports < MOR_MAX_LIST; i++)
        if (s->acts[i]->type == MOR_T_REPORT && mor_fp_eq(&s->acts[i]->call, &t->fp))
            out->reports[out->n_reports++] = s->acts[i];
    if (out->n_reports == 0) {
        /* G11.4(d): unanswered is said only by a verifier that reads as a side of that leg */
        bool side = false;
        const char *rd = s->opt.reader;
        if (rd) {
            side = out->doer && strcmp(rd, out->doer) == 0;
            for (int i = 0; k1r == MOR_YES && i < k1->n_shares && !side; i++)
                side = k1->shares[i].party.kind == MOR_REF_SIGNER && strcmp(k1->shares[i].party.name, rd) == 0;
        }
        out->call_status = out->holds == MOR_CANNOT ? MOR_UNCHECKED : side ? MOR_UNANSWERED : MOR_NOT_SHOWN;
        return;
    }
    out->call_status = MOR_SHOWN;
    /* G11.4(b),(c): two reports that name the same settlement are tied by it */
    for (int i = 0; i < out->n_reports; i++) {
        const mor_act *rp = out->reports[i];
        if (rp->side != MOR_SIDE_DOER || !out->doer || strcmp(rp->signer, out->doer) != 0)
            continue;
        for (int k = 0; k < rp->n_lines && out->n_ties < MOR_MAX_LIST; k++) {
            const mor_rline *l = &rp->lines[k];
            if (l->kind != MOR_L_PASSED)
                continue;
            mor_tie *tie = &out->ties[out->n_ties++];
            tie->settlement = l->via;
            tie->doer_report = rp;
            tie->doer_line = l;
            tie->receiving_side = line_receiver(s, rp, l);
            tie->status = MOR_ONE_SIDE_ONLY;
            bool differ = false, agree = false;
            for (int q = 0; q < out->n_reports; q++) {
                const mor_act *rr = out->reports[q];
                if (rr->side != MOR_SIDE_RECEIVING || !tie->receiving_side || strcmp(rr->signer, tie->receiving_side) != 0)
                    continue;
                for (int z = 0; z < rr->n_lines; z++) {
                    const mor_rline *m = &rr->lines[z];
                    if (m->kind != MOR_L_RECEIVED || !mor_fp_eq(&m->via, &l->via))
                        continue;
                    tie->recv_report = rr;
                    tie->recv_line = m;
                    if (amount_eq(&m->amount, &l->amount))
                        agree = true;
                    else
                        differ = true;
                }
            }
            if (differ)
                tie->status = MOR_MISMATCHED; /* both shown, neither chosen */
            else if (agree)
                tie->status = MOR_MATCHED;
            if (!tie->receiving_side)
                tie->why = "who receives for the agreement it was passed on to is not shown (G1.4)";
            const mor_act *st = mor_find(s, &l->via);
            if (!st) {
                tie->status = MOR_UNCHECKED;
                tie->why = "the settlement the reports name is not held (G11.4(d), G9.1)";
            }
        }
    }
    for (int i = 0; i < out->n_reports; i++) {
        const mor_act *rr = out->reports[i];
        if (rr->side != MOR_SIDE_RECEIVING)
            continue;
        for (int z = 0; z < rr->n_lines && out->n_ties < MOR_MAX_LIST; z++) {
            const mor_rline *m = &rr->lines[z];
            if (m->kind != MOR_L_RECEIVED)
                continue;
            bool tied = false;
            for (int q = 0; q < out->n_ties && !tied; q++)
                tied = out->ties[q].recv_line == m;
            if (tied)
                continue;
            mor_tie *tie = &out->ties[out->n_ties++];
            tie->settlement = m->via;
            tie->recv_report = rr;
            tie->recv_line = m;
            tie->receiving_side = rr->signer;
            tie->status = mor_find(s, &m->via) ? MOR_ONE_SIDE_ONLY : MOR_UNCHECKED;
            tie->why = "no report of the doer held names this settlement as passed on to this signer";
        }
    }
}

void mor_payment_read(mor_set *s, const mor_act *p, mor_payment *out)
{
    memset(out, 0, sizeof *out);
    out->payment = p;
    out->status = MOR_NOT_SHOWN;
    if (p->type != MOR_T_PAYMENT)
        return;
    const mor_act *ver = NULL;
    if (p->has_version) {
        ver = mor_find(s, &p->version);
        if (!ver) {
            out->receiver.ans.ans = MOR_UNCHECKED;
            out->receiver.ans.why = "the version the payment names is not held (G9.1)";
            out->status = MOR_UNCHECKED;
            out->why = out->receiver.ans.why;
            return;
        }
    }
    s->depth = 0;
    mor_receiver_for(s, &p->agreement, ver, &out->receiver);
    out->disputed = out->receiver.ans.disputed || (p->ev.rest & MOR_M_DISPUTED);
    out->fork = out->receiver.ans.fork;
    out->settlement = mor_find(s, &p->settlement);
    if (out->receiver.ans.ans != MOR_SHOWN) {
        out->status = out->receiver.ans.ans;
        out->why = "the receiving side of this leg is not established, so no confirmation can be read as its own (G11.7, G1.4)";
        return;
    }
    /* G11.7: the confirmation names the payment by the settlement both sides name */
    bool agree = false, differ = false;
    for (int i = 0; i < s->n_acts; i++) {
        const mor_act *r = s->acts[i];
        if (r->type != MOR_T_REPORT || r->side != MOR_SIDE_RECEIVING || !mor_fp_eq(&r->call, &p->settlement) ||
            strcmp(r->signer, out->receiver.who) != 0)
            continue;
        for (int k = 0; k < r->n_lines; k++) {
            const mor_rline *l = &r->lines[k];
            if (l->kind != MOR_L_RECEIVED || !mor_fp_eq(&l->via, &p->settlement))
                continue;
            out->confirmation = r;
            out->confirmation_line = l;
            if (amount_eq(&l->amount, &p->amount))
                agree = true;
            else
                differ = true;
        }
    }
    /* never "received" on the buyer's word alone (G8.3, G11.7) */
    out->status = differ ? MOR_MISMATCHED : agree ? MOR_MATCHED : MOR_ONE_SIDE_ONLY;
    out->why = differ  ? "the receiving side's confirmation states another amount: both shown, neither chosen (G11.7, G11.4(c))"
               : agree ? "the receiving side confirmed the same amount for the settlement both name (G11.7)"
                       : "the receiving side has not confirmed: the payer's word alone (G11.7, G8.3)";
    if (!out->settlement) {
        out->status = MOR_UNCHECKED;
        out->why = "the settlement the payment names is not held (G11.4(d), G9.1)";
    }
}

static mor_leg *add_leg(mor_trace *tr, int depth)
{
    if (tr->n >= MOR_MAX_LEGS)
        return NULL;
    mor_leg *l = &tr->legs[tr->n++];
    memset(l, 0, sizeof *l);
    l->depth = depth;
    return l;
}

static void follow(mor_set *s, mor_trace *tr, const char *who, const mor_act *d, const mor_fp *via, int depth)
{
    if (depth > MOR_MAX_DEPTH) {
        mor_leg *l = add_leg(tr, depth);
        if (l) {
            l->doer = who;
            l->status = MOR_UNCHECKED;
            l->note = "more legs than this verifier reads in one result: unchecked";
        }
        return;
    }
    mor_k8 *k8 = mor_alloc(s, sizeof *k8);
    mor_k8_read(s, d, k8);
    if (k8->holds == MOR_NO || !k8->doer || strcmp(k8->doer, who) != 0)
        return; /* who received holds for no one else under those Terms: the result ends at this leg */
    if (k8->holds == MOR_CANNOT || !k8->has_call) {
        mor_leg *hd = add_leg(tr, depth);
        if (!hd)
            return;
        hd->doer = who;
        hd->under = d;
        hd->status = k8->holds == MOR_CANNOT ? MOR_UNCHECKED : MOR_NO_REPORT_CALLED_FOR;
        hd->note = k8->holds == MOR_CANNOT
                       ? "whether the Terms that name this receiver hold the Control task is unchecked (G9.1)"
                       : "the Terms that hold the task carry no call; the legs beyond are not shown as part of this result (G11.4(d))";
        return;
    }
    bool any = false;
    for (int i = 0; i < k8->n_reports; i++) {
        const mor_act *rp = k8->reports[i];
        if (rp->side != MOR_SIDE_DOER || strcmp(rp->signer, who) != 0)
            continue;
        bool got = false;
        for (int k = 0; k < rp->n_lines && !got; k++)
            got = rp->lines[k].kind == MOR_L_RECEIVED && mor_fp_eq(&rp->lines[k].via, via);
        if (!got)
            continue;
        any = true;
        for (int q = 0; q < k8->n_ties; q++) {
            const mor_tie *t = &k8->ties[q];
            if (t->doer_report != rp)
                continue;
            mor_leg *l = add_leg(tr, depth);
            if (!l)
                return;
            l->doer = who;
            l->under = d;
            l->to = t->doer_line->to;
            l->receiving = t->receiving_side;
            l->has_settlement = true;
            l->settlement = t->settlement;
            l->status = t->status;
            l->doer_report = t->doer_report;
            l->recv_report = t->recv_report;
            l->note = t->why;
            if (t->doer_line->to.kind == MOR_REF_AGREEMENT && t->receiving_side) {
                const mor_act *ver = NULL;
                for (int z = 0; z < rp->n_relies_on && !ver; z++) {
                    const mor_act *x = mor_find(s, &rp->relies_on[z]);
                    if (x && x->type == MOR_T_TERMS && mor_fp_eq(&x->root, &t->doer_line->to.root))
                        ver = x;
                }
                mor_receiver *rc = mor_alloc(s, sizeof *rc);
                mor_receiver_for(s, &t->doer_line->to.root, ver, rc);
                if (rc->ans.ans == MOR_SHOWN && rc->n_read > 0)
                    follow(s, tr, rc->who, rc->read[rc->n_read - 1], &t->settlement, depth + 1);
            }
        }
    }
    if (!any) {
        mor_leg *hd = add_leg(tr, depth);
        if (!hd)
            return;
        hd->doer = who;
        hd->under = d;
        const char *rd = s->opt.reader;
        bool side = rd && strcmp(rd, who) == 0;
        /* G11.4(d): unanswered only for a reader who is a side of that leg; for any other, not shown */
        hd->status = k8->n_reports == 0 && side ? MOR_UNANSWERED : MOR_NOT_SHOWN;
        hd->note = hd->status == MOR_UNANSWERED
                       ? "the call has no report; the legs beyond are not shown as part of this result (G11.4(d))"
                       : "no report held ties this payment to what was passed on; the legs beyond are not shown (G11.4(d), G11.6)";
    }
}

void mor_trace_payment(mor_set *s, const mor_act *p, mor_trace *out)
{
    memset(out, 0, sizeof *out);
    mor_payment *pr = mor_alloc(s, sizeof *pr);
    mor_payment_read(s, p, pr);
    mor_leg *l = add_leg(out, 0);
    l->doer = p->signer; /* G11.7: the buyer, who signed the payment */
    l->receiving = pr->receiver.who;
    l->to.kind = MOR_REF_AGREEMENT;
    l->to.root = p->agreement;
    l->has_settlement = true;
    l->settlement = p->settlement;
    l->status = pr->status;
    l->under = p;
    l->recv_report = pr->confirmation;
    l->note = pr->why;
    if (pr->receiver.ans.ans == MOR_SHOWN && pr->receiver.n_read > 0)
        follow(s, out, pr->receiver.who, pr->receiver.read[pr->receiver.n_read - 1], &p->settlement, 1);
}

void mor_before_paying(mor_set *s, const mor_fp *root, mor_before *out)
{
    memset(out, 0, sizeof *out);
    const mor_fp *r = root;
    while (out->n <= MOR_MAX_DEPTH) {
        for (int i = 0; i < out->n; i++)
            if (mor_fp_eq(&out->e[i].root, r))
                return;
        mor_before_entry *e = &out->e[out->n++];
        e->root = *r;
        const mor_agreement *ag = mor_agreement_find(s, r);
        if (!ag || !ag->root_act || ag->in_force == MOR_UNCHECKED) {
            e->terms = MOR_UNCHECKED; /* G11.4(e): unchecked where it lacks them */
            return;
        }
        e->terms = ag->in_force;
        if (!live(ag))
            return;
        mor_k8_read(s, ag->cur, &e->k8);
        if (ag->cur->receiver.kind != MOR_REF_AGREEMENT)
            return;
        r = &ag->cur->receiver.root;
    }
}

/* ---- G11.8: the contest ---------------------------------------------------------------------- */

void mor_contest_read(mor_set *s, const mor_act *st, mor_contest *out)
{
    memset(out, 0, sizeof *out);
    out->ans.ans = MOR_NOT_SHOWN;
    out->buyers = MOR_NOT_SHOWN;
    out->said_by = st->signer;
    out->ans.qual.kind = MOR_Q_AS_SHOWN;
    if (st->type != MOR_T_STATEMENT || !st->kind || strcmp(st->kind, "contest") != 0) {
        out->ans.why = "not a Statement of kind contest (G11.8)";
        return;
    }
    if (st->ev.invalid) {
        out->ans.ans = MOR_INVALID;
        out->ans.why = st->ev.invalid;
        return;
    }
    if (!mor_counts(st)) {
        out->ans.why = "the act is a tip of a fork in its signer's line, built on one, or set aside (G6.2, G12.2)";
        return;
    }
    const mor_act *t = mor_find(s, &st->terms);
    if (!t) {
        out->ans.ans = MOR_UNCHECKED;
        out->ans.why = "the version it is made against is not held (G9.1)";
        return;
    }
    const mor_agreement *ag = mor_agreement_find(s, &st->root);
    if (ag)
        out->ans.qual = ag->qual;
    out->ans.disputed = ((st->ev.line | st->ev.rest) & MOR_M_DISPUTED) != 0;
    if (!ag || (!mor_is_version(ag, t) && t->ev.chain != MOR_C_SET_ASIDE)) {
        out->ans.ans = ag && ag->in_force == MOR_UNCHECKED ? MOR_UNCHECKED : MOR_NOT_SHOWN;
        out->ans.why = "the Terms it names are not shown as a version of the offer or vow (G11.8)";
        return;
    }
    out->ans.ans = MOR_SHOWN;
    out->ans.why = "a contest, its signer's word, made against that version; it reaches its signers and no one else (G11.8, G8.1)";
    /* whom it reaches: the signers of what it is made against, one level (G1.2) */
    for (int i = 0; i < t->n_parties; i++) {
        if (t->parties[i].ref.kind == MOR_REF_SIGNER && out->n_reaches < MOR_MAX_LIST)
            out->reaches[out->n_reaches++] = t->parties[i].ref.name;
        if (t->parties[i].ref.kind == MOR_REF_ROLE)
            out->names_role = true;
    }
    /* a buyer's contest names the version the payment read and the payment, in settlement */
    if (!st->has_settlement) {
        out->buyers_why = "it names no payment in settlement";
        return;
    }
    const mor_act *pay = NULL;
    for (int i = 0; i < s->n_acts && !pay; i++) {
        const mor_act *p = s->acts[i];
        if (p->type == MOR_T_PAYMENT && mor_fp_eq(&p->settlement, &st->settlement) && strcmp(p->signer, st->signer) == 0)
            pay = p;
    }
    if (!pay) {
        out->buyers = MOR_UNCHECKED;
        out->buyers_why = "the payment it names is not held (G9.1)";
    } else if (!mor_fp_eq(&pay->agreement, &st->root) || !pay->has_version || !mor_fp_eq(&pay->version, &st->terms)) {
        out->buyers_why = "it does not name the version the payment read (G11.8, G13.2)";
    } else {
        out->buyers = MOR_SHOWN;
        out->buyers_why = "made by the payer against the version the payment read; that it was not delivered is the buyer's word (G8.1)";
    }
}

/* ---- K13 and K10 through their stubs ----------------------------------------------------------- */

mor_tri mor_standing(mor_set *s, const char *signer, mor_k13_out *out)
{
    memset(out, 0, sizeof *out);
    out->signer = signer;
    const mor_plugins *p = s->opt.plugins;
    for (int i = 0; p && i < p->n; i++)
        if (!p->cmips[i].absent && p->cmips[i].task == 13 && p->cmips[i].k13) {
            p->cmips[i].k13(s, signer, out);
            return MOR_YES;
        }
    return MOR_CANNOT;
}

mor_tri mor_judge(mor_set *s, const mor_fp *k10, const mor_act *terms, mor_k10_out *out)
{
    memset(out, 0, sizeof *out);
    out->ans = MOR_CANNOT;
    const mor_cmip *c = mor_cmip_find(s, k10, 10);
    if (!c || !c->k10)
        return MOR_CANNOT;
    mor_k10_in in;
    memset(&in, 0, sizeof in);
    in.terms = terms;
    in.authors = terms->authors; /* G3.2 */
    in.n_authors = terms->n_authors;
    c->k10(&in, out);
    return out->ans;
}
