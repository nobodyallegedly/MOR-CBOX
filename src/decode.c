/* decode.c: the decoder of untrusted bytes (stand-in act format, STANDINS.md S1).
 *
 * A file is a run of records. A record is:
 *     mor-act 1 <type>\n
 *     <key>[ <token>]...\n        (tokens are printable ASCII, one space apart)
 *     sig <64 hex>\n              (acts only; documents carry none)
 *     end\n
 * Between records, blank lines and lines starting with '#' are skipped.
 * The fingerprint of a record is taken over its bytes from "mor-act" through
 * "end\n". Anything else is rejected: it never reaches the grammar rules. */
#include "internal.h"

#define MAX_TOK (MOR_MAX_LINE / 2 + 2)

typedef struct { const char *p; size_t n; } tok;

static bool tok_is(const tok *t, const char *s) { return strlen(s) == t->n && memcmp(t->p, s, t->n) == 0; }
static sym tsym(mor_ctx *c, const tok *t) { return intern(c, t->p, t->n); }

/* split a line into tokens; -1 if it holds anything but printable ASCII and single spaces */
static int split(const unsigned char *s, size_t n, tok *t)
{
	int nt = 0;
	size_t i = 0;
	if (n == 0 || n > MOR_MAX_LINE) return -1;
	while (i < n) {
		size_t st = i;
		while (i < n && s[i] != ' ') {
			if (s[i] < 0x21 || s[i] > 0x7e) return -1;
			i++;
		}
		if (i == st) return -1; /* leading or doubled space */
		t[nt].p = (const char *)s + st;
		t[nt].n = i - st;
		nt++;
		if (i < n) {
			i++; /* the space */
			if (i == n) return -1; /* trailing space */
		}
	}
	return nt;
}

static bool tok_fp(const tok *t)
{
	if (t->n != MOR_FP_LEN) return false;
	for (size_t i = 0; i < t->n; i++)
		if (!((t->p[i] >= '0' && t->p[i] <= '9') || (t->p[i] >= 'a' && t->p[i] <= 'f'))) return false;
	return true;
}

static bool tok_u64(const tok *t, uint64_t *out)
{
	uint64_t v = 0;
	if (t->n == 0 || t->n > 20) return false;
	for (size_t i = 0; i < t->n; i++) {
		if (t->p[i] < '0' || t->p[i] > '9') return false;
		uint64_t d = (uint64_t)(t->p[i] - '0');
		if (v > (UINT64_MAX - d) / 10) return false;
		v = v * 10 + d;
	}
	*out = v;
	return true;
}

#define E_TWICE "a field is given twice"
#define E_SHAPE "a field does not have the shape it must have"

static const char *set1(sym *dst, sym v)
{
	if (*dst) return E_TWICE;
	*dst = v;
	return NULL;
}

static const char *set_fp(mor_ctx *c, sym *dst, const tok *t, int nt)
{
	if (nt != 2 || !tok_fp(&t[1])) return E_SHAPE;
	return set1(dst, tsym(c, &t[1]));
}

static const char *push_fp(mor_ctx *c, symvec *v, const tok *t, int nt)
{
	if (nt != 2 || !tok_fp(&t[1])) return E_SHAPE;
	vec_push(c, *v, tsym(c, &t[1]));
	return NULL;
}

static const char *set_sock(mor_ctx *c, msock *s, const tok *t, int nt, int from)
{
	if (s->present) return E_TWICE;
	if (nt <= from) return E_SHAPE;
	s->present = true;
	s->is_fp = tok_fp(&t[from]);
	s->cmip = tsym(c, &t[from]);
	for (int i = from + 1; i < nt; i++) vec_push(c, s->params, tsym(c, &t[i]));
	return NULL;
}

