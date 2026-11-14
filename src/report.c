/* report.c: Statements as read (G4.3, G4.10, G5), spots (G10), legs and
 * reports (G11), the contest (G11.8), the baker (G14), and the questions. */
#include "internal.h"

static mrec *root_of(const mrec *r)
{
	mrec *x = rec_of(r->root);
	return (x && x->type == R_TERMS && !x->replaces) ? x : NULL;
}

static mview signing_view(mor_ctx *c, const mrec *t)
{
	if (t->vstate == V_VERSION) return view_of(c, t->judged_place);
	const mrec *o = rec_of(t->replaces);
	return view_of(c, (o && o->type == R_TERMS) ? o->place : t->place);
}

static bool in_force_now(const mrec *t, bool *fork)
{
	const mrec *rr = root_of(t);
	if (fork) *fork = rr && rr->cresult == CR_FORK;
	return rr && (rr->cresult == CR_IN_FORCE || rr->cresult == CR_FORK) && rr->clast == t;
}

static const char *amount_str(mor_ctx *c, uint64_t a, sym unit)
{
	return ar_printf(c, "%llu %s", (unsigned long long)a, unit ? unit : "");
}

/* ---------- the baker (G14) ---------- */

static const char *same_work_note(mor_ctx *c, const mrec *env)
{
	const char *note = "";
	for (size_t i = 0; i < c->recs.n; i++) {
		const mrec *o = c->recs.v[i];
		if (o->type != R_ENVELOPE || o == env || o->work != env->work) continue;
		note = ar_printf(c, "%s; the same work is baked by others too, as shown: envelope %.12s, whose baker it states "
		                    "as %s; no bake is chosen (G14.5)", note, o->fp, o->baker);
	}
	return note;
}

static int bound_as_version(mor_ctx *c, mrec *t, sym who)
{
	if (t->vstate == V_UNCHECKED) return T_UNK;
	if (t->vstate != V_VERSION) return T_NO; /* "are, or have been, a version" */
	mview v = view_of(c, t->judged_place);
	return assent_in_name(c, t, who, &v, 0);
}

static mor_answer baker_rec(mor_ctx *c, mrec *t, const char **text, int depth)
{
	mrec *env = rec_of(t->envelope);
	if (!env || env->type != R_ENVELOPE) {
		*text = "the envelope is not held: the baker is not known (G9.1)";
		return MOR_UNCHECKED;
	}
	sym baker = env->baker;
	const char *beside = same_work_note(c, env);
	bool names_baker = is_signer_party(t, baker), names_succ = false, lacking = false, unk = false, has_role = false;
	for (size_t i = 0; i < t->parties.n; i++)
		if (t->parties.v[i].how == PARTY_ROLE) has_role = true;
	if (names_baker) {
		int b = bound_as_version(c, t, baker);
		if (b == T_YES) {
			*text = ar_printf(c, "(a) the baker %s, as the envelope %.12s states it, is one of the parties and bound by "
			                     "these Terms%s", baker, env->fp, beside);
			return MOR_SHOWN;
		}
		if (b == T_UNK) unk = true;
	}
	for (size_t i = 0; i < t->relies_on.n; i++) {
		sym e = t->relies_on.v[i];
		if (!is_fp(e)) continue;
		mrec *x = rec_of(e);
		if (!x) {
			lacking = true; /* it might be the succession */
			continue;
		}
		if (x->type != R_TERMS || x->root == t->root || x->envelope != t->envelope) continue;
		names_succ = true;
		if (depth >= 16) {
			unk = true;
			continue;
		}
		const char *sub;
		mor_answer before = baker_rec(c, x, &sub, depth + 1);
		if (before == MOR_UNCHECKED) unk = true;
		if (before != MOR_SHOWN) continue;
		k1_reading rd;
		k1_read(c, x, &rd);
		if (rd.status != T_YES) {
			unk = true; /* lacks the cMIP by which the earlier agreement says the rights passed */
			continue;
		}
		for (size_t j = 0; j < rd.passes.n; j++) {
			sym p = rd.passes.v[j];
			if (!is_signer_party(t, p)) continue;
			int b = bound_as_version(c, t, p);
			if (b == T_UNK) unk = true;
			if (b != T_YES) continue;
			*text = ar_printf(c, "(b) %s, a party bound by these Terms, holds the baker's rights through a succession: "
			                     "the agreement of Terms %.12s passes them, as its own words say (read by its body cMIP); "
			                     "the baker is %s, as the envelope states it%s", p, x->fp, baker, beside);
			return MOR_SHOWN;
		}
	}
	if (!names_baker && !names_succ) {
		if (lacking) {
			*text = "these Terms name neither the baker nor a succession among what is held, and something their "
			        "`relies_on` names is lacking: it might be the succession (G9.1)";
			return MOR_UNCHECKED;
		}
		if (has_role) {
			*text = ar_printf(c, "these Terms name a party by role, which the baker %s might fill%s", baker, beside);
			return MOR_NOT_SHOWN;
		}
		*text = ar_printf(c, "these Terms name neither the baker %s among their parties nor a succession, and nothing "
		                     "they name is lacking: they cannot count as an agreement for that work%s", baker, beside);
		return MOR_INVALID;
	}
	if (unk) {
		*text = ar_printf(c, "these Terms name the baker or a succession, but an act or a cMIP needed to establish it "
		                     "is lacking: %s%s", c->why, beside);
		return MOR_UNCHECKED;
	}
	*text = ar_printf(c, "these Terms name the baker or a succession, and the acts held do not establish G14.2(a) or (b): "
	                     "an act not held might%s", beside);
	return MOR_NOT_SHOWN;
}

mor_answer baker_answer(mor_ctx *c, mrec *t, const char **text) { return baker_rec(c, t, text, 0); }

/* G14.3: the answer about the baked work is given with every answer that rests on those Terms */
const char *baker_note(mor_ctx *c, mrec *t)
{
	const char *text;
	if (!t || !t->envelope) return "";
	mor_answer a = baker_answer(c, t, &text);
	return ar_printf(c, " [these Terms as an agreement for the baked work of envelope %.12s: %s (G14.3): %s]", t->envelope,
	                 mor_answer_name(a), text);
}

static void bakers_run(mor_ctx *c)
{
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (r->type == R_TERMS && r->envelope && !r->invalid_rule) {
			const char *text;
			mor_answer a = baker_answer(c, r, &text);
			emit(c, MOR_ABOUT_BAKER, r->fp, r->envelope, a, mq(MOR_Q_AS_SHOWN, NULL), "G14.3", "%s", text);
		}
		if (r->type != R_ENVELOPE) continue;
		for (size_t j = 0; j < c->recs.n; j++) {
			const mrec *o = c->recs.v[j];
			if (o->type != R_ENVELOPE || o == r || o->work != r->work) continue;
			emit(c, MOR_ABOUT_SAME_WORK, r->fp, o->fp, MOR_SHOWN, mq(MOR_Q_AS_SHOWN, NULL), "G14.5(a)",
			     "the same work is baked by others too: envelope %.12s states its baker as %s (this one states %s); "
			     "an envelope not held may exist; no bake is chosen, and which baker made the work is not answered: "
			     "the bytes say who uploaded and when, not who made it (G14.5(b))", o->fp, o->baker, r->baker);
		}
	}
}

/* ---------- Statements ---------- */

static const char *signers_of(mor_ctx *c, const mrec *t)
{
	const char *s = "";
	for (size_t i = 0; i < t->parties.n; i++)
		if (t->parties.v[i].how == PARTY_SIGNER) s = ar_printf(c, "%s%s%s", s, s[0] ? ", " : "", t->parties.v[i].id);
	return s[0] ? s : "no one (it names no party as a signer)";
}

static mrec *payment_for_contest(mor_ctx *c, const mrec *s)
{
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *p = c->recs.v[i];
		if (p->type == R_PAYMENT && p->signer == s->signer && p->settlement == s->settlement && p->version == s->terms)
			return p;
	}
	return NULL;
}

