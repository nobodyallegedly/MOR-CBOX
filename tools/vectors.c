/* The vector runner: every case of every file in vectors/, answered by the
 * verifier, compared with the published answer. */
#include "vectors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

#include "../src/act.h"
#include "../src/cbor.h"
#include "../src/crypto.h"
#include "../src/mmr.h"
#include "../src/people.h"
#include "../src/sealed.h"
#include "../src/text.h"
#include "hex.h"

/* A table cell: line breaks written as \n, bars escaped. */
static void md_cell(const char *s)
{
	for (; *s; s++) {
		if (*s == '\n')
			fputs("\\n", stdout);
		else if (*s == '|')
			fputs("\\|", stdout);
		else
			putchar(*s);
	}
}

int vrecord(vrun *r, const char *casename, const char *expected, const char *got, const char *note)
{
	int ok = expected && got && !strcmp(expected, got);
	r->total++;
	r->agree += (unsigned)ok;
	if (r->markdown) {
		printf("| `%s` | ", r->file);
		md_cell(casename);
		fputs(" | `", stdout);
		md_cell(expected ? expected : "-");
		fputs("` | `", stdout);
		md_cell(got ? got : "-");
		printf("` | %s | ", ok ? "yes" : "**NO**");
		md_cell(note ? note : "");
		fputs(" |\n", stdout);
	} else if (!ok || r->verbose)
		printf("%s %s: %s\n    expected: %s\n    got:      %s%s%s\n", ok ? "ok  " : "FAIL", r->file,
		       casename, expected ? expected : "-", got ? got : "-", note ? "\n    note: " : "",
		       note ? note : "");
	return ok;
}

static const char *S(const json *j, const char *k)
{
	return json_str(json_get(j, k));
}

static char *cat(mor_arena *a, const char *p, const char *x)
{
	size_t n = strlen(p) + (x ? strlen(x) : 0) + 1;
	char *s = mor_arena_alloc(a, n);
	if (s)
		snprintf(s, n, "%s%s", p, x ? x : "");
	return s;
}

static const char *hex32(mor_arena *a, const uint8_t *h)
{
	return hex_encode(h, 32, a);
}

/* ---- tagged-hash.json ---- */
int v_tagged_hash(vrun *r, const json *j)
{
	const json *v = json_get(j, "vectors");
	for (size_t i = 0; v && i < v->len; i++) {
		const json *c = json_at(v, i);
		size_t n;
		uint8_t h[32];
		const uint8_t *d = hex_decode(S(c, "data"), &n, r->a);
		mor_tagged_hash(h, S(c, "tag"), d, d ? n : 0);
		vrecord(r, cat(r->a, "tagged_hash ", S(c, "tag")), S(c, "hash"), hex32(r->a, h), NULL);
		mor_arena_reset(r->a, 0);
	}
	return 0;
}

/* ---- cbor.json ---- */
int v_cbor(vrun *r, const json *j)
{
	static uint8_t enc[8192];
	const json *v = json_get(j, "valid");
	for (size_t i = 0; v && i < v->len; i++) {
		const json *c = json_at(v, i);
		size_t n;
		const mor_cbor *item;
		const uint8_t *d = hex_decode(S(c, "hex"), &n, r->a);
		int e = mor_cbor_decode(d, n, r->a, &item);
		const char *got = mor_cbor_err_name(e);
		if (!e) {
			mor_buf b;
			mor_buf_init(&b, enc, sizeof enc);
			mor_cbor_encode(&b, item);
			got = (!b.overflow && b.len == n && !memcmp(enc, d, n)) ? "accepted, re-encodes the same"
										 : "accepted, re-encodes differently";
		}
		char name[96];
		snprintf(name, sizeof name, "valid[%zu] %.60s", i, S(c, "diagnostic"));
		vrecord(r, name, "accepted, re-encodes the same", got, NULL);
		mor_arena_reset(r->a, 0);
	}
	v = json_get(j, "invalid");
	for (size_t i = 0; v && i < v->len; i++) {
		const json *c = json_at(v, i);
		size_t n;
		const mor_cbor *item;
		const uint8_t *d = hex_decode(S(c, "hex"), &n, r->a);
		int e = mor_cbor_decode(d, n, r->a, &item);
		char name[96], exp[64], got[64];
		snprintf(name, sizeof name, "invalid[%zu] %.60s", i, S(c, "why"));
		snprintf(exp, sizeof exp, "rejected (%s)", S(c, "error"));
		snprintf(got, sizeof got, e ? "rejected (%s)" : "accepted%s", e ? mor_cbor_err_name(e) : "");
		vrecord(r, name, exp, got, NULL);
		mor_arena_reset(r->a, 0);
	}
	return 0;
}

