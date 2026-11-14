/* engine.c: lines and forks (G6, G12), versions and what is in force (G4),
 * who signed and who is bound (G1.2, G5), the place (G7), order from citing
 * (G13.3) and the receiver for an act (G1.4). */
#include "internal.h"

/* ---------- small helpers ---------- */

bool is_signer_party(const mrec *t, sym who)
{
	for (size_t i = 0; i < t->parties.n; i++)
		if (t->parties.v[i].how == PARTY_SIGNER && t->parties.v[i].id == who) return true;
	return false;
}

const mparty *find_role(const mrec *t, sym label)
{
	for (size_t i = 0; i < t->parties.n; i++)
		if (t->parties.v[i].how == PARTY_ROLE && strcmp(t->parties.v[i].id, label) == 0) return &t->parties.v[i];
	return NULL;
}

bool is_resolver_shaped(const mrec *r)
{
	return r->resolves.n > 0 && r->continues && sym_in(&r->resolves, r->continues);
}

static bool is_agreement_act(const mrec *r) { return r->type == R_TERMS || r->type == R_STATEMENT; }

/* ---------- the place (G7; stand-in S4) ---------- */

static struct placeinfo *place_get(mor_ctx *c, sym id)
{
	for (size_t i = 0; i < c->places.n; i++)
		if (c->places.v[i].id == id) return &c->places.v[i];
	struct placeinfo pi;
	memset(&pi, 0, sizeof pi);
	pi.id = id;
	pi.status = PL_NO_ANSWER;
	for (size_t i = 0; i < c->recs.n; i++) {
		const mrec *r = c->recs.v[i];
		if (r->type != R_PLACE || r->place != id) continue;
		if (pi.status == PL_NO_ANSWER) pi.status = PL_OK;
		for (size_t j = 0; j < r->entries.n; j++) {
			mentry e = r->entries.v[j];
			bool dup = false;
			for (size_t k = 0; k < pi.entries.n; k++) {
				const mentry *o = &pi.entries.v[k];
				if (o->fp == e.fp && o->pos == e.pos) dup = true;
				else if (o->fp == e.fp || o->pos == e.pos) pi.status = PL_CONTRADICT; /* G7.2 */
			}
			if (!dup) vec_push(c, pi.entries, e);
		}
	}
	vec_push(c, c->places, pi);
	return &c->places.v[c->places.n - 1];
}

bool place_pos(mor_ctx *c, sym place, sym fp, uint64_t *pos)
{
	struct placeinfo *pi = place_get(c, place);
	if (pi->status != PL_OK) return false;
	for (size_t i = 0; i < pi->entries.n; i++)
		if (pi->entries.v[i].fp == fp) {
			*pos = pi->entries.v[i].pos;
			return true;
		}
	return false;
}

mview view_of(mor_ctx *c, sym place)
{
	mview v;
	memset(&v, 0, sizeof v);
	if (!place && c->anch_place) {
		place = c->anch_place; /* only where the agreement names none (G13.4) */
		v.anchoring = true;
	}
	if (place) {
		v.place = place;
		v.pstat = place_get(c, place)->status == PL_OK ? 1 : 2;
	}
	return v;
}

mview view_for_version(mor_ctx *c, const mrec *t) { return view_of(c, t->place); }

mqual qual_of_view(const mview *v)
{
	if (v->pstat == 0) return mq(MOR_Q_AS_SHOWN, NULL);
	return mq(v->anchoring ? MOR_Q_PER_ANCHORING : MOR_Q_PER_PLACE, v->place);
}

mqual qual_of(mor_ctx *c, const mrec *t)
{
	mview v = view_of(c, t ? t->place : NULL);
	return qual_of_view(&v);
}

bool visible(mor_ctx *c, const mview *v, const mrec *r)
{
	if (v->pstat == 2) return false;
	if (v->cited_by && !cites(c, v->cited_by, r)) return false;
	if (v->pstat == 1) {
		uint64_t pos;
		if (!place_pos(c, v->place, r->fp, &pos)) return false; /* G7.2: only once the place shows it */
		if (v->has_cut && pos > v->cut) return false;
	}
	return true;
}

/* ---------- order from citing (G13.3) ---------- */

static void push_ref(mor_ctx *c, sym s)
{
	mrec *x = rec_of(s);
	if (x && x->mark != c->epoch) {
		x->mark = c->epoch;
		vec_push(c, c->stack, x);
	}
}

/* does `a` name `b`, directly or through acts that name it? */
bool cites(mor_ctx *c, mrec *a, const mrec *b)
{
	if (a == b) return false;
	c->epoch++;
	c->stack.n = 0;
	a->mark = c->epoch;
	vec_push(c, c->stack, a);
	while (c->stack.n) {
		mrec *r = c->stack.v[--c->stack.n];
		if (r == b) return true;
		push_ref(c, r->terms);
		push_ref(c, r->under);
		push_ref(c, r->replaces);
		push_ref(c, r->previous);
		push_ref(c, r->settlement);
		push_ref(c, r->call);
		push_ref(c, r->version);
		for (size_t i = 0; i < r->answers.n; i++) push_ref(c, r->answers.v[i]);
		for (size_t i = 0; i < r->resolves.n; i++) push_ref(c, r->resolves.v[i]);
		for (size_t i = 0; i < r->relies_on.n; i++) push_ref(c, r->relies_on.v[i]);
		for (size_t i = 0; i < r->lines.n; i++) push_ref(c, r->lines.v[i].settlement);
	}
	return false;
}

/* how many acts after `a` in its signer's line `x` stands; 0 if it does not follow it */
size_t line_distance(const mor_ctx *c, const mrec *x, const mrec *a)
{
	const mrec *p = x;
	for (size_t steps = 1; steps <= c->recs.n + 1; steps++) {
		p = rec_of(p->previous);
		if (!p) return 0;
		if (p == a) return steps;
	}
	return 0;
}

bool follows_in_line(const mor_ctx *c, const mrec *x, const mrec *a) { return line_distance(c, x, a) != 0; }

/* ---------- counting an act (G12.2, G12.5) ---------- */

static mrec *root_rec(const mrec *r)
{
	mrec *x = rec_of(r->root);
	return (x && x->type == R_TERMS && !x->replaces) ? x : NULL;
}

static void ensure_chain(mor_ctx *c, const mrec *x)
{
	if (x->type != R_TERMS) return;
	mrec *rr = root_rec(x);
	if (rr && rr->ch == CH_TODO) chain_eval(c, rr);
}

bool branch_signed_by(mor_ctx *c, const mrec *x, sym signer)
{
	if (x->lstate == L_SET_ASIDE && x->signer == signer) return true;
	if (x->type != R_TERMS || x->cstate != L_SET_ASIDE) return false;
	/* the acts of the branch are its Terms acts: the tip and what follows it */
	const mrec *tip = x;
	for (size_t s = 0; s <= c->recs.n; s++) {
		const mrec *up = rec_of(tip->replaces);
		if (!up || up->cstate != L_SET_ASIDE) break;
		tip = up;
	}
	c->stack.n = 0;
	vec_push(c, c->stack, (mrec *)(uintptr_t)tip);
	c->epoch++;
	while (c->stack.n) {
		mrec *t = c->stack.v[--c->stack.n];
		if (t->mark == c->epoch) continue;
		t->mark = c->epoch;
		if (t->signer == signer) return true;
		for (size_t i = 0; i < t->ckids.n; i++) vec_push(c, c->stack, t->ckids.v[i]);
	}
	return false;
}

static bool stopped_standing(const mrec *x)
{
	return x->lstate == L_STANDING || (x->type == R_TERMS && x->cstate == L_STANDING);
}

static bool stopped_aside(const mrec *x)
{
	return x->lstate == L_SET_ASIDE || (x->type == R_TERMS && x->cstate == L_SET_ASIDE);
}

/* 0: rests on nothing stopped; 1: on a tip that stands; 2: on a tip set aside, as one who
 * signed that branch; 3: on a tip set aside, as someone else (stands, disputed) */
static int rest_one(mor_ctx *c, const mrec *r, sym name, const mrec *assent_to)
{
	mrec *x = rec_of(name);
	if (!x || !is_agreement_act(x)) return 0;
	ensure_chain(c, x);
	if (stopped_standing(x)) {
		/* an assent to a tip counts for whether that tip was signed (G12.2) */
		if (assent_to && x == assent_to && name == r->terms && x->lstate == L_OK) return 0;
		return 1;
	}
	if (stopped_aside(x)) return branch_signed_by(c, x, r->signer) ? 2 : 3;
	return 0;
}