static void statement_one(mor_ctx *c, mrec *s)
{
	mrec *t = rec_of(s->terms);
	mqual q0 = mq(MOR_Q_AS_SHOWN, NULL);
	if (s->invalid_rule) return;
	if (!t || t->type != R_TERMS) {
		emit(c, MOR_ABOUT_ACT, s->fp, s->terms, MOR_UNCHECKED, q0, "G9.1",
		     "the Terms this Statement is made within are not held: what it does is unchecked");
		return;
	}
	mqual q = qual_of(c, t);
	if (t->root != s->root) {
		emit(c, MOR_ABOUT_ACT, s->fp, s->terms, MOR_NOT_SHOWN, q, "section 3.2",
		     "its `agreement` is not the agreement of the Terms it names: not shown as a Statement within them");
		return;
	}
	if (s->lstate == L_STANDING) {
		emit(c, MOR_ABOUT_ACT, s->fp, s->terms, MOR_NOT_SHOWN, mq(q.bits | MOR_Q_WITH_FORK, q.anchor), "G12.2",
		     "a tip of a fork in %s's own line, or an act that follows one: it does not count while the fork stands", s->signer);
		return;
	}
	if (s->lstate == L_SET_ASIDE) {
		emit(c, MOR_ABOUT_ACT, s->fp, s->terms, MOR_NOT_SHOWN, q, "G12.3",
		     "a tip set aside in %s's own line, or an act that follows one: it never counted", s->signer);
		return;
	}
	bool counts = act_counts(c, s, s->kind == c->s_assent ? t : NULL);
	bool disputed = counts && rests_on_set_aside(c, s);
	if (!counts) {
		emit(c, MOR_ABOUT_ACT, s->fp, s->terms, MOR_NOT_SHOWN, mq(q.bits | MOR_Q_BUILT_ON_TIP, q.anchor), "G12.2",
		     "built on a tip (it names it in `terms`, `under`, `answers` or `relies_on`): it counts for nothing while the "
		     "fork stands; or it rests on a branch set aside that its own signer signed (G12.5)");
		return;
	}
	if (disputed) q.bits |= MOR_Q_DISPUTED;
	if (s->settlement) {
		const mrec *st = rec_of(s->settlement);
		emit(c, MOR_ABOUT_ACT, s->fp, s->settlement, MOR_NOTE, q0, "G8.5", "it names a settlement, %s; no value is shown as "
		     "moved on the strength of this act (G8.3)",
		     st && st->type == R_SETTLEMENT ? ar_printf(c, "reported on the word of %s, who gave it", st->signer)
		                                    : "which is not held");
	}
	/* an act made for someone under a power (G5.1(b)) */
	int pw = T_YES;
	const char *made_for = "";
	if (s->for_) {
		mrec *xt = NULL;
		bool by_all = false;
		c->why[0] = '\0';
		if (is_fp(s->for_) && agreement_terms(c, s->for_, &xt) == T_YES) {
			by_all = true;
			for (size_t i = 0; i < xt->powers.n; i++)
				if (xt->powers.v[i].kind == s->kind) by_all = false;
		}
		c->why[0] = '\0';
		if (by_all && s->kind == c->s_assent) {
			/* its Terms give no power for it: it signs by one act of each of its signers (G1.2) */
			mview sv = signing_view(c, t);
			pw = assent_by_agreement(c, t, s->for_, &sv, 0);
		} else if (by_all) {
			pw = T_UNK;
			why(c, "that agreement's Terms give no power for acts of this kind, so it signs by one act of each of its "
			       "signers; which acts of its signers are one such signing is established here for assents only (G9)");
		} else {
			pw = power_ok(c, s, s->kind, 0);
		}
		made_for = ar_printf(c, " made for %s under Terms %.12s by %s", s->for_, s->under, s->signer);
		emit(c, MOR_ABOUT_POWER, s->fp, s->for_, pw == T_YES ? MOR_SHOWN : (pw == T_NO ? MOR_NOT_SHOWN : MOR_UNCHECKED), q,
		     "G5.1(b)", "%s%s%s", pw == T_YES
		         ? (is_fp(s->for_) ? "this act is, or is part of, that agreement's signature: it binds every party bound by its Terms, as they say, and no one else (G1.2)"
		                           : "the Terms named in `under` bind that person, are in force, give a power for acts of this kind, and its rule says yes")
		         : "not counted as made for the one named: ", pw == T_YES ? "; it rests on its signer " : c->why,
		     pw == T_YES ? s->signer : "");
	}
	if (s->kind == c->s_assent) {
		bool fork;
		if (s->content.n) emit(c, MOR_ABOUT_UNREAD, s->fp, "content", MOR_NOTE, q0, "section 3.2", "an assent's `content` is empty: what it carries is left unread");
		if (t->vstate == V_TIP) {
			emit(c, MOR_ABOUT_ASSENT, s->fp, t->fp, MOR_NOT_SHOWN, mq(q.bits | MOR_Q_WITH_FORK, q.anchor), "G12.2",
			     "an assent to a tip of a fork that stands: it counts for whether that tip was signed, and for nothing "
			     "else; it binds no one, since the Terms it names are not in force (G4.3)");
			return;
		}
		if (t->vstate == V_SET_ASIDE || t->cstate == L_SET_ASIDE) {
			emit(c, MOR_ABOUT_ASSENT, s->fp, t->fp, MOR_SHOWN, mq(q.bits | MOR_Q_DISPUTED, q.anchor), "G12.5",
			     "an assent within a branch whose tip is set aside: it stands as an act and binds no one, since the Terms "
			     "it names never counted as a version");
			return;
		}
		if (pw != T_YES) return;
		mview v = signing_view(c, t);
		sym name = s->for_ ? s->for_ : s->signer;
		c->why[0] = '\0';
		int counted = is_fp(name) ? assent_by_agreement(c, t, name, &v, 0) : assent_in_name(c, t, name, &v, 0);
		if (in_force_now(t, &fork) && counted == T_YES)
			emit(c, MOR_ABOUT_ASSENT, s->fp, t->fp, MOR_SHOWN, mq(q.bits | (fork ? MOR_Q_WITH_FORK : 0), q.anchor), "G4.3",
			     "an assent%s counted in the name of %s; it binds while the Terms it names are in force, and they are",
			     made_for, name);
		else if (t->next)
			emit(c, MOR_ABOUT_ASSENT, s->fp, t->fp, MOR_NOT_SHOWN, q, "G4.3",
			     "the Terms it names have been replaced by %.12s: an assent binds only while the Terms it names are in "
			     "force; what it did when it was made is unchanged (G13.1), and whether %s is bound by the replacement is G5's",
			     t->next->fp, name);
		else
			emit(c, MOR_ABOUT_ASSENT, s->fp, t->fp, counted == T_UNK ? MOR_UNCHECKED : MOR_NOT_SHOWN, q, "G4.3",
			     "%s: no assent binds anyone until the Terms it names are in force (G4.10)%s%s",
			     counted == T_YES ? "counted, but the Terms it names are not in force"
			                      : (counted == T_NO ? "not counted for those Terms" : "whether it is counted is unchecked"),
			     c->why[0] ? ": " : "", c->why);
		return;
	}
	if (s->kind == c->s_counter) {
		/* the assent it is read against: the nearest one before it in its signer's line, unless
		 * another counter signature already stands between them */
		mrec *a = NULL;
		size_t best = 0, between = 0;
		for (size_t i = 0; i < t->stmts.n; i++) {
			mrec *x = t->stmts.v[i];
			size_t d;
			if (x->signer != s->signer || x->for_ || x == s || (d = line_distance(c, s, x)) == 0) continue;
			if (x->kind == c->s_assent && (!a || d < best)) {
				a = x;
				best = d;
			}
			if (x->kind == c->s_counter && (!between || d < between)) between = d;
		}
		if (a && between && between < best) a = NULL;
		if (!a) {
			emit(c, MOR_ABOUT_COUNTER, s->fp, t->fp, MOR_NOT_SHOWN, q, "G4.10",
			     "no assent by %s to those Terms is held that this counter signature follows in its signer's own line: "
			     "it cancels nothing", s->signer);
			return;
		}
		mview v = signing_view(c, t);
		c->why[0] = '\0';
		int before = in_force_signed(c, t, &v, 0);
		bool order_unknown = strstr(c->why, "no order is shown") != NULL;
		int counted = assent_in_name(c, t, s->signer, &v, 0);
		if (order_unknown)
			emit(c, MOR_ABOUT_COUNTER, s->fp, a->fp, MOR_NOT_SHOWN, q, "G4.10",
			     "whether the Terms were in force before this counter signature is not shown: with no place only citing "
			     "gives an order, and neither it nor the assents that complete the Terms name the other; so whether "
			     "those Terms are in force is not shown");
		else if (v.pstat == 2)
			emit(c, MOR_ABOUT_COUNTER, s->fp, a->fp, MOR_UNCHECKED, q, "G9.1", "no single answer is held from the place named");
		else if (before == T_UNK || counted == T_UNK)
			emit(c, MOR_ABOUT_COUNTER, s->fp, a->fp, MOR_UNCHECKED, q, "G9.1",
			     "whether those Terms were in force before this counter signature is unchecked: %s", c->why);
		else if (counted == T_YES && before == T_YES)
			emit(c, MOR_ABOUT_COUNTER, s->fp, a->fp, MOR_NOT_SHOWN, q, "G4.10",
			     "made for Terms that were in force: it changes nothing; Terms in force change only by replacement (G4.5, G4.9)");
		else
			emit(c, MOR_ABOUT_COUNTER, s->fp, a->fp, MOR_SHOWN, q, "G4.10",
			     "made before those Terms were in force: it cancels the assent %.12s of %s, and no one else's; that assent "
			     "is not counted by G4.1", a->fp, s->signer);
		return;
	}
	if (s->kind == c->s_contest) {
		bool role = false;
		for (size_t i = 0; i < t->parties.n; i++)
			if (t->parties.v[i].how == PARTY_ROLE) role = true;
		if (t->vstate != V_VERSION && t->vstate != V_SET_ASIDE) {
			emit(c, MOR_ABOUT_CONTEST, s->fp, t->fp, t->vstate == V_UNCHECKED ? MOR_UNCHECKED : MOR_NOT_SHOWN, q, "G11.8",
			     "its `terms` names Terms that are not shown as a version, nor a tip set aside");
			return;
		}
		if (t->vstate == V_SET_ASIDE) q.bits |= MOR_Q_DISPUTED;
		emit(c, MOR_ABOUT_CONTEST, s->fp, t->fp, pw == T_YES ? MOR_SHOWN : (pw == T_NO ? MOR_NOT_SHOWN : MOR_UNCHECKED), q, "G11.8",
		     "a contest, the word of %s%s (G8.1), made against the version %.12s; it reaches the signers of that "
		     "version and no one else: %s; it gives its signer sight of no leg; that version %s a party by role "
		     "(shown beside it; it changes no answer)%s",
		     s->for_ && pw == T_YES ? s->for_ : s->signer, made_for, t->fp, signers_of(c, t),
		     role ? "names" : "does not name", baker_note(c, t));
		if (s->settlement) {
			mrec *p = payment_for_contest(c, s);
			emit(c, MOR_ABOUT_CONTEST, s->fp, "buyer", p ? MOR_SHOWN : MOR_UNCHECKED, q, "G11.8",
			     p ? "a buyer's contest: a payment %.12s by the same signer names the same settlement and the same version; "
			         "that what was bought was not delivered is the buyer's word (G8.1)%s"
			       : "whether this contest is a buyer's is unchecked: no payment by the same signer that names the same "
			         "settlement and the same version is held (G9)%s%s",
			     p ? p->fp : "", "");
		}
		return;
	}
	/* a kind named by fingerprint (K7) */
	c->why[0] = '\0';
	int k = k7_kind(c, s->kind);
	emit(c, MOR_ABOUT_ACT, s->fp, s->kind, k == T_YES ? MOR_SHOWN : MOR_UNCHECKED, q, "K7",
	     k == T_YES ? "a Statement of a kind whose cMIP is held; what it states is that cMIP's to say, and it is its "
	                  "signer %s's word (G8.1)%s"
	                : "a Statement of a kind whose cMIP is not held: what it states is unchecked (G9.1); signed by %s%s",
	     s->signer, made_for);
}

