# Progress

Kept current so that a new session, started with the same prompt, can continue from here.

## State

- Branch: `claude/c-verifier-2`, made from `claude/c-verifier` (the first edition's verifier, pull request 1), with `main` (the box's second edition, snapshot of source commit `a68e7b1f`) merged in.
- Dependencies: `make deps` (runs `deps/build-deps.sh`) fetches and builds, pinned by tag and commit, OpenSSL 3.5.9, libsodium 1.0.21, libsecp256k1 0.7.1, utf8proc 2.11.3 into `deps/prefix` (not committed). Needs `libclang-rt-18-dev` for the sanitizers and libFuzzer (`apt-get install libclang-rt-18-dev`).
- Build: `make` (release, `build/rel/morv`), `make check` (ASan+UBSan: unit tests and every vector), `make fuzz` (libFuzzer targets in `build/fuzz/`).
- Run the vectors: `build/rel/morv vectors vectors` (summary), `--md` for the RESULTS.md table, `-v` for every case.

## Done

### First edition (branch `claude/c-verifier`)

- Library, CLI, unit tests, fuzz targets; 285/285 of the first edition's checks; 49 questions.

### Second edition (this branch)

1. Merged `main`, read the diff of the eight texts section by section (Agreements: "Belongs to" at any depth and the version in force for keepers, clone objects; Envelopes: client conformance, check order, rule 5's reach, rule 9 a client's duty; Identity: rotation check 7, "not counting", what stops a chain, scoped acknowledgements, rule 42; Money: rule 12 where the rail's answer is not held; Text: rule 14's client conformance, rule 13's order) and the vectors' "What changed".
2. First run against the new vectors: 279/334.
3. Canonical-text rules numbered 7 to 12; MIR hash over the whole text (status line kept).
4. Identity: not counting; seven stop reasons in the text's order; acknowledgements by scoped keys never rescue; private links opened with a held content key and valid only found at a home of their signer; succession (declared, confirmed and followed); a kept tip whose line is not held kept by its name.
5. Money rule 12: which payment from the rail's answer (`rail_payments`), never from bytes; answers judged both ways on one unknown pair, unknown on two or more; vow sales (pending, confirmed, contested, unsigned, unknown); refunds repaid.
6. Agreements: "belongs to" at any depth with unknown beyond an act not held; the keepers of the version in force; an agreement as a party (by its line, nesting bounded at 8); records putting operational clones in force at once; a collective's fork and closing (complete, not, unknown) and its keys counting for nothing after the line; a deal's version in force through its chain of judgment; a clone's objects naming its parent.
7. 334/334 of the second edition's checks agree. Unit tests rebased on the new ids, with variations of the heavy cases: 119/119. `make check` (ASan+UBSan) clean; `make gcc-check` clean.
8. Fuzz targets extended: the world and engine targets derive facts from their input (rail answers, which payment, lapses, out-of-turn settlements, private acts found) and ask every new question; corpora seeded from the new vectors.
9. `QUESTIONS.md`: 22 old questions marked as settled by the second edition (agreeing or not), new questions 50-65. `RESULTS.md` regenerated.

## Reading notes (where the verdict logic lives in the texts)

- Identity: genesis checks (l.111), everyday checks (l.169), receipt checks (l.289), rotation checks (l.367), which rotation counts (l.376), rules 33-35 kept ancestry (l.388-440), conflicting receipts (l.524), homeless/escape (l.570-618), audit (l.654), formats Part three (l.678+).
- Envelopes Part three (l.314+): act shape, sealed, running summary. Text rule 14 (l.59), text payload (l.127).
- Agreements formats Part four (l.2810+): terms 0, signature 1 ({0: act, ?1: signing-key}, objects [[signed, signed]]), keeper record 2 ({0: [act, signer]}), grant 9 / revocation 10 (l.3407), liveness 12, record 17 (l.3647), collectives in the identity chain (declaration kind 0, l.3787). Rules: 1 (exists when every party signed), 6, 33 (deadline, l.977), 36-42 (keepers, l.1057), 45-47 (clones, deal forks, l.1128-1305), 57-60 (done, two chains, areas, l.1527-1722), 65 (counting, l.1842), 72 (agreement in force, l.2104), 73-80 (grants, l.2131-2285), "Belongs to" (l.98).
- The verdict cases' acts can be dumped with `build/rel/morv pool vectors/verdicts.json`.

## Next

1. The fuzz campaign over every target (30 minutes each, started 17:24 UTC), then world and engine again; record it in REPORT.md; fix and keep any crash; minimise and commit the corpora.
2. REPORT.md for the second edition; final `sha256sum -c MANIFEST.sha256`; push; open the pull request.
