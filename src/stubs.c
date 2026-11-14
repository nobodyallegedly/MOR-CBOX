/* stubs.c: the eight sockets K1 to K8, each behind one function.
 *
 * A cMIP is a document named by fingerprint. In this stand-in (STANDINS.md S6)
 * the document is a `cmip` record whose `stub` line names a behaviour; the
 * socket's parameters come from the Terms. A cMIP the verifier does not hold
 * cannot be applied: the answer is unchecked (G9.1). The grammar never computes
 * a task's output itself: it calls these functions and uses what comes back. */
#include "internal.h"

static mrec *cmip_doc(mor_ctx *c, const msock *s)
{
	mrec *d = (s->present && s->is_fp) ? rec_of(s->cmip) : NULL;
	if (!d || d->type != R_CMIP) {
		why(c, "the cMIP %.12s is not held (G9.1)", s->present ? s->cmip : "(none)");
		return NULL;
	}
	return d;
}

static bool stub_is(const mrec *d, const char *name) { return strcmp(d->stub, name) == 0; }

static bool sym_u64(sym s, uint64_t *out)
{
	uint64_t v = 0;
	size_t i = 0;
	if (!s || !s[0]) return false;
	for (; s[i]; i++) {
		if (s[i] < '0' || s[i] > '9' || i >= 19) return false;
		v = v * 10 + (uint64_t)(s[i] - '0');
	}
	*out = v;
	return true;
}

static int unknown_stub(mor_ctx *c, const mrec *d)
{
	why(c, "the cMIP %.12s has behaviour `%s`, which this socket's stub cannot apply (G9.1)", d->fp, d->stub);
	return T_UNK;
}

/* K1 Body: what do these words say? Parts for machines only. */
void k1_read(mor_ctx *c, mrec *terms, k1_reading *out)
{
	memset(out, 0, sizeof *out);
	out->status = T_UNK;
	mrec *w = terms->body ? rec_of(terms->body) : NULL;
	if (!w || w->type != R_WORDS) {
		why(c, "the words of the body are not held (G9.1)");
		return;
	}
	mrec *d = cmip_doc(c, &terms->body_cmip);
	if (!d) return;
	if (!stub_is(d, "plain")) {
		unknown_stub(c, d);
		return;
	}
	out->status = T_YES;
	for (size_t i = 0; i < w->wlines.n; i++) {
		const symvec *t = &w->wlines.v[i].t;
		if (t->n == 0) continue;
		if (strcmp(t->v[0], "share") == 0 && t->n == 3) {
			struct mshare sh;
			sh.who = t->v[1];
			if (sym_u64(t->v[2], &sh.n)) vec_push(c, out->shares, sh);
		} else if (strcmp(t->v[0], "delegation") == 0 && t->n == 3) {
			out->is_delegation = true;
			out->delegator = t->v[1];
			out->agent = t->v[2];
		} else if (strcmp(t->v[0], "end") == 0 && t->n >= 2) {
			out->end = strcmp(t->v[1], "none") == 0 ? 0 : (strcmp(t->v[1], "agent-only") == 0 ? 1 : 2);
		} else if (strcmp(t->v[0], "passes") == 0 && t->n == 2) {
			vec_push(c, out->passes, t->v[1]);
		}
	}
}

/* K2 In force: are the assents that put these Terms in force present? */
int k2_in_force(mor_ctx *c, mrec *terms, const mview *v, int depth)
{
	mrec *d = cmip_doc(c, &terms->in_force);
	const symvec *p = &terms->in_force.params;
	if (!d) return T_UNK;
	if (stub_is(d, "yes")) return T_YES;
	if (stub_is(d, "no")) {
		why(c, "the `in_force` cMIP %.12s answers no", d->fp);
		return T_NO;
	}
	if (stub_is(d, "count")) {
		uint64_t need, yes = 0, unk = 0;
		if (p->n < 1 || !sym_u64(p->v[0], &need)) return unknown_stub(c, d);
		for (size_t i = 1; i < p->n; i++) {
			int t = is_fp(p->v[i]) ? assent_by_agreement(c, terms, p->v[i], v, depth)
			                       : assent_in_name(c, terms, p->v[i], v, depth);
			if (t == T_YES) yes++;
			else if (t == T_UNK) unk++;
		}
		if (yes >= need) return T_YES;
		if (yes + unk >= need) return T_UNK;
		why(c, "the `in_force` cMIP %.12s asks for %llu assents of those it lists; %llu are held", d->fp,
		    (unsigned long long)need, (unsigned long long)yes);
		return T_NO;
	}
	return unknown_stub(c, d);
}