/* ---------- spots (G10) ---------- */

static void rights_for(mor_ctx *c, mrec *vin, const mfilling *f)
{
	mrec *e = rec_of(f->earned);
	if (!e || e->type != R_TERMS || e->vstate != V_VERSION) return;
	for (mrec *l = e->next; l; l = l->next) {
		mview v = view_of(c, l->judged_place);
		mqual q = qual_of_view(&v);
		if (assent_in_name(c, l, f->signer, &v, 0) == T_YES) {
			emit(c, MOR_ABOUT_RIGHTS, f->proof->fp, l->fp, MOR_SHOWN, q, "G10.6(a)",
			     "the later version %.12s changes the rights of %s's filling (earned under %.12s) as far as its own words "
			     "say: it carries an assent signed by the holder in their own name; the filling stays a filling",
			     l->fp, f->signer, e->fp);
			continue;
		}
		if (!e->condition.present) {
			emit(c, MOR_ABOUT_RIGHTS, f->proof->fp, l->fp, MOR_NOT_SHOWN, q, "G10.6",
			     "the later version %.12s is not shown as changing the rights of %s's filling: the version it was earned "
			     "under (%.12s) names no update condition, and no assent of the holder to the later version is held",
			     l->fp, f->signer, e->fp);
			continue;
		}
		char ground[400];
		mqual gq;
		c->why[0] = '\0';
		int t = k4_condition(c, e, l, &v, ground, sizeof ground, &gq);
		emit(c, MOR_ABOUT_RIGHTS, f->proof->fp, l->fp, t == T_YES ? MOR_VALID : (t == T_NO ? MOR_NOT_VALID : MOR_UNCHECKED), gq,
		     "G10.6(b)", "the update condition %s of the version the filling was earned under (%.12s), given that version "
		     "and the later version %.12s: %s%s; %s", e->condition.cmip, e->fp, l->fp, ground, c->why,
		     t == T_YES ? "the later version reaches the filling's rights as far as its own words say"
		                : "the later version is not read as changing the filling's rights");
	}
	(void)vin;
}

static void spots_for(mor_ctx *c, mrec *vin, unsigned extra)
{
	mqual q = qual_of(c, vin);
	q.bits |= extra;
	for (size_t i = 0; i < vin->parties.n; i++) {
		const mparty *p = &vin->parties.v[i];
		if (p->how != PARTY_ROLE) continue;
		sym obj = intern0(c, ar_printf(c, "role:%s", p->id));
		k5_answer ka;
		c->why[0] = '\0';
		k5_spot(c, vin, p, &ka);
		if (ka.status != T_YES) {
			emit(c, MOR_ABOUT_FILLING, vin->fp, obj, MOR_UNCHECKED, q, "G10.2", "who fills this spot is unchecked: %s%s", c->why,
			     baker_note(c, vin));
			emit(c, MOR_ABOUT_SPOT_OPEN, vin->fp, obj, MOR_UNCHECKED, q, "G10.5",
			     "whether this spot still takes new identities is unchecked: %s", c->why);
			continue;
		}
		if (ka.fills.n == 0)
			emit(c, MOR_ABOUT_FILLING, vin->fp, obj, MOR_NOT_SHOWN, q, "G10.2",
			     "no filling of this spot is shown by the acts held; the confirmer its cMIP names is %s%s", ka.witness,
			     baker_note(c, vin));
		for (size_t j = 0; j < ka.fills.n; j++) {
			const mfilling *f = &ka.fills.v[j];
			const mrec *e = rec_of(f->earned);
			emit(c, MOR_ABOUT_FILLING, vin->fp, obj, MOR_SHOWN, mq(q.bits | (f->disputed ? MOR_Q_DISPUTED : 0), q.anchor),
			     "G10.2", "filled by %s (a %s spot; the cMIP says it holds %s) on the proof %.12s, signed and so confirmed "
			     "by %s (G8.4); earned under the version %.12s%s, whose rights and conditions it keeps (G10.6); a party in "
			     "the spot from then on, bound only by their own assent (G10.3)%s",
			     f->signer, p->kind, ka.many ? "many fillings" : "one filling", f->proof->fp, f->proof->signer, f->earned,
			     (e && e->type == R_TERMS && e->vstate == V_VERSION) ? "" : " (not shown as a version of this agreement)",
			     baker_note(c, vin));
			rights_for(c, vin, f);
		}
		emit(c, MOR_ABOUT_SPOT_OPEN, vin->fp, obj, ka.open == T_YES ? MOR_YES : MOR_NO, q, "G10.5",
		     "whether the spot still takes new identities, as its cMIP says of the version in force; cause: %s; a filling "
		     "already earned stays a filling and keeps its rights", ka.cause);
	}
}

/* ---------- legs and reports (G11) ---------- */

static const mcall *control_call(mor_ctx *c, const mrec *t)
{
	for (size_t i = 0; i < t->calls.n; i++)
		if (t->calls.v[i].task == c->s_control) return &t->calls.v[i];
	return NULL;
}

