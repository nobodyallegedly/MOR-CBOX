# Report

## In plain words

I built a verifier for MOR in C from the eight texts in `spec/` and nothing else. It is a library (`libmor.a`, the code in `src/`) and a command-line tool (`morv`). Given the bytes of acts, it decodes them strictly, opens them, checks their signatures, and answers the questions MOR's verdicts ask: is this act valid, invalid, unknown, pending, void, disputed, scoped or contested; which acts count on an identity chain and what stops it; does an agreement exist; is a deal forked; does a collective's act have its consent; is a grant key's act backed, binding, void or disputed; was a deadline met; what was paid toward an obligation; how many disagreements does a receiver show.

**Every published vector is answered, and every answer agrees: 285 of 285 checks**, in all eleven vector files (`RESULTS.md` lists every one). I stopped because every vector was answered, not because I could go no further. Parts of the protocol that no vector reaches are only partly built, or not built at all; they are listed at the end with the reason for each.

Where the texts left a choice, I picked a reading, wrote it down and carried on: 49 such questions are in `QUESTIONS.md`, each with the readings I weighed, what I chose and why, and which vectors (if any) settled it. Three are worth knowing first:

- **Content keys reused in the vectors (Q6).** Envelopes rule 9 says a content key must never lock two objects, but the published vectors reuse keys and nonces across acts. I read the rule as a duty of whoever locks, not a check a verifier makes, because enforcing it would make published verdicts fail.
- **X-Wing is defined outside the box (Q7).** Envelopes names an external draft (revision 11) for its post-quantum key exchange. I implemented it from my own knowledge, without fetching it. `sealed.json` fixes every input and confirms the result byte for byte.
- **Dishonest homes (Q13).** I count a home as proven dishonest only when its conflicting receipts survive its own rotations. Otherwise the position is contested and nothing more.

The draft MIR hashes, computed from the texts with Development rule 26 (Q46), are in `QUESTIONS.md`. They are not freeze hashes, and the vectors use stand-ins as their README says.

The manifest check passed at the start and at the end: `sha256sum -c MANIFEST.sha256` reported every file OK both times (see the end of this report). I never edited `spec/`, `vectors/` or `MANIFEST.sha256`.

I looked for nothing about MOR outside this repository, and I came across no other implementation of MOR. GitHub was used only to fetch the four pinned libraries' sources (below) and for this repository's branch and pull request.

## Memory discipline

**The library never calls malloc, calloc, realloc or free.** Every structure it builds is carved from an arena: a fixed byte region the caller owns and passes in. Decoded values point into the caller's input bytes or into that arena, and live as long as the shorter of the two. Scratch space is released in LIFO order by mark and reset. Running out of arena is an ordinary error, never a crash. The one exception is OpenSSL's EVP layer (SLH-DSA, ML-KEM, SHA-3), which allocates internally. It is confined to `src/crypto.c`, and every object it creates is freed before the call returns. The command-line tool allocates its arenas once at start and reads input files with one allocation each; the test builder (test code only) uses growable buffers; the fuzz targets use one static arena. The statement is at the top of `src/arena.h`. `grep` finds no allocator call in `src/` other than the EVP frees in `crypto.c`.

## Libraries, versions and why

All four are built statically into `deps/prefix` by `deps/build-deps.sh` (`make deps`). Each is pinned by its release tag and by the commit that tag must resolve to; the build stops on a mismatch.

| Library | Version (commit) | Used for | Why this one |
| --- | --- | --- | --- |
| OpenSSL | 3.5.9 (`45e844fa…`) | SHA-3/SHAKE, SLH-DSA-SHA2-128s/128f (FIPS 205, pure, context "MOR"), ML-KEM-768 (FIPS 203) | 3.5 is the LTS line and the first with FIPS 205 and 203 in the default provider. It is the most widely reviewed implementation of both. |
| libsodium | 1.0.21 (`d24faf56…`) | SHA-256, XChaCha20-Poly1305 (IETF), X25519 | A small, constant-time library that is the reference for XChaCha20-Poly1305. |
| libsecp256k1 | 0.7.1 (`1a53f496…`) | BIP-340 Schnorr verification (and signing, in tests only) | Bitcoin Core's library, the reference for BIP-340, with a static context that needs no allocation. |
| utf8proc | 2.11.3 (`e5e79922…`) | Unicode 17.0.0 normalization tables (NFC, Text rules 12-13) | The text pins Unicode 17.0. 2.11.3 ships exactly those tables, and the vector that only a verifier with 17.0 tables rejects passes. |