static int rest_state(mor_ctx *c, const mrec *r, const mrec *assent_to)
{
	int worst = 0, s;
	static const int rank[4] = { 0, 3, 2, 1 };
	sym single[2] = { r->terms, r->under };
	for (int i = 0; i < 2; i++)
		if (single[i] && (s = rest_one(c, r, single[i], assent_to)) != 0 && rank[s] > rank[worst]) worst = s;
	for (size_t i = 0; i < r->answers.n; i++)
		if ((s = rest_one(c, r, r->answers.v[i], assent_to)) != 0 && rank[s] > rank[worst]) worst = s;
	for (size_t i = 0; i < r->relies_on.n; i++)
		if ((s = rest_one(c, r, r->relies_on.v[i], assent_to)) != 0 && rank[s] > rank[worst]) worst = s;
	return worst;
}

/* does this act count, as far as forks go? (invalid acts count for nothing) */
bool act_counts(mor_ctx *c, const mrec *r, const mrec *assent_to)
{
	if (r->invalid_rule || r->lstate != L_OK) return false;
	int s = rest_state(c, r, assent_to);
	return s == 0 || s == 3;
}

bool rests_on_set_aside(mor_ctx *c, const mrec *r) { return rest_state(c, r, NULL) == 3; }

/* ---------- assents, counter signatures, powers (G4.1, G4.10, G5.1) ---------- */

static int counter_effective_in(mor_ctx *c, mrec *x, mrec *t, const mview *v, int depth);

/* Whether a counter signature cancels depends on whether the Terms were in force before it,
 * which depends on the other assents and their counter signatures in turn. Signers who
 * assent and cancel again and again can make that reading grow without bound, so the work
 * spent on one outermost counter signature is counted, and past the limit the verifier
 * stops: unchecked, never not shown (G9.2). */
static int counter_effective(mor_ctx *c, mrec *x, mrec *t, const mview *v, int depth)
{
	/* the plain reading (the whole view of the Terms' place, or of the acts held) is kept */
	bool plain = !v->has_cut && !v->cited_by && !v->skip;
	if (plain && x->ceff) {
		if (x->ceff_why) why(c, "%s", x->ceff_why);
		return x->ceff - 1;
	}
	if (c->cnest == 0) c->cwork = 0;
	if (depth > 16 || ++c->cwork > MOR_MAX_COUNTER_WORK || ++c->ctotal > MOR_MAX_COUNTER_TOTAL) {
		why(c, "stopped following counter signatures that lean on one another after %u readings (G9.2)", MOR_MAX_COUNTER_WORK);
		return T_UNK;
	}
	c->cnest++;
	int r = counter_effective_in(c, x, t, v, depth);
	c->cnest--;
	if (plain) {
		x->ceff = (signed char)(r + 1);
		x->ceff_why = (r != T_NO && c->why[0]) ? ar_printf(c, "%s", c->why) : NULL;
	}
	return r;
}

static int counter_effective_in(mor_ctx *c, mrec *x, mrec *t, const mview *v, int depth)
{
	mview v2 = *v;
	if (v->pstat == 1) {
		uint64_t pos;
		if (!place_pos(c, v->place, x->fp, &pos)) return T_NO;
		if (pos == 0) return T_YES;
		v2.has_cut = true;
		v2.cut = pos - 1;
		return tri_not(in_force_signed(c, t, &v2, depth + 1)); /* the positions settle it */
	}
	/* no place: only citing gives an order (G13.3) */
	v2.cited_by = x;
	int f = in_force_signed(c, t, &v2, depth + 1);
	if (f == T_YES) return T_NO; /* it names the assents that complete the Terms: it came after */
	if (f == T_UNK) return T_UNK;
	for (size_t i = 0; i < t->stmts.n; i++) {
		mrec *y = t->stmts.v[i];
		if (y->kind == c->s_assent && visible(c, v, y) && cites(c, y, x)) return T_YES;
	}
	/* were the Terms complete at all, leaving this counter signature out? if not, it
	 * came before every party had signed, as far as the acts held show */
	mview v3 = *v;
	v3.skip = x;
	int g = in_force_signed(c, t, &v3, depth + 1);
	if (g == T_NO) return T_YES;
	if (g == T_UNK) return T_UNK;
	/* neither names the other: the assent is not shown as counted, and so the Terms
	 * are not shown as in force (G4.10) */
	why(c, "a counter signature %.12s and the assents that complete the Terms do not name one another: "
	       "no order is shown (G4.10, G13.3)", x->fp);
	return T_YES;
}

static int cancelled(mor_ctx *c, mrec *a, mrec *t, const mview *v, int depth)
{
	/* the counter signature read against an assent is the nearest one that follows it in its
	 * signer's line: a later one follows an assent already answered */
	mrec *near = NULL;
	size_t best = 0;
	for (size_t i = 0; i < t->stmts.n; i++) {
		mrec *x = t->stmts.v[i];
		if (x->kind != c->s_counter || x->signer != a->signer || x == a || x->for_ || x == v->skip) continue;
		if (!act_counts(c, x, NULL) || !visible(c, v, x)) continue;
		size_t d = line_distance(c, x, a);
		if (d && (!near || d < best)) {
			near = x;
			best = d;
		}
	}
	return near ? counter_effective(c, near, t, v, depth) : T_NO;
}

static bool statement_in(const mrec *s, const mrec *t) { return s->root == t->root; }

int assent_in_name(mor_ctx *c, mrec *t, sym name, const mview *v, int depth)
{
	int best = T_NO;
	if (v->pstat == 2) return T_UNK;
	for (size_t i = 0; i < t->stmts.n && best != T_YES; i++) {
		mrec *s = t->stmts.v[i];
		if (s->kind != c->s_assent || !statement_in(s, t)) continue;
		int nm;
		if (!s->for_) nm = s->signer == name ? T_YES : T_NO; /* G5.3: own name only */
		else nm = s->for_ == name ? T_UNK : T_NO;
		if (nm == T_NO || !act_counts(c, s, t) || !visible(c, v, s)) continue;
		if (s->for_) nm = power_ok(c, s, c->s_assent, depth + 1);
		if (nm == T_NO) continue;
		/* a counter signature cancels its own signer's own assent, and no one else's (G4.10, P17) */
		int cn = s->for_ ? T_NO : cancelled(c, s, t, v, depth);
		best = tri_or(best, tri_and(nm, tri_not(cn)));
	}
	return best;
}

/* the Terms in force of agreement `root`, for reading what its signing requires (G1.2) */
int agreement_terms(mor_ctx *c, sym root, mrec **out)
{
	mrec *r = rec_of(root);
	*out = NULL;
	if (!r || r->type != R_TERMS || r->replaces) {
		why(c, "the acts of the agreement %.12s are not held (G9.1)", root);
		return T_UNK;
	}
	if (r->ch == CH_TODO) chain_eval(c, r);
	if (r->ch == CH_BUSY) {
		*out = r->ccur;
		if (!*out) why(c, "agreements lean on one another: the reading does not settle (G9.2)");
		return *out ? T_YES : T_UNK;
	}
	switch (r->cresult) {
	case CR_IN_FORCE:
	case CR_FORK: /* the last shared version stays in force (G4.7) */
		*out = r->clast;
		return T_YES;
	case CR_NOT_IN_FORCE:
		why(c, "the agreement %.12s has no Terms in force", root);
		return T_NO;
	default:
		why(c, "which Terms of the agreement %.12s are in force is unchecked", root);
		return T_UNK;
	}
}

