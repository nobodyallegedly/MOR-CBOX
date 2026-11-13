/* kit.h - builds acts in the stand-in format, for tests and for the grammar fuzzer. */
#ifndef MOR_KIT_H
#define MOR_KIT_H

#include "mor.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define KIT_CAP (1u << 20)

typedef struct {
    char *buf;
    size_t n;
    char rec[32768];
    size_t rn;
    char signer[64];
} kit;

static inline void kit_init(kit *k)
{
    memset(k, 0, sizeof *k);
    k->buf = malloc(KIT_CAP);
    if (!k->buf)
        abort();
}

static inline void kit_free(kit *k)
{
    free(k->buf);
    k->buf = NULL;
}

/* rotating scratch strings */
static inline char *kit_slot(void)
{
    static char slots[64][4096];
    static int at;
    at = (at + 1) % 64;
    return slots[at];
}

static inline const char *hx(mor_fp fp)
{
    char *s = kit_slot();
    mor_fp_hex(&fp, s);
    return s;
}

static inline const char *stub(const char *name)
{
    return hx(mor_stub_fp(name));
}

__attribute__((format(printf, 1, 2))) static inline const char *F(const char *fmt, ...)
{
    char *s = kit_slot();
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s, 4096, fmt, ap);
    va_end(ap);
    return s;
}

static inline void k_raw(kit *k, const char *line)
{
    size_t n = strlen(line);
    if (k->rn + n + 2 > sizeof k->rec)
        return;
    memcpy(k->rec + k->rn, line, n);
    k->rn += n;
    k->rec[k->rn++] = '\n';
}

/* lines separated by '|' */
static inline void k_lines(kit *k, const char *lines)
{
    char tmp[4096];
    while (lines && *lines) {
        const char *bar = strchr(lines, '|');
        size_t n = bar ? (size_t)(bar - lines) : strlen(lines);
        if (n >= sizeof tmp)
            n = sizeof tmp - 1;
        memcpy(tmp, lines, n);
        tmp[n] = 0;
        if (n)
            k_raw(k, tmp);
        lines = bar ? bar + 1 : NULL;
    }
}

static inline void k_open(kit *k, const char *type, const char *signer)
{
    k->rn = 0;
    snprintf(k->signer, sizeof k->signer, "%s", signer);
    k_raw(k, "mor-act 1");
    k_raw(k, F("type %s", type));
    k_raw(k, F("signer %s", signer));
}

static inline mor_fp k_close(kit *k)
{
    mor_fp fp;
    char sig[65];
    mor_fp_of((const unsigned char *)k->rec, k->rn, &fp);
    mor_standin_sign(k->signer, &fp, sig);
    k_raw(k, F("sig %s", sig));
    if (k->n + k->rn + 1 < KIT_CAP) {
        memcpy(k->buf + k->n, k->rec, k->rn);
        k->n += k->rn;
        k->buf[k->n++] = '\n';
    }
    return fp;
}

static inline mor_fp rec(kit *k, const char *type, const char *signer, const char *lines)
{
    k_open(k, type, signer);
    k_lines(k, lines);
    return k_close(k);
}

/* a body's words: data lines separated by '|' */
static inline mor_fp piece(kit *k, const char *words)
{
    char tmp[4096] = "";
    size_t at = 0;
    const char *w = words && *words ? words : "words for people";
    while (w && *w) {
        const char *bar = strchr(w, '|');
        size_t n = bar ? (size_t)(bar - w) : strlen(w);
        at += (size_t)snprintf(tmp + at, sizeof tmp - at, "%sdata %.*s", at ? "|" : "", (int)n, w);
        if (at >= sizeof tmp)
            break;
        w = bar ? bar + 1 : NULL;
    }
    return rec(k, "piece", "nobody", tmp);
}

/* Terms: `lines` are field lines; the body (words read by stub k1.lines) is added unless lines carry one */
static inline mor_fp terms(kit *k, const char *signer, const char *previous, const char *lines, const char *words)
{
    bool own_body = lines && (strstr(lines, "|body ") || strstr(lines, "|resolves ") || !strncmp(lines, "body ", 5) || !strncmp(lines, "resolves ", 9));
    mor_fp w = {{0}};
    if (!own_body)
        w = piece(k, words);
    k_open(k, "terms", signer);
    k_raw(k, F("previous %s", previous));
    k_lines(k, lines);
    if (!own_body)
        k_raw(k, F("body %s %s", hx(w), stub("k1.lines")));
    return k_close(k);
}

static inline mor_fp stmt(kit *k, const char *signer, mor_fp root, mor_fp t, const char *kind, const char *previous, const char *lines)
{
    k_open(k, "statement", signer);
    k_raw(k, F("agreement %s", hx(root)));
    k_raw(k, F("terms %s", hx(t)));
    k_raw(k, F("kind %s", kind));
    k_raw(k, F("previous %s", previous));
    k_lines(k, lines);
    return k_close(k);
}

static inline mor_fp assent(kit *k, const char *signer, mor_fp root, mor_fp t, const char *previous)
{
    return stmt(k, signer, root, t, "assent", previous, NULL);
}

#endif