/* do these Terms hold the Control task (G11.2)? */
static int holds_control(mor_ctx *c, mrec *t, const char **text)
{
	if (t->control.present) {
		mrec *d = rec_of(t->control.cmip);
		if (!d || d->type != R_CMIP) {
			*text = "`control` names a cMIP that is not held (G9.1)";
			return T_UNK;
		}
		*text = "`control` names a cMIP";
		return T_YES;
	}
	sym rv = t->receiver;
	bool own = rv && ((rv[0] == '@' && is_signer_party(t, rv)) || (strncmp(rv, "role:", 5) == 0 && find_role(t, rv + 5)));
	if (!own) {
		*text = "`control` is empty and these Terms name none of their own parties as receiver";
		return T_NO;
	}
	k1_reading rd;
	k1_read(c, t, &rd);
	if (rd.status != T_YES) {
		*text = "`control` is empty and the body cannot be read: whether a part is given to another party is not known (G9.1)";
		return T_UNK;
	}
	for (size_t i = 0; i < rd.shares.n; i++)
		if (rd.shares.v[i].who != rv) {
			*text = "`control` is empty; these Terms name one of their own parties as receiver and give a part to another "
			        "party: that party holds for the others, and passing on is its obligation";
			return T_YES;
		}
	*text = "`control` is empty and the body gives no part to a party other than the receiver";
	return T_NO;
}

/* the doer for K6: the party those Terms name as receiver among their own parties */
static sym doer_of(mor_ctx *c, mrec *t)
{
	sym rv = t->receiver;
	if (!rv) return NULL;
	if (rv[0] == '@') return is_signer_party(t, rv) ? rv : NULL;
	if (strncmp(rv, "role:", 5) == 0) {
		const mparty *role = find_role(t, rv + 5);
		k5_answer ka;
		if (!role) return NULL;
		k5_spot(c, t, role, &ka);
		return (ka.status == T_YES && ka.fills.n == 1) ? ka.fills.v[0].signer : NULL;
	}
	return NULL;
}

static bool same_settlement(sym a, sym b)
{
	if (a == b) return true;
	const mrec *x = rec_of(a), *y = rec_of(b);
	/* Money's answer to "the same payment" (stand-in S8) */
	return x && y && x->type == R_SETTLEMENT && y->type == R_SETTLEMENT && x->payment_id == y->payment_id;
}

static bool is_doer_claim(const mrec *r)
{
	for (size_t i = 0; i < r->lines.n; i++)
		if (r->lines.v[i].type != LN_RECEIVED) return true;
	return false;
}

/* the receiving side of a line passed on to `to` (G11.2): a signer, or an agreement's receiver for the act */
static mor_answer receiving_side(mor_ctx *c, const mrec *report, sym to, sym *who, const char **text)
{
	*text = "";
	if (to[0] == '@') {
		*who = to;
		return MOR_SHOWN;
	}
	sym version = NULL;
	for (size_t i = 0; i < report->relies_on.n; i++) {
		const mrec *x = rec_of(report->relies_on.v[i]);
		if (x && x->type == R_TERMS && x->root == to) version = x->fp;
	}
	mreceiver rc;
	receiver_for(c, to, version, &rc);
	*who = rc.party;
	*text = rc.text;
	return rc.ans;
}

static const char *lines_text(mor_ctx *c, const mrec *r)
{
	const char *s = "";
	for (size_t i = 0; i < r->lines.n; i++) {
		const mline *l = &r->lines.v[i];
		const char *a = amount_str(c, l->amount, l->unit);
		switch (l->type) {
		case LN_RECEIVED: s = ar_printf(c, "%s received %s by the settlement %.12s;", s, a, l->settlement); break;
		case LN_PASSED: s = ar_printf(c, "%s passed on %s to %.13s by the settlement %.12s;", s, a, l->to, l->settlement); break;
		case LN_OWN: s = ar_printf(c, "%s its own part %s (not a fee, and not read as kept);", s, a); break;
		case LN_KEPT: s = ar_printf(c, "%s kept %s (a fee);", s, a); break;
		default: s = ar_printf(c, "%s still owed %s to %.13s;", s, a, l->to); break;
		}
	}
	if (!s[0]) return " (no lines)";
	char *t = ar_printf(c, "%s", s);
	t[strlen(t) - 1] = '\0'; /* the last ';' */
	return t;
}

typedef struct {
	mor_answer ans;
	const char *text;
} mtie;

/* the tie for one line a doer states as passed on (G11.4(b), (c)) */
static mtie tie_line(mor_ctx *c, mrec *t, const mrec *dr, const mline *l)
{
	mtie out;
	sym who;
	const char *rtext;
	mor_answer ra = receiving_side(c, dr, l->to, &who, &rtext);
	const char *stated = ar_printf(c, "the doer %s states %s passed on to %.13s", dr->signer, amount_str(c, l->amount, l->unit), l->to);
	const mrec *st = rec_of(l->settlement);
	const mline *conf = NULL;
	const mrec *cr = NULL;
	for (size_t i = 0; i < c->recs.n && !conf; i++) {
		const mrec *r = c->recs.v[i];
		if (r->type != R_REPORT || r->call != t->fp || is_doer_claim(r)) continue;
		if (ra == MOR_SHOWN && r->signer != who) continue;
		for (size_t j = 0; j < r->lines.n; j++)
			if (same_settlement(r->lines.v[j].settlement, l->settlement)) {
				conf = &r->lines.v[j];
				cr = r;
				break;
			}
	}
	if (ra == MOR_UNCHECKED || (ra != MOR_SHOWN && conf)) {
		out.ans = ra == MOR_UNCHECKED ? MOR_UNCHECKED : MOR_ONE_SIDE_ONLY;
		out.text = ar_printf(c, "%s; who the receiving side is, is %s: %s%s", stated, mor_answer_name(ra), rtext,
		                     conf ? "; a report that states a receipt is listed as its signer's word and ties nothing" : "");
		return out;
	}
	if (!conf) {
		out.ans = MOR_ONE_SIDE_ONLY;
		out.text = ar_printf(c, "%s; no confirmation by the receiving side %s is held: the doer's word alone (G8.4)", stated,
		                     who ? who : "(not shown)");
		return out;
	}
	if (!st || st->type != R_SETTLEMENT) {
		out.ans = MOR_UNCHECKED;
		out.text = ar_printf(c, "the settlement both reports name is not held: the tie is unchecked (G9); what each side "
		                        "states is still shown: %s; the receiving side %s states %s received", stated, cr->signer,
		                     amount_str(c, conf->amount, conf->unit));
		return out;
	}
	bool same = conf->amount == l->amount && conf->unit == l->unit;
	out.ans = same ? MOR_MATCHED : MOR_MISMATCHED;
	out.text = ar_printf(c, "%s; the receiving side %s states %s received%s", stated, cr->signer,
	                     amount_str(c, conf->amount, conf->unit),
	                     same ? "" : "; both are shown and neither is chosen; what Money counts toward a debt is Money's answer, not this text's");
	return out;
}

static bool reader_is_side(mor_ctx *c, mrec *t, sym doer)
{
	if (!c->reader) return false; /* a verifier told nothing reads as no side of any leg */
	return c->reader == doer || is_signer_party(t, c->reader);
}

