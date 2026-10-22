/* Unit tests: the decoders' edges, rules exercised on acts made for them, and
 * variations of the published verdict cases. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/agreements.h"
#include "../src/cbor.h"
#include "../src/crypto.h"
#include "../src/development.h"
#include "../src/mmr.h"
#include "../src/identity.h"
#include "../src/people.h"
#include "../src/text.h"
#include "../src/verdict.h"
#include "../tools/hex.h"
#include "../tools/json.h"
#include "../tools/vectors.h"
#include "builder.h"

static int failures, checks;

#define CHECK(cond, ...)                                     \
	do {                                                 \
		checks++;                                    \
		if (!(cond)) {                               \
			failures++;                          \
			printf("FAIL %s:%d: ", __FILE__, __LINE__); \
			printf(__VA_ARGS__);                 \
			printf("\n");                        \
		}                                            \
	} while (0)

static mor_arena A;

static int cbor_err(const char *hx)
{
	size_t n;
	const mor_cbor *c;
	size_t mark = mor_arena_mark(&A);
	const uint8_t *d = hex_decode(hx, &n, &A);
	int e = mor_cbor_decode(d, n, &A, &c);
	mor_arena_reset(&A, mark);
	return e;
}

static void test_cbor(void)
{
	/* floats: shortest form, NaN payloads included */
	CHECK(cbor_err("f90001") == 0, "smallest half subnormal");
	CHECK(cbor_err("fa33800000") == MOR_CBOR_E_FLOAT, "2^-24 as a single fits a half");
	CHECK(cbor_err("fa00000001") == 0, "smallest single subnormal");
	CHECK(cbor_err("fb3e70000000000000") == MOR_CBOR_E_FLOAT, "2^-24 as a double fits a half");
	CHECK(cbor_err("fb7ff0000000000000") == MOR_CBOR_E_FLOAT, "infinity as a double");
	CHECK(cbor_err("fa7f800000") == MOR_CBOR_E_FLOAT, "infinity as a single");
	CHECK(cbor_err("f97c00") == 0, "infinity as a half");
	CHECK(cbor_err("fb8000000000000000") == MOR_CBOR_E_FLOAT, "-0.0 as a double");
	CHECK(cbor_err("fa7fc00001") == 0, "a NaN whose payload needs a single");
	CHECK(cbor_err("fb7ff8000000000001") == 0, "a NaN whose payload needs a double");
	CHECK(cbor_err("fb3ff0000000000001") == 0, "1.0000000000000002 needs a double");
	CHECK(cbor_err("fa47800000") == 0, "65536.0 needs a single (half max 65504)");
	CHECK(cbor_err("fa477fe000") == MOR_CBOR_E_FLOAT, "65504.0 fits a half");
	/* lengths that cannot be met are refused before anything is allocated */
	CHECK(cbor_err("5bffffffffffffffff") == MOR_CBOR_E_END, "a huge byte string");
	CHECK(cbor_err("9b0000000100000000") == MOR_CBOR_E_END, "a huge array");
	CHECK(cbor_err("bb0000000100000000") == MOR_CBOR_E_END, "a huge map");
	/* tags count toward depth; simple values */
	CHECK(cbor_err("c1c1c100") == 0, "nested tags");
	CHECK(cbor_err("f7") == 0, "undefined");
	CHECK(cbor_err("e0") == 0, "simple(0)");
	CHECK(cbor_err("f81f") == MOR_CBOR_E_SIMPLE, "simple(31) in two bytes");
	CHECK(cbor_err("f820") == 0, "simple(32)");
	CHECK(cbor_err("1f") == MOR_CBOR_E_RESERVED, "major 0 with ai 31");
	CHECK(cbor_err("7f") == MOR_CBOR_E_INDEFINITE, "indefinite text");
	CHECK(cbor_err("a20100200f") == 0, "{1: 0, -1: 15}: 0x01 before 0x20");
	CHECK(cbor_err("a220000100") == MOR_CBOR_E_UNSORTED, "{-1, 1} out of order");
	CHECK(cbor_err("62c0af") == MOR_CBOR_E_UTF8, "overlong UTF-8");
	CHECK(cbor_err("64f4908080") == MOR_CBOR_E_UTF8, "above U+10FFFF");
	CHECK(cbor_err("3bffffffffffffffff") == 0, "the most negative integer");
}

