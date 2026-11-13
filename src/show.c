/* show.c - what the verifier shows, in plain lines. Nothing here decides anything. */
#include "internal.h"

#include <string.h>

static void pa(FILE *o, const mor_answer *a)
{
    char q[96];
    fprintf(o, "%s", mor_ans_name(a->ans));
    if (a->qual.kind != MOR_Q_NONE)
        fprintf(o, " [%s]", mor_qual_str(&a->qual, q, sizeof q));
    if (a->fork)
        fprintf(o, " [fork shown: no branch chosen]");
    if (a->disputed)
        fprintf(o, " [disputed]");
    if (a->why)
        fprintf(o, " - %s", a->why);
}

static const char *chain_name(mor_chain c)
{
    switch (c) {
    case MOR_C_VERSION:
        return "a version";
    case MOR_C_ROOT_WAITING:
        return "not shown in force";
    case MOR_C_NOT_SHOWN:
        return "not shown as a replacement";
    case MOR_C_NOT_VALID:
        return "not valid";
    case MOR_C_UNCHECKED:
        return "unchecked";
    case MOR_C_TIP:
        return "fork: a tip (stopped)";
    case MOR_C_SET_ASIDE:
        return "set aside";
    case MOR_C_RESOLVER:
        return "a resolving update that resolved";
    case MOR_C_RESOLVES_NOTHING:
        return "resolves nothing";
    case MOR_C_UNREACHED:
        return "not shown as a replacement (the Terms it names are no version shown)";
    case MOR_C_STOPPED:
        return "does not count (fork)";
    default:
        return "not read";
    }
}

static void marks(FILE *o, const mor_act *a)
{
    unsigned m = a->ev.line | a->ev.rest;
    if (a->ev.invalid)
        fprintf(o, " INVALID: %s", a->ev.invalid);
    if (m & MOR_M_TIP)
        fprintf(o, " [fork: a tip in its signer's line; stopped (G6.2)]");
    if (m & MOR_M_BUILT)
        fprintf(o, " [built on a tip of a fork that stands: does not count (G12.2)]");
    if (m & MOR_M_SET_ASIDE)
        fprintf(o, " [set aside (G12.3)]");
    if (m & MOR_M_DISPUTED)
        fprintf(o, " [disputed (G12.5)]");
    if (m & MOR_M_RESOLVER)
        fprintf(o, " [resolving update]");
    if (a->ev.cancelled)
        fprintf(o, " [cancelled by a counter signature (G4.10)]");
    if (a->ev.unchecked)
        fprintf(o, " [unchecked: %s]", a->ev.unchecked);
    if (a->unread || a->n_relies_bad)
        fprintf(o, " [%d field(s) not read: not part of the act as read (G2.2)]", a->unread + a->n_relies_bad);
}

static void amount(FILE *o, const mor_amount *a)
{
    fprintf(o, "%llu %s", (unsigned long long)a->n, a->unit);
}

static void show_report(mor_set *s, FILE *o, const mor_act *r, const char *ind)
{
    static const char *const k[] = {"received", "passed on", "the doer's own part (not a fee)", "kept (a fee)", "still owed"};
    fprintf(o, "%sreport %s, stated by %s as %s (its signer's word, G8.4):\n", ind, mor_short(s, &r->fp), r->signer,
            r->side == MOR_SIDE_DOER ? "doer" : "receiving side");
    for (int i = 0; i < r->n_lines; i++) {
        const mor_rline *l = &r->lines[i];
        fprintf(o, "%s  %s ", ind, k[l->kind]);
        amount(o, &l->amount);
        if (l->to.kind != MOR_REF_NONE)
            fprintf(o, " to %s", mor_ref_str(s, &l->to));
        if (l->has_via)
            fprintf(o, " by settlement %s", mor_short(s, &l->via));
        fprintf(o, "\n");
    }
}

