/* fuzz_decode: untrusted bytes into the act decoder, then every grammar rule
 * and every question over whatever was taken. libFuzzer. */
#include "mor.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static FILE *sink;

static int accept_any_sig(const char *signer, const unsigned char *bytes, size_t n, const char *sig)
{
	(void)signer, (void)bytes, (void)n, (void)sig;
	return 1;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	mor_options opt;
	memset(&opt, 0, sizeof opt);
	/* the first byte picks the reader and whether the stand-in signature is checked, so that
	 * the fuzzer can get past it without computing SHA-256 */
	if (size > 0 && (data[0] & 1)) opt.sig_check = accept_any_sig;
	if (size > 0 && (data[0] & 2)) opt.reader = "@a";
	if (size > 0 && (data[0] & 4)) opt.anchoring_place = "place:p";
	if (size > 0 && (data[0] & 8)) opt.anchoring_clock = "clock:k";
	if (!sink) sink = fopen("/dev/null", "w");
	mor_ctx *c = mor_new(&opt);
	if (!c) return 0;
	if (mor_load(c, data, size) >= 0 && mor_verify(c) == 0) {
		size_t n = mor_finding_count(c);
		if (n > 64) n = 64;
		for (size_t i = 0; i < n; i++) {
			/* copy: the questions append findings, and the list may move */
			char subj[80], obj[80];
			const mor_finding *f = mor_finding_at(c, i);
			snprintf(subj, sizeof subj, "%s", f->subject);
			snprintf(obj, sizeof obj, "%s", f->object);
			mor_ask_receiver(c, subj, obj[0] ? obj : NULL);
			mor_ask_in_force(c, subj);
			mor_ask_bound(c, "@a", subj);
			mor_ask_order(c, subj, obj);
			mor_ask_data_path(c, "@b", subj);
			mor_ask_before_paying(c, subj);
			mor_ask_sequence(c, subj);
		}
		if (sink) mor_print(c, sink, (int)(size & 1));
	}
	mor_free(c);
	return 0;
}
