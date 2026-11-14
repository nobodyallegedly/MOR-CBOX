# Report: a verifier for the Agreements layer, built from the text alone

For Nobody, allegedly. Branch `claude/agreements-c-2`, made from `agreements-box-2`.

**Memory discipline, kept throughout.** One arena for each verifier context. Every byte the library keeps (decoded acts, interned strings, findings, scratch lists) is copied into that context's arena. Nothing is freed one by one. `mor_free()` releases the whole arena at once. The library holds no pointer into the caller's bytes after a call returns, and has no global state that changes. If memory runs out, or a limit on untrusted input is passed, the call unwinds to its public entry and returns an error; nothing leaks, because everything is in the arena. The arena is capped at 512 MB, a line at 4096 bytes, a context at 20 000 records. The tool and the tests use `malloc` only to read a file into a buffer, which they free themselves.

---

## 1. In plain words

**What this is.** A library (`build/libmor.a`, header `include/mor.h`) and a command-line tool (`build/morv`) in C11. They take a set of acts and give, grammar rule by grammar rule, the answers the Agreements text gives: shown, not shown, fork, valid, not valid, invalid, unchecked; the qualifiers per place, per clock, per anchoring, as shown, disputed; the report block's words matched, mismatched, one side only, unanswered, no report called for; and rejected for what is no act. Every answer names the grammar rule it comes from and says who said what it rests on.

**How far it goes.** Every grammar rule, G1 to G14, is built. Three of them are built with a stated limit (section 3 below). All eight tasks, K1 to K8, are sockets with a stub behind each, and the grammar runs with a stub's answer, including **unchecked** where no cMIP is held. I stopped because every grammar rule is built, not because I could go no further.

**What it was built from.** `spec/agreements-draft-19.md` and `spec/report-block-2.md`, and nothing else. There were no vectors, so the 78 test cases are my own reading of the text. They show that the code does what I read. They cannot show that I read it right. `QUESTIONS.md` lists every place where I had to choose: **33 of those choices change an answer a verifier gives.** That list is the real result of this work.

**Is the text enough for a stranger?** Mostly yes for the grammar itself: who is bound, what is in force, forks, receivers. Each of those could be built from the sentences as written, and the rules agree with one another far more often than not. It is not enough in three kinds of place:

1. **Where it leans on a layer that is not there.** Sixteen stand-ins were needed (`STANDINS.md`). The ones that shape answers most: what a place answers, how a report names its call, how two halves of a payment are known to be the same payment, and what in a body says "this is a delegation" or "this passes the baker's rights".
2. **Where two sentences pull apart.** The clearest: a resolving update without `continues` is "required" by section 3 and "resolves nothing" by G12.3 (question A1); "invalid" for Terms of a baked work in section 2 against G14.3 (A2); the block's "a report that names no call is no report" against the buyer's confirmation (A25).
3. **Where it is silent on a case a verifier meets at once.** Which place judges the first Terms act (A3); what a late rival to an old version does (A6); which acts of an agreement's several signers are one signing (A14); who the receiving side is when only one report is held (A22).

---

## 2. What was built

| Part | File | What it does |
|---|---|---|
| Arena, interning, findings, entry points | `src/core.c` | the memory discipline; the list of answers |
| Stand-ins for fingerprint and signature | `src/standin.c` | SHA-256 (OpenSSL); a keyless tag as "signature"; sealing a record |
| Decoder of untrusted bytes | `src/decode.c` | the text-record format; required fields (rejected); what an act shows by itself (invalid: G2.1, G3.1, G4.2, G10.1) |
| The eight sockets | `src/stubs.c` | one function per task K1 to K8, each with stub behaviours |
| The grammar's core | `src/engine.c` | lines and forks (G6, G12); versions, the update condition and what is in force (G4); assents, counter signatures, powers, who is bound (G1.2, G5); the place (G7); order from citing (G13.3); the receiver for an act (G1.4); where a verifier stops (G9.2) |
| What is read from it | `src/report.c` | Statements as read; spots, fillings and rights (G10); the Control task, calls, leg reports, ties, silence (G11); the buyer's leg (G11.7); the contest (G11.8); the baker and the same work baked again (G14); the questions a caller asks |
| Drafts | `src/draft.c` | writing acts by hand with labels (a convenience) |
| Tool | `tools/morv.c` | `morv verify`, `morv seal`, `morv fp` |
| Tests | `tests/test_mor.c`, `tests/cli.sh`, `tests/MUSTS.md` | 78 cases, 959 checks; the table of the 65 MUST and MUST NOT sentences |
| Fuzzing | `fuzz/` | three libFuzzer harnesses, seeds, corpus, and the one input a fuzzer stopped on |