static void show_k8(mor_set *s, FILE *o, const mor_k8 *k, const char *ind)
{
    fprintf(o, "%sControl task (K8): %s - %s\n", ind, k->holds == MOR_YES ? "held" : k->holds == MOR_NO ? "not held" : "unchecked",
            k->holds_why ? k->holds_why : "");
    if (k->holds == MOR_NO && !k->has_call)
        return;
    if (k->has_call)
        fprintf(o, "%s  call: reports expected from %s; it names no place and no reader (G11.6)\n", ind,
                k->from == MOR_FROM_DOER ? "the doer" : k->from == MOR_FROM_RECEIVING ? "the receiving side" : "both sides");
    if (k->doer)
        fprintf(o, "%s  doer (the controller): %s\n", ind, k->doer);
    fprintf(o, "%s  reports: %s\n", ind, k->call_status == MOR_SHOWN ? "held, read below" : mor_ans_name(k->call_status));
    for (int i = 0; i < k->n_reports; i++)
        show_report(s, o, k->reports[i], "      ");
    for (int i = 0; i < k->n_ties; i++) {
        const mor_tie *t = &k->ties[i];
        fprintf(o, "%s  settlement %s: %s", ind, mor_short(s, &t->settlement), mor_ans_name(t->status));
        if (t->doer_line) {
            fprintf(o, "; doer %s states ", t->doer_report->signer);
            amount(o, &t->doer_line->amount);
        }
        if (t->recv_line) {
            fprintf(o, "; receiving side %s states ", t->recv_report->signer);
            amount(o, &t->recv_line->amount);
        }
        if (t->why)
            fprintf(o, " - %s", t->why);
        fprintf(o, "\n");
    }
    if (k->holds == MOR_YES) {
        fprintf(o, "%s  whether what was passed on is what the Terms and their cMIPs require: %s (G11.3)\n", ind, mor_ans_name(k->conforms));
        fprintf(o, "%s  a path beyond the leg agreed by the signers (a data cMIP): %s - the text gives no way to name one (G11.6(a))\n", ind,
                mor_ans_name(k->path_beyond));
    }
}

static void show_spot(mor_set *s, FILE *o, const mor_spot *sp, const char *ind)
{
    fprintf(o, "%sfillings: ", ind);
    pa(o, &sp->ans);
    fprintf(o, "\n");
    for (int i = 0; i < sp->n; i++) {
        const mor_filling *f = &sp->fill[i];
        fprintf(o, "%s  filled by %s%s; proof:", ind, f->signer, f->disputed ? " [disputed]" : "");
        for (int k = 0; k < f->n_proof; k++)
            fprintf(o, " act %s signed by %s;", mor_short(s, &f->proof[k]->fp), f->proof[k]->signer);
        if (f->earned_under)
            fprintf(o, " earned under version %s", mor_short(s, &f->earned_under->fp));
        fprintf(o, "\n");
        for (int k = 0; k < f->n_changes; k++)
            fprintf(o, "%s    its rights are changed by version %s: %s - %s\n", ind, mor_short(s, &f->changes[k].version->fp),
                    mor_ans_name(f->changes[k].ans), f->changes[k].rests_on);
    }
    if (sp->ans.ans != MOR_UNCHECKED) {
        fprintf(o, "%s  holds %s; still takes new identities: ", ind, sp->many ? "many fillings" : "one filling");
        if (sp->takes_new.ans == MOR_UNCHECKED)
            fprintf(o, "unchecked");
        else
            fprintf(o, "%s", sp->takes ? "yes" : "no");
        char q[96];
        fprintf(o, " [%s] - %s\n", mor_qual_str(&sp->takes_new.qual, q, sizeof q), sp->takes_new.why ? sp->takes_new.why : "");
    }
}

static void show_terms(mor_set *s, FILE *o, const mor_act *t)
{
    mor_answer a;
    fprintf(o, "  Terms %s, stated by %s:", mor_short(s, &t->fp), t->signer);
    marks(o, t);
    fprintf(o, "\n");
    if (t->ev.invalid)
        return;
    if (t->has_replaces) {
        fprintf(o, "    as a replacement of %s: %s", mor_short(s, &t->replaces), chain_name(t->ev.chain));
        if (t->ev.signed_how && t->ev.chain == MOR_C_VERSION)
            fprintf(o, " - %s", t->ev.signed_how);
        if (t->ev.chain_why)
            fprintf(o, " - %s", t->ev.chain_why);
        fprintf(o, "\n");
        if (t->ev.cond_asked) {
            char q[96];
            fprintf(o, "    update condition %s of version %s: %s [%s] - %s\n", mor_short(s, &t->ev.cond_terms->condition.cmip),
                    mor_short(s, &t->ev.cond_terms->fp),
                    t->ev.cond == MOR_YES ? "valid" : t->ev.cond == MOR_NO ? "not valid" : "unchecked",
                    mor_qual_str(&t->ev.cond_qual, q, sizeof q), t->ev.cond_ground ? t->ev.cond_ground : "");
        }
    }
    mor_terms_in_force(s, t, &a);
    fprintf(o, "    in force: ");
    pa(o, &a);
    fprintf(o, "\n");
}