static const char *set_previous(mor_ctx *c, mrec *r, const tok *t, int nt)
{
	if (r->has_previous) return E_TWICE;
	if (nt != 2) return E_SHAPE;
	if (tok_is(&t[1], "none")) r->previous = NULL;
	else if (tok_fp(&t[1])) r->previous = tsym(c, &t[1]);
	else return E_SHAPE;
	r->has_previous = true;
	return NULL;
}

static const char *set_signer(mor_ctx *c, mrec *r, const tok *t, int nt)
{
	if (nt != 2 || t[1].p[0] != '@' || t[1].n < 2) return E_SHAPE;
	return set1(&r->signer, tsym(c, &t[1]));
}

static bool has_prefix(const tok *t, const char *pre)
{
	size_t n = strlen(pre);
	return t->n >= n && memcmp(t->p, pre, n) == 0;
}

static const char *line_terms(mor_ctx *c, mrec *r, const tok *t, int nt)
{
	const tok *k = &t[0];
	if (tok_is(k, "signer")) return set_signer(c, r, t, nt);
	if (tok_is(k, "previous")) return set_previous(c, r, t, nt);
	if (tok_is(k, "replaces")) return set_fp(c, &r->replaces, t, nt);
	if (tok_is(k, "agreement")) return set_fp(c, &r->agreement, t, nt);
	if (tok_is(k, "party")) {
		mparty p;
		memset(&p, 0, sizeof p);
		if (nt < 3) return E_SHAPE;
		if (tok_is(&t[1], "signer")) {
			if (nt != 3 || t[2].p[0] != '@' || t[2].n < 2) return E_SHAPE;
			p.how = PARTY_SIGNER;
			p.id = tsym(c, &t[2]);
		} else if (tok_is(&t[1], "agreement")) {
			if (nt != 3 || !tok_fp(&t[2])) return E_SHAPE;
			p.how = PARTY_AGREEMENT;
			p.id = tsym(c, &t[2]);
		} else if (tok_is(&t[1], "role")) {
			p.how = PARTY_ROLE;
			p.id = tsym(c, &t[2]);
			for (int i = 3; i < nt; i++) {
				tok v = t[i];
				if (!p.spot.present && has_prefix(&t[i], "kind=")) {
					if (p.kind) return E_TWICE;
					v.p += 5, v.n -= 5;
					if (v.n == 0) return E_SHAPE;
					p.kind = tsym(c, &v);
				} else if (!p.spot.present && has_prefix(&t[i], "what=")) {
					if (p.what) return E_TWICE;
					v.p += 5, v.n -= 5;
					if (v.n == 0) return E_SHAPE;
					p.what = tsym(c, &v);
				} else if (!p.spot.present && has_prefix(&t[i], "cmip=")) {
					v.p += 5, v.n -= 5;
					if (v.n == 0) return E_SHAPE;
					p.spot.present = true;
					p.spot.is_fp = tok_fp(&v);
					p.spot.cmip = tsym(c, &v);
				} else if (p.spot.present) {
					vec_push(c, p.spot.params, tsym(c, &t[i]));
				} else {
					return E_SHAPE;
				}
			}
		} else {
			return E_SHAPE;
		}
		for (size_t i = 0; i < r->parties.n; i++)
			if (r->parties.v[i].how == p.how && r->parties.v[i].id == p.id) return E_TWICE;
		vec_push(c, r->parties, p);
		return NULL;
	}
	if (tok_is(k, "author")) {
		if (nt != 2) return E_SHAPE;
		vec_push(c, r->authors, tsym(c, &t[1]));
		return NULL;
	}
	if (tok_is(k, "body")) {
		if (r->body) return E_TWICE;
		if (nt < 2) return E_SHAPE;
		r->body = tsym(c, &t[1]);
		if (nt > 2) return set_sock(c, &r->body_cmip, t, nt, 2);
		return NULL;
	}
	if (tok_is(k, "relies_on")) {
		if (nt != 2) return E_SHAPE;
		vec_push(c, r->relies_on, tsym(c, &t[1]));
		return NULL;
	}
	if (tok_is(k, "in_force")) return set_sock(c, &r->in_force, t, nt, 1);
	if (tok_is(k, "condition")) return set_sock(c, &r->condition, t, nt, 1);
	if (tok_is(k, "control")) return set_sock(c, &r->control, t, nt, 1);
	if (tok_is(k, "data")) return set_sock(c, &r->data, t, nt, 1);
	if (tok_is(k, "power")) {
		mpower p;
		memset(&p, 0, sizeof p);
		if (nt < 3) return E_SHAPE;
		p.kind = tsym(c, &t[1]);
		const char *e = set_sock(c, &p.rule, t, nt, 2);
		if (e) return e;
		vec_push(c, r->powers, p);
		return NULL;
	}
	if (tok_is(k, "place")) return nt == 2 ? set1(&r->place, tsym(c, &t[1])) : E_SHAPE;
	if (tok_is(k, "clock")) return nt == 2 ? set1(&r->clock, tsym(c, &t[1])) : E_SHAPE;
	if (tok_is(k, "receiver")) {
		if (nt != 2) return E_SHAPE;
		if (!(tok_fp(&t[1]) || (t[1].p[0] == '@' && t[1].n > 1) || (has_prefix(&t[1], "role:") && t[1].n > 5)))
			return E_SHAPE;
		return set1(&r->receiver, tsym(c, &t[1]));
	}
	if (tok_is(k, "envelope")) return set_fp(c, &r->envelope, t, nt);
	if (tok_is(k, "call")) {
		mcall cl;
		if (nt < 3) return E_SHAPE;
		if (!tok_is(&t[2], "doer") && !tok_is(&t[2], "receiving") && !tok_is(&t[2], "both")) return E_SHAPE;
		cl.task = tsym(c, &t[1]);
		cl.from = tsym(c, &t[2]);
		vec_push(c, r->calls, cl);
		/* a call says its task and from whom, and nothing else: anything more (a place,
		 * a reader) is not read (G11.6) */
		if (nt > 3) vec_push(c, r->unread, intern0(c, "call:more-than-task-and-from"));
		return NULL;
	}
	if (tok_is(k, "resolves")) return push_fp(c, &r->resolves, t, nt);
	if (tok_is(k, "continues")) return set_fp(c, &r->continues, t, nt);
	/* a field section 3 does not give: not read (G2.2) */
	vec_push(c, r->unread, tsym(c, k));
	return NULL;
}