/* K3 Power: was this act made by those who may? */
int k3_power(mor_ctx *c, mrec *terms, const mpower *p, mrec *act)
{
	mrec *d = cmip_doc(c, &p->rule);
	if (!d) return T_UNK;
	if (stub_is(d, "yes")) return T_YES;
	if (stub_is(d, "no")) {
		why(c, "the rule %.12s of the power answers no", d->fp);
		return T_NO;
	}
	if (stub_is(d, "signer")) {
		if (sym_in(&p->rule.params, act->signer)) return T_YES;
		why(c, "the rule %.12s of the power does not accept the signer %s", d->fp, act->signer);
		return T_NO;
	}
	if (stub_is(d, "author")) {
		/* a rule that is handed `authors` (G3.2): yes where an author signed */
		if (sym_in(&terms->authors, act->signer)) return T_YES;
		why(c, "the rule %.12s accepts only an author of the Terms", d->fp);
		return T_NO;
	}
	return unknown_stub(c, d);
}

/* K4 Update condition: can the agreement be updated like this? */
int k4_condition(mor_ctx *c, mrec *original, mrec *update, const mview *v, char *ground, size_t gcap, mqual *gq)
{
	mrec *d = cmip_doc(c, &original->condition);
	const symvec *p = &original->condition.params;
	*gq = qual_of_view(v);
	ground[0] = '\0';
	if (!d) return T_UNK;
	if (stub_is(d, "yes")) {
		snprintf(ground, gcap, "the cMIP answers valid to every update");
		return T_YES;
	}
	if (stub_is(d, "no")) {
		snprintf(ground, gcap, "the cMIP answers not valid to every update");
		return T_NO;
	}
	if (stub_is(d, "count")) {
		uint64_t need, yes = 0, unk = 0;
		size_t o = 0;
		if (p->n < 1 || !sym_u64(p->v[0], &need)) return unknown_stub(c, d);
		for (size_t i = 1; i < p->n; i++) {
			int t = is_fp(p->v[i]) ? assent_by_agreement(c, update, p->v[i], v, 0)
			                       : assent_in_name(c, update, p->v[i], v, 0);
			if (t == T_YES) {
				yes++;
				if (o < gcap) {
					int w = snprintf(ground + o, gcap - o, " %.20s", p->v[i]);
					if (w > 0) o += (size_t)w;
				}
			} else if (t == T_UNK) {
				unk++;
			}
		}
		if (o >= gcap) o = gcap - 1;
		snprintf(ground + o, gcap - o, "%s: %llu assent(s) to the update held, each reported by its signer; %llu asked",
		         yes ? "" : " (none)", (unsigned long long)yes, (unsigned long long)need);
		if (yes >= need) return T_YES;
		if (yes + unk >= need) {
			why(c, "an assent the cMIP counts cannot be checked");
			return T_UNK;
		}
		return T_NO;
	}
	if (stub_is(d, "after")) {
		uint64_t t0, now;
		int st;
		if (p->n < 1 || !sym_u64(p->v[0], &t0)) return unknown_stub(c, d);
		if (!clock_time(c, original, &now, gq, &st)) {
			why(c, st ? "no single answer is held from the clock (G9.1)"
			          : "the cMIP reads a time; the agreement names no clock and the reader accepts no anchoring");
			return T_UNK;
		}
		snprintf(ground, gcap, "a time: %llu, against %llu asked", (unsigned long long)now, (unsigned long long)t0);
		return now >= t0 ? T_YES : T_NO;
	}
	return unknown_stub(c, d);
}