static void leg_reports(mor_ctx *c, mrec *t, bool is_last)
{
	const char *htext;
	c->why[0] = '\0';
	int holds = holds_control(c, t, &htext);
	const mcall *call = holds == T_YES ? control_call(c, t) : NULL;
	mqual q = qual_of(c, t);
	sym doer = doer_of(c, t);
	const char *bn = baker_note(c, t);
	bool any_report = false, doer_report = false, recv_report = false;
	if (t->vstate == V_SET_ASIDE) q.bits |= MOR_Q_DISPUTED;
	if (is_last && root_of(t) && root_of(t)->cresult == CR_FORK) q.bits |= MOR_Q_WITH_FORK; /* G4.7 */
	if (is_last) {
		emit(c, MOR_ABOUT_CONTROL, t->fp, NULL, holds == T_YES ? MOR_SHOWN : (holds == T_NO ? MOR_NOT_SHOWN : MOR_UNCHECKED), q,
		     "G11.2", "whether these Terms hold the Control task (K6): %s", htext);
		for (size_t i = 0; i < t->calls.n; i++) {
			if (t->calls.v[i].task != c->s_control)
				emit(c, MOR_ABOUT_UNREAD, t->fp, t->calls.v[i].task, MOR_NOTE, q, "section 3.1",
				     "a call attached to another task than Control (K6): these grammar rules read only a call attached to K6");
			else if (holds == T_NO)
				emit(c, MOR_ABOUT_UNREAD, t->fp, t->calls.v[i].task, MOR_NOTE, q, "G11.2",
				     "a call for the Control task in Terms that do not hold it: not read");
		}
		if (holds == T_YES && call)
			emit(c, MOR_ABOUT_CALL, t->fp, call->from, MOR_SHOWN, q, "G11.2",
			     "these Terms carry a call for reports on the Control task, from: %s; the doer is %s; the call is made by "
			     "the signers of these Terms and binds no one else; it names no place and no reader (G11.6)%s",
			     call->from, doer ? doer : "not shown (no party of these Terms is named as receiver)", bn);
		else if (holds == T_YES)
			emit(c, MOR_ABOUT_CALL, t->fp, NULL, MOR_NO_REPORT_CALLED_FOR, q, "G11.4(d)",
			     "these Terms hold the Control task and carry no call: read as nothing else, not as kept, not as failed; "
			     "nothing ties what lies beyond this leg to what reached it");
		else if (holds == T_UNK && t->calls.n)
			emit(c, MOR_ABOUT_CALL, t->fp, NULL, MOR_UNCHECKED, q, "G9.1", "whether these Terms hold the task their call is for is unchecked");
	}
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (r->type != R_REPORT || r->call != t->fp) continue;
		any_report = true;
		if (t->vstate != V_VERSION && t->vstate != V_SET_ASIDE) {
			emit(c, MOR_ABOUT_REPORT, r->fp, t->fp, t->vstate == V_UNCHECKED ? MOR_UNCHECKED : MOR_NOT_SHOWN,
			     mq(q.bits | (t->vstate == V_TIP ? MOR_Q_WITH_FORK | MOR_Q_BUILT_ON_TIP : 0), q.anchor), "G11.3",
			     "the Terms whose call this report answers are not shown as a version; what its signer %s states:%s",
			     r->signer, lines_text(c, r));
			continue;
		}
		bool dclaim = is_doer_claim(r);
		const char *side;
		if (!call) side = "these Terms carry no call for the Control task that it could answer: listed as its signer's word (G8), it ties nothing";
		else if (dclaim && doer && r->signer == doer) {
			side = "the doer's leg report";
			doer_report = true;
		} else if (dclaim) side = "its signer is not the doer the Terms name: listed as its signer's word (G8), it ties nothing";
		else {
			side = "a report that confirms a receipt; it ties only where its signer is the receiving side of a leg (G11.4(b))";
			recv_report = true;
		}
		const char *conf = "";
		if (call && dclaim) {
			if (!t->control.present) conf = "; with `control` empty the leg report is read with no cMIP";
			else {
				int k = k6_control(c, t);
				conf = k == T_UNK ? "; whether what was passed on is what these Terms and their cMIPs require is unchecked (G9), which does not make the leg report itself unchecked"
				                  : (k == T_YES ? "; the Control cMIP answers that it was passed on as the Terms say"
				                                : "; the Control cMIP answers that it was not passed on as the Terms say");
			}
		}
		emit(c, MOR_ABOUT_REPORT, r->fp, t->fp, MOR_SHOWN, q, "G11.3", "%s, stated by its signer %s (G8.4), of its own leg only:%s%s%s",
		     side, r->signer, lines_text(c, r), conf, bn);
		if (!call || !dclaim || !doer || r->signer != doer || call->from != c->s_both) continue;
		for (size_t j = 0; j < r->lines.n; j++) {
			const mline *l = &r->lines.v[j];
			if (l->type != LN_PASSED) continue;
			mtie tie = tie_line(c, t, r, l);
			emit(c, MOR_ABOUT_TIE, l->settlement, t->fp, tie.ans, q, "G11.4", "%s", tie.text);
		}
	}
	if (!call || !(t->vstate == V_VERSION || t->vstate == V_SET_ASIDE) || !(is_last || any_report)) return;
	/* a receipt stated by a party of these Terms that no doer's report held speaks of: one side only */
	for (size_t i = 0; i < c->recs.n && call->from == c->s_both; i++) {
		const mrec *r = c->recs.v[i];
		if (r->type != R_REPORT || r->call != t->fp || is_doer_claim(r) || r->signer == doer || !is_signer_party(t, r->signer)) continue;
		for (size_t j = 0; j < r->lines.n; j++) {
			bool named = false;
			for (size_t k = 0; k < c->recs.n && !named; k++) {
				const mrec *d = c->recs.v[k];
				if (d->type != R_REPORT || d->call != t->fp || !doer || d->signer != doer) continue;
				for (size_t m = 0; m < d->lines.n; m++)
					if (d->lines.v[m].type == LN_PASSED && same_settlement(d->lines.v[m].settlement, r->lines.v[j].settlement)) named = true;
			}
			if (!named)
				emit(c, MOR_ABOUT_TIE, r->lines.v[j].settlement, t->fp, MOR_ONE_SIDE_ONLY, q, "G11.4",
				     "%s, a party of these Terms, states %s received; no doer's report held names that settlement: its "
				     "signer's word alone (G8.4)", r->signer, amount_str(c, r->lines.v[j].amount, r->lines.v[j].unit));
		}
	}
	/* silence (G11.4(d)) */
	bool want_doer = call->from != c->s_receiving, want_recv = call->from != c->s_doer;
	const char *ground = "";
	bool sided = reader_is_side(c, t, doer) || (c->reader && k8_data(c, t, c->reader, NULL, &ground) == T_YES);
	if ((want_doer && !doer_report) || (want_recv && !recv_report && !doer_report)) {
		if (sided)
			emit(c, MOR_ABOUT_CALL, t->fp, "silence", MOR_UNANSWERED, q, "G11.4(d)",
			     "a call with no report from %s: unanswered, read as nothing else; from this leg nothing ties what lies "
			     "beyond it to what reached it", want_doer && !doer_report ? "the doer" : "the receiving side");
		else
			emit(c, MOR_ABOUT_CALL, t->fp, "silence", MOR_NOT_SHOWN, q, "G11.4(d)",
			     "no report answering this call is held; this verifier reads as no side of that leg and along no path "
			     "its people signed for, and a leg's reports travel no further than the leg (G11.6): for it that leg's "
			     "reports are not shown, and the call is not shown as unanswered");
	}
}

/* ---------- payments (G11.7) ---------- */

static void payment_one(mor_ctx *c, mrec *p)
{
	mreceiver rc;
	receiver_for(c, p->to, p->version, &rc);
	if (rc.q.bits == 0) rc.q.bits = MOR_Q_AS_SHOWN;
	const char *bn = baker_note(c, rc.version);
	emit(c, MOR_ABOUT_RECEIVER, p->fp, p->to, rc.ans, rc.q, "G1.4",
	     "the receiver for this payment, which names %s%.12s: %s%s", p->version ? "the version " : "no version; read from the Terms in force of ",
	     p->version ? p->version : p->to, rc.text, bn);
	const mrec *st = rec_of(p->settlement);
	const mline *conf = NULL;
	const mrec *cr = NULL;
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (r->type != R_REPORT) continue;
		if (r->call != p->fp && !same_settlement(r->call, p->settlement)) continue;
		bool side = rc.ans == MOR_SHOWN && r->signer == rc.party;
		emit(c, MOR_ABOUT_REPORT, r->fp, p->fp, MOR_SHOWN, mq(MOR_Q_AS_SHOWN, NULL), "G11.7",
		     "%s, stated by its signer %s (G8.4):%s", side ? "the receiving side's confirmation of this payment, answering the call "
		     "the payment carries by default" : "a report that names this payment by a signer who is not shown as its receiving side: "
		     "listed as its signer's word, it ties nothing", r->signer, lines_text(c, r));
		if (!side || conf) continue;
		for (size_t j = 0; j < r->lines.n; j++)
			if (r->lines.v[j].type == LN_RECEIVED && same_settlement(r->lines.v[j].settlement, p->settlement)) {
				conf = &r->lines.v[j];
				cr = r;
				break;
			}
	}
	const char *payer = ar_printf(c, "the payer %s states a payment of %s by the settlement %.12s (the payer's word, G8.1; the "
	                                 "settlement is %s)", p->signer, amount_str(c, p->amount, p->unit),
	                              p->settlement ? p->settlement : "",  /* never NULL: a required field */
	                              st && st->type == R_SETTLEMENT ? ar_printf(c, "the word of %s, who gave it", st->signer) : "not held");
	if (!conf) {
		emit(c, MOR_ABOUT_PAYMENT, p->fp, p->settlement, MOR_ONE_SIDE_ONLY, rc.q, "G11.7",
		     "%s; the receiving side%s%s has not confirmed it: it is not shown as received on the buyer's word alone "
		     "(G8.3); the call binds only its signer, and no one is made to confirm%s", payer, rc.party ? " " : "",
		     rc.party ? rc.party : (rc.ans == MOR_UNCHECKED ? " (unchecked)" : " (not shown)"), bn);
		return;
	}
	if (!st || st->type != R_SETTLEMENT) {
		emit(c, MOR_ABOUT_PAYMENT, p->fp, p->settlement, MOR_UNCHECKED, rc.q, "G11.4(b)",
		     "the settlement both name is not held: the tie is unchecked (G9), and what each side states is still shown: "
		     "%s; the receiving side %s states %s received%s", payer, cr->signer, amount_str(c, conf->amount, conf->unit), bn);
		return;
	}
	bool same = conf->amount == p->amount && conf->unit == p->unit;
	emit(c, MOR_ABOUT_PAYMENT, p->fp, p->settlement, same ? MOR_MATCHED : MOR_MISMATCHED, rc.q, "G11.7",
	     "%s; the receiving side %s confirms %s received (its signer's word, G8.4)%s; after the confirmation comes access to "
	     "what was bought, which these grammar rules do not read; the buyer is shown nothing further by default%s", payer,
	     cr->signer, amount_str(c, conf->amount, conf->unit), same ? "" : ": another amount; both are shown, neither is chosen", bn);
}