static const char *line_statement(mor_ctx *c, mrec *r, const tok *t, int nt)
{
	const tok *k = &t[0];
	if (tok_is(k, "signer")) return set_signer(c, r, t, nt);
	if (tok_is(k, "previous")) return set_previous(c, r, t, nt);
	if (tok_is(k, "agreement")) return set_fp(c, &r->agreement, t, nt);
	if (tok_is(k, "terms")) return set_fp(c, &r->terms, t, nt);
	if (tok_is(k, "kind")) {
		if (nt != 2) return E_SHAPE;
		if (!tok_fp(&t[1]) && !tok_is(&t[1], "assent") && !tok_is(&t[1], "counter_signature") &&
		    !tok_is(&t[1], "contest"))
			return E_SHAPE;
		return set1(&r->kind, tsym(c, &t[1]));
	}
	if (tok_is(k, "content")) {
		if (r->has_content) return E_TWICE;
		r->has_content = true;
		for (int i = 1; i < nt; i++) vec_push(c, r->content, tsym(c, &t[i]));
		return NULL;
	}
	if (tok_is(k, "for")) {
		if (nt != 2 || !(tok_fp(&t[1]) || (t[1].p[0] == '@' && t[1].n > 1))) return E_SHAPE;
		return set1(&r->for_, tsym(c, &t[1]));
	}
	if (tok_is(k, "under")) return set_fp(c, &r->under, t, nt);
	if (tok_is(k, "answers")) return push_fp(c, &r->answers, t, nt);
	if (tok_is(k, "relies_on")) {
		if (nt != 2) return E_SHAPE;
		vec_push(c, r->relies_on, tsym(c, &t[1]));
		return NULL;
	}
	if (tok_is(k, "settlement")) return set_fp(c, &r->settlement, t, nt);
	if (tok_is(k, "resolves")) return push_fp(c, &r->resolves, t, nt);
	if (tok_is(k, "continues")) return set_fp(c, &r->continues, t, nt);
	vec_push(c, r->unread, tsym(c, k));
	return NULL;
}