/* ---- canonical-text.json ---- */
int v_canonical_text(vrun *r, const json *j)
{
	const json *v = json_get(j, "vectors");
	char ver[32];
	snprintf(ver, sizeof ver, "Unicode %s", mor_text_unicode_version());
	vrecord(r, "pinned Unicode version", "Unicode 17.0.0", ver, NULL);
	for (size_t i = 0; v && i < v->len; i++) {
		const json *c = json_at(v, i);
		size_t n;
		const uint8_t *d = hex_decode(S(c, "utf8"), &n, r->a);
		int rule = mor_text_check(d, n, r->a);
		const json *er = json_get(c, "rule");
		char exp[32], got[32];
		if (er && er->type == JSON_NUM)
			snprintf(exp, sizeof exp, "not canonical, rule %lld", (long long)er->i);
		else
			snprintf(exp, sizeof exp, "canonical");
		if (rule)
			snprintf(got, sizeof got, "not canonical, rule %d", rule);
		else
			snprintf(got, sizeof got, "canonical");
		char name[128];
		snprintf(name, sizeof name, "[%zu] %.90s", i, S(c, "note"));
		vrecord(r, name, exp, got, NULL);
		mor_arena_reset(r->a, 0);
	}
	return 0;
}

/* ---- lock.json ---- */
int v_lock(vrun *r, const json *j)
{
	const json *v = json_get(j, "vectors");
	for (size_t i = 0; v && i < v->len; i++) {
		const json *c = json_at(v, i);
		size_t pn, ln;
		uint8_t key[32], nonce[24], h[32];
		const uint8_t *pt = hex_decode(S(c, "plaintext"), &pn, r->a);
		const uint8_t *lk = hex_decode(S(c, "locked"), &ln, r->a);
		hex_fixed(S(c, "key"), key, 32);
		hex_fixed(S(c, "nonce"), nonce, 24);
		uint8_t *sealed = mor_arena_alloc(r->a, pn + 16);
		uint8_t *opened = mor_arena_alloc(r->a, ln + 1);
		mor_xchacha_seal(sealed, pt, pn, nonce, key);
		mor_sha256(h, sealed, pn + 16);
		char name[64];
		snprintf(name, sizeof name, "[%zu] locked bytes", i);
		vrecord(r, name, S(c, "locked"), hex_encode(sealed, pn + 16, r->a), NULL);
		snprintf(name, sizeof name, "[%zu] locked hash", i);
		vrecord(r, name, S(c, "locked_hash"), hex32(r->a, h), NULL);
		int o = mor_xchacha_open(opened, lk, ln, nonce, key);
		snprintf(name, sizeof name, "[%zu] opens to the plaintext", i);
		vrecord(r, name, S(c, "plaintext"), o ? "does not open" : hex_encode(opened, ln - 16, r->a), NULL);
		mor_arena_reset(r->a, 0);
	}
	return 0;
}

/* ---- running-summary.json ---- */
int v_running_summary(vrun *r, const json *j)
{
	const json *ids = json_get(j, "act_ids"), *sums = json_get(j, "summaries");
	for (size_t i = 0; sums && i < sums->len; i++) {
		const json *c = json_at(sums, i);
		const json *acts = json_get(c, "acts");
		mor_mmr m;
		uint8_t root[32], id[32];
		mor_mmr_init(&m);
		for (int64_t k = 0; acts && k < acts->i; k++) {
			hex_fixed(json_str(json_at(ids, (size_t)k)), id, 32);
			mor_mmr_add(&m, id);
		}
		mor_mmr_root(&m, root);
		char name[32];
		snprintf(name, sizeof name, "S(%lld)", acts ? (long long)acts->i : -1LL);
		vrecord(r, name, S(c, "summary"), hex32(r->a, root), NULL);
		mor_arena_reset(r->a, 0);
	}
	return 0;
}