/* ---------- the run ---------- */

void report_run(mor_ctx *c)
{
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (r->type == R_STATEMENT) statement_one(c, r);
	}
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (r->type != R_TERMS || r->invalid_rule) continue;
		bool is_last = false;
		if (!r->replaces && (r->cresult == CR_IN_FORCE || r->cresult == CR_FORK) && r->clast) {
			/* answers that rest on the Terms in force are given from the last version; while a
			 * fork stands, from the Terms both branches replace, with the fork (G4.7) */
			mrec *vin = r->clast;
			unsigned extra = r->cresult == CR_FORK ? MOR_Q_WITH_FORK : 0;
			spots_for(c, vin, extra);
			if (vin->receiver) {
				mreceiver rc;
				receiver_for(c, r->fp, NULL, &rc);
				emit(c, MOR_ABOUT_RECEIVER, r->fp, vin->fp, rc.ans, rc.q, "G1.4",
				     "the receiver of this agreement for an act that names no version, read from its Terms in force: %s%s",
				     rc.text, baker_note(c, vin));
			}
		}
		const mrec *rr = root_of(r);
		is_last = rr && rr->clast == r && (rr->cresult == CR_IN_FORCE || rr->cresult == CR_FORK);
		leg_reports(c, r, is_last);
	}
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (r->type == R_PAYMENT) payment_one(c, r);
		if (r->type != R_REPORT) continue;
		const mrec *t = rec_of(r->call);
		if (!t)
			emit(c, MOR_ABOUT_REPORT, r->fp, r->call, MOR_UNCHECKED, mq(MOR_Q_AS_SHOWN, NULL), "G11.4(d)",
			     "the act that holds the call this report answers is not held: that part is unchecked (G9); what its "
			     "signer %s states:%s", r->signer, lines_text(c, r));
		else if (t->type != R_TERMS && t->type != R_PAYMENT && t->type != R_SETTLEMENT)
			emit(c, MOR_ABOUT_REPORT, r->fp, r->call, MOR_NOT_SHOWN, mq(MOR_Q_AS_SHOWN, NULL), "G11.3",
			     "the act this report names as holding its call holds no task: listed as the word of %s:%s", r->signer,
			     lines_text(c, r));
		else if (t->type == R_SETTLEMENT) {
			bool found = false;
			for (size_t j = 0; j < c->recs.n; j++)
				if (c->recs.v[j]->type == R_PAYMENT && same_settlement(c->recs.v[j]->settlement, t->fp)) found = true;
			if (!found)
				emit(c, MOR_ABOUT_REPORT, r->fp, r->call, MOR_UNCHECKED, mq(MOR_Q_AS_SHOWN, NULL), "G11.7",
				     "a confirmation that names a settlement for which no payment is held: which payment it confirms is "
				     "unchecked (G9); what its signer %s states:%s", r->signer, lines_text(c, r));
		}
	}
	bakers_run(c);
}

/* ---------- questions ---------- */

#define ASK_ENTER(c) \
	do { \
		if (!(c) || (c)->oom || !(c)->verified) return NULL; \
		if (setjmp((c)->jb)) return NULL; \
	} while (0)

static const mor_finding *first_new(const mor_ctx *c, size_t before)
{
	return before < c->finds.n ? &c->finds.v[before] : NULL;
}

const mor_finding *mor_ask_receiver(mor_ctx *c, const char *agreement_root, const char *version_or_null)
{
	ASK_ENTER(c);
	size_t before = c->finds.n;
	sym root = intern0(c, agreement_root ? agreement_root : "");
	sym ver = version_or_null ? intern0(c, version_or_null) : NULL;
	mreceiver rc;
	receiver_for(c, root, ver, &rc);
	if (rc.q.bits == 0) rc.q.bits = MOR_Q_AS_SHOWN;
	emit(c, MOR_ABOUT_RECEIVER, root, ver, rc.ans, rc.q, "G1.4", "%s%s", rc.text, baker_note(c, rc.version));
	return first_new(c, before);
}

const mor_finding *mor_ask_in_force(mor_ctx *c, const char *terms)
{
	ASK_ENTER(c);
	size_t before = c->finds.n;
	sym fp = intern0(c, terms ? terms : "");
	mrec *t = rec_of(fp);
	mrec *rr = (t && t->type == R_TERMS) ? root_of(t) : NULL;
	if (!rr) {
		emit(c, MOR_ABOUT_VERSION, fp, NULL, MOR_UNCHECKED, mq(MOR_Q_AS_SHOWN, NULL), "G9.1", "these Terms, or the root of their agreement, are not held");
		return first_new(c, before);
	}
	mqual q = qual_of(c, rr->clast ? rr->clast : t);
	bool fork;
	if (t->invalid_rule)
		emit(c, MOR_ABOUT_VERSION, fp, NULL, MOR_INVALID, mq(0, NULL), t->invalid_rule, "%s", t->invalid_why);
	else if (in_force_now(t, &fork))
		emit(c, MOR_ABOUT_VERSION, fp, NULL, MOR_SHOWN, mq(q.bits | (fork ? MOR_Q_WITH_FORK : 0), q.anchor), "G4.6",
		     "these Terms are in force: the last version%s", fork ? "; two updates of them are a fork that stands, and they stay in force (G4.7)" : "");
	else if (t->next)
		emit(c, MOR_ABOUT_VERSION, fp, t->next->fp, MOR_NOT_SHOWN, q, "G4.9",
		     "these Terms have been replaced by %.12s; the agreement goes on in its last version (%.12s)", t->next->fp,
		     rr->clast ? rr->clast->fp : "unchecked");
	else if (t->vstate == V_TIP)
		emit(c, MOR_ABOUT_VERSION, fp, t->shared->fp, MOR_FORK, mq(q.bits | MOR_Q_WITH_FORK, q.anchor), "G4.7",
		     "a tip of a fork that stands: neither tip is in force; the Terms both replace (%.12s) stay in force", t->shared->fp);
	else if (rr->cresult == CR_UNCHECKED || t->vstate == V_UNCHECKED)
		emit(c, MOR_ABOUT_VERSION, fp, NULL, MOR_UNCHECKED, q, "G9.1", "which Terms of this agreement are in force is unchecked; the last version established is %.12s",
		     rr->clast ? rr->clast->fp : "none");
	else
		emit(c, MOR_ABOUT_VERSION, fp, NULL, MOR_NOT_SHOWN, q, "G4.1", "these Terms are not shown as in force");
	return first_new(c, before);
}

