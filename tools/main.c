/* mor-agreements - command-line tool over libmoragreements. */
#include "mor.h"

#include <stdlib.h>
#include <string.h>

static void usage(void)
{
    fputs("usage: mor-agreements <command> [options] [file...]\n"
          "commands:\n"
          "  verify         every answer the grammar gives for the acts in the files\n"
          "  trace          --payment FP   a result read leg by leg (G11.4)\n"
          "  before-paying  --root FP      which agreements of a sequence carry a call (G11.4(e))\n"
          "  order          --a FP --b FP  order from citing (G13.3)\n"
          "  bound          --signer ID --act FP   is the signer bound by the act (G5)\n"
          "  standing       --signer ID    what one signer's acts show (K13 stub)\n"
          "  seal           read records ending in a bare 'sig' line on stdin; write them sealed\n"
          "  stubs          list the stub cMIPs and their fingerprints\n"
          "options:\n"
          "  --reader ID            read as this signer (a side of its own legs, G11.4(d))\n"
          "  --anchoring-place ID   an anchoring the reader accepts where Terms name no place (G13.4)\n"
          "  --anchoring-clock ID   likewise for time\n"
          "  --without NAME         the verifier does not hold this stub cMIP (may repeat)\n"
          "  --no-plugins           the verifier holds no cMIP at all\n"
          "FP may be a unique prefix of at least 8 hex digits.\n",
          stderr);
}

static unsigned char *slurp(FILE *f, size_t *len)
{
    size_t cap = 1 << 16, n = 0;
    unsigned char *b = malloc(cap);
    if (!b)
        return NULL;
    for (;;) {
        size_t r = fread(b + n, 1, cap - n, f);
        n += r;
        if (r == 0)
            break;
        if (n == cap) {
            if (cap > MOR_MAX_INPUT) {
                free(b);
                return NULL;
            }
            unsigned char *nb = realloc(b, cap * 2);
            if (!nb) {
                free(b);
                return NULL;
            }
            b = nb;
            cap *= 2;
        }
    }
    *len = n;
    return b;
}

static const mor_act *by_prefix(const mor_set *s, const char *hex)
{
    size_t n = hex ? strlen(hex) : 0;
    const mor_act *found = NULL;
    if (n < 8 || n > 64)
        return NULL;
    for (int i = 0; i < mor_n_acts(s); i++) {
        char h[65];
        mor_fp_hex(&mor_act_at(s, i)->fp, h);
        if (memcmp(h, hex, n) == 0) {
            if (found)
                return NULL;
            found = mor_act_at(s, i);
        }
    }
    return found;
}

