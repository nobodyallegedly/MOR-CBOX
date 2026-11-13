/* set.c - the arena, the fingerprint and signature stand-ins, and the set of acts. */
#include "internal.h"

#include <openssl/sha.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* ---- arena ---------------------------------------------------------------- */

#define MOR_BLK_MIN (64u * 1024u)

void *mor_alloc(mor_set *s, size_t n)
{
    size_t a = _Alignof(max_align_t);
    size_t hdr = (sizeof(mor_blk) + a - 1) / a * a;
    if (n == 0)
        n = 1;
    if (n > SIZE_MAX / 2)
        abort();
    n = (n + a - 1) / a * a;
    mor_blk *b = s->blocks;
    if (!b || b->cap - b->used < n) {
        size_t cap = n > MOR_BLK_MIN ? n : MOR_BLK_MIN;
        b = calloc(1, hdr + cap);
        if (!b)
            abort();
        b->cap = cap;
        b->next = s->blocks;
        s->blocks = b;
    }
    void *p = (unsigned char *)b + hdr + b->used;
    b->used += n;
    return p; /* zeroed: blocks come from calloc and are never reused */
}

const char *mor_strdup_n(mor_set *s, const char *p, size_t n)
{
    char *d = mor_alloc(s, n + 1);
    if (n)
        memcpy(d, p, n);
    d[n] = 0;
    return d;
}

const char *mor_sprintf(mor_set *s, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0)
        n = 0;
    if ((size_t)n >= sizeof buf)
        n = (int)sizeof buf - 1;
    return mor_strdup_n(s, buf, (size_t)n);
}

mor_set *mor_set_new(const mor_options *opt)
{
    mor_set *s = calloc(1, sizeof *s);
    if (!s)
        abort();
    if (opt)
        s->opt = *opt;
    s->acts = mor_alloc(s, sizeof(mor_act *) * MOR_MAX_ACTS);
    s->by_fp = mor_alloc(s, sizeof(mor_act *) * MOR_MAX_ACTS);
    s->rejects = mor_alloc(s, sizeof(mor_reject) * MOR_MAX_ACTS);
    s->seen = mor_alloc(s, sizeof(unsigned) * MOR_MAX_ACTS);
    return s;
}

void mor_set_free(mor_set *s)
{
    if (!s)
        return;
    mor_blk *b = s->blocks;
    while (b) {
        mor_blk *n = b->next;
        free(b);
        b = n;
    }
    free(s);
}

/* ---- fingerprint stand-in: SHA-256 of the fixed bytes ------------------------ */

void mor_fp_of(const unsigned char *bytes, size_t len, mor_fp *out)
{
    SHA256(bytes, len, out->b);
}