Everything that is MOR's own I wrote myself: deterministic CBOR, canonical text rules, tagged hashes, the act and its lock, the running summary, bech32m forms, the sealed container, the X-Wing combiner and key expansion, RFC 9162 inclusion proofs, and every layer's rules.

Toolchain: clang 18.1.3 (release, sanitizers, libFuzzer) and gcc 13.3 (`make gcc-check`), both with `-std=c17`. Warnings are errors, with a broad set turned on: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual -Wvla -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes -Wimplicit-fallthrough -Wswitch-enum -Wnull-dereference -Wdouble-promotion -Wundef -Wcomma -Wunreachable-code -Wmissing-variable-declarations` (with gcc: also `-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wjump-misses-init`). The whole code base builds clean under both compilers.

## Tests, sanitizers and fuzzing

**Unit tests** (`tests/unit.c`, 112 checks, all passing) cover:
- decoder edges for CBOR, text and forms;
- rules exercised on acts built and signed for the test, including SLH-DSA-128f rotations, a scheme-hash identity, rotations, terms and grants;
- variations of the published cases;
- the MIR-hash files.

**Sanitizers.** `make check` builds the library, the tests and the CLI with AddressSanitizer and UndefinedBehaviorSanitizer. That includes the `unsigned-integer-overflow`, `implicit-conversion`, `float-divide-by-zero` and `nullability` checks, with `-fno-sanitize-recover=all`. It then runs the 112 unit checks and all 285 vector checks. Result: clean, no reports, about 10 seconds in total. UBSan's unsigned-overflow check caught several `i-- > 0` loops early on; I rewrote them.

**Fuzzing** (libFuzzer, with ASan and UBSan). There is one target per decoder of untrusted bytes:
- `cbor`: the CBOR decoder, with a re-encode round trip;
- `text`: canonical text;
- `act`: act decode and open;
- `world`: a set of acts and the questions asked of it;
- `people`: the written-for-people forms;
- `sealed`: the sealed container;
- `spec`: specification decoding and the MIR hash;
- `json`: the vector-file parser.

A ninth, `engine`, is structure-aware. It builds well-formed acts from the fuzzer's bytes, with signature checks stubbed out in a separate build (`MOR_FUZZ_NOSIG`) so that the verdict logic past the signature is reachable. The corpora were seeded from the vectors (`fuzz/make-corpus.py`) and are kept in `fuzz/corpus/<target>/`. Every crash found is kept in `fuzz/regressions/` and replayed by `make regress`.

The campaign (`fuzz/run.sh 1800`, 4 cores, 22 October 2026, 07:39-09:09 UTC) ran each target for 30 minutes. World and engine are the slow targets: every input is a set of acts and their verdict logic, signatures and all. They then had a second 30-minute run each, with two workers (09:10-09:40 UTC). "Features" is libFuzzer's measure of distinct behaviour reached at the end of the run.

| Target | Time | Executions | Exec/s | Features | Corpus kept (minimised) | Crashes, leaks, timeouts |
| --- | --- | --- | --- | --- | --- | --- |
| `cbor` | 30 min | 61,962,208 | 34,404 | 1,134 | 438 | none |
| `text` | 30 min | 461,454,963 | 256,221 | 413 | 89 | none |
| `act` | 30 min | 45,979,510 | 25,529 | 1,835 | 374 | none |
| `world` | 30 min + 30 min (2 workers) | 762,735 + 2,159,784 | 423 / 391 | 5,551 / 5,805 | 662 | none |
| `engine` | 30 min + 30 min (2 workers) | 5,821,145 + 9,575,410 | 3,232 / 2,726 | 6,100 / 6,465 | 1,161 | none |
| `people` | 30 min | 59,538,104 | 33,058 | 501 | 160 | none |
| `sealed` | 30 min | 80,157,538 | 44,507 | 874 | 283 | none |
| `spec` | 30 min | 100,621,024 | 55,869 | 1,777 | 534 | none |
| `json` | 30 min | 6,490,720 | 3,603 | 1,261 | 408 | none |

In total that is about 835 million executions over 5.5 hours of fuzzing time, with no crash, leak, sanitizer report or timeout.

Two bugs were found earlier, in the short smoke runs made while I wrote the targets. Both are fixed and kept in `fuzz/regressions/`:
- `world-null-signer-consent`: asking a genesis for its consent led to a null chain lookup. Fixed with guards in `mor_chain_get` and the consent code.
- `engine-unformatted-obligation`: Money code read typed payload fields of an act that was not in its format. Fixed by computing, when the world is built, whether each act is in its MIR's format, and making every typed read require it (`mor_world_is_mir`).

The logs are not committed (`fuzz/logs/` is ignored), and the numbers above are copied from them. The `cbor` target's figures include its round trip through the encoder.

## What is not implemented, and why

The vectors reach the core pieces and a chosen set of verdicts. A great deal of the protocol is not covered by any vector. What follows is implemented only as far as described, or not at all. Where a rule needs a missing part, the verifier answers **unknown** rather than guessing.

**Identity**
- *Consistency proofs between log summaries* ("non-extending summaries stop the audit"): not implemented. Only inclusion proofs are checked. No vector hands a consistency proof.
- *Links* (claim, confirmation, termination; link proofs by cMIP): formats are checked; the link's standing, and display rules, are not.
- *Succession*: a rotation declaring a successor counts as a rotation (vector identity[11]). The confirmation by the successor and the "declared until confirmed" display are not built.
- *Routes and payee pointers as versioned chains*: formats and version heads are checked. Fork detection on those chains ("a fork contested") is not built.
- *Signature schemes named by hash*: answered unknown, as the text requires of a verifier without the scheme.
- *Witness acts and objections*: formats are checked, and acknowledgements count. Objections count in the homeless procedure only.

**Envelopes and Text**
- *Key delivery*: the format is checked. Whether a delivered key opens what it names is not checked.
- *Vows* (their own chain, forks), *publications* (media size against unlocked media, segmentation, withdrawal racing a payment) and *relays and inbox delivery*: formats only, where a format exists. Media bytes are never fetched.
- *Unknown types adopted by opt-in*: acts of specifications this verifier does not implement are judged as Envelopes says (unknown unless something checkable fails). There is no adoption mechanism.

**Money**
- Implemented: double entry (disagreements), paid toward, the signer checks of every Money format, and rail answers taken as facts.
- Not implemented: routes across rails, forwards and batches, flow and vault entries and limits, good-faith payment to an invalidated pointer, refunds and referrals. They need rail Modules' specifications and proofs that no vector supplies. A rail Module's kind (field 11) and binding (field 13) are checked only for presence (Development's rule for Modules under a payment cMIP).

**Agreements**
- Implemented: agreement exists, deal clones and marks, deal fork, keepers and keeper records, belongs-to (one level), collective consent (areas, member keys, done, the constitutional rule), grants and revocations (backing), deadlines, Text rule 14's decisions, and the format checks of every Agreements type and of terms.
- Not implemented:
  - stakes arithmetic beyond the format: splits, largest remainder, role shares, fees and the owners' matters;
  - the fork and closing of a collective, succession plans, abandonment declarations and contests, and liveness;
  - chains of judgment and judges' answers, offers (proven or promised), public-domain releases, and negotiation records;
  - agreements as parties of other agreements (nesting, loops);
  - records (type 17) putting operational clones in force at once (Q41);
  - following a deal's line to the version in force (Q36).

  These rules mostly act on long chains of chain signatures. The vectors' README says it leaves them out for that reason (megabytes of SLH-DSA signatures), so I could not check any reading of them against a published answer.

**Development**
- Implemented: specification decoding (steps 1-6, every kind), a Module under a payment cMIP, and MIR hashes.
- Not implemented: the RV32IM verification-rule profile (MOR 1 publishes none, as the suite says), extensions acting on layers, and proof checkers.

**Conformance rules** (what a client shows before signing, and so on) are a client's business, not a verifier's, and are not addressed.

## Manifest

`sha256sum -c MANIFEST.sha256` was run at the start of the work and again at the end (22 October 2026, 09:45 UTC, after the last commit to code). Both times all 22 files it lists were OK: `README.md`, `SNAPSHOT.md`, the eight texts in `spec/` and the twelve files in `vectors/`. `git status` shows no change to any of them.