/* ---- sequence-three-acts.json ---- */
int v_sequence(vrun *r, const json *j)
{
	const json *acts = json_get(j, "acts");
	mor_mmr m;
	mor_mmr_init(&m);
	for (size_t i = 0; acts && i < acts->len; i++) {
		const json *c = json_at(acts, i);
		size_t on, ln, in;
		mor_act act;
		char name[96];
		const uint8_t *out = hex_decode(S(c, "outside"), &on, r->a);
		const uint8_t *lk = hex_decode(S(c, "locked"), &ln, r->a);
		const uint8_t *inside = hex_decode(S(c, "inside"), &in, r->a);
		int o = mor_act_open_parts(&act, out, on, lk, ln, r->a);
		snprintf(name, sizeof name, "act %zu opens", i + 1);
		vrecord(r, name, "ok", mor_open_name(o), NULL);
		if (o)
			continue;
		uint8_t h[32];
		snprintf(name, sizeof name, "act %zu id", i + 1);
		vrecord(r, name, S(c, "act_id"), hex32(r->a, act.id), NULL);
		snprintf(name, sizeof name, "act %zu inside", i + 1);
		vrecord(r, name, S(c, "inside"), hex_encode(act.inside_bytes, act.inside_len, r->a), NULL);
		mor_tagged_hash(h, "MOR/inside", inside, in);
		snprintf(name, sizeof name, "act %zu inside commitment", i + 1);
		vrecord(r, name, S(c, "inside_commitment"), hex32(r->a, h), NULL);
		mor_sha256(h, lk, ln);
		snprintf(name, sizeof name, "act %zu locked hash", i + 1);
		vrecord(r, name, S(c, "locked_hash"), hex32(r->a, h), NULL);
		/* What the act carries: spec, text, prev, position, summary. */
		snprintf(name, sizeof name, "act %zu spec, signer, binding", i + 1);
		char want[256], got[256];
		snprintf(want, sizeof want, "%s %s %s", S(j, "spec"), S(j, "signer"), S(j, "binding"));
		snprintf(got, sizeof got, "%s %s %s", hex32(r->a, act.spec), act.signer ? hex32(r->a, act.signer) : "-",
			 act.binding ? hex32(r->a, act.binding) : "-");
		vrecord(r, name, want, got, NULL);
		const mor_cbor *t = mor_cbor_map_get(act.payload, 0);
		snprintf(name, sizeof name, "act %zu text", i + 1);
		char *tx = mor_arena_alloc(r->a, t && t->type == MOR_CBOR_TEXT ? (size_t)t->v + 1 : 2);
		if (t && t->type == MOR_CBOR_TEXT)
			memcpy(tx, t->data, (size_t)t->v);
		vrecord(r, name, S(c, "text"), tx, NULL);
		snprintf(name, sizeof name, "act %zu position", i + 1);
		const json *pj = json_get(c, "position");
		snprintf(want, sizeof want, "%lld", pj ? (long long)pj->i : -1LL);
		snprintf(got, sizeof got, "%llu", act.has_pos ? (unsigned long long)act.pos : 0ULL);
		vrecord(r, name, want, got, NULL);
		snprintf(name, sizeof name, "act %zu prev", i + 1);
		const json *pv = json_get(c, "prev");
		snprintf(want, sizeof want, "%s", pv && pv->len ? json_str(json_at(pv, 0)) : "(empty)");
		snprintf(got, sizeof got, "%s",
			 act.prev && act.prev->v ? hex32(r->a, act.prev->items[0].data) : "(empty)");
		vrecord(r, name, want, got, NULL);
		/* The summary it carries is S(position - 1), computed from the line. */
		uint8_t s[32];
		mor_mmr_root(&m, s);
		snprintf(name, sizeof name, "act %zu carries S(p-1) of its line", i + 1);
		vrecord(r, name, S(c, "summary"), act.summary ? hex32(r->a, act.summary) : "-",
			hex32(r->a, s));
		mor_mmr_add(&m, act.id);
		mor_arena_reset(r->a, 0);
	}
	uint8_t s[32];
	mor_mmr_root(&m, s);
	vrecord(r, "summary including the last act", S(j, "summary_including_last"), hex32(r->a, s), NULL);
	mor_arena_reset(r->a, 0);
	return 0;
}