**Grammar rule by grammar rule.**

| Rule | Built | Notes |
|---|---|---|
| G1 An agreement as a party | yes | named and not bound; an agreement's signature under a power, or by all its signers (for assents: A14); one level; the receiver for an act through agreements, with every not shown and unchecked case of G1.4 |
| G2 Whole | yes | invalid, and left unread, as G2.2 separates them |
| G3 Author | yes | G3.1 invalid; G3.2 is a duty of cMIPs: `authors` is handed to every power's rule |
| G4 In force | yes | the default and the `in_force` task; replacing by the update condition alone or the default; not shown against not valid; G4.8(e); never ended; the counter signature by place and by citing; no "locked" |
| G5 Own signature | yes | G5.1(a), (b), (c); no power for replacement; G5.3; G5.4 by place and by citing; the delegator's default and the agent-only condition; G5.6 as what a verifier does not read |
| G6 Own line | yes | invalid previous; unchecked before an act not held; fork |
| G7 Place to look | yes | per place; contradiction and no answer are unchecked; one place for every reader; the clock |
| G8 Say who said it | yes | every answer about the world names its signer; value is never shown as moved |
| G9 Unchecked is not passed | yes | lacking cMIP, act, agreement, place answer; where a verifier stops, with how far; a reading that does not settle |
| G10 Spots | yes | the five kinds; fillings with proof and confirmer; still open, with cause; rights and later versions under (a) and (b); never "triggered" |
| G11 Legs of a sequence | yes, with limit 1 | the Control task with its socket filled or empty; the call; leg reports readable without cMIPs; ties; silence by who reads; before paying; one leg; the data task; the buyer's leg; the contest |
| G12 Forks | yes, with limit 2 | both tips stop; built on a tip; resolving update, by default, by condition, under a power, in a line and for Terms; rival resolving updates, to any depth; set aside; disputed |
| G13 Now is what an act cites | yes | read against what is named; order from citing with no qualifier; anchoring |
| G14 The baker | yes, with limit 3 | (a) and (b); invalid, not shown, unchecked as G14.3 separates them; the answer given with a receiver, a filling and a leg report; the same work shown beside, never picked |

---

## 3. What I could not build, and why

1. **Reading across many legs (G11.4) is built one path deep and simply.** `mor_ask_sequence` reads the buyer's leg, then the legs a doer reports, then the legs under the Terms of each agreement passed on to, and stops at a silent leg. It does not handle pools (P6), one report for many payments, or money shown as still owed and later paid (P20): the text leaves all three open, and there is nothing to build them from.
2. **Disputed (G12.5) is built for an act that rests on the tip set aside itself.** A payment that names a version *further along* a branch set aside is answered not shown: reading it "as if the tip had stood alone" needs the whole branch read again as if there had been no fork (question A30).
3. **The baker's succession (G14.2(b)) rests on a stand-in.** The text defines no act that passes the baker's rights (P18). I read it from the earlier agreement's body through its cMIP (a line `passes`), which is my invention. A baker who reaches Terms only by filling a role is always answered not shown (A28).
4. **An agreement that signs by all its signers** is built for assents only (A14). For another kind of act the answer is unchecked.
5. **Nothing was built for** a judge, a tally, identity, assurance, delivery or standing (section 5.3: modules of cMIPs, named by no field), for a payment that is pending, or for G12.8, which is a client's duty that leaves no act.
6. **The signature proves nothing** (stand-in S3). Every answer of this verifier about who signed is only as good as the Sign layer that will replace it.

---

## 4. Stand-ins

Sixteen, each behind one small interface, each with the sentence of section 8.1 or 9 it stands in for: see `STANDINS.md`. In short: a text record as the act; SHA-256 as the one fingerprint; a keyless tag as the signature; a signed list as the place; one number as the clock; a `cmip` document with a named stub behaviour, and a function per socket; a `report` record of its own; opaque payment and settlement records with a whole-number amount; an envelope that names a baker and a work; a `words` record with lines for machines; options for who is reading and which anchoring the reader accepts.