void mor_fp_hex(const mor_fp *fp, char out[65])
{
    static const char h[] = "0123456789abcdef";
    for (int i = 0; i < MOR_FP_LEN; i++) {
        out[2 * i] = h[fp->b[i] >> 4];
        out[2 * i + 1] = h[fp->b[i] & 15];
    }
    out[64] = 0;
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

bool mor_fp_parse(const char *hex, size_t len, mor_fp *out)
{
    if (len != 64)
        return false;
    for (int i = 0; i < MOR_FP_LEN; i++) {
        int a = hexval(hex[2 * i]), b = hexval(hex[2 * i + 1]);
        if (a < 0 || b < 0)
            return false;
        out->b[i] = (unsigned char)(a * 16 + b);
    }
    return true;
}

int mor_fp_cmp(const mor_fp *a, const mor_fp *b)
{
    return memcmp(a->b, b->b, MOR_FP_LEN);
}

bool mor_fp_eq(const mor_fp *a, const mor_fp *b)
{
    return memcmp(a->b, b->b, MOR_FP_LEN) == 0;
}

/* ---- signature stand-in ---------------------------------------------------------
 * NOT a signature. A tag anyone can compute: SHA-256 over a fixed label, the
 * signer's identifier and the act's fingerprint. It stands where the Sign layer
 * will stand (STANDINS.md), holds no key, and proves nothing about who made an act.
 */
void mor_standin_sign(const char *signer, const mor_fp *act, char out[65])
{
    static const char label[] = "mor-standin-signature-1\n";
    unsigned char buf[sizeof label + 64 + MOR_FP_LEN];
    size_t n = sizeof label - 1, sl = strlen(signer);
    if (sl > 64)
        sl = 64;
    memcpy(buf, label, n);
    memcpy(buf + n, signer, sl);
    n += sl;
    buf[n++] = '\n';
    memcpy(buf + n, act->b, MOR_FP_LEN);
    n += MOR_FP_LEN;
    mor_fp t;
    mor_fp_of(buf, n, &t);
    mor_fp_hex(&t, out);
}

bool mor_standin_verify(const char *signer, const mor_fp *act, const char *sig_hex)
{
    char want[65];
    mor_standin_sign(signer, act, want);
    return strlen(sig_hex) == 64 && memcmp(want, sig_hex, 64) == 0;
}

/* ---- the set ------------------------------------------------------------------- */

static int cmp_act_fp(const void *a, const void *b)
{
    const mor_act *x = *(const mor_act *const *)a, *y = *(const mor_act *const *)b;
    return mor_fp_cmp(&x->fp, &y->fp);
}

int mor_set_load(mor_set *s, const unsigned char *bytes, size_t len)
{
    if (len > MOR_MAX_INPUT)
        return -1;
    int before = s->n_acts;
    mor_decode(s, bytes, len);
    memcpy(s->by_fp, s->acts, sizeof(mor_act *) * (size_t)s->n_acts);
    qsort(s->by_fp, (size_t)s->n_acts, sizeof(mor_act *), cmp_act_fp);
    s->evaluated = false;
    return s->n_acts - before;
}

mor_act *mor_find_mut(const mor_set *s, const mor_fp *fp)
{
    int lo = 0, hi = s->n_acts - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int c = mor_fp_cmp(&s->by_fp[mid]->fp, fp);
        if (c == 0)
            return s->by_fp[mid];
        if (c < 0)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return NULL;
}

const mor_act *mor_find(const mor_set *s, const mor_fp *fp)
{
    return mor_find_mut(s, fp);
}

int mor_n_acts(const mor_set *s)
{
    return s->n_acts;
}

const mor_act *mor_act_at(const mor_set *s, int i)
{
    return i >= 0 && i < s->n_acts ? s->acts[i] : NULL;
}

int mor_n_rejects(const mor_set *s)
{
    return s->n_rejects;
}

const mor_reject *mor_reject_at(const mor_set *s, int i)
{
    return i >= 0 && i < s->n_rejects ? &s->rejects[i] : NULL;
}

bool mor_ref_eq(const mor_ref *a, const mor_ref *b)
{
    if (a->kind != b->kind)
        return false;
    if (a->kind == MOR_REF_AGREEMENT)
        return mor_fp_eq(&a->root, &b->root);
    if (a->kind == MOR_REF_NONE)
        return true;
    return strcmp(a->name, b->name) == 0;
}

const char *mor_short(mor_set *s, const mor_fp *fp)
{
    char h[65];
    mor_fp_hex(fp, h);
    return mor_strdup_n(s, h, 12);
}

const char *mor_ref_str(mor_set *s, const mor_ref *r)
{
    switch (r->kind) {
    case MOR_REF_SIGNER:
        return mor_sprintf(s, "signer %s", r->name);
    case MOR_REF_ROLE:
        return mor_sprintf(s, "role %s", r->name);
    case MOR_REF_AGREEMENT:
        return mor_sprintf(s, "agreement %s", mor_short(s, &r->root));
    default:
        return "none";
    }
}

const char *mor_ans_name(mor_ans a)
{
    switch (a) {
    case MOR_SHOWN:
        return "shown";
    case MOR_NOT_SHOWN:
        return "not shown";
    case MOR_FORK:
        return "fork";
    case MOR_VALID:
        return "valid";
    case MOR_NOT_VALID:
        return "not valid";
    case MOR_INVALID:
        return "invalid";
    case MOR_UNCHECKED:
        return "unchecked";
    case MOR_MATCHED:
        return "matched";
    case MOR_MISMATCHED:
        return "mismatched";
    case MOR_ONE_SIDE_ONLY:
        return "one side only";
    case MOR_UNANSWERED:
        return "unanswered";
    case MOR_NO_REPORT_CALLED_FOR:
        return "no report called for";
    }
    return "?";
}

const char *mor_qual_str(const mor_qual *q, char *buf, size_t n)
{
    switch (q->kind) {
    case MOR_Q_AS_SHOWN:
        snprintf(buf, n, "as shown");
        break;
    case MOR_Q_PER_PLACE:
        snprintf(buf, n, "per place %s", q->name ? q->name : "?");
        break;
    case MOR_Q_PER_CLOCK:
        snprintf(buf, n, "per clock %s", q->name ? q->name : "?");
        break;
    case MOR_Q_PER_ANCHORING:
        snprintf(buf, n, "per anchoring %s", q->name ? q->name : "?");
        break;
    default:
        if (n)
            buf[0] = 0;
    }
    return buf;
}
