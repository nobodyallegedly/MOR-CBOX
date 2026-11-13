# Report: a verifier for Agreements draft 17, built from the text alone

**Memory discipline, kept throughout.** One arena for each set of acts. Everything the library decodes or works out lives in that arena and is freed in one call, `mor_set_free()`. Nothing is freed one piece at a time, nothing is reallocated, and the library keeps no pointer into bytes the caller gave it: input is copied in. Every count is bounded by a limit written in `include/mor.h`; a set that passes a limit gets a rejected record or the answer **unchecked**, never a silent cut. Running out of memory is the one failure that is not an answer: the arena calls `abort()`. The only `malloc` and `free` in the library are in the arena (`src/set.c`). Plug-ins are handed the set and allocate from the same arena.

## In plain words

**What was built.** A C17 library (`libmoragreements`) and a command-line tool (`mor-agreements`). You give them a set of acts: Terms and Statements, and beside them the stand-in records the text leans on (payments, settlements, reports, envelopes, a place's answer, a clock's answer, the words of a body). They give back the answers the text gives, grammar rule by grammar rule: **shown**, **not shown**, **fork**, **valid**, **not valid**, **invalid**, **unchecked**; each with its qualifier (**as shown**, **per place**, **per clock**, **per anchoring** with the anchoring named, **disputed**); and, of reports, the report block's words: **matched**, **mismatched**, **one side only**, **unanswered**, **no report called for**. Every answer carries a line of plain words naming the grammar rule it rests on.

**Where it stops.** Every grammar rule, G1 to G14, is built. None is skipped. What each one could and could not be taken to is in the table below; the thin places are thin because the text, or a layer it leans on, does not say, and each is written down in `QUESTIONS.md`.

**Was the text enough?** Enough to build every grammar rule: yes. Enough for two careful strangers to build verifiers that give the same answers: not yet. I had to choose a reading in 37 places, and in 24 of them the choice changes an answer (`QUESTIONS.md`, Part A). The four that matter most:

1. **A fork of Terms undoes itself as written** (Q1). A tip is a tip because of the assents to it (G4.7); G12.2 then says nothing built on a tip counts, which takes those assents away, and with them the fork. I read an assent to a tip as still counting for whether the tip was signed, and for nothing else.
2. **A delegator ending a delegation alone** (Q2, Q3). G5.5 lets the delegator's part alone replace the Terms; G12.1 also asks for what the new version's own `in_force` requires, which by default is the agent too. And nothing in the fields says that Terms are a delegation, who the delegator is, or what "narrows" a power: I took all of it from the body's reading (K1), which makes many answers **unchecked** for a verifier that cannot read the body.
3. **What a verifier answers for an act that breaks a MUST** (Q4). The text names one case of **invalid** (G14.3). For Terms that break G2.1, G3.1, G4.2 or G10.1, or an act that breaks G6.1, it names no answer. I answer **invalid**.
4. **Where Terms name the cMIP of a task** (Q12). G11.2 turns on Terms that "name a cMIP that answers" the Control task; no field does that. I looked in `relies_on`.

**What could not be built, and why.**

- **A path beyond one leg** (G11.6(a)). The text says a data cMIP gives one, and that "what such a cMIP is, which task it answers, and how Terms name it, are not said here". Nothing was invented: the verifier never reads a report as travelling beyond its leg, and answers **unchecked** for whether the signers agreed to such a path.
- **Whether what was passed on is what the Terms require** (G11.3). That is the K8 cMIP's to answer, and the text gives no shape for the answer. It is always **unchecked**; the leg reports themselves are read without any cMIP, as the text requires.
- **Real signatures.** The signature is a stand-in that proves nothing (`STANDINS.md`, S3). The task asked for no keys in the repository and the Sign layer is not in the box.
- **G12.8** is a duty of clients, and "no verifier can check it". The tool's own output keeps it (a tip is never shown as resolved), and a test checks that; nothing more can be done.
- **A time written inside an act** (G8.2): no field of section 3 holds one, so there is nothing to report.
- **Pools, the pro rata of one payment in a batch, a report that answers several calls** (P6): open in the text; not built.
- **K2, K3, K5, K7, K9**: no grammar rule asks them anything, so no stub was needed.

## Grammar rule by grammar rule