static void show_agreement(mor_set *s, FILE *o, const mor_agreement *ag)
{
    char q[96];
    fprintf(o, "\n== agreement %s ==\n", mor_short(s, &ag->root));
    if (!ag->root_act) {
        fprintf(o, "  unchecked - %s\n", ag->why ? ag->why : "the first Terms act is not held (G9.1)");
    } else {
        fprintf(o, "  Terms in force: ");
        if (ag->in_force == MOR_SHOWN || ag->in_force == MOR_FORK)
            fprintf(o, "version %s, %s [%s]", mor_short(s, &ag->cur->fp), ag->in_force == MOR_FORK ? "the last shared version" : "shown",
                    mor_qual_str(&ag->qual, q, sizeof q));
        else
            fprintf(o, "%s [%s]", mor_ans_name(ag->in_force), mor_qual_str(&ag->qual, q, sizeof q));
        if (ag->why)
            fprintf(o, " - %s", ag->why);
        fprintf(o, "\n");
        if (ag->in_force == MOR_FORK) {
            fprintf(o, "  FORK of Terms: which Terms replace %s is answered fork; tips, none chosen:", mor_short(s, &ag->cur->fp));
            for (int i = 0; i < ag->n_tips; i++)
                fprintf(o, " %s", mor_short(s, &ag->tips[i]->fp));
            fprintf(o, "\n");
        }
    }
    for (int i = 0; i < s->n_acts; i++)
        if (s->acts[i]->type == MOR_T_TERMS && mor_fp_eq(&s->acts[i]->root, &ag->root))
            show_terms(s, o, s->acts[i]);
    const mor_act *cur = (ag->in_force == MOR_SHOWN || ag->in_force == MOR_FORK) ? ag->cur : NULL;
    if (cur) {
        fprintf(o, "  parties of the version in force:\n");
        for (int i = 0; i < cur->n_parties; i++) {
            const mor_party *p = &cur->parties[i];
            mor_answer a;
            if (p->ref.kind == MOR_REF_SIGNER) {
                mor_bound_by_terms(s, p->ref.name, cur, &a);
                fprintf(o, "    signer %s: bound: ", p->ref.name);
                pa(o, &a);
                fprintf(o, "\n");
            } else if (p->ref.kind == MOR_REF_ROLE) {
                mor_spot sp;
                mor_spot_read(s, &ag->root, p->ref.name, &sp);
                fprintf(o, "    role %s:\n", p->ref.name);
                show_spot(s, o, &sp, "      ");
            } else {
                s->depth = 0;
                mor_view v = mor_view_for(s, cur);
                mor_tri sg = mor_assent_in_name(s, cur, &p->ref, &v);
                fprintf(o, "    agreement %s: named; its signing: %s (naming asks nothing of it, G1.2)\n", mor_short(s, &p->ref.root),
                        sg == MOR_YES ? "shown" : sg == MOR_NO ? "not shown" : "unchecked");
            }
        }
        mor_k1_out *k1 = mor_alloc(s, sizeof *k1);
        if (cur->n_powers > 0 && mor_k1_read(s, cur, k1) != MOR_YES)
            fprintf(o, "  whether these Terms are a delegation, and on what conditions it ends: unchecked - their words could not be read (G5.5, G9.1)\n");
        else if (cur->n_powers > 0 && k1->delegation) {
            fprintf(o, "  a delegation: %s gives %s a power (G5.5). ", mor_ref_str(s, &k1->delegator), mor_ref_str(s, &k1->agent));
            if (k1->end_mode == MOR_END_DEFAULT)
                fprintf(o, "By default the delegator can end it at any time, alone.\n");
            else
                fprintf(o, "Condition on ending it, as the Terms the delegator signed state it: \"%s\"; whether it will be ended: %s.\n",
                        k1->end_words, k1->end_mode == MOR_END_OPAQUE ? "unchecked (the condition could not be read)" : "not shown");
        }
        mor_receiver rc;
        mor_receiver_for(s, &ag->root, NULL, &rc);
        fprintf(o, "  receiver for an act that names no version: %s%s", rc.who ? rc.who : "", rc.who ? ": " : "");
        pa(o, &rc.ans);
        fprintf(o, "\n");
        mor_k8 *k8 = mor_alloc(s, sizeof *k8);
        mor_k8_read(s, cur, k8);
        show_k8(s, o, k8, "  ");
        if (cur->has_envelope) {
            mor_answer w;
            const char *baker = NULL;
            mor_work_read(s, cur, &w, &baker);
            fprintf(o, "  counts as an agreement for the baked work %s (baker, as the envelope states: %s): ", mor_short(s, &cur->envelope),
                    baker ? baker : "?");
            pa(o, &w);
            fprintf(o, "\n    (given with every answer above that rests on these Terms, G14.3)\n");
        }
    }
    bool head = false;
    for (int i = 0; i < s->n_acts; i++) {
        const mor_act *x = s->acts[i];
        if (x->type != MOR_T_STATEMENT || !mor_fp_eq(&x->root, &ag->root))
            continue;
        if (!head)
            fprintf(o, "  Statements:\n");
        head = true;
        const mor_cmip *c = x->kind_is_fp ? mor_cmip_find(s, &x->kind_fp, 11) : NULL;
        fprintf(o, "    %s %s by %s within %s", mor_short(s, &x->fp),
                x->kind_is_fp ? (c ? c->k11_meaning : "(kind unchecked: its K11 cMIP is not held)") : x->kind, x->signer,
                mor_short(s, &x->terms));
        if (x->for_.kind != MOR_REF_NONE)
            fprintf(o, " for %s", mor_ref_str(s, &x->for_));
        fprintf(o, ":");
        marks(o, x);
        fprintf(o, "\n");
        if (x->ev.invalid)
            continue;
        if (x->for_.kind != MOR_REF_NONE) {
            mor_answer a;
            if (x->for_.kind == MOR_REF_AGREEMENT) {
                mor_agreement_signature(s, x, &a);
                fprintf(o, "      as that agreement's signature: ");
            } else {
                mor_bound_by_act(s, x->for_.name, x, &a);
                fprintf(o, "      binds %s: ", x->for_.name);
            }
            pa(o, &a);
            fprintf(o, "\n");
        }
        if (strcmp(x->kind, "contest") == 0) {
            mor_contest ct;
            mor_contest_read(s, x, &ct);
            fprintf(o, "      contest, said by %s: ", ct.said_by);
            pa(o, &ct.ans);
            fprintf(o, "\n        reaches:");
            for (int k = 0; k < ct.n_reaches; k++)
                fprintf(o, " %s", ct.reaches[k]);
            fprintf(o, " (and no one else); buyer's contest: %s - %s\n", mor_ans_name(ct.buyers), ct.buyers_why ? ct.buyers_why : "");
        }
        if (x->has_settlement) {
            const mor_act *st = mor_find(s, &x->settlement);
            fprintf(o, "      tied to settlement %s, reported on the word of %s (G8.5)\n", mor_short(s, &x->settlement),
                    st ? st->signer : "(not held: unchecked)");
        }
    }
}