const mor_finding *mor_ask_order(mor_ctx *c, const char *later, const char *earlier)
{
	ASK_ENTER(c);
	size_t before = c->finds.n;
	sym a = intern0(c, later ? later : ""), b = intern0(c, earlier ? earlier : "");
	mrec *ra = rec_of(a), *rb = rec_of(b);
	if (ra && rb && cites(c, ra, rb))
		emit(c, MOR_ABOUT_ORDER, a, b, MOR_SHOWN, mq(0, NULL), "G13.3",
		     "the first names the second, directly or through acts that name it: it came after it; shown from the acts alone");
	else
		emit(c, MOR_ABOUT_ORDER, a, b, (ra && rb) ? MOR_NOT_SHOWN : MOR_UNCHECKED, mq(0, NULL), "G13.3",
		     "%s", (ra && rb) ? "the first does not name the second: no order is shown from the acts alone; whether anything "
		     "is later, and what time it was, are never shown by citing (G13.4)" : "an act asked about is not held");
	return first_new(c, before);
}

const mor_finding *mor_ask_bound(mor_ctx *c, const char *person, const char *act)
{
	ASK_ENTER(c);
	size_t before = c->finds.n;
	sym who = intern0(c, person ? person : ""), fp = intern0(c, act ? act : "");
	mrec *r = rec_of(fp);
	mqual q = mq(MOR_Q_AS_SHOWN, NULL);
	c->why[0] = '\0';
	if (!r || (r->type != R_TERMS && r->type != R_STATEMENT)) {
		emit(c, MOR_ABOUT_BOUND, who, fp, MOR_UNCHECKED, q, "G9.1", "the act is not held");
		return first_new(c, before);
	}
	if (r->invalid_rule) {
		emit(c, MOR_ABOUT_BOUND, who, fp, MOR_NOT_SHOWN, q, r->invalid_rule, "the act is invalid: it counts for nothing");
		return first_new(c, before);
	}
	mrec *t = r->type == R_TERMS ? r : rec_of(r->terms);
	if (t && t->type == R_TERMS) q = qual_of(c, t);
	if (r->type == R_TERMS) {
		int b = bound_by_terms(c, who, r, 0);
		bool fork;
		bool now = in_force_now(r, &fork);
		bool via_c = false;
		mrec *o = rec_of(r->replaces);
		mview v = signing_view(c, r);
		if (b == T_YES && assent_in_name(c, r, who, &v, 0) != T_YES && o) via_c = true;
		if (b == T_YES && now)
			emit(c, MOR_ABOUT_BOUND, who, fp, MOR_SHOWN, mq(q.bits | (fork ? MOR_Q_WITH_FORK : 0), q.anchor), via_c ? "G5.1(c)" : "G5.1(a)",
			     "%s", via_c ? "bound with no assent of their own: these Terms replace Terms by which they are bound, and the "
			     "update condition of those Terms answers valid for them (G5.2)" : "an assent that counts in their name is held, and the Terms are in force");
		else if (b == T_YES)
			emit(c, MOR_ABOUT_BOUND, who, fp, MOR_NOT_SHOWN, q, "G4.3", "they signed these Terms, which are not in force now%s%.12s",
			     r->next ? ": replaced by " : "", r->next ? r->next->fp : "");
		else
			emit(c, MOR_ABOUT_BOUND, who, fp, b == T_UNK ? MOR_UNCHECKED : MOR_NOT_SHOWN, q, "G5.1",
			     "no assent that counts in their name is shown, and no update condition binds them to these Terms; a "
			     "stranger's assent makes no one a party; an agreement named as a party that has not signed is not bound, "
			     "and neither are its parties (G1.2) %s", c->why);
		return first_new(c, before);
	}
	/* a Statement */
	if (!r->for_) {
		if (r->signer != who)
			emit(c, MOR_ABOUT_BOUND, who, fp, MOR_NOT_SHOWN, q, "G5.3",
			     "an act with no `for` and no `under` is made in its signer's own name only (%s): it is never counted as made for anyone else", r->signer);
		else if (r->kind == c->s_assent && !(t && t->type == R_TERMS && in_force_now(t, NULL)))
			emit(c, MOR_ABOUT_BOUND, who, fp, MOR_NOT_SHOWN, q, "G4.3", "they signed this assent in their own name; it binds only while the Terms it names are in force, and they are not shown as in force");
		else if (!act_counts(c, r, NULL))
			emit(c, MOR_ABOUT_BOUND, who, fp, MOR_NOT_SHOWN, q, "G12.2", "they signed it, and it does not count: it is stopped by a fork or built on a tip");
		else
			emit(c, MOR_ABOUT_BOUND, who, fp, MOR_SHOWN, q, "G5.1(a)", "they signed it in their own name");
		return first_new(c, before);
	}
	int pw = power_ok(c, r, r->kind, 0);
	int reach = T_NO;
	if (r->for_ == who) reach = T_YES;
	else if (is_fp(r->for_)) {
		mrec *xr = rec_of(r->for_);
		if (xr && xr->type == R_TERMS && !xr->replaces && xr->clast) reach = bound_by_terms(c, who, xr->clast, 0);
		else reach = T_UNK;
	}
	int b = tri_and(pw, reach);
	emit(c, MOR_ABOUT_BOUND, who, fp, b == T_YES ? MOR_SHOWN : (b == T_NO ? MOR_NOT_SHOWN : MOR_UNCHECKED), q, "G5.1(b)",
	     "%s%s", b == T_YES ? "the act's `for` names them, or an agreement by whose Terms they are bound; its `under` names Terms that are "
	     "in force and give a power for acts of this kind; that power's rule says yes; it rests on its signer " : "not shown as bound through `for`: ",
	     b == T_YES ? r->signer : (reach == T_NO ? "the act's `for` names neither them nor an agreement by whose Terms they are bound" : c->why));
	return first_new(c, before);
}

const mor_finding *mor_ask_data_path(mor_ctx *c, const char *reader, const char *report)
{
	ASK_ENTER(c);
	size_t before = c->finds.n;
	sym who = intern0(c, reader ? reader : ""), fp = intern0(c, report ? report : "");
	mrec *r = rec_of(fp);
	mqual q = mq(MOR_Q_AS_SHOWN, NULL);
	if (!r || r->type != R_REPORT) {
		emit(c, MOR_ABOUT_DATA_PATH, who, fp, MOR_UNCHECKED, q, "G9.1", "the report is not held");
		return first_new(c, before);
	}
	mrec *t = rec_of(r->call);
	sym payer = (t && t->type == R_PAYMENT) ? t->signer : NULL;
	for (size_t i = 0; i < c->recs.n && t && t->type == R_SETTLEMENT && !payer; i++)
		if (c->recs.v[i]->type == R_PAYMENT && same_settlement(c->recs.v[i]->settlement, t->fp)) payer = c->recs.v[i]->signer;
	if (who == r->signer || (payer && who == payer)) {
		emit(c, MOR_ABOUT_DATA_PATH, who, fp, MOR_YES, q, "G11.6", "this reader is a side of the report's own leg: no path beyond the leg is asked");
		return first_new(c, before);
	}
	if (!t || t->type != R_TERMS) {
		emit(c, MOR_ABOUT_DATA_PATH, who, fp, t ? MOR_NO : MOR_UNCHECKED, q, "G11.6(b)",
		     "%s", t ? "a report of a buyer's leg carries the buyer's data, and a buyer signs no Terms that name a data cMIP: the "
		     "people of a leg are not read as having agreed to be read from beyond it unless Terms they signed say so" : "the Terms whose call it answers are not held");
		return first_new(c, before);
	}
	if (who == r->signer || who == doer_of(c, t)) {
		emit(c, MOR_ABOUT_DATA_PATH, who, fp, MOR_YES, q, "G11.6", "this reader is a side of the report's own leg: no path beyond the leg is asked");
		return first_new(c, before);
	}
	const char *ground;
	int all = k8_data(c, t, who, r, &ground);
	const char *text = ar_printf(c, "the data cMIP of the Terms %.12s: %s", t->fp, ground);
	/* a report carries the data of both sides: every Terms whose people it speaks of is asked */
	for (size_t i = 0; i < r->lines.n; i++) {
		sym to = r->lines.v[i].to;
		if (!to || !is_fp(to)) continue;
		mrec *xr = rec_of(to);
		if (!xr || xr->type != R_TERMS || xr->replaces || !xr->clast) {
			all = tri_and(all, T_UNK);
			text = ar_printf(c, "%s; the Terms of the agreement %.12s it speaks of are not held or not in force", text, to);
			continue;
		}
		int k = k8_data(c, xr->clast, who, r, &ground);
		all = tri_and(all, k);
		text = ar_printf(c, "%s; the data cMIP of the Terms %.12s: %s", text, xr->clast->fp, ground);
	}
	emit(c, MOR_ABOUT_DATA_PATH, who, fp, all == T_YES ? MOR_YES : (all == T_NO ? MOR_NO : MOR_UNCHECKED), q, "G11.6(a)",
	     "may this reader read this report beyond its leg: only where every data cMIP asked answers yes: %s", text);
	return first_new(c, before);
}