**The one cryptographic primitive is SHA-256, from OpenSSL's libcrypto (EVP interface).** Reason: it is the established, audited implementation already on the system, and writing a hash by hand was ruled out by the floor. Pinned: the code refuses to compile against anything but the OpenSSL 3 series (`src/standin.c`), and was built and tested against **OpenSSL 3.0.13** (Ubuntu package `libssl-dev 3.0.13-0ubuntu3.16`). Nothing else from the library is used.

---

## 5. Build, sanitizers, fuzzing

**Build.** `make` (library and tool), `make test`, `make fuzzers`, `make fuzz`. C11 (`-std=c11`), with `-Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes`. It builds clean with gcc 13.3.0 and clang 18.1.3.

**Tests under AddressSanitizer and UndefinedBehaviorSanitizer.** `make test` builds the test program and the tool with `-fsanitize=address,undefined -fno-sanitize-recover=all` and runs them with leak detection on.

| Run | Result |
|---|---|
| gcc 13.3.0, ASan + UBSan (`make test-gcc`) | 78 cases, 959 checks, 0 failures; no sanitizer report, no leak |
| clang 18.1.3, ASan + UBSan (`make test-clang`) | the same |
| `tests/cli.sh` (the tool end to end, random bytes included) | ok with both |

One sanitizer report was found and fixed during the work, and it was in the tests, not the library: a draft that held `%n` was passed to `printf` as a format. The marker for a made-up fingerprint was changed from `%` to `~`.

**Fuzzing.** libFuzzer (clang 18.1.3), each harness built with ASan and UBSan. Every decoder of untrusted bytes has a harness:

| Harness | What it feeds | Seeds |
|---|---|---|
| `fuzz/fuzz_decode.c` | raw bytes into the act decoder, then every grammar rule and every question over what was taken. The first byte of the input chooses the reader, the anchoring, and whether the stand-in signature is checked, so that the fuzzer can get past the tag without computing SHA-256. | the sealed acts of every test case and the example (157 files) |
| `fuzz/fuzz_draft.c` | raw bytes into the draft assembler (labels, sealing), then the grammar rules | the drafts of every test case and the example (158 files) |
| `fuzz/fuzz_engine.c` | structure-aware: the bytes are a small program that writes Terms, Statements, reports, payments, places, clocks and envelopes that name one another by label, so that acts with real fingerprints that really cite each other reach the grammar: forks, rival resolving updates, powers through powers, receivers through agreements. Then every question is asked of every act. | 9 small files |

The runs, in order (each harness on one core, `-timeout=20 -rss_limit_mb=2048`):

| Run | Code | decode | draft | engine | Found |
|---|---|---|---|---|---|
| 1 | first complete build | 2 636 929 runs, 901 s | 2 684 878 runs, 901 s | 402 337 runs, 901 s | nothing |
| 2 | after the last reading changes | 1 840 996 runs, 601 s | 1 502 713 runs, 601 s | 219 408 runs, 601 s | nothing |
| 3 | the same | | | stopped after about 150 s | **one input that did not end within 20 seconds** |
| 4 | after the first fix (a limit on the work) | 545 480 runs, 601 s | 158 418 runs, 602 s | 193 789 runs, 601 s | nothing new; but the runs were slow |
| 5 | **the code handed back** (the reading made lighter) | 1 115 664 runs, 421 s | 695 609 runs, 421 s | 139 284 runs, 421 s | nothing |

**No memory error, no undefined behaviour and no leak was found in any run.**

**The one finding.** The structure-aware harness wrote two signers who assent and cancel their assent many times over, with no place and nothing naming anything. Reading one counter signature (G4.10) asks whether the Terms were in force before it, which asks about the other assents and their counter signatures, and so on: the work doubled with every round, and the verifier did not answer in 20 seconds. It was a hang, not a memory fault. Fixed in two steps: the verifier now stops after a set number of such readings and answers unchecked, saying so (G9.2; `QUESTIONS.md`, B13); and the reading itself was made lighter (an assent is read against the nearest counter signature after it, and each reading is kept; `QUESTIONS.md`, A9). The input is kept as `fuzz/crashes/engine/timeout-1e70b5ea0fb79e7eeefcef12e8f4b679af3cd6e5`; it now runs in 23 ms, `make fuzz-regress` runs it again, and the test case `g9_2_counter_work` builds the same shape by hand.

