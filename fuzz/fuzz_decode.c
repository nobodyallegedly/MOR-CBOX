/* fuzz_decode.c - libFuzzer harness for the decoder of untrusted bytes (src/decode.c),
 * followed by the whole grammar and every line the tool can show.
 * Each input is run twice: as it is, and with the stand-in signature check off, so that
 * the fuzzer reaches the field parsers and the grammar without having to guess a tag. */
#include "mor.h"

#include <stdlib.h>
#include <string.h>

static FILE *sink;

static void one(const uint8_t *data, size_t size, bool skip_sig, bool plugins)
{
    static mor_cmip stubs[32];
    mor_plugins plug = {stubs, mor_stub_count()};
    mor_options opt;
    mor_stub_fill(stubs);
    memset(&opt, 0, sizeof opt);
    opt.plugins = plugins ? &plug : NULL;
    opt.standin_skip_signature = skip_sig;
    opt.reader = "alice";
    opt.anchoring_place = "plc1";
    mor_set *s = mor_set_new(&opt);
    mor_set_load(s, data, size);
    mor_evaluate(s);
    mor_show_all(s, sink);
    for (int i = 0; i < mor_n_acts(s); i++) {
        const mor_act *a = mor_act_at(s, i);
        mor_show_before(s, &a->fp, sink);
        mor_answer ans;
        mor_bound_by_act(s, "alice", a, &ans);
        if (i > 0)
            (void)mor_order_by_citing(s, a, mor_act_at(s, i - 1));
    }
    mor_set_free(s);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (!sink)
        sink = fopen("/dev/null", "w");
    if (!sink)
        abort();
    one(data, size, false, true);
    one(data, size, true, true);
    one(data, size, true, false);
    return 0;
}