static const char *line_report(mor_ctx *c, mrec *r, const tok *t, int nt)
{
	const tok *k = &t[0];
	mline ln;
	memset(&ln, 0, sizeof ln);
	if (tok_is(k, "signer")) return set_signer(c, r, t, nt);
	if (tok_is(k, "call")) return set_fp(c, &r->call, t, nt);
	if (tok_is(k, "relies_on")) return push_fp(c, &r->relies_on, t, nt);
	if (nt < 3 || !tok_u64(&t[1], &ln.amount)) return E_SHAPE;
	ln.unit = tsym(c, &t[2]);
	if (tok_is(k, "received")) {
		if (nt != 4 || !tok_fp(&t[3])) return E_SHAPE;
		ln.type = LN_RECEIVED;
		ln.settlement = tsym(c, &t[3]);
	} else if (tok_is(k, "passed")) {
		if (nt != 5 || !tok_fp(&t[4]) || !(tok_fp(&t[3]) || t[3].p[0] == '@')) return E_SHAPE;
		ln.type = LN_PASSED;
		ln.to = tsym(c, &t[3]);
		ln.settlement = tsym(c, &t[4]);
	} else if (tok_is(k, "own") || tok_is(k, "kept")) {
		if (nt != 3) return E_SHAPE;
		ln.type = tok_is(k, "own") ? LN_OWN : LN_KEPT;
	} else if (tok_is(k, "owed")) {
		if (nt != 4 || !(tok_fp(&t[3]) || t[3].p[0] == '@')) return E_SHAPE;
		ln.type = LN_OWED;
		ln.to = tsym(c, &t[3]);
	} else {
		return "a report line this stand-in does not know";
	}
	vec_push(c, r->lines, ln);
	return NULL;
}