/* has agreement `root` signed (assented to) Terms t, as its own Terms require? One level. */
int assent_by_agreement(mor_ctx *c, mrec *t, sym root, const mview *v, int depth)
{
	bool any = false;
	if (v->pstat == 2) return T_UNK;
	for (size_t i = 0; i < t->stmts.n && !any; i++)
		if (t->stmts.v[i]->kind == c->s_assent && t->stmts.v[i]->for_ == root) any = true;
	if (!any) {
		why(c, "no act held is made for the agreement %.12s", root);
		return T_NO; /* an act not held might */
	}
	if (depth > (int)c->opt.max_power_depth) {
		why(c, "stopped following powers through powers after %u levels (G9.2)", c->opt.max_power_depth);
		return T_UNK;
	}
	mrec *x;
	int have = agreement_terms(c, root, &x);
	if (have != T_YES) return have;
	bool has_power = false;
	for (size_t i = 0; i < x->powers.n; i++)
		if (x->powers.v[i].kind == c->s_assent) has_power = true;
	if (has_power) {
		int best = T_NO;
		for (size_t i = 0; i < t->stmts.n; i++) {
			mrec *s = t->stmts.v[i];
			if (s->kind != c->s_assent || s->for_ != root || !statement_in(s, t)) continue;
			if (!act_counts(c, s, t) || !visible(c, v, s)) continue;
			best = tri_or(best, power_ok(c, s, c->s_assent, depth + 1));
		}
		return best;
	}
	/* no power for it: one act by each of its signers (the parties its Terms name as signers) */
	int all = T_YES;
	bool any_signer = false;
	for (size_t p = 0; p < x->parties.n; p++) {
		if (x->parties.v[p].how != PARTY_SIGNER) continue;
		any_signer = true;
		int got = T_NO;
		for (size_t i = 0; i < t->stmts.n; i++) {
			mrec *s = t->stmts.v[i];
			if (s->kind != c->s_assent || s->for_ != root || s->signer != x->parties.v[p].id) continue;
			if (!statement_in(s, t) || !act_counts(c, s, t) || !visible(c, v, s)) continue;
			mrec *w = rec_of(s->under);
			if (!w || w->type != R_TERMS || w->root != root) continue; /* not Terms of that agreement */
			got = tri_or(got, terms_in_force_for_act(c, w, s));
		}
		if (got == T_NO) why(c, "no act for the agreement %.12s by its signer %s", root, x->parties.v[p].id);
		all = tri_and(all, got);
	}
	if (!any_signer) {
		why(c, "the agreement %.12s names no party as a signer", root);
		return T_NO;
	}
	return all;
}

int in_force_signed(mor_ctx *c, mrec *t, const mview *v, int depth)
{
	if (v->pstat == 2) {
		why(c, "no single answer is held from the place %s (G9.1)", v->place);
		return T_UNK;
	}
	if (t->in_force.present) return k2_in_force(c, t, v, depth);
	int res = T_YES;
	for (size_t i = 0; i < t->parties.n; i++) {
		if (t->parties.v[i].how != PARTY_SIGNER) continue; /* G4.4: an agreement is not counted */
		int a = assent_in_name(c, t, t->parties.v[i].id, v, depth);
		if (a == T_NO && res != T_NO && !strstr(c->why, "G4.10"))
			why(c, "no assent is held in the name of %s", t->parties.v[i].id);
		res = tri_and(res, a);
	}
	return res;
}

/* the position at which replacing Terms came into force, per a place (G5.4) */
static bool force_pos(mor_ctx *c, mrec *n, sym place, uint64_t *out)
{
	uint64_t pos, best;
	if (!place_pos(c, place, n->fp, &best)) return false;
	for (size_t i = 0; i < n->parties.n; i++) {
		if (n->parties.v[i].how != PARTY_SIGNER) continue;
		bool have = false;
		uint64_t first = 0;
		for (size_t j = 0; j < n->stmts.n; j++) {
			mrec *s = n->stmts.v[j];
			if (s->kind != c->s_assent) continue;
			if (!(s->signer == n->parties.v[i].id || s->for_ == n->parties.v[i].id)) continue;
			if (!place_pos(c, place, s->fp, &pos)) continue;
			if (!have || pos < first) first = pos;
			have = true;
		}
		if (have && first > best) best = first;
	}
	*out = best;
	return true;
}

/* were Terms w in force for act s made under them? (G5.1(b), G5.4) */
int terms_in_force_for_act(mor_ctx *c, mrec *w, mrec *s)
{
	mrec *rr = root_rec(w);
	if (!rr) {
		why(c, "the root of the Terms named in `under` is not held (G9.1)");
		return T_UNK;
	}
	if (rr->ch == CH_TODO) chain_eval(c, rr);
	bool last = (rr->ch == CH_BUSY) ? rr->ccur == w
	                                : ((rr->cresult == CR_IN_FORCE || rr->cresult == CR_FORK) && rr->clast == w);
	if (last) return T_YES;
	if (!w->next) {
		if (rr->ch == CH_DONE && rr->cresult == CR_UNCHECKED && (w == rr->clast || w->vstate == V_UNCHECKED)) {
			why(c, "which Terms of that agreement are in force is unchecked");
			return T_UNK;
		}
		why(c, "the Terms named in `under` are not in force");
		return T_NO;
	}
	/* replaced: bound if the act was made before the replacing Terms were in force (G5.4) */
	mrec *n = w->next;
	if (w->place) {
		uint64_t ps, pf;
		if (place_get(c, w->place)->status != PL_OK) {
			why(c, "no single answer is held from the place %s (G9.1)", w->place);
			return T_UNK;
		}
		if (place_pos(c, w->place, s->fp, &ps) && force_pos(c, n, w->place, &pf) && ps < pf) return T_YES;
		why(c, "per place %s the act is not before the Terms that replace those named in `under` (G5.4)", w->place);
		return T_NO;
	}
	if (cites(c, s, n)) {
		why(c, "the act names the Terms that replace those in `under`: it was made after them (G5.4)");
		return T_NO;
	}
	if (cites(c, n, s)) return T_YES;
	for (size_t i = 0; i < n->stmts.n; i++)
		if (n->stmts.v[i]->kind == c->s_assent && cites(c, n->stmts.v[i], s)) return T_YES;
	why(c, "the Terms in `under` have been replaced, and neither the act nor the replacement names the other (G5.4)");
	return T_NO;
}

/* G5.1(b): an act made for someone under a power */
int power_ok(mor_ctx *c, mrec *s, sym kind, int depth)
{
	if (!s->for_ || !s->under) return T_NO;
	if (depth > (int)c->opt.max_power_depth) {
		why(c, "stopped following powers through powers after %u levels (G9.2)", c->opt.max_power_depth);
		return T_UNK;
	}
	mrec *w = rec_of(s->under);
	if (!w || w->type != R_TERMS) {
		why(c, "the Terms named in `under` are not held (G9.1)");
		return T_UNK;
	}
	if (w->invalid_rule) {
		why(c, "the Terms named in `under` are invalid (%s)", w->invalid_rule);
		return T_NO;
	}
	int bound;
	if (is_fp(s->for_)) {
		if (w->root != s->for_) {
			why(c, "`for` names an agreement and `under` names Terms of another: not that agreement's signature (G1.2)");
			return T_NO;
		}
		bound = T_YES;
	} else {
		bound = bound_by_terms(c, s->for_, w, depth + 1);
		if (bound == T_NO) {
			why(c, "%s is not shown as bound by the Terms named in `under`", s->for_);
			return T_NO;
		}
	}
	int inf = terms_in_force_for_act(c, w, s);
	if (inf == T_NO) return T_NO;
	int rule = T_NO;
	bool found = false;
	for (size_t i = 0; i < w->powers.n; i++) {
		if (w->powers.v[i].kind != kind) continue;
		found = true;
		rule = tri_or(rule, k3_power(c, w, &w->powers.v[i], s));
	}
	if (!found) {
		why(c, "the Terms named in `under` give no power for acts of this kind");
		return T_NO;
	}
	if (rule == T_NO) return T_NO;
	return tri_and(tri_and(bound, inf), rule);
}

static int fills_any_role(mor_ctx *c, mrec *w, sym person)
{
	int res = T_NO;
	for (size_t i = 0; i < w->parties.n; i++) {
		if (w->parties.v[i].how != PARTY_ROLE) continue;
		k5_answer ka;
		k5_spot(c, w, &w->parties.v[i], &ka);
		if (ka.status != T_YES) {
			res = tri_or(res, T_UNK);
			continue;
		}
		for (size_t j = 0; j < ka.fills.n; j++)
			if (ka.fills.v[j].signer == person) return T_YES;
	}
	return res;
}

/* is `person` bound by Terms w: by an assent that counts in their name (G5.1(a), (b)),
 * or because w replaced Terms that bound them and their update condition answered valid (G5.1(c)) */
int bound_by_terms(mor_ctx *c, sym person, mrec *w, int depth)
{
	if (depth > (int)c->opt.max_power_depth + 64) return T_UNK;
	ensure_chain(c, w);
	mrec *rr = root_rec(w);
	if (!rr) return T_UNK;
	bool version = w->vstate == V_VERSION || (rr->ch == CH_BUSY && rr->ccur == w);
	if (!version) return w->vstate == V_UNCHECKED ? T_UNK : T_NO;
	int member = is_signer_party(w, person) ? T_YES : fills_any_role(c, w, person);
	int res = T_NO;
	if (member != T_NO) {
		mview v = view_of(c, w->judged_place);
		res = tri_and(member, assent_in_name(c, w, person, &v, depth));
	}
	if (res == T_YES) return T_YES;
	mrec *o = rec_of(w->replaces);
	if (o && o->type == R_TERMS && o->condition.present && o->next == w)
		res = tri_or(res, bound_by_terms(c, person, o, depth + 1));
	return res;
}