/* K5 Spot: who fills this spot, by which proof, on whose word; is it still open? */
void k5_spot(mor_ctx *c, mrec *version, const mparty *role, k5_answer *out)
{
	memset(out, 0, sizeof *out);
	out->status = T_UNK;
	mrec *d = cmip_doc(c, &role->spot);
	const symvec *p = &role->spot.params;
	if (!d) return;
	if (!stub_is(d, "witness") || p->n < 2) {
		unknown_stub(c, d);
		return;
	}
	mview v = view_for_version(c, version);
	if (v.pstat == 2) {
		why(c, "no answer is held from the place the Terms name (G9.1)");
		return;
	}
	out->status = T_YES;
	out->witness = p->v[0];
	out->many = strcmp(p->v[1], "many") == 0;
	for (size_t i = 0; i < c->recs.n; i++) {
		mrec *s = c->recs.v[i];
		if (s->type != R_STATEMENT || s->signer != out->witness || s->root != version->root) continue;
		if (s->content.n != 3 || strcmp(s->content.v[0], "fills") != 0 || s->content.v[1] != role->id) continue;
		if (!is_fp(s->kind) || !act_counts(c, s, NULL) || !visible(c, &v, s)) continue;
		mfilling f;
		f.signer = s->content.v[2];
		f.proof = s;
		f.earned = s->terms;
		f.disputed = rests_on_set_aside(c, s);
		vec_push(c, out->fills, f);
	}
	uint64_t max;
	if (p->n >= 4 && strcmp(p->v[2], "max") == 0 && sym_u64(p->v[3], &max)) {
		out->open = out->fills.n < max ? T_YES : T_NO;
		out->cause = ar_printf(c, "a count the version in force states: %zu of %llu fillings", out->fills.n,
		                       (unsigned long long)max);
	} else {
		out->open = T_YES;
		out->cause = "the version in force states no limit";
	}
}

/* K6 Control: has what these Terms receive been passed on as they say? */
int k6_control(mor_ctx *c, mrec *terms)
{
	mrec *d = cmip_doc(c, &terms->control);
	if (!d) return T_UNK;
	if (stub_is(d, "yes")) return T_YES;
	if (stub_is(d, "no")) return T_NO;
	return unknown_stub(c, d);
}

/* K7 Kind: definition only; held or not. */
int k7_kind(mor_ctx *c, sym kind)
{
	mrec *d = rec_of(kind);
	if (!d || d->type != R_CMIP) {
		why(c, "the cMIP of the kind %.12s is not held (G9.1)", kind);
		return T_UNK;
	}
	return T_YES;
}

/* K8 Data: may this reader read this report beyond its own leg? */
int k8_data(mor_ctx *c, mrec *terms, sym reader, mrec *report, const char **ground)
{
	(void)report;
	if (!terms->data.present) {
		*ground = "`data` is empty: no path beyond the leg (G11.6(a))";
		return T_NO;
	}
	mrec *d = cmip_doc(c, &terms->data);
	if (!d) {
		*ground = "the data cMIP is not held (G9.1)";
		return T_UNK;
	}
	if (stub_is(d, "yes")) {
		*ground = "the data cMIP answers yes for every reader";
		return T_YES;
	}
	if (stub_is(d, "no")) {
		*ground = "the data cMIP answers no";
		return T_NO;
	}
	if (stub_is(d, "readers")) {
		if (reader && sym_in(&terms->data.params, reader)) {
			*ground = "the data cMIP lists this reader";
			return T_YES;
		}
		*ground = "the data cMIP does not list this reader";
		return T_NO;
	}
	*ground = "the data cMIP's behaviour is not one this stub can apply (G9.1)";
	return T_UNK;
}