What is kept: the corpus the three harnesses arrived at, minimised with `-merge=1` (2426, 2303 and 1840 inputs), in `fuzz/corpus.tar.gz` (`make fuzz` unpacks it and goes on from it); the hand-made seeds of the engine harness in `fuzz/seeds/engine/` (the seeds of the other two are made from the test cases by `make fuzz`); and `fuzz/crashes/`, which holds the one input above and nothing else.

**What fuzzing does not show.** It shows the code does not break memory or loop on hostile input. It says nothing about whether an answer is the right one.

---

## 6. The manifest, at the start and at the end

At the start, on a fresh checkout of `agreements-box-2`:

```
$ sha256sum -c MANIFEST.sha256
README.md: OK
SNAPSHOT.md: OK
spec/agreements-draft-19.md: OK
spec/report-block-2.md: OK
```

At the end, on the commit handed back:

```
$ sha256sum -c MANIFEST.sha256
README.md: OK
SNAPSHOT.md: OK
spec/agreements-draft-19.md: OK
spec/report-block-2.md: OK
```

`spec/`, `README.md`, `SNAPSHOT.md` and `MANIFEST.sha256` were never edited; `git diff --stat agreements-box-2 -- spec README.md SNAPSHOT.md MANIFEST.sha256` is empty.

---

## 7. The insulation

I did not look for MOR anywhere outside the box: no web search, no other repository, no registry. I read no file of any other branch.

**Two things to report.**

1. **What I saw by accident.** The session began checked out on a working branch that had been made from `main`, not from `agreements-box-2`. Before I knew that, I listed the top of the tree and the last three commit subjects. I saw that the tree held a `vectors` directory and a `spec` directory beside `README.md`, `SNAPSHOT.md` and `MANIFEST.sha256`, and I saw the three subject lines (they speak of "eight freeze-ready texts and the published vectors"). I opened no file there, read no vector and no text, and took nothing from it: I fetched `agreements-box-2` by name, made my branch from it, and worked only there. I later listed the *names* of remote branches matching `claude/agreements-c-2*` and `agreements-box-2`, to be sure I would overwrite no one's branch; I read nothing of them.
2. **What the box itself tells a builder about an earlier build.** The text mentions "the C session" several times: that its verifier follows eight levels (P2), that it used sixteen stand-ins, text records and one hash, a signed list as the place, one signed number as the clock, a table for its stubs (section 9). That is inside the sealed text, so I could not avoid reading it, and some of my stand-ins are the same simple choices. I saw none of that build's code, files or answers.

---

## 8. Using the tool

```
make
build/morv seal examples/song.draft > song.acts     # write acts by hand, with labels
build/morv verify song.acts                         # every answer, grammar rule by grammar rule
build/morv verify --reader @ben song.acts           # read as a side of a leg (G11.4(d))
build/morv verify --anchoring-place place:x ...     # the anchoring the reader accepts (G13.4)
build/morv verify --in-force FP --receiver ROOT[:VERSION] --bound WHO:ACT --order A:B \
                  --data-path WHO:REPORT --before-paying ROOT --sequence PAYMENT  FILE...
```

One line is printed for each answer: the grammar rule, what the answer is about, the fingerprints, the answer, its qualifiers, and the reason with who said it. For example, from `examples/song.acts`:

```
[G4.7] replaced by 9679f40d8573: fork, as shown -- two or more updates of the same Terms, each signed by all who needed to sign it: tips f678b1f71bd1, 2cdb0678f172; no tip is chosen, by position or in any other way; both tips stop and nothing built on either counts (G12.2)
[G11.4] tie 4f6f0bf86164 / 9679f40d8573: mismatched, as shown, fork shown -- the doer @ada states 49 EUR passed on to @ben; the receiving side @ben states 48 EUR received; both are shown and neither is chosen; what Money counts toward a debt is Money's answer, not this text's
```

The library's interface is `include/mor.h`: `mor_new`, `mor_load`, `mor_verify`, the list of findings, and seven questions (`mor_ask_receiver`, `mor_ask_bound`, `mor_ask_in_force`, `mor_ask_order`, `mor_ask_data_path`, `mor_ask_before_paying`, `mor_ask_sequence`).
