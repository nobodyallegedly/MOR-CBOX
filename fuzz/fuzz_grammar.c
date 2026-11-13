/* fuzz_grammar.c - libFuzzer harness for the grammar. The input is a little program that builds
 * well-formed, sealed acts which name one another by fingerprint (something a byte-level fuzzer
 * cannot do, since a fingerprint is a hash), then asks the verifier everything. */
#include "kit.h"

#include <stdio.h>

static FILE *sink;
static const uint8_t *in;
static size_t left;

static unsigned u8(void)
{
    if (!left)
        return 0;
    left--;
    return *in++;
}

#define MAXREF 48
static mor_fp ref[MAXREF], root[MAXREF];
static int nref;

static void push(mor_fp fp, mor_fp r)
{
    if (nref < MAXREF) {
        ref[nref] = fp;
        root[nref] = r;
        nref++;
    }
}

static int pick(void)
{
    return nref ? (int)(u8() % (unsigned)nref) : -1;
}

static const char *const who[] = {"alice", "bob", "carol", "sam", "judy", "zed"};
#define WHO() who[u8() % 6]

static const char *prev(void)
{
    int i = pick();
    return (i < 0 || (u8() & 1)) ? "none" : hx(ref[i]);
}

static const char *rule(void)
{
    static const char *const r[] = {"k4.yes", "k4.no", "k4.threshold", "k4.delegate", "k4.before", "k4.same-parties", "no.such.cmip"};
    unsigned k = u8() % 7;
    char *s = kit_slot();
    if (k == 2)
        snprintf(s, 4096, "%s %u signer %s signer %s", stub(r[k]), u8() % 3, WHO(), WHO());
    else if (k == 3)
        snprintf(s, 4096, "%s %s %s", stub(r[k]), WHO(), WHO());
    else if (k == 4)
        snprintf(s, 4096, "%s %u", stub(r[k]), u8());
    else
        snprintf(s, 4096, "%s", stub(r[k]));
    return s;
}