| Rule | Built | Notes |
|---|---|---|
| G1 An agreement as a party | yes | Named without signing; an agreement's signature under a power (`for`, `under`, the power's rule); one level; signing with no power by P17's least reading (Q11); receiver for an act, through agreements, with **not shown** for no receiver, an unfilled spot or an agreement met twice, and **unchecked** where acts are lacking. |
| G2 Whole | yes | Body and cMIPs must be fingerprints (**invalid** otherwise); anything named another way is not read and is counted as not part. |
| G3 Author | yes | `authors` checked; the K10 interface carries them. |
| G4 In force | yes | Default and `in_force` rule; replacement on signatures (G4.8), with **not shown** for one that lacks them; forks of Terms (G4.7); never "ended" (G4.9); counter signature, with a place or by citing (G4.10); the update condition, valid or not valid, named, with its ground and qualifier, **unchecked** without its cMIP (G4.11). |
| G5 Own signature | yes | Bound by own act or by a power; replacement under a power; `for`/`under` never assumed; acts under replaced Terms answered from the place or as shown; delegation by default, with conditions read from the body. Depth limit of 8 (Q22). |
| G6 Own line | yes | Lines by `previous`; forks; both tips stop. |
| G7 Place to look | yes | Acts count only once the place shows them; every answer qualified; the reader's own anchoring is used only where the agreement names none. |
| G8 Say who said it | yes | Every answer about the world carries who signed it: proofs, reports, settlements, the baker, a contest. Value is never shown as moved. |
| G9 Unchecked | yes | Missing cMIP, missing act, missing agreement, silent place: **unchecked**, carried to every answer that rests on it. Every stub can be marked absent. |
| G10 Spots | yes | Roles checked; fillings with proofs, through K12; whether a spot still takes new identities, with its cause; rights: the version a filling was earned under, and each later version that changes them, with what that rests on. No answer "triggered" or "no longer applies" exists. |
| G11 Legs of a sequence | yes, as far as the block goes | Whether Terms hold the Control task; the call; leg reports read with no cMIP; ties by settlement (**matched**, **mismatched**, **one side only**); silence (**unanswered** only for a side of the leg, otherwise **not shown**; **no report called for**); a payment's default call and its confirmation; a result read leg by leg; before paying; the contest. No path beyond a leg. |
| G12 Forks | yes | Both tips stop, with what is built on them; resolving updates for Terms and for a signer's line; a power to resolve; rival resolving updates; what strangers relied on stands, **disputed**. |
| G13 Now is what an act cites | yes | An act is read against the version it names; never as having relied on one it does not name; order from citing; anchoring. |
| G14 The baker | yes | Envelope and baker; (a) and (b); **invalid**, **not shown**, **unchecked**; the answer attached to receivers, fillings and leg reports that rest on those Terms. What passes the baker's rights is a stub (Q15). |

## Stand-ins

All in `STANDINS.md`, each with the sentence of section 8.1 it stands in for and the interface it sits behind. In one line each: acts are text records; a fingerprint is SHA-256 of a record's bytes; the "signature" is a keyless tag that proves nothing; a cMIP is an entry in a table, named by fingerprint; a place is a signed ordered list of fingerprints; a clock is a signed number; an amount is a whole number and a unit; an envelope is a record that names its baker; a report and a payment are records of their own.

## Plug-ins

Every task is a slot. `mor_plugins` is a table of cMIPs the caller hands in; a cMIP that is not in the table, or is marked absent, is one the verifier lacks, and every answer that needed it is **unchecked**. Stubs are shipped for K1, K4 (six rules), K8, K10, K11 (three kinds), K12 and K13; the place (K6) is a stand-in record. `mor-agreements --without <name>` and `--no-plugins` run the grammar without them.

## How to build and run

```
make            # library and tool, C17, -Wall -Wextra -Werror and more
make test       # tests under ASan and UBSan, the MUST coverage check, the tool end to end
make fuzz       # both fuzzers, FUZZ_SECONDS each (default 60)
make fuzz-replay   # replay the kept corpus and any kept crash

build/mor-agreements verify examples/click.mor
build/mor-agreements verify --reader sam --without k1.lines examples/click.mor
build/mor-agreements stubs
```

Needs a C17 compiler, `make`, `pkg-config`, `python3` (for the coverage check only), OpenSSL's libcrypto with headers, and clang with compiler-rt for the fuzzers.

**The one cryptographic primitive** is SHA-256, from OpenSSL libcrypto, pinned to version 3.0.13 (the `Makefile` refuses any other unless `CRYPTO_PIN` is set). The reason: it was already on the machine with its headers, it is the most widely reviewed implementation there is, and one function (`SHA256`) is all that is used, in one place (`mor_fp_of`, `src/set.c`). No cryptography was written by hand.

## Layout

```
include/mor.h      the public interface: acts, answers, plug-ins, questions
src/set.c          the arena, fingerprint and signature stand-ins, the set
src/decode.c       the decoder of untrusted bytes (the only one)
src/grammar.c      G2 to G7, G9, G12, G13: what counts, who is bound, which Terms are in force
src/legs.c         G1.4, G8, G10, G11, G14: receivers, spots, legs, reports, the contest, the baker
src/stubs.c        the stub cMIPs
src/show.c         what the tool prints
tools/main.c       the command-line tool
tests/             test.c (the cases), kit.h (builds acts), must_coverage.py, cli.sh
fuzz/              two libFuzzer harnesses, a dictionary, the corpus, crashes (none)
examples/          four sets to read with the tool
```

## Tests

No vectors were given, so the cases are written from the text: `tests/test.c`, 69 tests, 437 checks. Each test quotes the sentence it tests. `tests/must_coverage.py` takes every sentence of the two texts that holds MUST or MUST NOT (58, leaving out the three that only say how the word is used), and checks that a test quotes it, and that every quotation is word for word in the texts.