/* ---------- the fork (G12), for lines and for chains alike ---------- */

enum { FK_NONE, FK_ONE, FK_FORK, FK_RESOLVED };
typedef struct {
	int outcome;
	mrec *winner;        /* the next act, or the tip that continues */
	recvec tips;    /* every tip */
	recvec won;     /* resolving updates that resolve */
	recvec lost;    /* rival resolving updates not continued */
	recvec rivals;  /* resolving updates that are themselves a fork (G12.7) */
} forkres;

static bool covers(const mrec *r, mrec **all, const int *lvl, size_t n, int level)
{
	bool cont = false;
	for (size_t i = 0; i < n; i++) {
		if (lvl[i] != level) continue;
		if (!sym_in(&r->resolves, all[i]->fp)) return false; /* MUST name every tip */
		if (all[i]->fp == r->continues) cont = true;
	}
	return cont;
}

static bool level_is_fork(mrec **all, const int *lvl, size_t n, int level)
{
	size_t count = 0;
	sym first = NULL;
	for (size_t i = 0; i < n; i++) {
		if (lvl[i] != level) continue;
		count++;
		if (level == 0) continue;
		if (!first) first = all[i]->continues;
		else if (all[i]->continues != first) return true; /* rivals continue different tips */
	}
	return level == 0 && count >= 2;
}

/* kids: updates of one act; ext: resolving updates signed under a power, in another line */
static void fork_solve(mor_ctx *c, mrec **kids, size_t nk, mrec **ext, size_t ne, forkres *out)
{
	size_t n = nk + ne;
	mrec **all = ar_alloc(c, (n + 1) * sizeof *all);
	int *lvl = ar_alloc(c, (n + 1) * sizeof *lvl);
	bool *forced = ar_alloc(c, (n + 1) * sizeof *forced);
	int top = 0;
	memset(out, 0, sizeof *out);
	for (size_t i = 0; i < nk; i++) all[i] = kids[i];
	for (size_t i = 0; i < ne; i++) all[nk + i] = ext[i];
	for (;;) {
		bool again = false;
		for (size_t i = 0; i < n; i++)
			lvl[i] = (i < nk && (forced[i] || !is_resolver_shaped(all[i]))) ? 0 : -1;
		top = 0;
		while (level_is_fork(all, lvl, n, top)) {
			bool any = false;
			for (size_t i = 0; i < n; i++)
				if (lvl[i] == -1 && covers(all[i], all, lvl, n, top)) {
					lvl[i] = -2;
					any = true;
				}
			if (!any) break;
			top++;
			for (size_t i = 0; i < n; i++)
				if (lvl[i] == -2) lvl[i] = top;
		}
		/* an act that names `resolves` and resolves nothing is an act like any other (G12.3) */
		for (size_t i = 0; i < nk; i++)
			if (lvl[i] == -1) {
				forced[i] = true;
				again = true;
			}
		if (!again) break;
	}
	for (size_t i = 0; i < n; i++)
		if (lvl[i] == 0) vec_push(c, out->tips, all[i]);
	if (out->tips.n == 0) {
		out->outcome = FK_NONE;
		return;
	}
	if (out->tips.n == 1 && top == 0) {
		out->outcome = FK_ONE;
		out->winner = out->tips.v[0];
		return;
	}
	if (top == 0 || level_is_fork(all, lvl, n, top)) {
		out->outcome = FK_FORK;
		for (size_t i = 0; i < n; i++)
			if (lvl[i] > 0) vec_push(c, out->rivals, all[i]);
		return;
	}
	/* resolved: follow `continues` down from the top level to a tip */
	sym pick = NULL;
	for (size_t i = 0; i < n; i++)
		if (lvl[i] == top) {
			pick = all[i]->continues;
			vec_push(c, out->won, all[i]);
		}
	for (int level = top - 1; level >= 1; level--) {
		sym nextpick = NULL;
		for (size_t i = 0; i < n; i++) {
			if (lvl[i] != level) continue;
			if (all[i]->fp == pick) {
				vec_push(c, out->won, all[i]);
				nextpick = all[i]->continues;
			} else {
				vec_push(c, out->lost, all[i]);
			}
		}
		pick = nextpick;
	}
	out->outcome = FK_RESOLVED;
	out->winner = rec_of(pick);
}

static const char *list_fps(mor_ctx *c, mrec **v, size_t n)
{
	char *buf = ar_alloc(c, n * 14 + 8);
	size_t o = 0;
	for (size_t i = 0; i < n; i++) o += (size_t)snprintf(buf + o, 15, "%s%.12s", i ? ", " : "", v[i]->fp);
	if (n == 0) memcpy(buf, "none", 5);
	return buf;
}

/* ---------- one signer's line (G6, G12) ---------- */

static void mark_line(mor_ctx *c, mrec *start, int state)
{
	size_t base = c->stack.n;
	vec_push(c, c->stack, start);
	while (c->stack.n > base) {
		mrec *r = c->stack.v[--c->stack.n];
		if (r->walked && r->lstate == state) continue;
		r->walked = true;
		r->lstate = state;
		for (size_t i = 0; i < r->lkids.n; i++) vec_push(c, c->stack, r->lkids.v[i]);
	}
}

static void walk_line(mor_ctx *c, mrec **kids0, size_t n0, sym signer, sym root, mrec *shared)
{
	recvec kids = { 0 }, carry = { 0 };
	for (size_t i = 0; i < n0; i++) vec_push(c, kids, kids0[i]);
	for (size_t guard = 0; guard <= c->recs.n; guard++) {
		if (kids.n == 0) {
			if (carry.n == 0) return;
			kids = carry;
			memset(&carry, 0, sizeof carry);
		}
		recvec ext = { 0 };
		if (kids.n >= 2)
			for (size_t i = 0; i < c->exts.n; i++) {
				mrec *e = c->exts.v[i];
				if (e->for_ == signer && e->root == root && e->signer != signer && e->ext_ok_prev == T_YES &&
				    e->lstate == L_OK && !e->invalid_rule)
					vec_push(c, ext, e);
			}
		forkres fr;
		fork_solve(c, kids.v, kids.n, ext.v, ext.n, &fr);
		if (fr.outcome == FK_NONE) return;
		if (fr.outcome == FK_FORK) {
			for (size_t i = 0; i < kids.n; i++) mark_line(c, kids.v[i], L_STANDING);
			for (size_t i = 0; i < carry.n; i++) mark_line(c, carry.v[i], L_STANDING);
			emit(c, MOR_ABOUT_LINE, signer, root, MOR_FORK, mq(MOR_Q_AS_SHOWN, NULL), "G6.2",
			     "two or more acts by %s name the same `previous` (%.12s): tips %s%s%s; both tips stop and "
			     "nothing built on them counts until a resolving update resolves the fork (G12.2, G12.3); "
			     "no tip is chosen",
			     signer, shared ? shared->fp : "none", list_fps(c, fr.tips.v, fr.tips.n),
			     fr.rivals.n ? "; rival resolving updates that continue different tips (G12.7): " : "",
			     fr.rivals.n ? list_fps(c, fr.rivals.v, fr.rivals.n) : "");
			return;
		}
		mrec *w = fr.winner;
		if (!w || w->walked) return;
		if (fr.outcome == FK_RESOLVED) {
			for (size_t i = 0; i < fr.tips.n; i++)
				if (fr.tips.v[i] != w) mark_line(c, fr.tips.v[i], L_SET_ASIDE);
			for (size_t i = 0; i < fr.lost.n; i++)
				if (fr.lost.v[i]->signer == signer) mark_line(c, fr.lost.v[i], L_SET_ASIDE);
			bool by_power = false;
			for (size_t i = 0; i < fr.won.n; i++) {
				mrec *r = fr.won.v[i];
				if (r->signer != signer) {
					by_power = true;
					emit(c, MOR_ABOUT_POWER, r->fp, r->under, MOR_SHOWN, mq(MOR_Q_AS_SHOWN, NULL), "G12.4",
					     "a resolving update signed by %s for %s under a power to resolve given by Terms %.12s; "
					     "it rests on its signer and on that power's rule",
					     r->signer, signer, r->under);
					continue;
				}
				r->walked = true;
				for (size_t j = 0; j < r->lkids.n; j++) vec_push(c, carry, r->lkids.v[j]);
			}
			emit(c, MOR_ABOUT_LINE, signer, root, MOR_SHOWN, mq(MOR_Q_AS_SHOWN, NULL), "G12.3",
			     "a fork of %s's line after %.12s (tips %s) is resolved by %s%s: %.12s continues; every other "
			     "tip is set aside and never counted as the next act",
			     signer, shared ? shared->fp : "none", list_fps(c, fr.tips.v, fr.tips.n),
			     list_fps(c, fr.won.v, fr.won.n), by_power ? " (under a power, G12.4)" : "", w->fp);
		}
		w->walked = true;
		shared = w;
		memset(&kids, 0, sizeof kids);
		for (size_t i = 0; i < w->lkids.n; i++) vec_push(c, kids, w->lkids.v[i]);
	}
}