/* ---- open-act.json ---- */
int v_open_act(vrun *r, const json *j)
{
	const json *v = json_get(j, "vectors");
	for (size_t i = 0; v && i < v->len; i++) {
		const json *c = json_at(v, i);
		size_t on, ln;
		mor_act act;
		const uint8_t *out = hex_decode(S(c, "outside"), &on, r->a);
		const uint8_t *lk = hex_decode(S(c, "locked"), &ln, r->a);
		int o = mor_act_open_parts(&act, out, on, lk, ln, r->a);
		char name[128];
		snprintf(name, sizeof name, "[%zu] %.90s", i, S(c, "why"));
		vrecord(r, name, S(c, "result"), mor_open_name(o), NULL);
		mor_arena_reset(r->a, 0);
	}
	return 0;
}

/* ---- textual-forms.json ---- */
static int label_from(const char *s)
{
	static const char *n[] = { "", "mid", "mact", "mkey", "mspec", "mwork" };
	for (int i = 1; i <= MOR_L_MWORK; i++)
		if (s && !strcmp(s, n[i]))
			return i;
	return MOR_L_NONE;
}

int v_textual_forms(vrun *r, const json *j)
{
	const json *forms = json_get(j, "forms");
	char out[4096], name[128];
	for (size_t i = 0; forms && i < forms->len; i++) {
		const json *c = json_at(forms, i);
		size_t n;
		const uint8_t *d = hex_decode(S(c, "data"), &n, r->a);
		int l = label_from(S(c, "kind"));
		size_t w = mor_form_encode(out, sizeof out, l, d, n);
		snprintf(name, sizeof name, "%s: written", S(c, "kind"));
		vrecord(r, name, S(c, "form"), w ? out : "-", NULL);
		mor_form f;
		const char *fs = S(c, "form");
		int e = mor_form_decode(&f, fs, strlen(fs), l, r->a);
		snprintf(name, sizeof name, "%s: read back", S(c, "kind"));
		vrecord(r, name, S(c, "data"), e ? mor_form_err_name(e) : hex_encode(f.data, f.len, r->a), NULL);
		mor_arena_reset(r->a, 0);
	}
	const char *keys[] = { "upper", "bare", "lives" };
	for (size_t k = 0; k < 3; k++) {
		const json *c = json_get(j, keys[k]);
		if (!c)
			continue;
		mor_form f;
		const char *fs = S(c, "form");
		int e = mor_form_decode(&f, fs, strlen(fs), label_from(S(c, "want")), r->a);
		snprintf(name, sizeof name, "%s: read as %s", keys[k], S(c, "want"));
		vrecord(r, name, S(c, "data"), e ? mor_form_err_name(e) : hex_encode(f.data, f.len, r->a), NULL);
		const json *hints = json_get(c, "hints");
		if (hints && !e) {
			char want[1024] = "", got[1024] = "";
			for (size_t h = 0; h < hints->len; h++) {
				strncat(want, json_str(json_at(hints, h)), sizeof want - strlen(want) - 2);
				strncat(want, " ", sizeof want - strlen(want) - 1);
			}
			for (size_t h = 0; h < f.nhints; h++) {
				strncat(got, f.hints[h], sizeof got - strlen(got) - 2);
				strncat(got, " ", sizeof got - strlen(got) - 1);
			}
			vrecord(r, "lives: hints", want, got, NULL);
		}
		mor_arena_reset(r->a, 0);
	}
	const json *ref = json_get(j, "refused");
	for (size_t i = 0; ref && i < ref->len; i++) {
		const json *c = json_at(ref, i);
		mor_form f;
		const char *fs = S(c, "form");
		int e = mor_form_decode(&f, fs, strlen(fs), label_from(S(c, "want")), r->a);
		const char *why = S(c, "why");
		/* The published reason is prose; the verifier's reason is its error name. */
		const char *exp = !strncmp(why, "wrong kind", 10) ? "refused: wrong kind" : cat(r->a, "refused: ", why);
		char got[96];
		snprintf(got, sizeof got, e ? "refused: %s" : "accepted%s", e ? mor_form_err_name(e) : "");
		snprintf(name, sizeof name, "refused[%zu] (%s)", i, why);
		vrecord(r, name, exp, got, NULL);
		mor_arena_reset(r->a, 0);
	}
	return 0;
}