Result: **69 of 69 tests pass; 58 of 58 MUST sentences are cited; 91 quotations, all verbatim.** The tool's own end-to-end script (`tests/cli.sh`) passes.

A test passing means the code does what I read the sentence to say. Where my reading is one of several, the test fixes that reading and no more; `QUESTIONS.md` says which.

## Sanitizers

`make test` builds the library, the tests and the tool with `-fsanitize=address,undefined -fno-sanitize-recover=all` and runs them with leak detection on. **Clean: no report from AddressSanitizer, UndefinedBehaviorSanitizer or LeakSanitizer.** Compiled with gcc 13.3 under `-std=c17 -Wall -Wextra -Werror -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wconversion -pedantic`, and with clang 18 under `-Wall -Wextra -Werror` for the fuzzers: no warnings.

During the work the sanitizers found only mistakes in the tests themselves (a NULL passed to `strstr`, sets not freed), all fixed.

## Fuzzing

libFuzzer (clang 18), with ASan and UBSan, two harnesses:

- `fuzz/fuzz_decode.c`: raw bytes into the one decoder of untrusted bytes, then the whole grammar and every line the tool prints. Each input is run three times: as it is; with the stand-in signature check off, so that mutated records reach the field parsers and the grammar; and with no plug-in.
- `fuzz/fuzz_grammar.c`: a byte-level fuzzer cannot make one act name another (a fingerprint is a hash), so this harness reads its input as a small program that builds sealed acts naming one another (Terms, replacements, assents, counter signatures, resolving updates, claims, payments, reports, places, clocks), with stubs present or absent, then asks every question.

Final runs, on the code as committed, ten minutes each on one core, inputs up to 8 KiB (decoder) and 1 KiB (grammar), 10 second timeout:

| Harness | Runs | Crashes, sanitizer reports, leaks, timeouts | Edges covered | Corpus kept |
|---|---|---|---|---|
| `fuzz_decode` | 736,263 | none | 1,559 | 1,304 files, `fuzz/corpus/decode` |
| `fuzz_grammar` | 419,286 | none | 2,026 | 2,117 files, `fuzz/corpus/grammar` |

Earlier runs on earlier code (about eight minutes each, roughly 490,000 and 600,000 runs) also found nothing. The corpus was seeded from the sets the tests build, then minimised with `-merge=1`; `make fuzz-replay` replays it (clean). **No crash was ever found, so `fuzz/crashes/` holds none.** The fuzzers check memory safety, undefined behaviour, leaks and hangs. They do not check that an answer is the right one: only the tests do that.

One thing fuzzing did not find and reading did: an early version read forks of Terms in a fixed number of rounds, and a fork came and went from round to round (the matter of Q1). It is fixed, the rounds now run until two agree, and a set that never settles is reported as such (Q24).

## The manifest

`sha256sum -c MANIFEST.sha256`, at the start, before anything was written:

```
README.md: OK
SNAPSHOT.md: OK
spec/agreements-draft-17.md: OK
spec/report-block-2.md: OK
```

And at the end, after the last change, the same four lines: all **OK**. `git status` shows none of them modified.

`spec/`, `README.md`, `SNAPSHOT.md` and `MANIFEST.sha256` were not edited. Build instructions are here and not in `README.md` for that reason.

## The insulation

- The work was made on `claude/agreements-c-1`, from `agreements-box-1`. No other branch was opened, checked out, diffed or read. When the session started, the working copy was on another branch that the tooling had prepared; my first command listed branch names, fetched `agreements-box-1` and switched to it before listing or reading any file, so I saw two branch names (`main` and the tooling's own) and nothing of their contents.
- Nothing about MOR was looked for or read anywhere else: no web search, no other repository, no registry. The only network use was the system package manager, to install clang's sanitizer and fuzzer runtime (`libclang-rt-18-dev`).
- I met no other implementation or text of MOR.
- No personal name is written anywhere. The signer identifiers in tests and examples (`alice`, `bob`, `sam`, …) are placeholders and are nobody's names. No key and no server address exists in the repository: the signature stand-in has no key.

## Known limits

- **Rounds.** Forks and powers to resolve lean on one another; the verifier reads in rounds until two agree (at most six) and says so if they do not (Q24). No input found so far fails to settle.
- **Cost.** Work is polynomial in the size of the set and was not engineered further. With the limits as set (1024 records, 4 MiB) a set built to be slow can take long; the fuzzers ran with inputs up to 8 KiB.
- **Counter signatures without a place** are answered exactly for one counter signature per Terms act, and conservatively (**not shown** unless the order cannot matter) for more (Q5).
- **The arena only grows** until the set is freed; a long-lived set that is asked many questions keeps what each answer allocated.
- **The stubs are not cMIPs.** `k12.claim` in particular reads `max=` and `until=` only for whether a spot still takes new identities; it does not refuse a claim that came too late.
