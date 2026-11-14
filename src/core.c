/* core.c: the arena, interning, findings, and the public entry points. */
#include "internal.h"
#include <stdarg.h>
#include <stdlib.h>

#define CHUNK_SIZE ((size_t)1 << 16)

struct chunk {
	struct chunk *next;
	max_align_t align_;
	unsigned char data[];
};

void mor_fail(mor_ctx *c)
{
	c->oom = true;
	longjmp(c->jb, 1);
}

void *ar_alloc(mor_ctx *c, size_t n)
{
	if (n > MOR_MAX_ARENA) mor_fail(c);
	n = (n + 15u) & ~(size_t)15u;
	if (n == 0) n = 16;
	if (n > c->left) {
		size_t sz = n > CHUNK_SIZE ? n : CHUNK_SIZE;
		if (c->total + sz > MOR_MAX_ARENA) mor_fail(c);
		struct chunk *k = malloc(sizeof *k + sz);
		if (!k) mor_fail(c);
		k->next = c->chunks;
		c->chunks = k;
		c->cur = k->data;
		c->left = sz;
		c->total += sz;
	}
	void *p = c->cur;
	c->cur += n;
	c->left -= n;
	memset(p, 0, n);
	return p;
}

void vec_grow(mor_ctx *c, void *pv, size_t *cap, size_t n, size_t es)
{
	if (n < *cap) return;
	size_t ncap = *cap ? *cap * 2 : 4;
	if (ncap > MOR_MAX_ARENA / es) mor_fail(c);
	void *old, *nw = ar_alloc(c, ncap * es);
	memcpy(&old, pv, sizeof old);
	if (n) memcpy(nw, old, n * es);
	memcpy(pv, &nw, sizeof nw);
	*cap = ncap;
}

static uint32_t fnv(const char *s, size_t n)
{
	uint32_t h = 2166136261u;
	for (size_t i = 0; i < n; i++) h = (h ^ (unsigned char)s[i]) * 16777619u;
	return h;
}

static void itab_grow(mor_ctx *c)
{
	size_t ncap = c->icap ? c->icap * 2 : 1024;
	struct ient **nt = ar_alloc(c, ncap * sizeof *nt);
	for (size_t i = 0; i < c->icap; i++) {
		struct ient *e = c->itab[i];
		if (!e) continue;
		size_t j = e->hash & (ncap - 1);
		while (nt[j]) j = (j + 1) & (ncap - 1);
		nt[j] = e;
	}
	c->itab = nt;
	c->icap = ncap;
}

static struct ient *ilookup(const mor_ctx *c, const char *s, size_t n, uint32_t h)
{
	if (!c->icap) return NULL;
	size_t j = h & (c->icap - 1);
	for (struct ient *e; (e = c->itab[j]) != NULL; j = (j + 1) & (c->icap - 1))
		if (e->hash == h && strlen(e->s) == n && memcmp(e->s, s, n) == 0) return e;
	return NULL;
}

sym intern(mor_ctx *c, const char *s, size_t n)
{
	uint32_t h = fnv(s, n);
	struct ient *e = ilookup(c, s, n, h);
	if (e) return e->s;
	if ((c->icount + 1) * 2 > c->icap) itab_grow(c);
	e = ar_alloc(c, sizeof *e + n + 1);
	e->hash = h;
	memcpy(e->s, s, n);
	e->s[n] = '\0';
	size_t j = h & (c->icap - 1);
	while (c->itab[j]) j = (j + 1) & (c->icap - 1);
	c->itab[j] = e;
	c->icount++;
	return e->s;
}

sym intern0(mor_ctx *c, const char *s) { return intern(c, s, strlen(s)); }

sym lookup_sym(const mor_ctx *c, const char *s)
{
	if (!s) return NULL;
	size_t n = strlen(s);
	struct ient *e = ilookup(c, s, n, fnv(s, n));
	return e ? e->s : NULL;
}

