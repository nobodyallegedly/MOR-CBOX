# Report: the C verifier, second edition

## In plain words

**Every vector of the box's second edition is answered, and every answer agrees: 334 of 334 checks**, in all eleven files, the 108 verdict cases included (`RESULTS.md` lists each one). I stopped because every vector was answered, not because I could go no further.

I started from my own first-edition verifier, which agreed with all 285 checks of the first edition. The second edition's texts changed in about forty lines across five MIRs, and its vectors grew from 63 verdict cases to 108. Run unchanged against the new vectors, the old verifier agreed with 279 of 334. Two kinds of change closed the gap.

**Where a text change made things easier.** Most of the edits settled questions I had answered by choice, and most settled them the way I had chosen:
- the order of the canonical-text rules and of an act's opening checks;
- every text string being canonical at any depth;
- content-key reuse being a client's duty that no verdict reads;
- a kept tip that does not fit keeping nothing without making the rotation invalid;
- an act bound to a contested rotation being pending.

Each now has a second-edition note in `QUESTIONS.md`. Fixing what the edition reversed was quick:
- the MIR hash now keeps the draft's status line, which I had dropped;
- a rotation that loses its position is now "not counting", where I had said void;
- an acknowledgement signed with a scoped key no longer rescues a disowned act;
- "same payment" now comes only from the rail's answer, never from the proof's bytes, which I had compared.

That last change made Money cleaner: the rail's answer is now a fact like any other.

**Where it made things harder.** Money rule 12's "judge both ways" is new in kind. An answer that rests on one pair of records whose payment the rail did not name is computed twice, once taking them as one payment and once as two, and stands only if both agree. Two such pairs make it unknown. A list, such as a vow's sales, keeps each entry found both ways and moves the rest to "unknown". To do this the code has to know which pairs an answer actually asked about, so every Money answer is now computed through one sameness check that records the unknown pairs it meets (Q52).

**The heavy rules, and what they asked of me.** These are the rules my first report listed as not built. The new vectors reach them, and they are now built:
- **"Belongs to" at any depth.** Each objects entry or answered act is one step, followed as far as the held acts allow, and unknown past an act not held. This turned a two-line lookup into a walk over the held acts. It also changed "status after keepers": an act whose keeper might speak for it, beyond an act not held, is now unknown rather than void (Q55).
- **The keepers of the version in force.** The latest complete version, on the named version's line, that the act's history holds. This needed the history walk I already had for revocations (Q56).
- **Records putting an operational clone in force at once.** For a collective's own key, the agreement in force is the declared one, moved along its line by records the act's history holds (Q57).
- **An agreement as a party.** A deal that is a party signs where every party of its version in force signs, recursively, bounded at eight layers, with a loop answering unknown (Q61).
- **A collective's fork and closing.** This was the largest piece:
  - the ending's chain act and every kept tip are checked against the collective's identity chain;
  - the agreement named must be the one in force at the line, records counted;
  - the members' chain signatures (SLH-DSA, on their own identity chains) are counted against the constitutional change rule;
  - a fork's successors must be founded with exactly their side's members;
  - the debts in the cited history must be handed out (a fork) or paid (a closing);
  - a history not wholly held answers unknown, never "not complete";
  - after the line, the collective's keys count for nothing (Q58, Q59).

  The texts here are long and exact, and I built the parts the vectors test plus the plain parts around them. What I left is listed below.
- **The chain of judgment.** A deal's fork is settled by the judge of forks, or by the next link once the time reference says the judge's period lapsed. Only the active link's settlement counts; one signed out of turn, or after its link's period, counts for nothing (Q60).
- **Succession confirmed, and private links found at home.** Smaller pieces (Q62, Q63).

`QUESTIONS.md` now holds 65 questions. 22 of the first 49 carry a note saying whether the second edition settled them and whether that agreed with my reading. 16 are new, numbered 50 to 65. Each says what it decided.

**What is still not built, and why.** The rules no vector reaches stay unbuilt. Where a rule needs a missing part, the verifier says unknown rather than guess:
- stakes arithmetic, splits and the owners' matters;
- succession plans and abandonment;
- offers, and routes across rails;
- consistency proofs between log summaries;
- the finer parts of endings: finality and ties among several endings, share-counted rules, kept holders' pay.

The list with reasons is at the end.

The manifest check passed at the start and at the end (see the last section). I never edited `spec/`, `vectors/` or `MANIFEST.sha256`. I looked for nothing about MOR outside this repository and came across no other implementation of it. GitHub was used only to fetch the four pinned libraries (in the first session) and for this repository's branch and pull request.

## Memory discipline