static const char *line_other(mor_ctx *c, mrec *r, const tok *t, int nt)
{
	const tok *k = &t[0];
	switch (r->type) {
	case R_PAYMENT:
		if (tok_is(k, "signer")) return set_signer(c, r, t, nt);
		if (tok_is(k, "to")) return set_fp(c, &r->to, t, nt);
		if (tok_is(k, "version")) return set_fp(c, &r->version, t, nt);
		if (tok_is(k, "settlement")) return set_fp(c, &r->settlement, t, nt);
		break;
	case R_SETTLEMENT:
		if (tok_is(k, "signer")) return set_signer(c, r, t, nt);
		if (tok_is(k, "payment")) return nt == 2 ? set1(&r->payment_id, tsym(c, &t[1])) : E_SHAPE;
		break;
	case R_ENVELOPE:
		if (tok_is(k, "baker")) return (nt == 2 && t[1].p[0] == '@') ? set1(&r->baker, tsym(c, &t[1])) : E_SHAPE;
		if (tok_is(k, "work")) return set_fp(c, &r->work, t, nt);
		break;
	case R_PLACE:
		if (tok_is(k, "place")) return nt == 2 ? set1(&r->place, tsym(c, &t[1])) : E_SHAPE;
		if (tok_is(k, "entry")) {
			mentry e;
			if (nt != 3 || !tok_u64(&t[1], &e.pos) || !tok_fp(&t[2])) return E_SHAPE;
			e.fp = tsym(c, &t[2]);
			vec_push(c, r->entries, e);
			return NULL;
		}
		break;
	case R_CLOCK:
		if (tok_is(k, "clock")) return nt == 2 ? set1(&r->clock, tsym(c, &t[1])) : E_SHAPE;
		if (tok_is(k, "time")) {
			if (r->has_time) return E_TWICE;
			if (nt != 2 || !tok_u64(&t[1], &r->time)) return E_SHAPE;
			r->has_time = true;
			return NULL;
		}
		break;
	case R_CMIP:
		if (tok_is(k, "stub")) return nt == 2 ? set1(&r->stub, tsym(c, &t[1])) : E_SHAPE;
		if (tok_is(k, "note")) return NULL;
		break;
	case R_WORDS:
		if (tok_is(k, "text")) return NULL;
		if (tok_is(k, "line")) {
			mwline w;
			memset(&w, 0, sizeof w);
			for (int i = 1; i < nt; i++) vec_push(c, w.t, tsym(c, &t[i]));
			vec_push(c, r->wlines, w);
			return NULL;
		}
		break;
	default:
		break;
	}
	if (tok_is(k, "amount") && (r->type == R_PAYMENT || r->type == R_SETTLEMENT)) {
		if (r->has_amount) return E_TWICE;
		if (nt != 3 || !tok_u64(&t[1], &r->amount)) return E_SHAPE;
		r->unit = tsym(c, &t[2]);
		r->has_amount = true;
		return NULL;
	}
	return "a field this stand-in record does not have";
}

/* required fields (section 3): an act that lacks one is not an Agreements act */
static const char *required(const mrec *r)
{
	switch (r->type) {
	case R_TERMS:
		if (!r->signer) return "Terms without a signer";
		if (!r->has_previous) return "Terms without `previous` (section 3.1: required)";
		if ((r->replaces != NULL) != (r->agreement != NULL))
			return "`agreement` must be present exactly when `replaces` is (section 3.1)";
		if (r->continues && r->resolves.n == 0)
			return "`continues` without `resolves` (section 3.1: exactly when `resolves` is present)";
		if (r->resolves.n == 0) {
			if (r->parties.n == 0) return "Terms without `parties` (section 3.1: required)";
			if (r->authors.n == 0) return "Terms without `authors` (section 3.1: required)";
			if (!r->body) return "Terms without `body` (section 3.1: required)";
		}
		return NULL;
	case R_STATEMENT:
		if (!r->signer) return "Statement without a signer";
		if (!r->has_previous) return "Statement without `previous` (section 3.2: required)";
		if (!r->agreement) return "Statement without `agreement` (section 3.2: required)";
		if (!r->terms) return "Statement without `terms` (section 3.2: required)";
		if (!r->kind) return "Statement without `kind` (section 3.2: required)";
		if ((r->for_ != NULL) != (r->under != NULL))
			return "`for` and `under` must come together (section 3.2)";
		if (r->continues && r->resolves.n == 0) return "`continues` without `resolves` (section 3.2)";
		return NULL;
	case R_REPORT:
		if (!r->signer) return "report without a signer";
		if (!r->call) return "a report that names no call is no report (report block R2)";
		return NULL;
	case R_PAYMENT:
		if (!r->signer || !r->to || !r->settlement || !r->has_amount) return "payment lacks a field of the stand-in";
		return NULL;
	case R_SETTLEMENT:
		if (!r->signer || !r->payment_id) return "settlement lacks a field of the stand-in";
		return NULL;
	case R_ENVELOPE:
		if (!r->baker || !r->work) return "envelope lacks a field of the stand-in";
		return NULL;
	case R_PLACE:
		if (!r->place) return "place answer lacks its place";
		return NULL;
	case R_CLOCK:
		if (!r->clock || !r->has_time) return "clock answer lacks a field of the stand-in";
		return NULL;
	case R_CMIP:
		if (!r->stub) return "cMIP document lacks its stub line";
		return NULL;
	default:
		return NULL;
	}
}