static void lines_run(mor_ctx *c)
{
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (!is_agreement_act(r) || r->walked) continue;
		if (r->previous && !r->line_open_before) continue; /* reached from its previous act */
		if (r->line_open_before) {
			emit(c, MOR_ABOUT_LINE, r->signer, r->root, MOR_UNCHECKED, mq(MOR_Q_AS_SHOWN, NULL), "G6.1",
			     "`previous` of %.12s names an act not held: the act counts, and %s's line is unchecked before it",
			     r->fp, r->signer);
			r->walked = true;
			walk_line(c, r->lkids.v, r->lkids.n, r->signer, r->root, r);
			continue;
		}
		/* every act of this signer in this agreement that names no previous */
		recvec firsts = { 0 };
		for (size_t j = i; j < c->recs.n; j++) {
			mrec *o = c->recs.v[j];
			if (is_agreement_act(o) && !o->previous && o->signer == r->signer && o->root == r->root)
				vec_push(c, firsts, o);
		}
		walk_line(c, firsts.v, firsts.n, r->signer, r->root, NULL);
		for (size_t j = 0; j < firsts.n; j++) firsts.v[j]->walked = true;
	}
}

/* ---------- versions (G4) ---------- */

typedef struct { sym who; bool agr; } member;
typedef VEC(member) membervec;

static void member_add(mor_ctx *c, membervec *mv, sym who, bool agr)
{
	for (size_t i = 0; i < mv->n; i++)
		if (mv->v[i].who == who) return;
	member m = { who, agr };
	vec_push(c, *mv, m);
}

/* "everyone who signed the original" (G4.8(a)); T_UNK when who they are cannot be checked */
static int signed_set(mor_ctx *c, mrec *o, membervec *out, int depth)
{
	mview v = view_of(c, o->judged_place);
	int known = T_YES;
	for (size_t i = 0; i < o->parties.n; i++) {
		const mparty *p = &o->parties.v[i];
		int t;
		if (p->how == PARTY_SIGNER) {
			t = assent_in_name(c, o, p->id, &v, 0);
			if (t == T_YES) member_add(c, out, p->id, false);
		} else if (p->how == PARTY_AGREEMENT) {
			t = assent_by_agreement(c, o, p->id, &v, 0);
			if (t == T_YES) member_add(c, out, p->id, true);
		} else {
			k5_answer ka;
			bool stranger = false;
			k5_spot(c, o, p, &ka);
			for (size_t j = 0; j < o->stmts.n; j++)
				if (o->stmts.v[j]->kind == c->s_assent && !is_signer_party(o, o->stmts.v[j]->signer)) stranger = true;
			t = (ka.status == T_YES || !stranger) ? T_YES : T_UNK;
			for (size_t j = 0; j < ka.fills.n; j++)
				if (assent_in_name(c, o, ka.fills.v[j].signer, &v, 0) == T_YES)
					member_add(c, out, ka.fills.v[j].signer, false);
		}
		if (t == T_UNK) known = T_UNK;
	}
	/* bound with no assent of their own, by the update condition of the version before (G5.1(c)) */
	mrec *prev = rec_of(o->replaces);
	if (prev && prev->type == R_TERMS && prev->condition.present && prev->next == o && depth < 64) {
		membervec pv = { 0 };
		if (signed_set(c, prev, &pv, depth + 1) == T_UNK) known = T_UNK;
		for (size_t i = 0; i < pv.n; i++) {
			bool named = false;
			for (size_t j = 0; j < o->parties.n; j++)
				if (o->parties.v[j].id == pv.v[i].who &&
				    o->parties.v[j].how == (pv.v[i].agr ? PARTY_AGREEMENT : PARTY_SIGNER))
					named = true;
			if (named) member_add(c, out, pv.v[i].who, pv.v[i].agr);
		}
	}
	return known;
}

static bool power_eq(const mpower *a, const mpower *b)
{
	if (a->kind != b->kind || a->rule.cmip != b->rule.cmip || a->rule.params.n != b->rule.params.n) return false;
	for (size_t i = 0; i < a->rule.params.n; i++)
		if (a->rule.params.v[i] != b->rule.params.v[i]) return false;
	return true;
}

/* a version removes or narrows the power when its powers are those before less one or more (G5.5) */
static bool narrows(const mrec *o, const mrec *u)
{
	if (u->powers.n >= o->powers.n) return false;
	for (size_t i = 0; i < u->powers.n; i++) {
		bool found = false;
		for (size_t j = 0; j < o->powers.n && !found; j++) found = power_eq(&u->powers.v[i], &o->powers.v[j]);
		if (!found) return false;
	}
	return true;
}

enum { US_NOT, US_UPDATE, US_UNK };