static int text_rule(const char *hx)
{
	size_t n, mark = mor_arena_mark(&A);
	const uint8_t *d = hex_decode(hx, &n, &A);
	int r = mor_text_check(d, n, &A);
	mor_arena_reset(&A, mark);
	return r;
}

static void test_text(void)
{
	CHECK(text_rule("cdb8") == 0, "U+0378, unassigned in 17.0, is left as it is");
	CHECK(text_rule("61f09fbfbe") == MOR_TEXT_NONCHAR, "U+1FFFE");
	CHECK(text_rule("61c2a00a62") == MOR_TEXT_TRAILING, "a line ending with NO-BREAK SPACE");
	CHECK(text_rule("61e2808a") == MOR_TEXT_TRAILING, "ending with U+200A HAIR SPACE");
	CHECK(text_rule("61e2808b") == 0, "U+200B is not a space");
	CHECK(text_rule("0d") == MOR_TEXT_LINEBREAK, "CR alone");
	CHECK(text_rule("610d20") == MOR_TEXT_LINEBREAK, "rules in order: CR before the trailing space");
	CHECK(text_rule("efbbbf0d") == MOR_TEXT_UTF8, "U+FEFF before CR");
	CHECK(text_rule("c3a9") == 0, "e acute precomposed");
	CHECK(text_rule("e18480e185a1e186a8") == MOR_TEXT_NFC, "L V T jamo compose");
	CHECK(text_rule("61cc81cca3") == MOR_TEXT_NFC, "a, acute (230), dot below (220): out of canonical order");
	CHECK(text_rule("e1baa1cc81") == 0, "a with dot below, then acute: NFC");
	CHECK(strcmp(mor_text_unicode_version(), "17.0.0") == 0, "Unicode tables pinned at 17.0.0");
}

static void test_forms(void)
{
	mor_form f;
	size_t mark = mor_arena_mark(&A);
	const char *bare_upper = "E4FE5F7FE5048DD6D85510B6CEAE17DF36B5C0530117D4D23B09A64F27C3CE52";
	CHECK(mor_form_decode(&f, bare_upper, 64, MOR_L_MACT, &A) != 0, "bare hex is small letters");
	char out[200];
	uint8_t d[32] = { 1, 2, 3 };
	size_t n = mor_form_encode(out, sizeof out, MOR_L_MWORK, d, 32);
	CHECK(n == 64, "a 32-byte form is 58 characters plus its label and separator (got %zu)", n);
	CHECK(mor_form_decode(&f, out, n, MOR_L_MWORK, &A) == 0 && f.len == 32 && !memcmp(f.data, d, 32), "round trip");
	/* a key: [scheme, key] in deterministic CBOR; a 31-byte key is refused */
	uint8_t kd[35] = { 0x82, 0x01, 0x58, 0x1f };
	n = mor_form_encode(out, sizeof out, MOR_L_MKEY, kd, 35);
	CHECK(mor_form_decode(&f, out, n, MOR_L_MKEY, &A) == MOR_FORM_DATA, "a key of the wrong length");
	mor_arena_reset(&A, mark);
}

/* ---- acts made for the rules ---- */

static uint8_t MIR[MOR_NMIRS][32];
static char MIRH[MOR_NMIRS][65];
static const char *CMIP = "8186c970119aafbf1d078955694a185aa1e4e9fc688ab51ec99724c1d77338f7";

typedef struct held {
	const uint8_t *b[64];
	size_t n[64];
	size_t k;
} held;

static void hold(held *h, const tb_act *a)
{
	uint8_t *copy = mor_arena_alloc(&A, a->len);
	memcpy(copy, a->bytes, a->len);
	h->b[h->k] = copy;
	h->n[h->k] = a->len;
	h->k++;
}

/* The world of the acts held, and the act asked about. */
static int world_with(mor_world *w, held *h, const tb_act *a)
{
	const uint8_t *b[65];
	size_t n[65];
	memcpy(b, h->b, h->k * sizeof *b);
	memcpy(n, h->n, h->k * sizeof *n);
	b[h->k] = a->bytes;
	n[h->k] = a->len;
	return mor_world_init(w, &A, (const uint8_t(*)[32])MIR, b, n, h->k + 1);
}

static int status_in(held *h, const tb_act *a, int flags)
{
	mor_world w;
	size_t mark = mor_arena_mark(&A);
	if (world_with(&w, h, a))
		return -1;
	int st = mor_status(&w, a->id, flags);
	mor_arena_reset(&A, mark);
	return st;
}