Unchanged from the first edition: **the library never calls malloc, calloc, realloc or free.** Every structure is carved from a caller-owned arena. Decoded values point into the caller's bytes or that arena. Scratch space is released in LIFO order by mark and reset, and running out of arena is an ordinary error. The only allocation the library does not own is inside OpenSSL's EVP layer (SLH-DSA, ML-KEM, SHA-3), confined to `src/crypto.c` and freed before each call returns. The new code follows it:
- Money's both-ways evaluation keeps its pairs in fixed arrays on the stack;
- "belongs to", the history walks and the ending checks take their queues from the arena and reset it;
- the vow answer lives in caller-provided or static storage.

The statement is at the top of `src/arena.h`. The command-line tool and the tests allocate their arenas once.

## Libraries, versions and why

Unchanged; built statically by `deps/build-deps.sh` (`make deps`), each pinned by release tag and by the commit that tag must resolve to.

| Library | Version (commit) | Used for |
| --- | --- | --- |
| OpenSSL | 3.5.9 (`45e844fa…`) | SHA-3/SHAKE, SLH-DSA-SHA2-128s/128f (FIPS 205, pure, context "MOR"), ML-KEM-768 (FIPS 203) |
| libsodium | 1.0.21 (`d24faf56…`) | SHA-256, XChaCha20-Poly1305, X25519 |
| libsecp256k1 | 0.7.1 (`1a53f496…`) | BIP-340 Schnorr |
| utf8proc | 2.11.3 (`e5e79922…`) | Unicode 17.0.0 normalization tables |

Why each: OpenSSL 3.5 is the LTS line and the first with FIPS 205 and 203 in its default provider. libsodium is the reference for XChaCha20-Poly1305. libsecp256k1 is the reference for BIP-340, with a static context that needs no allocation. utf8proc 2.11.3 ships exactly the Unicode 17.0 tables the text pins. Everything that is MOR's own I wrote myself.