/* is Terms act u an update of version o (the original): valid, and signed as its own in_force asks */
static int update_status(mor_ctx *c, mrec *u, mrec *o, const mview *v)
{
	mqual q = qual_of_view(v);
	u->evald = true;
	if (u->invalid_rule) return US_NOT;
	if (u->root != o->root) {
		emit(c, MOR_ABOUT_REPLACES, u->fp, o->fp, MOR_NOT_SHOWN, q, "G4.8",
		     "its `agreement` does not name the root of the Terms it would replace: not shown as a version of that agreement");
		return US_NOT;
	}
	if (!act_counts(c, u, NULL)) {
		emit(c, MOR_ABOUT_REPLACES, u->fp, o->fp, MOR_NOT_SHOWN, q, "G12.2",
		     "the act is stopped by a fork in its signer's line, or built on a tip: it counts for nothing while the fork stands");
		return US_NOT;
	}
	if (!visible(c, v, u)) {
		emit(c, MOR_ABOUT_REPLACES, u->fp, o->fp, MOR_NOT_SHOWN, q, "G4.8(d)",
		     "the place the original names does not show this Terms act: it counts only once that place shows it");
		return US_NOT;
	}
	int valid = T_NO, inf;
	bool forced = false;
	const char *how = "";
	c->why[0] = '\0';
	if (is_resolver_shaped(u)) {
		/* a power to resolve, read from the shared Terms: kind `resolves` and no other (G12.4) */
		for (size_t i = 0; i < o->powers.n; i++) {
			if (o->powers.v[i].kind != c->s_resolves) continue;
			int t = k3_power(c, o, &o->powers.v[i], u);
			if (t == T_YES) {
				emit(c, MOR_ABOUT_REPLACES, u->fp, o->fp, MOR_SHOWN, q, "G12.4",
				     "a resolving update signed under the power to resolve that the shared Terms give (rule %.12s); "
				     "it rests on its signer %s and on that rule; the update condition is not asked of it",
				     o->powers.v[i].rule.cmip, u->signer);
				valid = T_YES;
				forced = true;
			}
		}
	}
	if (forced) {
		how = "under a power to resolve";
	} else if (o->condition.present) {
		char ground[400];
		mqual gq;
		valid = k4_condition(c, o, u, v, ground, sizeof ground, &gq);
		if (valid == T_UNK)
			emit(c, MOR_ABOUT_REPLACES, u->fp, o->fp, MOR_UNCHECKED, q, "G4.11(h)",
			     "whether this update replaces is unchecked: %s", c->why);
		else
			emit(c, MOR_ABOUT_REPLACES, u->fp, o->fp, valid == T_YES ? MOR_VALID : MOR_NOT_VALID, gq,
			     valid == T_YES ? "G4.11(d)" : "G4.11(c)", "update condition %s; ground:%s%s%s", o->condition.cmip,
			     ground[0] == ' ' ? "" : " ", ground,
			     valid == T_YES ? "" : "; it replaces nothing, whoever signed it, and is not a tip of a fork");
		how = "by the update condition named";
	} else {
		/* the default update condition: everyone who signed the original (G4.11(e)) */
		membervec ms = { 0 };
		int known = signed_set(c, o, &ms, 0);
		char lacking[160] = "";
		valid = T_YES;
		for (size_t i = 0; i < ms.n; i++) {
			int t = ms.v[i].agr ? assent_by_agreement(c, u, ms.v[i].who, v, 0) : assent_in_name(c, u, ms.v[i].who, v, 0);
			if (t == T_NO && !lacking[0]) snprintf(lacking, sizeof lacking, "%.80s", ms.v[i].who);
			valid = tri_and(valid, t);
		}
		if (valid == T_YES && known == T_UNK) valid = T_UNK;
		const char *note = "";
		if (valid != T_YES && o->powers.n > 0) {
			/* a delegation: read from the body (G5.5) */
			k1_reading rd;
			k1_read(c, o, &rd);
			if (rd.status != T_YES) {
				valid = T_UNK;
				why(c, "the body of Terms that carry `powers` cannot be read, so whether they are a delegation "
				       "that its delegator can end alone cannot be told (G5.5)");
			} else if (rd.is_delegation && narrows(o, u)) {
				sym part = rd.end == 1 ? rd.agent : rd.delegator;
				if (rd.end == 2) {
					valid = T_UNK;
					why(c, "the delegation states conditions on ending it that this verifier cannot read (G5.5)");
				} else {
					int t = is_fp(part) ? assent_by_agreement(c, u, part, v, 0) : assent_in_name(c, u, part, v, 0);
					if (t == T_YES) {
						valid = T_YES;
						forced = true;
						note = rd.end == 1 ? "; a delegation whose stated condition leaves ending to the agent: the agent's part alone (G5.5)"
						                   : "; a delegation, ended or narrowed by its delegator's part alone (G5.5)";
					} else if (t == T_UNK) {
						valid = T_UNK;
					} else if (rd.end == 1) {
						note = "; a delegation whose signed condition leaves ending to the agent alone: the delegator's "
						       "part alone replaces nothing; whether the agent will end it is not shown (G5.5)";
					}
				}
			}
		}
		if (valid == T_YES)
			emit(c, MOR_ABOUT_REPLACES, u->fp, o->fp, MOR_VALID, mq(q.bits | MOR_Q_BY_DEFAULT, q.anchor), "G4.11(e)",
			     "by default: an assent to the update is held in the name of each of the %zu who signed the original%s",
			     ms.n, note);
		else if (valid == T_NO)
			emit(c, MOR_ABOUT_REPLACES, u->fp, o->fp, MOR_NOT_SHOWN, q, "G4.8(c)",
			     "as a replacement, by default: no assent to it is held in the name of %s, who signed the original; "
			     "an assent not held may exist; the Terms it names stay in force and it is no tip of a fork%s",
			     lacking, note);
		else
			emit(c, MOR_ABOUT_REPLACES, u->fp, o->fp, MOR_UNCHECKED, q, "G9.1",
			     "whether this update replaces is unchecked: %s", c->why);
		how = "by default";
	}
	if (valid == T_NO) return US_NOT;
	if (valid == T_UNK) return US_UNK;
	c->why[0] = '\0';
	inf = forced ? T_YES : in_force_signed(c, u, v, 0);
	if (inf == T_NO) {
		emit(c, MOR_ABOUT_VERSION, u->fp, o->fp, MOR_NOT_SHOWN, q, "G4.5",
		     "the update is answered valid (%s), but these Terms are not in force by their own `in_force` (G4.1): %s; "
		     "the Terms they name stay in force",
		     how, c->why);
		return US_NOT;
	}
	if (inf == T_UNK) {
		emit(c, MOR_ABOUT_VERSION, u->fp, o->fp, MOR_UNCHECKED, q, "G9.1",
		     "whether these Terms are in force by their own `in_force` is unchecked: %s", c->why);
		return US_UNK;
	}
	return US_UPDATE;
}

static void mark_chain(mor_ctx *c, mrec *start, int state)
{
	size_t base = c->stack.n;
	vec_push(c, c->stack, start);
	c->epoch++;
	while (c->stack.n > base) {
		mrec *r = c->stack.v[--c->stack.n];
		if (r->mark == c->epoch) continue;
		r->mark = c->epoch;
		r->cstate = state;
		for (size_t i = 0; i < r->ckids.n; i++) vec_push(c, c->stack, r->ckids.v[i]);
	}
}

static void chain_finish(mor_ctx *c, mrec *r, int result, mrec *last, mqual q, const char *rule, const char *text)
{
	r->cresult = result;
	r->clast = last;
	r->ch = CH_DONE;
	mor_answer a = result == CR_IN_FORCE || result == CR_FORK ? MOR_SHOWN
	               : (result == CR_NOT_IN_FORCE ? MOR_NOT_SHOWN : MOR_UNCHECKED);
	if (result == CR_FORK) q.bits |= MOR_Q_WITH_FORK;
	emit(c, MOR_ABOUT_IN_FORCE, r->fp, last ? last->fp : NULL, a, q, rule, "%s", text);
	/* Terms that have been replaced: not shown as in force; the agreement goes on (G4.9) */
	for (mrec *x = r; x && x != last && x->next; x = x->next)
		emit(c, MOR_ABOUT_VERSION, x->fp, x->next->fp, MOR_NOT_SHOWN, q, "G4.9",
		     "these Terms have been replaced by %.12s; the agreement goes on in its last version, and every "
		     "version replaced stays an act of it",
		     x->next->fp);
}