bool is_fp(sym s)
{
	if (!s) return false;
	size_t i = 0;
	for (; s[i]; i++) {
		if (i >= MOR_FP_LEN) return false;
		if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return false;
	}
	return i == MOR_FP_LEN;
}

bool sym_in(const symvec *v, sym s)
{
	for (size_t i = 0; i < v->n; i++)
		if (v->v[i] == s) return true;
	return false;
}

static char *ar_vprintf(mor_ctx *c, const char *fmt, va_list ap)
{
	va_list ap2;
	va_copy(ap2, ap);
	int n = vsnprintf(NULL, 0, fmt, ap2);
	va_end(ap2);
	if (n < 0) n = 0;
	char *p = ar_alloc(c, (size_t)n + 1);
	vsnprintf(p, (size_t)n + 1, fmt, ap);
	return p;
}

char *ar_printf(mor_ctx *c, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char *p = ar_vprintf(c, fmt, ap);
	va_end(ap);
	return p;
}

void why(mor_ctx *c, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(c->why, sizeof c->why, fmt, ap);
	va_end(ap);
}

const mor_finding *emit(mor_ctx *c, mor_about about, sym subj, sym obj, mor_answer a, mqual q,
                        const char *rule, const char *fmt, ...)
{
	if (c->finds.n >= MOR_MAX_FINDINGS) mor_fail(c);
	va_list ap;
	va_start(ap, fmt);
	char *text = ar_vprintf(c, fmt, ap);
	va_end(ap);
	mor_finding f;
	f.about = about;
	f.subject = subj ? subj : "";
	f.object = obj ? obj : "";
	f.answer = a;
	f.quals = q.bits;
	f.anchoring = q.anchor ? q.anchor : "";
	f.rule = rule ? rule : "";
	f.text = text;
	vec_push(c, c->finds, f);
	return &c->finds.v[c->finds.n - 1];
}

static int init_guarded(mor_ctx *c)
{
	if (setjmp(c->jb)) return -1;
	c->s_assent = intern0(c, "assent");
	c->s_counter = intern0(c, "counter_signature");
	c->s_contest = intern0(c, "contest");
	c->s_resolves = intern0(c, "resolves");
	c->s_replaces = intern0(c, "replaces");
	c->s_none = intern0(c, "none");
	c->s_control = intern0(c, "control");
	c->s_doer = intern0(c, "doer");
	c->s_receiving = intern0(c, "receiving");
	c->s_both = intern0(c, "both");
	c->reader = c->opt.reader ? intern0(c, c->opt.reader) : NULL;
	c->anch_place = c->opt.anchoring_place ? intern0(c, c->opt.anchoring_place) : NULL;
	c->anch_clock = c->opt.anchoring_clock ? intern0(c, c->opt.anchoring_clock) : NULL;
	/* the option strings are the caller's: keep only the interned copies */
	c->opt.reader = c->reader;
	c->opt.anchoring_place = c->anch_place;
	c->opt.anchoring_clock = c->anch_clock;
	return 0;
}

mor_ctx *mor_new(const mor_options *opt)
{
	mor_ctx *c = calloc(1, sizeof *c);
	if (!c) return NULL;
	if (opt) c->opt = *opt;
	if (!c->opt.fingerprint) c->opt.fingerprint = mor_standin_fingerprint;
	if (!c->opt.sig_check) c->opt.sig_check = mor_standin_sig_check;
	if (!c->opt.max_power_depth) c->opt.max_power_depth = 8;
	if (!c->opt.max_receiver_hops) c->opt.max_receiver_hops = 8;
	if (!c->opt.max_chain) c->opt.max_chain = 1024;
	if (init_guarded(c) != 0) {
		mor_free(c);
		return NULL;
	}
	return c;
}

void mor_free(mor_ctx *c)
{
	if (!c) return;
	for (struct chunk *k = c->chunks; k;) {
		struct chunk *nx = k->next;
		free(k);
		k = nx;
	}
	free(c);
}