/* ---- signatures.json ---- */
int v_signatures(vrun *r, const json *j)
{
	char name[160];
	uint8_t msg[32], h[32];
	const json *sc = json_get(j, "schnorr");
	hex_fixed(S(j, "msg"), msg, 32);
	if (sc) {
		uint8_t sk[32], aux[32], sig[64], pk[32];
		secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
		secp256k1_keypair kp;
		secp256k1_xonly_pubkey xpk;
		hex_fixed(S(sc, "secret"), sk, 32);
		hex_fixed(S(sc, "aux"), aux, 32);
		if (secp256k1_keypair_create(ctx, &kp, sk) && secp256k1_keypair_xonly_pub(ctx, &xpk, NULL, &kp))
			secp256k1_xonly_pubkey_serialize(ctx, pk, &xpk);
		vrecord(r, "schnorr: public key from the secret", S(sc, "public_key"), hex32(r->a, pk), NULL);
		secp256k1_schnorrsig_sign32(ctx, sig, msg, &kp, aux);
		vrecord(r, "schnorr: signature with aux", S(sc, "sig"), hex_encode(sig, 64, r->a), NULL);
		static const uint8_t zero[32];
		secp256k1_schnorrsig_sign32(ctx, sig, msg, &kp, zero);
		vrecord(r, "schnorr: signature with zero aux", S(sc, "sig_zero_aux"), hex_encode(sig, 64, r->a), NULL);
		secp256k1_context_destroy(ctx);
		uint8_t sch = 1;
		mor_tagged_hash2(h, "MOR/safety", &sch, 1, pk, 32);
		vrecord(r, "schnorr: commitment if scheme 1", S(sc, "commitment_if_scheme_1"), hex32(r->a, h), NULL);
		const char *sigs[2] = { "sig", "sig_zero_aux" };
		for (int k = 0; k < 2; k++) {
			hex_fixed(S(sc, sigs[k]), sig, 64);
			snprintf(name, sizeof name, "schnorr: %s verifies", sigs[k]);
			vrecord(r, name, "valid", mor_schnorr_verify(pk, msg, sig) ? "valid" : "invalid", NULL);
		}
		const json *inv = json_get(sc, "invalid");
		for (size_t i = 0; inv && i < inv->len; i++) {
			const json *c = json_at(inv, i);
			size_t kn, sn, mn;
			const uint8_t *k = hex_decode(S(c, "key"), &kn, r->a);
			const uint8_t *s = hex_decode(S(c, "sig"), &sn, r->a);
			const uint8_t *m = hex_decode(S(c, "msg"), &mn, r->a);
			/* The founding scheme's lengths are checked first (Identity, "Signature schemes"). */
			int ok = kn == 32 && sn == 64 && mn == 32 && mor_schnorr_verify(k, m, s);
			snprintf(name, sizeof name, "schnorr invalid[%zu]: %s", i, S(c, "why"));
			vrecord(r, name, S(c, "verdict"), ok ? "valid" : "invalid", NULL);
			mor_arena_reset(r->a, 0);
		}
	}
	const json *slh = json_get(j, "slh_dsa");
	for (size_t i = 0; slh && i < slh->len; i++) {
		const json *c = json_at(slh, i);
		const json *sj = json_get(c, "scheme");
		int scheme = sj ? (int)sj->i : 0;
		uint8_t sks[16], skp[16], pks[16], pk[32], rnd[16];
		const char *nm = S(c, "name");
		size_t sl = mor_slhdsa_sig_len(scheme);
		hex_fixed(S(c, "sk_seed"), sks, 16);
		hex_fixed(S(c, "sk_prf"), skp, 16);
		hex_fixed(S(c, "pk_seed"), pks, 16);
		hex_fixed(S(c, "opt_rand"), rnd, 16);
		int kg = mor_slhdsa_keygen(scheme, pk, sks, skp, pks);
		snprintf(name, sizeof name, "%s: public key from the seeds", nm);
		vrecord(r, name, S(c, "public_key"), kg ? "keygen failed" : hex32(r->a, pk), NULL);
		uint8_t sch = (uint8_t)scheme;
		mor_tagged_hash2(h, "MOR/safety", &sch, 1, pk, 32);
		snprintf(name, sizeof name, "%s: chain-key commitment", nm);
		vrecord(r, name, S(c, "commitment"), hex32(r->a, h), NULL);
		uint8_t *sig = mor_arena_alloc(r->a, sl);
		int sg = mor_slhdsa_sign(scheme, sig, sks, skp, pks, msg, 32, (const uint8_t *)"MOR", 3, NULL);
		snprintf(name, sizeof name, "%s: deterministic signature", nm);
		vrecord(r, name, S(c, "deterministic"), sg ? "sign failed" : hex_encode(sig, sl, r->a), NULL);
		sg = mor_slhdsa_sign(scheme, sig, sks, skp, pks, msg, 32, (const uint8_t *)"MOR", 3, rnd);
		snprintf(name, sizeof name, "%s: hedged signature (opt_rand)", nm);
		vrecord(r, name, S(c, "hedged"), sg ? "sign failed" : hex_encode(sig, sl, r->a), NULL);
		const char *sigs[2] = { "deterministic", "hedged" };
		for (int k = 0; k < 2; k++) {
			size_t sn;
			const uint8_t *s = hex_decode(S(c, sigs[k]), &sn, r->a);
			int v = mor_slhdsa_verify(scheme, pk, msg, 32, s, sn, (const uint8_t *)"MOR", 3);
			snprintf(name, sizeof name, "%s: %s signature verifies", nm, sigs[k]);
			vrecord(r, name, "valid", v == 1 ? "valid" : "invalid", NULL);
		}
		const json *inv = json_get(c, "invalid");
		for (size_t k = 0; inv && k < inv->len; k++) {
			const json *x = json_at(inv, k);
			size_t kn, sn, mn;
			const uint8_t *key = hex_decode(S(x, "key"), &kn, r->a);
			const uint8_t *s = hex_decode(S(x, "sig"), &sn, r->a);
			const uint8_t *m = hex_decode(S(x, "msg"), &mn, r->a);
			int v = kn == 32 && mor_slhdsa_verify(scheme, key, m, mn, s, sn, (const uint8_t *)"MOR", 3) == 1;
			snprintf(name, sizeof name, "%s invalid[%zu]: %s", nm, k, S(x, "why"));
			vrecord(r, name, S(x, "verdict"), v ? "valid" : "invalid", NULL);
		}
		mor_arena_reset(r->a, 0);
	}
	return 0;
}