void chain_eval(mor_ctx *c, mrec *r)
{
	if (r->ch != CH_TODO) return;
	r->ch = CH_BUSY;
	r->evald = true;
	mview v = view_of(c, r->place);
	mqual q = qual_of_view(&v);
	if (r->invalid_rule) {
		chain_finish(c, r, CR_NOT_IN_FORCE, NULL, q, r->invalid_rule, "the first Terms act is invalid: it counts for nothing");
		return;
	}
	if (!act_counts(c, r, NULL)) {
		chain_finish(c, r, CR_NOT_IN_FORCE, NULL, q, "G12.2",
		             "the first Terms act is stopped by a fork in its signer's line, or built on a tip");
		return;
	}
	if (v.pstat == 2) {
		chain_finish(c, r, CR_UNCHECKED, NULL, q, "G9.1",
		             ar_printf(c, "no single answer is held from the place %s: which Terms are in force is unchecked (G7.2)", v.place));
		return;
	}
	if (!visible(c, &v, r)) {
		chain_finish(c, r, CR_NOT_IN_FORCE, NULL, q, "G7.2", "the place the Terms name does not show the first Terms act");
		return;
	}
	c->why[0] = '\0';
	int f = in_force_signed(c, r, &v, 0);
	if (f != T_YES) {
		chain_finish(c, r, f == T_NO ? CR_NOT_IN_FORCE : CR_UNCHECKED, NULL, q, f == T_NO ? "G4.1" : "G9.1",
		             ar_printf(c, "no Terms of this agreement are %s as in force: %s", f == T_NO ? "shown" : "checked", c->why));
		return;
	}
	r->vstate = V_VERSION;
	r->judged_place = r->place;
	mrec *cur = r;
	recvec carry = { 0 };
	for (unsigned steps = 0;; steps++) {
		r->ccur = cur;
		v = view_of(c, cur->place);
		q = qual_of_view(&v);
		if (steps >= c->opt.max_chain) {
			chain_finish(c, r, CR_UNCHECKED, cur, q, "G9.2",
			             ar_printf(c, "stopped following the versions of this chain after %u; the last version established is %.12s",
			                       steps, cur->fp));
			return;
		}
		if (v.pstat == 2) {
			chain_finish(c, r, CR_UNCHECKED, cur, q, "G9.1",
			             ar_printf(c, "no single answer is held from the place %s (two answers that contradict, or none): "
			                          "the last version established is %.12s", v.place, cur->fp));
			return;
		}
		recvec kids = { 0 }, upd = { 0 };
		for (size_t i = 0; i < cur->ckids.n; i++) vec_push(c, kids, cur->ckids.v[i]);
		if (kids.n == 0 && carry.n) {
			kids = carry;
			memset(&carry, 0, sizeof carry);
		}
		bool unk = false;
		for (size_t i = 0; i < kids.n; i++) {
			int st = update_status(c, kids.v[i], cur, &v);
			if (st == US_UPDATE) vec_push(c, upd, kids.v[i]);
			if (st == US_UNK) {
				kids.v[i]->vstate = V_UNCHECKED;
				unk = true;
			}
		}
		if (unk) {
			for (size_t i = 0; i < upd.n; i++) upd.v[i]->vstate = V_UNCHECKED;
			chain_finish(c, r, CR_UNCHECKED, cur, q, "G4.8(e)",
			             ar_printf(c, "a Terms act that names %.12s cannot be checked as an update: which Terms are in force is "
			                          "unchecked from there on; the last version established is %.12s", cur->fp, cur->fp));
			return;
		}
		forkres fr;
		fork_solve(c, upd.v, upd.n, NULL, 0, &fr);
		if (fr.outcome == FK_NONE) {
			chain_finish(c, r, CR_IN_FORCE, cur, q, "G4.6",
			             ar_printf(c, "the last version; the answer stands on %s",
			                       v.pstat ? "the place named, as of its answers held" : "the acts this verifier holds: an act not held may be later"));
			return;
		}
		if (fr.outcome == FK_FORK) {
			for (size_t i = 0; i < upd.n; i++) mark_chain(c, upd.v[i], L_STANDING);
			for (size_t i = 0; i < fr.tips.n; i++) {
				fr.tips.v[i]->vstate = V_TIP;
				fr.tips.v[i]->shared = cur;
			}
			emit(c, MOR_ABOUT_REPLACED_BY, cur->fp, NULL, MOR_FORK, q, "G4.7",
			     "two or more updates of the same Terms, each signed by all who needed to sign it: tips %s%s%s; no tip is "
			     "chosen, by position or in any other way; both tips stop and nothing built on either counts (G12.2)",
			     list_fps(c, fr.tips.v, fr.tips.n),
			     fr.rivals.n ? "; rival resolving updates that continue different tips, themselves a fork (G12.7): " : "",
			     fr.rivals.n ? list_fps(c, fr.rivals.v, fr.rivals.n) : "");
			chain_finish(c, r, CR_FORK, cur, q, "G4.7",
			             "the last shared version stays in force while the fork stands; every answer that rests on the "
			             "Terms in force is given from it, with the fork");
			return;
		}
		mrec *w = fr.winner;
		if (!w || w->vstate == V_VERSION) { /* cannot happen with real fingerprints */
			chain_finish(c, r, CR_UNCHECKED, cur, q, "G9.2", "the reading of this chain does not settle");
			return;
		}
		if (fr.outcome == FK_RESOLVED) {
			for (size_t i = 0; i < fr.tips.n; i++) {
				if (fr.tips.v[i] == w) continue;
				mark_chain(c, fr.tips.v[i], L_SET_ASIDE);
				fr.tips.v[i]->vstate = V_SET_ASIDE;
				fr.tips.v[i]->shared = cur;
			}
			for (size_t i = 0; i < fr.lost.n; i++) mark_chain(c, fr.lost.v[i], L_SET_ASIDE);
			for (size_t i = 0; i < fr.won.n; i++)
				for (size_t j = 0; j < fr.won.v[i]->ckids.n; j++) vec_push(c, carry, fr.won.v[i]->ckids.v[j]);
			emit(c, MOR_ABOUT_REPLACED_BY, cur->fp, w->fp, MOR_SHOWN, q, "G12.3",
			     "a fork of Terms (tips %s) is resolved by the resolving update %s, signed as an update of the shared "
			     "Terms must be: %.12s continues and counts as the next version; every other tip is set aside and never "
			     "counted as the next version",
			     list_fps(c, fr.tips.v, fr.tips.n), list_fps(c, fr.won.v, fr.won.n), w->fp);
		}
		cur->next = w;
		w->vstate = V_VERSION;
		w->judged_place = cur->place;
		cur = w;
	}
}

/* ---------- the receiver for an act (G1.4) ---------- */

void receiver_for(mor_ctx *c, sym root, sym version, mreceiver *out)
{
	sym seen[66];
	unsigned nseen = 0;
	unsigned extra = 0;
	memset(out, 0, sizeof *out);
	out->ans = MOR_NOT_SHOWN;
	out->text = "";
	for (;;) {
		mrec *r = rec_of(root);
		mrec *vv = NULL;
		if (!r || r->type != R_TERMS || r->replaces) {
			out->ans = MOR_UNCHECKED;
			out->text = ar_printf(c, "the acts of the agreement %.12s are not held (G9.1)", root);
			return;
		}
		chain_eval(c, r);
		if (r->ch == CH_BUSY) {
			out->ans = MOR_UNCHECKED;
			out->text = "agreements lean on one another: the reading does not settle (G9.2)";
			return;
		}
		if (version) {
			vv = rec_of(version);
			if (!vv || vv->type != R_TERMS) {
				out->ans = MOR_UNCHECKED;
				out->text = ar_printf(c, "the version %.12s the act names is not held (G9.1)", version);
				return;
			}
			mqual vq = qual_of(c, vv);
			out->q.bits |= vq.bits;
			out->q.anchor = vq.anchor;
			if (vv->root != root || vv->vstate == V_NONE) {
				out->text = ar_printf(c, "the Terms %.12s the act names are not shown as a version of that agreement", version);
				return;
			}
			if (vv->vstate == V_UNCHECKED) {
				out->ans = MOR_UNCHECKED;
				out->text = ar_printf(c, "whether %.12s is a version of that agreement is unchecked", version);
				return;
			}
			if (vv->vstate == V_TIP) {
				/* a tip of a fork that stands: the receiver of the Terms both branches replace */
				extra |= MOR_Q_WITH_FORK | MOR_Q_BUILT_ON_TIP;
				vv = vv->shared;
			} else if (vv->vstate == V_SET_ASIDE) {
				extra |= MOR_Q_DISPUTED; /* read as if the tip had stood alone (G12.5) */
			}
		} else {
			if (r->cresult == CR_NOT_IN_FORCE) {
				out->text = ar_printf(c, "the agreement %.12s has no Terms in force", root);
				return;
			}
			if (r->cresult == CR_UNCHECKED) {
				out->ans = MOR_UNCHECKED;
				out->text = ar_printf(c, "which Terms of %.12s are in force is unchecked; the last version established is %.12s",
				                      root, r->clast ? r->clast->fp : "none");
				return;
			}
			vv = r->clast;
			if (r->cresult == CR_FORK) extra |= MOR_Q_WITH_FORK;
			mqual vq = qual_of(c, vv);
			out->q.bits |= vq.bits;
			if (!out->q.anchor) out->q.anchor = vq.anchor;
		}
		out->q.bits |= extra;
		out->version = vv;
		sym rv = vv->receiver;
		if (!rv) {
			out->text = ar_printf(c, "the version %.12s names no receiver: no one receives", vv->fp);
			return;
		}
		if (rv[0] == '@') {
			if (!is_signer_party(vv, rv)) {
				out->text = ar_printf(c, "the version %.12s names as receiver %s, who is not among its own parties", vv->fp, rv);
				return;
			}
			out->ans = MOR_SHOWN;
			out->party = rv;
			out->text = ar_printf(c, "%s, named as receiver by the version %.12s%s", rv, vv->fp,
			                      (extra & MOR_Q_WITH_FORK) ? " (the Terms both branches replace; no branch is chosen)" : "");
			return;
		}
		if (strncmp(rv, "role:", 5) == 0) {
			const mparty *role = find_role(vv, rv + 5);
			k5_answer ka;
			if (!role) {
				out->text = ar_printf(c, "the version %.12s names as receiver a role that is not among its own parties", vv->fp);
				return;
			}
			k5_spot(c, vv, role, &ka);
			if (ka.status != T_YES) {
				out->ans = MOR_UNCHECKED;
				out->text = ar_printf(c, "who fills the spot named as receiver is unchecked: %s", c->why);
				return;
			}
			if (ka.fills.n != 1) {
				out->text = ar_printf(c, "the spot named as receiver is %s: which of them receives is not said",
				                      ka.fills.n ? "filled more than once" : "not filled");
				return;
			}
			out->ans = MOR_SHOWN;
			out->party = ka.fills.v[0].signer;
			if (ka.fills.v[0].disputed) out->q.bits |= MOR_Q_DISPUTED;
			out->text = ar_printf(c, "%s, who fills the spot `%s` named as receiver by the version %.12s, on the proof %.12s "
			                         "signed by %s (G8.4)", out->party, role->id, vv->fp, ka.fills.v[0].proof->fp,
			                      ka.fills.v[0].proof->signer);
			return;
		}
		/* another agreement: an agreement of a sequence (G11.1); read its receiver the same way */
		for (unsigned i = 0; i < nseen; i++)
			if (seen[i] == rv) {
				out->text = ar_printf(c, "the agreement %.12s is met a second time on the way: no one receives", rv);
				return;
			}
		if (root == rv) {
			out->text = ar_printf(c, "the agreement %.12s names itself as receiver: no one receives", rv);
			return;
		}
		if (nseen >= 64 || (unsigned)out->hops >= c->opt.max_receiver_hops) {
			out->ans = MOR_UNCHECKED;
			out->text = ar_printf(c, "stopped following receivers from agreement to agreement after %d (G9.2)", out->hops);
			return;
		}
		seen[nseen++] = root;
		out->hops++;
		root = rv;
		version = NULL;
	}
}