static void set_invalid(mrec *r, const char *rule, const char *whytext)
{
	if (r->invalid_rule) return;
	r->invalid_rule = rule;
	r->invalid_why = whytext;
}

static bool sock_bad(const msock *s) { return s->present && !s->is_fp; }

static bool is_kind_word(sym k)
{
	static const char *const kinds[] = { "done", "received", "holding", "fits", "future" };
	for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++)
		if (strcmp(k, kinds[i]) == 0) return true;
	return false;
}

/* the cases of "invalid" that an act shows by itself (section 2) */
static void intrinsic(mor_ctx *c, mrec *r)
{
	if (r->type != R_TERMS) return;
	bool has_role = false;
	if (r->body && !is_fp(r->body)) set_invalid(r, "G2.1", "`body` is not named by fingerprint");
	if (sock_bad(&r->body_cmip) || sock_bad(&r->in_force) || sock_bad(&r->condition) || sock_bad(&r->control) ||
	    sock_bad(&r->data))
		set_invalid(r, "G2.1", "a cMIP in a socket is not named by fingerprint");
	for (size_t i = 0; i < r->powers.n; i++)
		if (sock_bad(&r->powers.v[i].rule)) set_invalid(r, "G2.1", "the cMIP of a power is not named by fingerprint");
	for (size_t i = 0; i < r->parties.n; i++) {
		const mparty *p = &r->parties.v[i];
		if (p->how != PARTY_ROLE) continue;
		has_role = true;
		if (!p->kind || !is_kind_word(p->kind))
			set_invalid(r, "G10.1", "a role lacks its kind (done, received, holding, fits or future)");
		else if (!p->spot.present)
			set_invalid(r, "G10.1", "a role lacks its cMIP (task K5)");
		else if (!p->spot.is_fp)
			set_invalid(r, "G2.1", "the cMIP of a role is not named by fingerprint");
		else if (strcmp(p->kind, "future") != 0 && (!p->what || !is_fp(p->what)))
			set_invalid(r, "G10.1", "a role lacks its `what`, by fingerprint");
	}
	for (size_t i = 0; i < r->authors.n; i++) {
		sym a = r->authors.v[i];
		bool ok = false;
		for (size_t j = 0; j < r->parties.n && !ok; j++) {
			const mparty *p = &r->parties.v[j];
			if (p->how == PARTY_SIGNER && p->id == a) ok = true;
			if (p->how == PARTY_ROLE && strncmp(a, "role:", 5) == 0 && strcmp(a + 5, p->id) == 0) ok = true;
		}
		if (!ok) set_invalid(r, "G3.1", "`authors` names someone who is not a party named as a signer or a role");
	}
	if (has_role && !r->in_force.present)
		set_invalid(r, "G4.2", "Terms name a party by role and carry no `in_force`");
	(void)c;
}

static void reject(mor_ctx *c, sym subject, const char *reason)
{
	mor_finding f;
	if (c->rejects.n >= MOR_MAX_REJECTS) return;
	memset(&f, 0, sizeof f);
	f.about = MOR_ABOUT_ACT;
	f.subject = subject;
	f.object = "";
	f.answer = MOR_REJECTED;
	f.anchoring = "";
	f.rule = "section 2";
	f.text = reason;
	vec_push(c, c->rejects, f);
}

static const char *const type_names[R_NTYPES] = { "terms", "statement", "report", "payment", "settlement",
	                                          "envelope", "place", "clock", "cmip", "words" };