/* ---- sealed.json ---- */
int v_sealed(vrun *r, const json *j)
{
	uint8_t xsk[32], rcpt[32], pk[MOR_XWING_PK];
	size_t n, in;
	hex_fixed(S(j, "recipient_xwing_secret"), xsk, 32);
	hex_fixed(S(j, "recipient"), rcpt, 32);
	int e = mor_xwing_public_from_secret(pk, xsk);
	vrecord(r, "recipient's X-Wing public key from its secret", S(j, "recipient_xwing_public"),
		e ? "failed" : hex_encode(pk, sizeof pk, r->a), NULL);
	const uint8_t *sealed = hex_decode(S(j, "sealed"), &n, r->a);
	mor_sealed_open so;
	int o = mor_sealed_open_as(&so, sealed, n, rcpt, xsk, r->a);
	vrecord(r, "the recipient opens the container", "ok", mor_sealed_err_name(o), NULL);
	vrecord(r, "the inner act", S(j, "inner_act"), so.inner ? hex_encode(so.inner, so.inner_len, r->a) : "-", NULL);
	vrecord(r, "the inner act's content key", S(j, "content_key"), so.key ? hex32(r->a, so.key) : "-", NULL);
	vrecord(r, "the inner act's id", S(j, "inner_act_id"), so.inner ? hex32(r->a, so.act.id) : "-", NULL);
	/* Rebuild the container from every fixed input: it must be the same bytes. */
	uint8_t eseed[64], ck[32], nonce[24], ss[32], wk[32], ct[MOR_XWING_CT], wrapped[48];
	static const uint8_t zero_nonce[24];
	hex_fixed(S(j, "eseed"), eseed, 64);
	hex_fixed(S(j, "container_key"), ck, 32);
	hex_fixed(S(j, "nonce"), nonce, 24);
	const uint8_t *inner = hex_decode(S(j, "inner_act"), &in, r->a);
	uint8_t key[32];
	hex_fixed(S(j, "content_key"), key, 32);
	e = mor_xwing_encaps_derand(ct, ss, pk, eseed);
	mor_tagged_hash(wk, "MOR/sealed/wrap", ss, 32);
	mor_xchacha_seal(wrapped, ck, 32, zero_nonce, wk);
	static uint8_t contents[16384], lac[32768], out[40000];
	mor_buf b;
	mor_buf_init(&b, contents, sizeof contents);
	mor_cbor_put_head(&b, 4, 2);
	mor_cbor_put_bytes(&b, inner, in);
	mor_cbor_put_bytes(&b, key, 32);
	uint8_t *locked = mor_arena_alloc(r->a, b.len + 16);
	mor_xchacha_seal(locked, contents, b.len, nonce, ck);
	size_t locked_len = b.len + 16;
	mor_buf_init(&b, lac, sizeof lac);
	mor_cbor_put_head(&b, 4, 3);
	mor_cbor_put_head(&b, 4, 1);
	mor_cbor_put_head(&b, 4, 2);
	mor_cbor_put_bytes(&b, ct, sizeof ct);
	mor_cbor_put_bytes(&b, wrapped, sizeof wrapped);
	mor_cbor_put_bytes(&b, nonce, 24);
	mor_cbor_put_bytes(&b, locked, locked_len);
	size_t lacn = b.len;
	mor_buf_init(&b, out, sizeof out);
	mor_cbor_put_head(&b, 4, 2);
	mor_cbor_put_head(&b, 4, 1);
	mor_cbor_put_bytes(&b, rcpt, 32);
	mor_cbor_put_bytes(&b, lac, lacn);
	vrecord(r, "the container rebuilt from its fixed inputs", S(j, "sealed"),
		e || b.overflow ? "failed" : hex_encode(out, b.len, r->a), NULL);
	mor_arena_reset(r->a, 0);
	return 0;
}