/* ---------- the clock (G7.4; stand-in S5) ---------- */

bool clock_time(mor_ctx *c, const mrec *terms, uint64_t *t, mqual *q, int *status)
{
	sym id = terms->clock ? terms->clock : c->anch_clock;
	bool have = false;
	*status = 0;
	if (!id) return false;
	*q = mq(terms->clock ? MOR_Q_PER_CLOCK : MOR_Q_PER_ANCHORING, id);
	*status = 2;
	for (size_t i = 0; i < c->recs.n; i++) {
		const mrec *r = c->recs.v[i];
		if (r->type != R_CLOCK || r->clock != id) continue;
		if (have && r->time != *t) return false; /* two answers that contradict */
		*t = r->time;
		have = true;
	}
	return have;
}

/* ---------- the run ---------- */

static void reset_analysis(mor_ctx *c)
{
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		r->lstate = r->cstate = L_OK;
		r->vstate = V_NONE;
		r->next = r->shared = NULL;
		r->judged_place = NULL;
		r->walked = r->evald = false;
		r->ceff = 0;
		r->ceff_why = NULL;
		r->ch = CH_TODO;
		r->cresult = CR_NONE;
		r->clast = r->ccur = NULL;
	}
	c->finds.n = c->base_finds;
	c->places.n = 0;
}

static void prepare(mor_ctx *c)
{
	c->finds.n = 0;
	c->exts.n = 0;
	c->ctotal = 0;
	c->unsettled = false;
	for (size_t i = 0; i < c->rejects.n; i++) vec_push(c, c->finds, c->rejects.v[i]);
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		r->lkids.n = r->ckids.n = r->stmts.n = 0;
		r->line_open_before = false;
		r->ext_ok = r->ext_ok_prev = T_NO;
	}
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (!is_agreement_act(r)) continue;
		if (r->previous) {
			mrec *p = rec_of(r->previous);
			if (!p || !is_agreement_act(p)) {
				r->line_open_before = true;
			} else if (p->signer != r->signer || p->root != r->root) {
				if (!r->invalid_rule) {
					r->invalid_rule = "G6.1";
					r->invalid_why = p->signer != r->signer ? "`previous` names an act of another signer"
					                                        : "`previous` names an act of another agreement";
				}
			} else {
				vec_push(c, p->lkids, r);
			}
		}
		if (r->type == R_TERMS && r->replaces) {
			mrec *o = rec_of(r->replaces);
			if (o && o->type == R_TERMS) vec_push(c, o->ckids, r);
		}
		if (r->type == R_STATEMENT) {
			mrec *t = rec_of(r->terms);
			if (t && t->type == R_TERMS) vec_push(c, t->stmts, r);
			if (r->for_ && is_resolver_shaped(r)) vec_push(c, c->exts, r);
		}
	}
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (r->invalid_rule)
			emit(c, MOR_ABOUT_ACT, r->fp, NULL, MOR_INVALID, mq(0, NULL), r->invalid_rule,
			     "%s; an invalid act counts for nothing", r->invalid_why);
		for (size_t j = 0; j < r->unread.n; j++)
			emit(c, MOR_ABOUT_UNREAD, r->fp, r->unread.v[j], MOR_NOTE, mq(0, NULL), "G2.2",
			     "the field `%s` is not one section 3 gives: left unread; it does not make the act invalid", r->unread.v[j]);
		for (size_t j = 0; j < r->relies_on.n && is_agreement_act(r); j++)
			if (!is_fp(r->relies_on.v[j]))
				emit(c, MOR_ABOUT_UNREAD, r->fp, r->relies_on.v[j], MOR_NOTE, mq(0, NULL), "G2.2",
				     "an entry of `relies_on` that is not a fingerprint: left unread, not part of this act");
		if (r->type != R_TERMS) continue;
		for (size_t j = 0; j < r->powers.n; j++) {
			sym k = r->powers.v[j].kind;
			if (k == c->s_replaces)
				emit(c, MOR_ABOUT_UNREAD, r->fp, k, MOR_NOTE, mq(0, NULL), "G5.2",
				     "an entry of `powers` of kind `replaces` gives no power: left unread; no power makes a Terms act replace");
			else if (k != c->s_assent && k != c->s_counter && k != c->s_contest && k != c->s_resolves && !is_fp(k))
				emit(c, MOR_ABOUT_UNREAD, r->fp, k, MOR_NOTE, mq(0, NULL), "G2.2",
				     "an entry of `powers` whose kind is none section 3.1 gives: left unread; it gives no power");
		}
	}
	c->base_finds = c->finds.n;
}

void engine_run(mor_ctx *c)
{
	prepare(c);
	bool settled = false;
	for (int pass = 0; pass < 4 && !settled; pass++) {
		reset_analysis(c);
		lines_run(c);
		for (size_t i = 0; i < c->recs.n; i++) {
			mrec *r = c->recs.v[i];
			if (r->type == R_TERMS && !r->replaces) chain_eval(c, r);
		}
		settled = true;
		for (size_t i = 0; i < c->exts.n; i++) {
			mrec *e = c->exts.v[i];
			e->ext_ok = power_ok(c, e, c->s_resolves, 0);
			if (e->ext_ok != e->ext_ok_prev) settled = false;
			e->ext_ok_prev = e->ext_ok;
		}
	}
	if (!settled) {
		/* forks and powers to resolve them lean on one another (G9.2) */
		c->unsettled = true;
		for (size_t i = c->base_finds; i < c->finds.n; i++) {
			mor_finding *f = &c->finds.v[i];
			if (f->answer == MOR_FORK || (f->quals & MOR_Q_WITH_FORK) || f->about == MOR_ABOUT_LINE)
				f->answer = MOR_UNCHECKED;
		}
		emit(c, MOR_ABOUT_SETTLING, "forks", NULL, MOR_UNCHECKED, mq(0, NULL), "G9.2",
		     "forks, and powers to resolve them, lean on one another so that the reading of this set of acts does not "
		     "settle: every answer that rests on a fork in it is unchecked");
	}
	/* Terms acts that name Terms never reached as a version */
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *r = c->recs.v[i];
		if (r->type != R_TERMS || !r->replaces || r->evald || r->invalid_rule) continue;
		mrec *o = rec_of(r->replaces);
		mrec *rr = root_rec(r);
		mqual q = qual_of(c, o && o->type == R_TERMS ? o : NULL);
		if (!o || o->type != R_TERMS || !rr)
			emit(c, MOR_ABOUT_REPLACES, r->fp, r->replaces, MOR_UNCHECKED, q, "G9.1",
			     "the Terms it names in `replaces`, or the root of its agreement, are not held");
		else if (r->cstate == L_STANDING)
			emit(c, MOR_ABOUT_REPLACES, r->fp, r->replaces, MOR_NOT_SHOWN, mq(q.bits | MOR_Q_WITH_FORK, q.anchor), "G12.2",
			     "it follows a tip of a fork that stands: nothing built on either tip counts until the fork is resolved");
		else if (r->cstate == L_SET_ASIDE)
			emit(c, MOR_ABOUT_REPLACES, r->fp, r->replaces, MOR_NOT_SHOWN, q, "G12.3",
			     "it follows a tip set aside: no act that follows such a tip in the line counts");
		else if (rr->cresult == CR_UNCHECKED)
			emit(c, MOR_ABOUT_REPLACES, r->fp, r->replaces, MOR_UNCHECKED, q, "G4.8(e)",
			     "it lies beyond the last version this verifier has established (%.12s)", rr->clast ? rr->clast->fp : "none");
		else
			emit(c, MOR_ABOUT_REPLACES, r->fp, r->replaces, MOR_NOT_SHOWN, q, "G4.8",
			     "the Terms it names are not shown as a version of the agreement: it replaces nothing");
	}
}