int decode_all(mor_ctx *c, const unsigned char *bytes, size_t n)
{
	tok *t = ar_alloc(c, MAX_TOK * sizeof *t);
	size_t pos = 0, rec_start = 0, sig_off = 0;
	bool in_rec = false, junk = false, last_was_sig = false;
	const char *bad = NULL;
	mrec *r = NULL;
	sym sig = NULL;
	int taken = 0;

	while (pos < n) {
		const unsigned char *nl = memchr(bytes + pos, '\n', n - pos);
		size_t len = nl ? (size_t)(nl - (bytes + pos)) : n - pos;
		const unsigned char *line = bytes + pos;
		size_t line_off = pos;
		pos += len + (nl ? 1 : 0);

		if (!in_rec) {
			if (len == 0 || line[0] == '#') continue;
			int nt = (nl && len <= MOR_MAX_LINE) ? split(line, len, t) : -1;
			if (nt == 3 && tok_is(&t[0], "mor-act") && tok_is(&t[1], "1")) {
				in_rec = true;
				junk = false;
				rec_start = line_off;
				bad = NULL;
				sig = NULL;
				last_was_sig = false;
				r = ar_alloc(c, sizeof *r);
				r->type = -1;
				for (int i = 0; i < R_NTYPES; i++)
					if (tok_is(&t[2], type_names[i])) r->type = i;
				if (r->type < 0) bad = "a record type this verifier does not know";
			} else if (!junk) {
				junk = true;
				reject(c, ar_printf(c, "bytes@%zu", line_off), "bytes that cannot be read as an act");
			}
			continue;
		}
		if (!nl) break; /* truncated: handled below */
		if (len == 3 && memcmp(line, "end", 3) == 0) {
			char fp[MOR_FP_LEN + 1];
			in_rec = false;
			if (c->opt.fingerprint(bytes + rec_start, pos - rec_start, fp) != 0) mor_fail(c);
			sym fps = intern(c, fp, MOR_FP_LEN);
			bool act = r->type != R_CMIP && r->type != R_WORDS && r->type != R_ENVELOPE;
			if (!bad) bad = required(r);
			if (!bad && act) {
				sym who = r->signer ? r->signer : (r->type == R_PLACE ? r->place : r->clock);
				if (!sig || !last_was_sig) bad = "an act without its signature as the last line";
				else if (!c->opt.sig_check(who, bytes + rec_start, sig_off - rec_start, sig))
					bad = "the signature does not check";
			}
			if (!bad && !act && sig) bad = "a document carries no signature";
			if (bad) {
				reject(c, fps, bad);
				continue;
			}
			if (SYM_ENT(fps)->rec) continue; /* the same record twice */
			if (c->recs.n >= MOR_MAX_RECORDS) {
				reject(c, fps, "more records than this verifier takes");
				continue;
			}
			r->fp = fps;
			r->index = c->recs.n;
			if (r->type == R_TERMS) r->root = r->replaces ? r->agreement : r->fp;
			else if (r->type == R_STATEMENT) r->root = r->agreement;
			intrinsic(c, r);
			SYM_ENT(fps)->rec = r;
			vec_push(c, c->recs, r);
			taken++;
			continue;
		}
		if (bad) continue; /* skip to `end` */
		int nt = split(line, len, t);
		if (nt < 1) {
			bad = "a line that is not printable tokens one space apart";
			continue;
		}
		if (last_was_sig) {
			bad = "the signature is not the last line";
			continue;
		}
		if (tok_is(&t[0], "sig")) {
			if (nt != 2 || !tok_fp(&t[1])) {
				bad = E_SHAPE;
				continue;
			}
			sig = tsym(c, &t[1]);
			sig_off = line_off;
			last_was_sig = true;
			continue;
		}
		switch (r->type) {
		case R_TERMS: bad = line_terms(c, r, t, nt); break;
		case R_STATEMENT: bad = line_statement(c, r, t, nt); break;
		case R_REPORT: bad = line_report(c, r, t, nt); break;
		default: bad = line_other(c, r, t, nt); break;
		}
	}
	if (in_rec) reject(c, ar_printf(c, "bytes@%zu", rec_start), "a record that is cut off before its `end`");
	return taken;
}