static int verdict_in(held *h, const tb_act *a)
{
	mor_world w;
	size_t mark = mor_arena_mark(&A);
	if (world_with(&w, h, a))
		return -1;
	int st = mor_verdict(&w, a->id);
	mor_arena_reset(&A, mark);
	return st;
}

static tb_key alice, bob, alice_chain, alice_chain2, alice_signing2, bob_chain, bob_signing2;
static tb_act g_alice, g_bob;
static char salt_n;

static const char *salt(void)
{
	static char s[40];
	snprintf(s, sizeof s, "h'000000000000000000000000000000%02x'", (unsigned char)++salt_n);
	return s;
}

static void genesis(tb_act *g, const tb_key *sk, const tb_key *ck)
{
	char c[65], pk[65], d[1024];
	tb_commit_hex(ck, c);
	tb_hex(sk->pk, 32, pk);
	snprintf(d, sizeof d, "{0: h'%s', 1: 0, 4: {0: [1, h'%s'], 1: [3, h'%s'], 2: [[null, \"https://self.example\"]]}, 10: %s}",
		 MIRH[MOR_IDENTITY], pk, c, salt());
	if (tb_make(g, d, NULL, NULL, sk, NULL))
		abort();
}

/* An everyday act: spec, type, payload, prev (hex or NULL), position, summary, extra fields. */
static void everyday(tb_act *out, const char *spec, int type, const char *payload, const char *prev, int pos,
		     const char *summary, const char *extra, const char *signer, const char *binding, const tb_key *k,
		     const tb_opts *o)
{
	char d[8192];
	char pv[80];
	if (prev)
		snprintf(pv, sizeof pv, "[h'%s']", prev);
	else
		snprintf(pv, sizeof pv, "[]");
	snprintf(d, sizeof d, "{0: h'%s', 1: %d, 2: %s, 4: %s, 5: %d, 6: h'%s', 10: %s%s%s}", spec, type, pv, payload, pos,
		 summary, salt(), extra ? ", " : "", extra ? extra : "");
	if (tb_make(out, d, signer, binding, k, o)) {
		printf("cannot build: %s\n", d);
		abort();
	}
}

static const char *Z = "0000000000000000000000000000000000000000000000000000000000000000";