static char *slurp(const char *path, size_t *n)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;
	if (fseek(f, 0, SEEK_END)) {
		fclose(f);
		return NULL;
	}
	long l = ftell(f);
	if (l < 0 || fseek(f, 0, SEEK_SET)) {
		fclose(f);
		return NULL;
	}
	char *p = malloc((size_t)l + 1);
	if (p && fread(p, 1, (size_t)l, f) != (size_t)l) {
		free(p);
		p = NULL;
	}
	fclose(f);
	if (p) {
		p[l] = 0;
		*n = (size_t)l;
	}
	return p;
}

int run_all_vectors(const char *dir, int markdown, int verbose)
{
	static const struct {
		const char *file;
		int (*fn)(vrun *, const json *);
	} files[] = {
		{ "tagged-hash.json", v_tagged_hash },
		{ "cbor.json", v_cbor },
		{ "canonical-text.json", v_canonical_text },
		{ "lock.json", v_lock },
		{ "running-summary.json", v_running_summary },
		{ "sequence-three-acts.json", v_sequence },
		{ "open-act.json", v_open_act },
		{ "textual-forms.json", v_textual_forms },
		{ "signatures.json", v_signatures },
		{ "sealed.json", v_sealed },
		{ "verdicts.json", v_verdicts },
	};
	/* The tool, not the library, owns these: one arena for the parsed file
	 * and one for each case's scratch. */
	size_t jcap = (size_t)64 << 20, scap = (size_t)256 << 20;
	void *jmem = malloc(jcap), *smem = malloc(scap);
	mor_arena ja, sa;
	vrun r = { 0 };
	if (!jmem || !smem) {
		free(jmem);
		free(smem);
		return 2;
	}
	mor_arena_init(&sa, smem, scap);
	r.a = &sa;
	r.markdown = markdown;
	r.verbose = verbose;
	if (markdown)
		printf("| File | Case | Published | Verifier | Agrees | Rule |\n| --- | --- | --- | --- | --- | --- |\n");
	for (size_t i = 0; i < sizeof files / sizeof files[0]; i++) {
		char path[4096];
		size_t n = 0;
		snprintf(path, sizeof path, "%s/%s", dir, files[i].file);
		char *text = slurp(path, &n);
		const json *root;
		mor_arena_init(&ja, jmem, jcap);
		r.file = files[i].file;
		if (!text || json_parse(text, n, &ja, &root)) {
			vrecord(&r, "(file)", "readable JSON", "unreadable", NULL);
			free(text);
			continue;
		}
		unsigned before = r.total, agree = r.agree;
		files[i].fn(&r, root);
		if (!markdown)
			fprintf(stderr, "%-26s %4u/%-4u agree\n", files[i].file, r.agree - agree, r.total - before);
		free(text);
	}
	fprintf(stderr, "TOTAL %u/%u agree\n", r.agree, r.total);
	free(jmem);
	free(smem);
	return r.agree == r.total ? 0 : 1;
}