const mor_finding *mor_ask_before_paying(mor_ctx *c, const char *agreement_root)
{
	ASK_ENTER(c);
	size_t before = c->finds.n;
	sym root = intern0(c, agreement_root ? agreement_root : "");
	sym seen[66];
	unsigned n = 0;
	for (;;) {
		mrec *r = rec_of(root);
		if (!r || r->type != R_TERMS || r->replaces || r->cresult == CR_UNCHECKED || r->cresult == CR_NONE) {
			emit(c, MOR_ABOUT_SEQUENCE, root, NULL, MOR_UNCHECKED, mq(MOR_Q_AS_SHOWN, NULL), "G11.4(e)",
			     "the Terms in force of this agreement are not held or unchecked: whether it carries a call is unchecked");
			break;
		}
		if (r->cresult == CR_NOT_IN_FORCE) {
			emit(c, MOR_ABOUT_SEQUENCE, root, NULL, MOR_NOT_SHOWN, qual_of(c, r), "G11.4(e)", "this agreement has no Terms in force");
			break;
		}
		mrec *t = r->clast;
		mqual q = qual_of(c, t);
		if (r->cresult == CR_FORK) q.bits |= MOR_Q_WITH_FORK;
		const char *htext;
		int holds = holds_control(c, t, &htext);
		const mcall *call = holds == T_YES ? control_call(c, t) : NULL;
		if (holds == T_UNK)
			emit(c, MOR_ABOUT_CALL, root, t->fp, MOR_UNCHECKED, q, "G11.4(e)", "whether these Terms hold the Control task is unchecked: %s", htext);
		else if (call)
			emit(c, MOR_ABOUT_CALL, root, t->fp, MOR_SHOWN, q, "G11.4(e)", "read from the Terms alone, before any payment: they carry a call for reports, from: %s", call->from);
		else if (holds == T_YES)
			emit(c, MOR_ABOUT_CALL, root, t->fp, MOR_NO_REPORT_CALLED_FOR, q, "G11.4(e)", "they hold the Control task and carry no call");
		else
			emit(c, MOR_ABOUT_CALL, root, t->fp, MOR_NOT_SHOWN, q, "G11.4(e)", "these Terms do not hold the Control task, and no call of theirs is read: %s", htext);
		if (t->data.present)
			emit(c, MOR_ABOUT_DATA_PATH, root, t->data.cmip, MOR_SHOWN, q, "G11.4(e)",
			     "these Terms name a data cMIP, by which their signers agreed to a path beyond the leg: %s", t->data.cmip);
		else
			emit(c, MOR_ABOUT_DATA_PATH, root, NULL, MOR_NO, q, "G11.4(e)", "`data` is empty: no path beyond the leg is agreed in those Terms");
		/* the agreements of a sequence, for this question: follow `receiver` from agreement to agreement */
		sym rv = t->receiver;
		if (!rv || !is_fp(rv)) break;
		bool loop = rv == root;
		for (unsigned i = 0; i < n; i++)
			if (seen[i] == rv) loop = true;
		if (loop) {
			emit(c, MOR_ABOUT_SEQUENCE, rv, NULL, MOR_NOT_SHOWN, q, "G1.4", "an agreement is met a second time on the way: no one receives");
			break;
		}
		if (n >= 64 || n >= c->opt.max_receiver_hops) {
			emit(c, MOR_ABOUT_SEQUENCE, rv, NULL, MOR_UNCHECKED, q, "G9.2", "stopped following receivers from agreement to agreement after %u", n);
			break;
		}
		seen[n++] = root;
		root = rv;
	}
	return first_new(c, before);
}

/* a result that depends on a sequence, read leg by leg from the reports held (G11.4) */
static void sequence_from(mor_ctx *c, sym subject, mrec *t, sym party, int depth)
{
	mqual q = qual_of(c, t);
	const char *htext;
	if (depth > (int)c->opt.max_receiver_hops) {
		emit(c, MOR_ABOUT_SEQUENCE, subject, t->fp, MOR_UNCHECKED, q, "G9.2", "stopped reading legs after %d", depth);
		return;
	}
	int holds = holds_control(c, t, &htext);
	if (holds != T_YES || doer_of(c, t) != party) {
		emit(c, MOR_ABOUT_SEQUENCE, subject, t->fp, holds == T_UNK ? MOR_UNCHECKED : MOR_NOT_SHOWN, q, "G11.4",
		     "after %s received: the Terms %.12s %s; no further leg is read from them", party, t->fp,
		     holds == T_UNK ? "cannot be checked for the Control task" : "do not make that party the doer of a Control task");
		return;
	}
	const mcall *call = control_call(c, t);
	if (!call) {
		emit(c, MOR_ABOUT_SEQUENCE, subject, t->fp, MOR_NO_REPORT_CALLED_FOR, q, "G11.4(d)",
		     "%s holds for others under the Terms %.12s, which carry no call: the legs beyond are not shown as part of this result", party, t->fp);
		return;
	}
	const mrec *dr = NULL;
	for (size_t i = 0; i < c->recs.n; i++) {
		const mrec *r = c->recs.v[i];
		if (r->type == R_REPORT && r->call == t->fp && r->signer == party && is_doer_claim(r)) dr = r;
	}
	if (!dr) {
		bool sided = reader_is_side(c, t, party);
		emit(c, MOR_ABOUT_SEQUENCE, subject, t->fp, sided ? MOR_UNANSWERED : MOR_NOT_SHOWN, q, "G11.4(d)",
		     "no leg report by the doer %s under the Terms %.12s is held%s: the legs beyond are not shown as part of this result",
		     party, t->fp, sided ? "" : "; this verifier reads as no side of that leg, so the call is not shown as unanswered");
		return;
	}
	for (size_t j = 0; j < dr->lines.n; j++) {
		const mline *l = &dr->lines.v[j];
		if (l->type != LN_PASSED) continue;
		mtie tie = tie_line(c, t, dr, l);
		emit(c, MOR_ABOUT_SEQUENCE, subject, l->settlement, call->from == c->s_both ? tie.ans : MOR_SHOWN, q, "G11.4",
		     "a leg under the Terms %.12s, signed by %s (G8.4): %s", t->fp, dr->signer, tie.text);
		if (!is_fp(l->to)) continue;
		sym who;
		const char *rtext;
		if (receiving_side(c, dr, l->to, &who, &rtext) != MOR_SHOWN) continue;
		mreceiver rc;
		receiver_for(c, l->to, NULL, &rc);
		if (rc.version) sequence_from(c, subject, rc.version, who, depth + 1);
	}
}

const mor_finding *mor_ask_sequence(mor_ctx *c, const char *payment)
{
	ASK_ENTER(c);
	size_t before = c->finds.n;
	sym fp = intern0(c, payment ? payment : "");
	mrec *p = rec_of(fp);
	if (!p || p->type != R_PAYMENT) {
		emit(c, MOR_ABOUT_SEQUENCE, fp, NULL, MOR_UNCHECKED, mq(MOR_Q_AS_SHOWN, NULL), "G9.1", "the payment is not held");
		return first_new(c, before);
	}
	const mor_finding *pf = mor_find(c, MOR_ABOUT_PAYMENT, fp, NULL);
	mreceiver rc;
	receiver_for(c, p->to, p->version, &rc);
	emit(c, MOR_ABOUT_SEQUENCE, fp, p->settlement, pf ? pf->answer : MOR_UNCHECKED, rc.q, "G11.4",
	     "the buyer's leg: %s", pf ? pf->text : "");
	if (rc.ans == MOR_SHOWN && rc.version) sequence_from(c, fp, rc.version, rc.party, 0);
	else
		emit(c, MOR_ABOUT_SEQUENCE, fp, p->to, rc.ans, rc.q, "G11.4", "who received is %s: no further leg is read: %s",
		     mor_answer_name(rc.ans), rc.text);
	return first_new(c, before);
}