static int seal(void)
{
    size_t len;
    unsigned char *b = slurp(stdin, &len);
    if (!b)
        return 1;
    size_t i = 0, start = 0;
    bool in = false;
    char signer[64] = "";
    while (i < len) {
        size_t j = i;
        while (j < len && b[j] != '\n')
            j++;
        size_t n = j - i;
        if (!in && n == 9 && memcmp(b + i, "mor-act 1", 9) == 0) {
            in = true;
            start = i;
            signer[0] = 0;
        } else if (in && n > 7 && n < 7 + sizeof signer && memcmp(b + i, "signer ", 7) == 0) {
            memcpy(signer, b + i + 7, n - 7);
            signer[n - 7] = 0;
        } else if (in && n == 3 && memcmp(b + i, "sig", 3) == 0) {
            mor_fp fp;
            char hex[65], sig[65];
            mor_fp_of(b + start, i - start, &fp);
            mor_fp_hex(&fp, hex);
            mor_standin_sign(signer, &fp, sig);
            fwrite(b + start, 1, i - start, stdout);
            printf("sig %s\n# fingerprint %s\n\n", sig, hex);
            in = false;
        }
        i = j + 1;
    }
    free(b);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 2;
    }
    const char *cmd = argv[1];
    if (strcmp(cmd, "seal") == 0)
        return seal();
    int nstub = mor_stub_count();
    mor_cmip *stubs = calloc((size_t)nstub, sizeof *stubs);
    if (!stubs)
        return 1;
    mor_stub_fill(stubs);
    if (strcmp(cmd, "stubs") == 0) {
        for (int i = 0; i < nstub; i++) {
            char h[65];
            mor_fp_hex(&stubs[i].fp, h);
            printf("%s  K%-2d %s\n", h, stubs[i].task, stubs[i].name);
        }
        free(stubs);
        return 0;
    }
    mor_plugins plug = {stubs, nstub};
    mor_options opt;
    memset(&opt, 0, sizeof opt);
    opt.plugins = &plug;
    const char *payment = NULL, *root = NULL, *fa = NULL, *fb = NULL, *signer = NULL, *act = NULL;
    const char *files[64];
    int nfiles = 0, rc = 0;
    for (int i = 2; i < argc; i++) {
        const char *a = argv[i];
        const char **dst = NULL;
        if (strcmp(a, "--no-plugins") == 0) {
            opt.plugins = NULL;
            continue;
        }
        if (strcmp(a, "--reader") == 0)
            dst = &opt.reader;
        else if (strcmp(a, "--anchoring-place") == 0)
            dst = &opt.anchoring_place;
        else if (strcmp(a, "--anchoring-clock") == 0)
            dst = &opt.anchoring_clock;
        else if (strcmp(a, "--payment") == 0)
            dst = &payment;
        else if (strcmp(a, "--root") == 0)
            dst = &root;
        else if (strcmp(a, "--a") == 0)
            dst = &fa;
        else if (strcmp(a, "--b") == 0)
            dst = &fb;
        else if (strcmp(a, "--signer") == 0)
            dst = &signer;
        else if (strcmp(a, "--act") == 0)
            dst = &act;
        if (dst || strcmp(a, "--without") == 0) {
            if (i + 1 >= argc) {
                usage();
                free(stubs);
                return 2;
            }
            if (dst) {
                *dst = argv[++i];
            } else {
                i++;
                for (int k = 0; k < nstub; k++)
                    if (strcmp(stubs[k].name, argv[i]) == 0)
                        stubs[k].absent = true;
            }
            continue;
        }
        if (a[0] == '-' && a[1] == '-') {
            usage();
            free(stubs);
            return 2;
        }
        if (nfiles < 64)
            files[nfiles++] = a;
    }
    mor_set *s = mor_set_new(&opt);
    for (int i = 0; i < nfiles && rc == 0; i++) {
        FILE *f = strcmp(files[i], "-") == 0 ? stdin : fopen(files[i], "rb");
        size_t len = 0;
        unsigned char *b = f ? slurp(f, &len) : NULL;
        if (!b || mor_set_load(s, b, len) < 0) {
            fprintf(stderr, "mor-agreements: cannot read %s\n", files[i]);
            rc = 1;
        }
        free(b);
        if (f && f != stdin)
            fclose(f);
    }
    if (rc == 0) {
        mor_evaluate(s);
        if (strcmp(cmd, "verify") == 0) {
            mor_show_all(s, stdout);
        } else if (strcmp(cmd, "trace") == 0) {
            const mor_act *p = by_prefix(s, payment);
            if (p && p->type == MOR_T_PAYMENT)
                mor_show_trace(s, p, stdout);
            else
                rc = 2;
        } else if (strcmp(cmd, "before-paying") == 0) {
            const mor_act *r = by_prefix(s, root);
            if (r)
                mor_show_before(s, &r->fp, stdout);
            else
                rc = 2;
        } else if (strcmp(cmd, "order") == 0) {
            const mor_act *a = by_prefix(s, fa), *b = by_prefix(s, fb);
            if (a && b) {
                mor_order o = mor_order_by_citing(s, a, b);
                puts(o == MOR_ORDER_A_FIRST   ? "shown: a came before b (G13.3)"
                     : o == MOR_ORDER_B_FIRST ? "shown: b came before a (G13.3)"
                                              : "no order is shown from the acts alone (G13.3)");
            } else
                rc = 2;
        } else if (strcmp(cmd, "bound") == 0) {
            const mor_act *a = by_prefix(s, act);
            if (a && signer) {
                mor_answer ans;
                char q[96];
                mor_bound_by_act(s, signer, a, &ans);
                printf("%s [%s]%s - %s\n", mor_ans_name(ans.ans), mor_qual_str(&ans.qual, q, sizeof q), ans.disputed ? " [disputed]" : "",
                       ans.why ? ans.why : "");
            } else
                rc = 2;
        } else if (strcmp(cmd, "standing") == 0 && signer) {
            mor_k13_out k;
            if (mor_standing(s, signer, &k) == MOR_YES)
                printf("%s [as shown]: forks standing %d, forks resolved %d, contests made %d, assents %d\n", signer, k.forks_standing,
                       k.forks_resolved, k.contests_made, k.assents);
            else
                puts("unchecked: no K13 cMIP is held (G9.1)");
        } else {
            rc = 2;
        }
        if (rc == 2)
            usage();
    }
    mor_set_free(s);
    free(stubs);
    return rc;
}