static void test_envelope_rules(void)
{
	held h = { 0 };
	hold(&h, &g_alice);
	tb_act t;
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"hello\"}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_VALID, "a plain text act (got %s)", mor_status_name(status_in(&h, &t, 0)));

	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"hello\"}", NULL, 1, Z, "7: [h'0101010101010101010101010101010101010101010101010101010101010101']",
		 g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a text act carrying acks (Envelopes rule 18)");
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"hello\"}", NULL, 1, Z, "9: \"hint \"", g_alice.idhex, g_alice.idhex, &alice,
		 NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a hint with a trailing space (Text rule 1)");
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"hello\", 2: 1}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a text act with a field 2");
	everyday(&t, MIRH[MOR_TEXT], 7, "{0: \"hello\"}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "type 7 under Text");
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: h'00'}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a byte string in field 0");
	/* invalid beats unknown */
	everyday(&t, CMIP, 0, "{0: \"x\"}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(verdict_in(&h, &t) == MOR_ST_UNKNOWN, "a cMIP act: unknown");
	CHECK(status_in(&h, &t, 0) == MOR_ST_VALID, "its Identity standing: valid");
	everyday(&t, CMIP, 0, "{0: \"x \"}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(verdict_in(&h, &t) == MOR_ST_INVALID, "a cMIP act with a text not canonical: invalid");
	everyday(&t, CMIP, 0, "{\"key \": 1}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(verdict_in(&h, &t) == MOR_ST_INVALID, "a text map key not canonical: invalid");
	everyday(&t, CMIP, 0, "{0: 1}", NULL, 1, Z, "7: [h'0101010101010101010101010101010101010101010101010101010101010101']",
		 g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(verdict_in(&h, &t) == MOR_ST_INVALID, "a cMIP act carrying acks (rule 19)");
	/* An identity born under a signing scheme named by hash, which this verifier lacks. */
	tb_opts sh = { 0, 0, 1, NULL };
	tb_act g_carol;
	{
		char c[65], pk[65], d[1024], shh[65];
		uint8_t schash[32];
		mor_sha256(schash, (const uint8_t *)"a scheme by hash", 16);
		tb_hex(schash, 32, shh);
		tb_commit_hex(&alice_chain, c);
		tb_hex(bob.pk, 32, pk);
		snprintf(d, sizeof d,
			 "{0: h'%s', 1: 0, 4: {0: [h'%s', h'%s'], 1: [3, h'%s'], 2: [[null, \"https://self.example\"]]}, 10: %s}",
			 MIRH[MOR_IDENTITY], shh, pk, c, salt());
		if (tb_make(&g_carol, d, NULL, NULL, &bob, &sh))
			abort();
	}
	CHECK(status_in(&h, &g_carol, 0) == MOR_ST_UNKNOWN, "a genesis under a scheme by hash: unknown");
	hold(&h, &g_carol);
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"x\"}", NULL, 1, Z, NULL, g_carol.idhex, g_carol.idhex, &bob, &sh);
	CHECK(status_in(&h, &t, 0) == MOR_ST_UNKNOWN, "an act under a scheme by hash: unknown");
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"x\"}", NULL, 1, Z, "7: [h'0101010101010101010101010101010101010101010101010101010101010101']",
		 g_carol.idhex, g_carol.idhex, &bob, &sh);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a scheme by hash and acks on a text: invalid");
	tb_opts privbad = { 1, 1, 0, NULL };
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"x\"}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, &privbad);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a private act with a broken signature: invalid");
	tb_opts bad = { 0, 1, 0, NULL };
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"x\"}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, &bad);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a broken signature");
	everyday(&t, MIRH[MOR_ENVELOPES], 2, "{}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "Envelopes type 2, retired");
	everyday(&t, MIRH[MOR_DEVELOPMENT], 0, "{}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "Development defines no act types");
	everyday(&t, MIRH[MOR_IDENTITY], 11, "{}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "Identity type 11, retired");
	everyday(&t, MIRH[MOR_IDENTITY], 15, "{}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a witness act acknowledging nothing (Identity rule 10)");
	tb_opts priv = { 1, 0, 0, NULL };
	everyday(&t, MIRH[MOR_IDENTITY], 3, "{0: 1, 2: []}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, &priv);
	CHECK(status_in(&h, &t, 0) == MOR_ST_UNKNOWN, "a private act it cannot open: unknown");
	everyday(&t, MIRH[MOR_IDENTITY], 3, "{0: 0, 2: []}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "routes version 0");
	everyday(&t, MIRH[MOR_IDENTITY], 3, "{0: 1, 2: [[null, [\"https://x.example\"], 0]]}", NULL, 1, Z, NULL,
		 g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a route's kind 0 written");
	/* Money: who signs what */
	char pointer[256];
	snprintf(pointer, sizeof pointer, "{0: h'%s', 1: 1, 3: [[h'%s', h'61']]}", g_bob.idhex, CMIP);
	everyday(&t, MIRH[MOR_MONEY], 0, pointer, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a payee pointer signed by another than its payee");
	char ob[512];
	snprintf(ob, sizeof ob, "{0: h'%s', 1: h'%s', 2: [h'%s', 5], 3: h'%s'}", g_bob.idhex, g_alice.idhex, CMIP, CMIP);
	everyday(&t, MIRH[MOR_MONEY], 1, ob, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "an obligation signed by its creditor");
	/* bindings */
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"x\"}", NULL, 1, Z, NULL, g_alice.idhex, g_bob.idhex, &alice, NULL);
	hold(&h, &g_bob);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "bound to another identity's genesis");
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"x\"}", NULL, 1, Z, NULL, g_alice.idhex,
		 "0202020202020202020202020202020202020202020202020202020202020202", &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_UNKNOWN, "bound to an act not held: unknown");
	everyday(&t, MIRH[MOR_TEXT], 0, "{0: \"x\"}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &bob, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "signed by a key its binding did not set");
}

/* A rotation of `who` at position pos after prev, keeping tips, disowning some. */
static void rotation(tb_act *r, const tb_act *g, const char *prev, int pos, const tb_key *chain, const tb_key *next_sk,
		     const tb_key *next_chain, const char *kept, const char *disowned)
{
	char c[65], pk[65], d[2048];
	tb_commit_hex(next_chain, c);
	tb_hex(next_sk->pk, 32, pk);
	snprintf(d, sizeof d, "{0: h'%s', 1: 1, 4: {0: h'%s', 1: %d, 2: [1, h'%s'], 3: [3, h'%s'], 4: %s%s%s}, 10: %s}",
		 MIRH[MOR_IDENTITY], prev, pos, pk, c, kept, disowned ? ", 5: " : "", disowned ? disowned : "", salt());
	if (tb_make(r, d, g->idhex, NULL, chain, NULL))
		abort();
}


static void s_of(const tb_act *const *acts, size_t n, char out[65])
{
	mor_mmr m;
	uint8_t s[32];
	mor_mmr_init(&m);
	for (size_t i = 0; i < n; i++)
		mor_mmr_add(&m, acts[i]->id);
	mor_mmr_root(&m, s);
	tb_hex(s, 32, out);
}

static void test_rotation(void)
{
	held h = { 0 };
	tb_act p1, p2, p3, r1, w1, r2;
	char s1[65], s2[65], s3[65], kept[512], dis[256];
	hold(&h, &g_alice);
	hold(&h, &g_bob);
	everyday(&p1, CMIP, 0, "{0: \"one\"}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	const tb_act *l1[] = { &p1 };
	s_of(l1, 1, s1);
	everyday(&p2, CMIP, 0, "{0: \"two\"}", p1.idhex, 2, s1, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	const tb_act *l2[] = { &p1, &p2 };
	s_of(l2, 2, s2);
	everyday(&p3, CMIP, 0, "{0: \"three\"}", p2.idhex, 3, s2, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	const tb_act *l3[] = { &p1, &p2, &p3 };
	s_of(l3, 3, s3);
	hold(&h, &p1);
	hold(&h, &p2);
	hold(&h, &p3);
	/* The rotation keeps the line up to p2 and disowns p1. */
	snprintf(kept, sizeof kept, "[[h'%s', 2, h'%s']]", p2.idhex, s2);
	snprintf(dis, sizeof dis, "[h'%s']", p1.idhex);
	rotation(&r1, &g_alice, g_alice.idhex, 1, &alice_chain, &alice_signing2, &alice_chain2, kept, dis);
	hold(&h, &r1);
	CHECK(status_in(&h, &r1, 0) == MOR_ST_VALID, "a self-hosted identity's rotation counts on itself");
	CHECK(status_in(&h, &p2, 0) == MOR_ST_VALID, "kept: valid (rule 33)");
	CHECK(status_in(&h, &p1, 0) == MOR_ST_VOID, "kept but disowned: void (rule 34)");
	CHECK(status_in(&h, &p3, 0) == MOR_ST_VOID, "outside the kept ancestry: void");
	/* Bob relies on p3 by a witness act: disputed. */
	char ack[128];
	snprintf(ack, sizeof ack, "7: [h'%s']", p3.idhex);
	everyday(&w1, MIRH[MOR_IDENTITY], 15, "{}", NULL, 1, Z, ack, g_bob.idhex, g_bob.idhex, &bob, NULL);
	hold(&h, &w1);
	CHECK(status_in(&h, &p3, 0) == MOR_ST_DISPUTED, "acknowledged by another identity: disputed");
	/* Bob's rotation keeps nothing: his witness act is voided, and decides nothing. */
	rotation(&r2, &g_bob, g_bob.idhex, 1, &bob_chain, &bob_signing2, &alice_chain2, "[]", NULL);
	hold(&h, &r2);
	CHECK(status_in(&h, &w1, 0) == MOR_ST_VOID, "the witness act, disowned by its signer's rotation");
	CHECK(status_in(&h, &p3, 0) == MOR_ST_VOID, "evidence counts only while its signer's chain keeps it");
	/* A kept line the verifier cannot rebuild: unknown, never void. */
	held h2 = { 0 };
	hold(&h2, &g_alice);
	hold(&h2, &p2);
	hold(&h2, &p3);
	hold(&h2, &r1);
	CHECK(status_in(&h2, &p3, 0) == MOR_ST_UNKNOWN, "the kept line cannot be rebuilt: unknown");
	/* A rotation whose chain key is not the one committed is no rotation. */
	tb_act bad;
	rotation(&bad, &g_alice, g_alice.idhex, 1, &alice_chain2, &alice_signing2, &alice_chain2, "[]", NULL);
	hold(&h2, &bad);
	CHECK(status_in(&h2, &bad, 0) == MOR_ST_INVALID, "a rotation revealing a key not committed");
	/* Two rotations served at one position of a self-hosted identity: contested for good. */
	held h3 = { 0 };
	tb_act r1b;
	hold(&h3, &g_alice);
	hold(&h3, &r1);
	rotation(&r1b, &g_alice, g_alice.idhex, 1, &alice_chain, &bob, &alice_chain2, "[]", NULL);
	hold(&h3, &r1b);
	CHECK(status_in(&h3, &r1, 0) == MOR_ST_CONTESTED, "two rotations at a self-hosted position: contested");
	/* An act under the new key, bound to the rotation. */
	tb_act n1;
	everyday(&n1, CMIP, 0, "{0: \"new\"}", NULL, 1, Z, NULL, g_alice.idhex, r1.idhex, &alice_signing2, NULL);
	hold(&h, &n1);
	CHECK(status_in(&h, &n1, 0) == MOR_ST_VALID, "an act under the rotated key");
	tb_act n2;
	everyday(&n2, CMIP, 0, "{0: \"old key\"}", NULL, 1, Z, NULL, g_alice.idhex, r1.idhex, &alice, NULL);
	CHECK(status_in(&h, &n2, 0) == MOR_ST_INVALID, "bound to the rotation, signed with the old key");
}

static void test_terms(void)
{
	held h = { 0 };
	hold(&h, &g_alice);
	tb_act t;
	char d[1024];
	snprintf(d, sizeof d, "{0: [h'%s', h'%s'], 1: \"Terms.\", 2: {}, 4: [0], 5: [0]}", g_alice.idhex, g_bob.idhex);
	everyday(&t, MIRH[MOR_AGREEMENTS], 0, d, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_VALID, "deal terms");
	snprintf(d, sizeof d, "{0: [h'%s', h'%s'], 1: \"Terms.\", 2: {}, 4: [0], 5: [1, 1]}", g_alice.idhex, g_bob.idhex);
	everyday(&t, MIRH[MOR_AGREEMENTS], 0, d, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a deal whose field 5 is not [0]");
	snprintf(d, sizeof d, "{0: [h'%s', h'%s'], 1: \"Terms.\", 2: {}, 4: [1, 1], 5: [0]}", g_alice.idhex, g_bob.idhex);
	everyday(&t, MIRH[MOR_AGREEMENTS], 0, d, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "founding terms whose field 4 is not [0]");
	snprintf(d, sizeof d,
		 "{0: [h'%s', h'%s'], 1: \"Terms.\", 2: {}, 4: [0], 5: [0], 7: [[h'%s', [[h'%s', 600000], [h'%s', 300000]]]]}",
		 g_alice.idhex, g_bob.idhex, CMIP, g_alice.idhex, g_bob.idhex);
	everyday(&t, MIRH[MOR_AGREEMENTS], 0, d, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "stakes not summing to 1,000,000");
	snprintf(d, sizeof d, "{0: [h'%s'], 1: \"Terms.\", 2: {}, 4: [0], 5: [0], 25: 1}", g_alice.idhex);
	everyday(&t, MIRH[MOR_AGREEMENTS], 0, d, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "terms carrying the withdrawn field 25");
	snprintf(d, sizeof d, "{0: [h'%s'], 1: \"Terms.\", 2: {10: h'%s', 6: h'%s'}, 4: [0], 5: [0]}", g_alice.idhex, CMIP,
		 CMIP);
	everyday(&t, MIRH[MOR_AGREEMENTS], 0, d, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a judge named for another task");
	snprintf(d, sizeof d, "{0: [h'%s'], 1: \"Terms.\", 2: {}, 4: [0], 5: [0], 9: {0: [1, 1], 1: [0]}}", g_alice.idhex);
	everyday(&t, MIRH[MOR_AGREEMENTS], 0, d, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a deal whose absence authority is a threshold");
	/* a signature act must name what it signs as chain and predecessor */
	snprintf(d, sizeof d, "{0: h'%s'}", CMIP);
	everyday(&t, MIRH[MOR_AGREEMENTS], 1, d, NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "a signature naming nothing in objects");
	/* Agreements types 7, 11, 15 and 21 are retired */
	everyday(&t, MIRH[MOR_AGREEMENTS], 11, "{}", NULL, 1, Z, NULL, g_alice.idhex, g_alice.idhex, &alice, NULL);
	CHECK(status_in(&h, &t, 0) == MOR_ST_INVALID, "Agreements type 11, retired");
}

/* ---- variations of the published verdict cases ---- */

static const json *ROOT;

static int case_world(mor_world *w, const char *layer, size_t idx, const char *drop, const char *add)
{
	const json *c = json_at(json_get(json_get(ROOT, "cases"), layer), idx);
	(void)add;
	return vbuild_world(&A, ROOT, c, w, drop);
}

static const uint8_t *idp(const char *prefix)
{
	const json *pool = json_get(ROOT, "pool");
	static uint8_t id[32];
	for (size_t i = 0; i < pool->len; i++)
		if (!strncmp(pool->keys[i], prefix, strlen(prefix))) {
			hex_fixed(pool->keys[i], id, 32);
			return id;
		}
	return NULL;
}

static void test_variations(void)
{
	mor_world w;
	size_t mark = mor_arena_mark(&A);
	/* identity[2] without its receipt: the rotation is pending */
	case_world(&w, "identity", 2, "a793587d", NULL);
	CHECK(mor_status(&w, idp("5a770188"), 0) == MOR_ST_PENDING, "a rotation whose receipt is not held: pending");
	mor_arena_reset(&A, mark);
	/* identity[6] without the kept tip: its line cannot be rebuilt */
	case_world(&w, "identity", 6, "265b9760", NULL);
	CHECK(mor_status(&w, idp("c89f26d2"), 0) == MOR_ST_UNKNOWN, "the kept tip not held: unknown, never void");
	mor_arena_reset(&A, mark);
	/* identity[8] asked plainly: without Agreements' keepers it is void */
	case_world(&w, "identity", 8, NULL, NULL);
	CHECK(mor_status(&w, idp("37685485"), 0) == MOR_ST_VOID, "status without keepers: void");
	mor_arena_reset(&A, mark);
	/* identity[13]: the third home's receipt missing; the second alone cannot meet two of three */
	case_world(&w, "identity", 13, "b1d83b29", NULL);
	{
		const mor_chain *c = mor_chain_get(&w, idp("866ce6c0"));
		CHECK(c && c->npos == 2 && c->next == MOR_NEXT_CONTESTED, "home 1 conflicted, home 3 silent: contested at 2 (npos %zu next %s)",
		      c ? c->npos : 0, c ? mor_next_name(c->next) : "-");
	}
	mor_arena_reset(&A, mark);
	/* text[5] without the collective's genesis: unknown */
	case_world(&w, "text", 5, "f6c5debc", NULL);
	CHECK(mor_status(&w, idp("a5e29e6d"), 0) == MOR_ST_UNKNOWN, "a collective's text, its chain not held");
	mor_arena_reset(&A, mark);
	/* agreements[6]: without the revocation, the act is backed */
	case_world(&w, "agreements", 6, "26364aeb", NULL);
	CHECK(mor_backing(&w, idp("906a9643")) == MOR_BK_BACKED, "no revocation held: backed");
	mor_arena_reset(&A, mark);
	/* agreements[5] without the grantee's acceptance: not backed */
	case_world(&w, "agreements", 5, "10f113d6", NULL);
	CHECK(mor_backing(&w, idp("4eafa2ef")) == MOR_BK_NOT_BACKED, "a grant its grantee never accepted");
	mor_arena_reset(&A, mark);
	/* agreements[1] holding a third signature from a stranger changes nothing */
	case_world(&w, "agreements", 0, NULL, NULL);
	CHECK(mor_agreement_exists(&w, idp("8036edf3")) == MOR_AGR_NOT, "one party unsigned");
	mor_arena_reset(&A, mark);
	/* agreements[4] dropping the first holder's signature: one of two */
	case_world(&w, "agreements", 4, "5a502fa7", NULL);
	CHECK(mor_consent_counts(&w, idp("e0ec8805")) == 0, "one holder of two");
	mor_arena_reset(&A, mark);
	/* deadlines: rule [1, 1] (any one keeper) with one anchored record */
	{
		uint8_t k[2][32];
		const mor_cbor *r11, *r0;
		static const uint8_t b11[3] = { 0x82, 0x01, 0x01 }, b0[2] = { 0x81, 0x00 };
		size_t clen;
		const uint8_t *clock = hex_decode("8258204c8d82437c02a1ad62aeac389c2e110f7fd480d5db553f72fb55e1a31f3caee200", &clen, &A);
		memcpy(k[0], idp("ff49dada"), 32);
		memcpy(k[1], idp("46ce1d38"), 32);
		mor_cbor_decode(b11, 3, &A, &r11);
		mor_cbor_decode(b0, 2, &A, &r0);
		case_world(&w, "agreements", 14, NULL, NULL);
		mor_anchor an;
		memcpy(an.act, idp("00bf42cc"), 32);
		an.clock = clock;
		an.clock_len = clen;
		an.point = 95;
		w.anchors = &an;
		w.nanchors = 1;
		CHECK(mor_deadline(&w, idp("63bcbe53"), (const uint8_t(*)[32])k, 2, r0, clock, clen, 100) == MOR_DL_UNDETERMINED,
		      "one keeper of two showing it met, rule all: undetermined");
		CHECK(mor_deadline(&w, idp("63bcbe53"), (const uint8_t(*)[32])k, 2, r11, clock, clen, 100) == MOR_DL_MET_KEEPERS,
		      "one keeper of two showing it met, rule any one: met");
		an.point = 101;
		CHECK(mor_deadline(&w, idp("63bcbe53"), (const uint8_t(*)[32])k, 2, r11, clock, clen, 100) == MOR_DL_UNDETERMINED,
		      "a record of the act anchored past the point shows nothing");
		mor_arena_reset(&A, mark);
	}
}

static void test_mir_hashes(void)
{
	/* The six drafts' spec hashes as their files stand, status lines dropped. */
	static const char *files[] = { "spec/MIR-identity-draft-15.md", "spec/MIR-envelopes-draft-11.md",
				       "spec/MIR-text-draft-11.md", "spec/MIR-money-draft-11.md",
				       "spec/MIR-agreements-draft-17.md", "spec/MIR-development-draft-11.md" };
	for (size_t i = 0; i < 6; i++) {
		FILE *f = fopen(files[i], "rb");
		if (!f) {
			printf("note: %s not found (run from the repository root)\n", files[i]);
			continue;
		}
		static uint8_t buf[600000];
		size_t n = fread(buf, 1, sizeof buf, f);
		fclose(f);
		uint8_t h[32];
		const uint8_t *t;
		size_t tl;
		size_t mark = mor_arena_mark(&A);
		int r = mor_mir_hash(buf, n, h, 4, &A, &t, &tl);
		CHECK(r == 0, "%s reads as canonical text", files[i]);
		mor_arena_reset(&A, mark);
	}
}

int main(void)
{
	size_t cap = (size_t)512 << 20;
	void *mem = malloc(cap);
	if (!mem)
		return 2;
	mor_arena_init(&A, mem, cap);
	test_cbor();
	test_text();
	test_forms();

	/* the stand-in MIR hashes of the verdict vectors */
	FILE *f = fopen("vectors/verdicts.json", "rb");
	if (!f) {
		printf("run from the repository root\n");
		return 2;
	}
	static char text[1 << 21];
	size_t n = fread(text, 1, sizeof text, f);
	fclose(f);
	if (json_parse(text, n, &A, &ROOT))
		return 2;
	static const char *keys[MOR_NMIRS] = { "identity", "envelopes", "text", "money", "agreements", "development" };
	for (int i = 0; i < MOR_NMIRS; i++) {
		hex_fixed(json_str(json_get(json_get(ROOT, "mirs"), keys[i])), MIR[i], 32);
		tb_hex(MIR[i], 32, MIRH[i]);
	}
	tb_key_schnorr(&alice, "alice");
	tb_key_schnorr(&bob, "bob");
	tb_key_schnorr(&alice_signing2, "alice 2");
	tb_key_schnorr(&bob_signing2, "bob 2");
	tb_key_slh(&alice_chain, "alice chain");
	tb_key_slh(&alice_chain2, "alice chain 2");
	tb_key_slh(&bob_chain, "bob chain");
	genesis(&g_alice, &alice, &alice_chain);
	genesis(&g_bob, &bob, &bob_chain);
	test_envelope_rules();
	test_rotation();
	test_terms();
	test_variations();
	test_mir_hashes();
	printf("%d checks, %d failures\n", checks, failures);
	free(mem);
	return failures ? 1 : 0;
}