void mor_show_trace(mor_set *s, const mor_act *p, FILE *o)
{
    mor_trace *tr = mor_alloc(s, sizeof *tr);
    mor_trace_payment(s, p, tr);
    fprintf(o, "result read leg by leg, from the reports held (G11.4); each part is its signer's word (G8.4):\n");
    for (int i = 0; i < tr->n; i++) {
        const mor_leg *l = &tr->legs[i];
        fprintf(o, "  %*sleg: doer %s", l->depth * 2, "", l->doer ? l->doer : "?");
        if (l->to.kind != MOR_REF_NONE)
            fprintf(o, " -> %s", mor_ref_str(s, &l->to));
        if (l->to.kind == MOR_REF_AGREEMENT || !l->has_settlement)
            fprintf(o, " (receiving side: %s)", l->receiving ? l->receiving : "not shown");
        if (l->has_settlement)
            fprintf(o, " by settlement %s", mor_short(s, &l->settlement));
        fprintf(o, ": %s", mor_ans_name(l->status));
        if (l->under)
            fprintf(o, " [under %s %s]", l->under->type == MOR_T_PAYMENT ? "payment" : "Terms", mor_short(s, &l->under->fp));
        if (l->note)
            fprintf(o, " - %s", l->note);
        fprintf(o, "\n");
    }
}