static void terms_fields(char *out, size_t cap, int replaces)
{
    size_t at = 0;
    unsigned m = u8(), o = u8(), p = u8();
#define ADD(...) at += (size_t)snprintf(out + at, at < cap ? cap - at : 0, __VA_ARGS__), at = at < cap ? at : cap - 1
    if (replaces >= 0)
        ADD("replaces %s|agreement %s|", hx(ref[replaces]), hx(root[replaces]));
    for (int i = 0; i < 3; i++)
        if (m & (1u << i) || i == 0)
            ADD("party signer %s|", who[i]);
    if (m & 8)
        ADD("party role buyer %s %s %s %s|", (m & 16) ? "future" : "received", (m & 16) ? "-" : stub("thing"), stub("k12.claim"),
            (m & 32) ? "one max=1" : "many until=9");
    if ((m & 64) && nref)
        ADD("party agreement %s|", hx(root[pick()]));
    ADD("author signer %s|", (m & 128) ? "zed" : "alice");
    if (o & 1)
        ADD("in_force %s|", rule());
    if (o & 2)
        ADD("power %s %s|", (o & 4) ? "replaces" : (o & 8) ? "resolves" : "assent", rule());
    if (o & 16)
        ADD("condition %s|", rule());
    if (o & 32)
        ADD("place plc1|");
    if (o & 64)
        ADD("clock ck1|");
    if (o & 128)
        ADD("call K8 %s|", (p & 1) ? "both" : "doer");
    if (p & 2)
        ADD("receiver signer %s|", who[(p >> 2) % 3]);
    else if ((p & 4) && nref)
        ADD("receiver agreement %s|", hx(root[pick()]));
    else if (p & 8)
        ADD("receiver role buyer|");
    if ((p & 16) && nref)
        ADD("envelope %s|", hx(ref[pick()]));
    if ((p & 32) && nref)
        ADD("relies_on %s|", hx(ref[pick()]));
#undef ADD
    out[at] = 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static kit K;
    static mor_cmip stubs[32];
    char lines[4096];
    if (!sink) {
        sink = fopen("/dev/null", "w");
        kit_init(&K);
    }
    if (!sink)
        abort();
    in = data;
    left = size;
    K.n = 0;
    nref = 0;
    unsigned absent = u8() | (u8() << 8), mode = u8();
    for (int step = 0; step < 40 && left > 0; step++) {
        unsigned op = u8() % 14;
        int a = pick(), b = pick(), c = pick();
        mor_fp fp;
        switch (op) {
        case 0:
            terms_fields(lines, sizeof lines, -1);
            fp = terms(&K, WHO(), prev(), lines, (u8() & 1) ? "share signer alice 5|share signer bob 5|delegation signer alice signer bob default" : NULL);
            push(fp, fp);
            break;
        case 1:
            if (a < 0)
                break;
            terms_fields(lines, sizeof lines, a);
            fp = terms(&K, WHO(), prev(), lines, (u8() & 1) ? "share signer bob 1|delegation signer alice signer bob agent" : NULL);
            push(fp, root[a]);
            break;
        case 2:
        case 3:
        case 4:
            if (a < 0)
                break;
            lines[0] = 0;
            if ((u8() & 3) == 0 && b >= 0)
                snprintf(lines, sizeof lines, "for %s|under %s", (u8() & 1) ? "signer alice" : F("agreement %s", hx(root[b])), hx(ref[b]));
            else if ((u8() & 3) == 0 && b >= 0)
                snprintf(lines, sizeof lines, "settlement %s|relies_on %s", hx(ref[b]), hx(ref[c]));
            fp = stmt(&K, WHO(), root[a], ref[a], op == 2 ? "assent" : op == 3 ? "counter_signature" : "contest", prev(), lines);
            push(fp, root[a]);
            break;
        case 5:
            if (a < 0)
                break;
            snprintf(lines, sizeof lines, "replaces %s|agreement %s|resolves %s|resolves %s|continues %s", hx(ref[a]), hx(root[a]), hx(ref[b]),
                     hx(ref[c]), hx(ref[(u8() & 1) ? b : c]));
            fp = terms(&K, WHO(), prev(), lines, NULL);
            push(fp, root[a]);
            break;
        case 6:
            if (a < 0)
                break;
            snprintf(lines, sizeof lines, "resolves %s|resolves %s|continues %s%s", hx(ref[b]), hx(ref[c]), hx(ref[b]),
                     (u8() & 1) ? F("|for signer alice|under %s", hx(ref[a])) : "");
            fp = stmt(&K, WHO(), root[a], ref[a], "contest", prev(), lines);
            push(fp, root[a]);
            break;
        case 7:
            if (a < 0)
                break;
            fp = stmt(&K, WHO(), root[a], ref[a], stub((u8() & 1) ? "k11.claim" : "k11.pass-rights"), prev(), (u8() & 1) ? "content buyer" : "content carol");
            push(fp, root[a]);
            break;
        case 8:
            if (a < 0)
                break;
            fp = rec(&K, "payment", WHO(),
                     F("agreement %s|%ssettlement %s|amount %u EUR", hx(root[a]), (u8() & 1) ? F("version %s|", hx(ref[a])) : "", hx(ref[b]), u8() % 3));
            push(fp, root[a]);
            break;
        case 9:
            fp = rec(&K, "settlement", "bank", F("amount %u EUR", u8()));
            push(fp, fp);
            break;
        case 10:
            if (a < 0)
                break;
            fp = rec(&K, "report", WHO(),
                     F("call %s|side %s|relied %s|received %u EUR %s|passed %u EUR %s %s|own 1 EUR|kept 1 EUR|owed 2 EUR signer bob", hx(ref[a]),
                       (u8() & 1) ? "doer" : "receiving", hx(ref[b]), u8() % 3, hx(ref[b]), u8() % 3,
                       (u8() & 1) ? "signer bob" : F("agreement %s", hx(root[c])), hx(ref[c])));
            push(fp, fp);
            break;
        case 11:
            fp = rec(&K, "envelope", "sealer", F("baker %s", WHO()));
            push(fp, fp);
            break;
        case 12: {
            size_t at = 0;
            unsigned long long mask = 0;
            for (int i = 0; i < 6; i++)
                mask = (mask << 8) | u8();
            lines[0] = 0;
            for (int i = 0; i < nref && at + 80 < sizeof lines; i++)
                if (mask & (1ull << i))
                    at += (size_t)snprintf(lines + at, sizeof lines - at, "%sentry %s", at ? "|" : "", hx(ref[i]));
            rec(&K, "place", (u8() & 7) ? "plc1" : "mine", lines);
            break;
        }
        default:
            rec(&K, "clock", "ck1", F("now %u", u8()));
        }
    }
    mor_plugins plug = {stubs, mor_stub_count()};
    mor_stub_fill(stubs);
    for (int i = 0; i < plug.n; i++)
        stubs[i].absent = (absent >> i) & 1u;
    mor_options opt;
    memset(&opt, 0, sizeof opt);
    opt.plugins = (mode & 1) ? NULL : &plug;
    opt.reader = (mode & 2) ? who[(mode >> 4) % 6] : NULL;
    opt.anchoring_place = (mode & 4) ? "mine" : NULL;
    opt.anchoring_clock = (mode & 8) ? "ck1" : NULL;
    mor_set *s = mor_set_new(&opt);
    mor_set_load(s, (const unsigned char *)K.buf, K.n);
    mor_evaluate(s);
    mor_show_all(s, sink);
    for (int i = 0; i < mor_n_acts(s); i++) {
        const mor_act *x = mor_act_at(s, i);
        mor_answer ans;
        mor_k13_out k13;
        mor_show_before(s, &x->fp, sink);
        mor_bound_by_act(s, who[i % 6], x, &ans);
        mor_standing(s, x->signer, &k13);
        if (x->type == MOR_T_TERMS) {
            mor_spot sp;
            mor_spot_read_version(s, x, "buyer", &sp);
            mor_work_read(s, x, &ans, NULL);
        }
        if (i > 0)
            (void)mor_order_by_citing(s, x, mor_act_at(s, i - 1));
    }
    mor_set_free(s);
    return 0;
}