int mor_load(mor_ctx *c, const unsigned char *bytes, size_t n)
{
	if (!c || c->oom) return -1;
	if (setjmp(c->jb)) return -1;
	c->verified = false;
	return decode_all(c, bytes, n);
}

int mor_verify(mor_ctx *c)
{
	if (!c || c->oom) return -1;
	if (setjmp(c->jb)) return -1;
	engine_run(c);
	report_run(c);
	c->verified = true;
	return 0;
}

size_t mor_finding_count(const mor_ctx *c) { return c ? c->finds.n : 0; }

const mor_finding *mor_finding_at(const mor_ctx *c, size_t i)
{
	return (c && i < c->finds.n) ? &c->finds.v[i] : NULL;
}

const mor_finding *mor_find(const mor_ctx *c, mor_about about, const char *subject, const char *object)
{
	if (!c || !subject) return NULL;
	for (size_t i = 0; i < c->finds.n; i++) {
		const mor_finding *f = &c->finds.v[i];
		if (f->about != about || strcmp(f->subject, subject) != 0) continue;
		if (object && strcmp(f->object, object) != 0) continue;
		return f;
	}
	return NULL;
}

const char *mor_answer_name(mor_answer a)
{
	static const char *const names[] = {
		"shown", "not shown", "fork", "valid", "not valid", "invalid", "unchecked", "rejected",
		"matched", "mismatched", "one side only", "unanswered", "no report called for",
		"yes", "no", "note"
	};
	return (unsigned)a < sizeof names / sizeof names[0] ? names[a] : "?";
}

const char *mor_about_name(mor_about a)
{
	static const char *const names[] = {
		"act", "left unread", "line", "in force", "version", "replaces", "replaced by", "assent",
		"counter signature", "power", "bound", "receiver", "filling", "spot open", "rights",
		"control task", "call", "report", "tie", "payment", "contest", "baker", "same work",
		"data path", "order", "sequence", "settling"
	};
	return (unsigned)a < sizeof names / sizeof names[0] ? names[a] : "?";
}

static void print_id(const char *s, FILE *out, int full)
{
	size_t n = strlen(s);
	if (!full && n == MOR_FP_LEN) fprintf(out, "%.12s", s);
	else fputs(s, out);
}

void mor_print_finding(const mor_finding *f, FILE *out, int full)
{
	fprintf(out, "[%s] %s ", f->rule[0] ? f->rule : "-", mor_about_name(f->about));
	print_id(f->subject, out, full);
	if (f->object[0]) {
		fputs(" / ", out);
		print_id(f->object, out, full);
	}
	fprintf(out, ": %s", mor_answer_name(f->answer));
	if (f->quals & MOR_Q_BY_DEFAULT) fputs(", by default", out);
	if (f->quals & MOR_Q_PER_PLACE) fprintf(out, ", per place %s", f->anchoring);
	if (f->quals & MOR_Q_PER_CLOCK) fprintf(out, ", per clock %s", f->anchoring);
	if (f->quals & MOR_Q_PER_ANCHORING) fprintf(out, ", per anchoring %s", f->anchoring);
	if (f->quals & MOR_Q_AS_SHOWN) fputs(", as shown", out);
	if (f->quals & MOR_Q_DISPUTED) fputs(", disputed (G12.5)", out);
	if (f->quals & MOR_Q_WITH_FORK) fputs(", fork shown", out);
	if (f->quals & MOR_Q_BUILT_ON_TIP) fputs(", built on a tip", out);
	if (f->text[0]) fprintf(out, " -- %s", f->text);
	fputc('\n', out);
}

void mor_print(const mor_ctx *c, FILE *out, int full)
{
	for (size_t i = 0; i < c->finds.n; i++) mor_print_finding(&c->finds.v[i], out, full);
}