void mor_show_before(mor_set *s, const mor_fp *root, FILE *o)
{
    mor_before *b = mor_alloc(s, sizeof *b);
    mor_before_paying(s, root, b);
    fprintf(o, "before paying, one agreement at a time, from their Terms alone (G11.4(e)):\n");
    for (int i = 0; i < b->n; i++) {
        const mor_before_entry *e = &b->e[i];
        fprintf(o, "  agreement %s: Terms %s\n", mor_short(s, &e->root),
                e->terms == MOR_SHOWN ? "in force, shown" : e->terms == MOR_FORK ? "fork: the last shared version is read" : mor_ans_name(e->terms));
        if (e->k8.terms) {
            char q[96];
            fprintf(o, "    [%s]\n", mor_qual_str(&e->k8.in_force.qual, q, sizeof q));
            show_k8(s, o, &e->k8, "    ");
        }
    }
}

void mor_show_all(mor_set *s, FILE *o)
{
    fprintf(o, "MOR Agreements, draft 17: a verifier's answers. It answers questions; it does not decide disputes.\n");
    fprintf(o, "records: %d taken, %d rejected (not Agreements acts)\n", s->n_acts, s->n_rejects);
    for (int i = 0; i < s->n_rejects; i++)
        fprintf(o, "  rejected at byte %zu: %s\n", s->rejects[i].offset, s->rejects[i].reason);
    if (s->opt.reader)
        fprintf(o, "reader: %s\n", s->opt.reader);
    if (s->unsettled)
        fprintf(o, "UNCHECKED: forks and powers in this set lean on one another so that reading them did not settle; take every "
                   "answer below that rests on a fork as unchecked (G9.1)\n");
    for (int i = 0; i < s->n_agr; i++)
        show_agreement(s, o, s->agr[i]);
    if (s->n_lforks)
        fprintf(o, "\n== forks in signers' own lines (G6.2) ==\n");
    for (int i = 0; i < s->n_lforks; i++) {
        const mor_line_fork *f = &s->lforks[i];
        fprintf(o, "  fork for signer %s in agreement %s [as shown]; tips:", f->signer, mor_short(s, &f->root));
        for (int k = 0; k < f->n_tips; k++)
            fprintf(o, " %s", mor_short(s, &f->tips[k]->fp));
        if (f->resolved)
            fprintf(o, "; resolved by %s: %s continues, the others are set aside%s%s\n", mor_short(s, &f->resolver->fp),
                    mor_short(s, &f->continued->fp), f->resolver_power ? "; " : "", f->resolver_power ? f->resolver_power : "");
        else
            fprintf(o, "; unresolved: both tips stop, none chosen (G12.2)\n");
    }
    bool head = false;
    for (int i = 0; i < s->n_acts; i++) {
        const mor_act *p = s->acts[i];
        if (p->type != MOR_T_PAYMENT)
            continue;
        if (!head)
            fprintf(o, "\n== payments (the buyer's leg, G11.7) ==\n");
        head = true;
        mor_payment *pr = mor_alloc(s, sizeof *pr);
        mor_payment_read(s, p, pr);
        fprintf(o, "  payment %s by %s to agreement %s, ", mor_short(s, &p->fp), p->signer, mor_short(s, &p->agreement));
        amount(o, &p->amount);
        fprintf(o, ", settlement %s (on the word of %s, G8.5)\n", mor_short(s, &p->settlement),
                pr->settlement ? pr->settlement->signer : "no one held");
        fprintf(o, "    receiving side: %s%s", pr->receiver.who ? pr->receiver.who : "", pr->receiver.who ? ": " : "");
        pa(o, &pr->receiver.ans);
        fprintf(o, "\n    %s%s - %s\n", mor_ans_name(pr->status), pr->disputed ? " [disputed]" : "", pr->why ? pr->why : "");
        if (pr->confirmation)
            show_report(s, o, pr->confirmation, "    ");
        if (pr->receiver.has_work) {
            fprintf(o, "    the work those Terms are for: ");
            pa(o, &pr->receiver.work);
            fprintf(o, "\n");
        }
        mor_show_trace(s, p, o);
    }
    head = false;
    for (int i = 0; i < s->n_acts; i++) {
        const mor_act *r = s->acts[i];
        if (r->type != MOR_T_REPORT)
            continue;
        if (!head)
            fprintf(o, "\n== reports held, read as stated by their signers, with or without any cMIP (G11.3) ==\n");
        head = true;
        show_report(s, o, r, "  ");
        fprintf(o, "    answers the call held by %s%s\n", mor_short(s, &r->call),
                mor_find(s, &r->call) ? "" : ", an act that is not held: that part is unchecked (G11.4(d), G9.1)");
    }
}