Toolchain: clang 18.1.3 (release, sanitizers, libFuzzer) and gcc 13.3 (`make gcc-check`), `-std=c17`, warnings as errors with a broad set (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual -Wvla -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes -Wimplicit-fallthrough -Wswitch-enum -Wnull-dereference -Wdouble-promotion -Wundef -Wcomma -Wunreachable-code -Wmissing-variable-declarations`; gcc also `-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wjump-misses-init`). The whole code base builds clean under both.

## Tests, sanitizers and fuzzing

**Unit tests** (`tests/unit.c`, 119 checks, all passing). The first edition's tests are rebased on the second edition's acts, whose ids all changed (mapped by their place in each case). There are seven new checks, made by holding back one act from a heavy case:
- a closing whose kept tip is not held: unknown;
- the same act with no closing held: it counts;
- a judge's settlement naming a request not held, and no settlement held: the reference;
- a succession whose confirming identity's chain is not held: unknown;
- a refund with and without its repayment: 300 and 0.

**Sanitizers.** `make check` builds the library, the tests and the CLI with AddressSanitizer and UndefinedBehaviorSanitizer (with `unsigned-integer-overflow`, `implicit-conversion`, `float-divide-by-zero`, `nullability`, and `-fno-sanitize-recover=all`). It runs the 119 unit checks and all 334 vector checks. Result: **clean**, no reports, about 10 seconds. `make gcc-check` is also clean, and the gcc build agrees on all 334.

**Fuzzing** (libFuzzer, with ASan and UBSan). The same nine targets cover every decoder of untrusted bytes: CBOR, canonical text, act decode and open, a world of acts and its questions, the written-for-people forms, the sealed container, specification decoding and the MIR hash, the vector-file JSON, and the structure-aware engine (signatures stubbed out, so the logic past them is reached). New code is fuzzed through the world and engine targets:
- they now build the facts the second edition added from the input itself: a valid rail answer for each record, which payment a proof is for some records but not others (so pairs of unknown sameness arise), judges' lapsed periods, out-of-turn settlements, and private acts found at their signer's home;
- they ask every new question about every act: succession, fork and closing, a deal's version in force, a vow's sales, refunds repaid, and the status questions through "belongs to", records and endings.

The corpora were seeded again from the second edition's vectors, are kept in `fuzz/corpus/<target>/` (minimised), and every crash is kept in `fuzz/regressions/` and replayed by `make regress`.

The campaign ran on 22 October 2026, on 4 cores:
- `fuzz/run.sh 1800`, 17:24-18:54 UTC: each target for 30 minutes;
- 18:54-19:24 UTC: world and engine, the slow targets whose inputs are whole sets of acts, for a second 30 minutes each, with two workers, on the final build.

The first round ran on a build from before my last two small changes (a kept tip whose line is not held is kept by its name; a clone's objects name its parent). The second round of world and engine ran on the final code. "Features" is libFuzzer's measure of distinct behaviour reached.

| Target | Time | Executions | Exec/s | Features | Corpus kept (minimised) | Crashes, leaks, timeouts |
| --- | --- | --- | --- | --- | --- | --- |
| `cbor` | 30 min | 180,796,285 | 100,386 | 1,138 | 441 | none |
| `text` | 30 min | 645,674,345 | 358,508 | 415 | 88 | none |
| `act` | 30 min | 48,700,819 | 27,040 | 2,050 | 424 | none |
| `world` | 30 min + 30 min (2 workers) | 878,836 + 1,626,407 | 487 / 386 | 8,170 / 8,316 | 901 | none |
| `engine` | 30 min + 30 min (2 workers) | 5,327,732 + 9,858,185 | 2,958 / 2,799 | 7,610 / 7,875 | 1,343 | none |
| `people` | 30 min | 99,980,316 | 55,513 | 501 | 151 | none |
| `sealed` | 30 min | 81,242,131 | 45,109 | 875 | 292 | none |
| `spec` | 30 min | 146,775,832 | 81,496 | 1,778 | 521 | none |
| `json` | 30 min | 14,010,657 | 7,779 | 1,271 | 428 | none |

In total: about 1.23 billion executions over 5.5 target-hours, with no crash, leak, sanitizer report or timeout. The world and engine targets reached more features than in the first edition's campaign (8,316 against 5,805, and 7,875 against 6,465): that is the new code (Money's both-ways rule, "belongs to" at any depth, endings, the chain of judgment) being reached.

The two crashes found in the first edition's smoke runs stay in `fuzz/regressions/` and still pass (`make regress`). The first edition's campaign (about 835 million executions, nothing found) is in that edition's report on `claude/c-verifier`.

## What is not implemented, and why

The vectors reach the core and a chosen set of verdicts. What follows is built only as far as described, or not at all. Where a rule needs a missing part, the verifier answers **unknown** rather than guessing.

**Identity**
- *Consistency proofs between log summaries* ("non-extending summaries stop the audit"): not built. No vector hands one; only inclusion proofs are checked.
- *Links*: formats are checked, and private links must be found at a home. The standing of a link (claim plus confirmation, ended by either) is not used by any verdict here.
- *Routes and payee pointers as versioned chains*: formats and version heads only; fork detection on those chains is not built.
- *Contests among final homeless rotations* in the stop order (Q64): not modelled apart from contests under the home rule.
- *Signature schemes named by hash*: unknown, as the text requires.

**Envelopes and Text**
- Key delivery (whether a delivered key opens what it names), vows' own chains and forks, publications' media, relays: formats only. A vow's sales are built (Money below).

**Money**
- Built: double entry, paid toward, which payment from the rail's answer with the both-ways rule, a vow's sales, refunds repaid, and every Money format's signer checks.
- Not built: routes across rails, forwards and batches, flow and vault entries and limits (including the vault limit on a refund paid to the payer's refund rail), good-faith payment to an invalidated pointer, refunds to a bare key, referrals. They need rail Modules' specifications and proofs that no vector supplies.

**Agreements**
- Built:
  - agreement exists, including an agreement as a party;
  - deal clones and marks, deal fork, and a deal's version in force through its chain of judgment;
  - keepers and keeper records, "belongs to" at any depth, and the version in force for keepers;
  - collective consent: areas, member keys, done, records putting clones in force, and keys after an ending;
  - grants and revocations, deadlines, Text rule 14's decisions;
  - a collective's fork and closing (Q58);
  - the format of every Agreements type and of terms.
- Not built:
  - stakes arithmetic: splits, largest remainder, role shares, fees, and the owners' matters (rules 94 to 97);
  - succession plans, abandonment declarations and contests, liveness, resignations' effect on voices;
  - offers (proven or promised) beyond a vow's sales, public-domain releases, and negotiation records;
  - in endings: departed voices, share-counted rules (unknown), kept holders' pay compared, successors' signatures on assigned debts, creditor releases, money owed back left open, a broken collective, finality and ties among several endings;
  - in the chain of judgment: a judge contradicting itself, a settlement having to name every tip it saw, and competing parties' settlements;
  - in records: whether the signatures a record names meet the clone's mark (rule 65).

  The vectors' README says it leaves succession plans, issuance and the owners' matters out because their acts are long chains of SLH-DSA chain signatures; I could check no reading of them against a published answer.

**Development**
- Built: specification decoding, a Module under a payment cMIP, MIR hashes (`morv mir-hash`).
- Not built: the RV32IM verification-rule profile (MOR 1 publishes none), extensions acting on layers, proof checkers.

**Conformance rules** (what a client shows or refuses before signing) are a client's business; the second edition now marks them as such ("*Client conformance*"), and no verdict reads them.

## Manifest

`sha256sum -c MANIFEST.sha256` was run at the start of this session, right after merging `main` (the second edition), and again at the end (22 October 2026, 19:25 UTC, after the last change to code and corpora). Both times all 22 files it lists were OK: `README.md`, `SNAPSHOT.md`, the eight texts in `spec/` and the twelve files in `vectors/`. `git diff origin/main` on those paths is empty.
