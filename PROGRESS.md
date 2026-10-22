# Progress

Kept current so that a new session, started with the same prompt, can continue from here.

## State

- Branch: `claude/c-verifier`.
- Dependencies: `make deps` (runs `deps/build-deps.sh`) fetches and builds, pinned by tag and commit, OpenSSL 3.5.9, libsodium 1.0.21, libsecp256k1 0.7.1, utf8proc 2.11.3 into `deps/prefix` (not committed). Needs `libclang-rt-18-dev` for the sanitizers and libFuzzer (`apt-get install libclang-rt-18-dev`).
- Build: `make` (release, `build/rel/morv`), `make check` (ASan+UBSan: unit tests and every vector), `make fuzz` (libFuzzer targets in `build/fuzz/`).
- Run the vectors: `build/rel/morv vectors vectors` (summary), `--md` for the RESULTS.md table, `-v` for every case.

## Done

- Library (`src/`): arena; strict deterministic CBOR decoder and encoder; canonical text (utf8proc, Unicode 17.0.0); SHA-256, tagged hash; XChaCha20-Poly1305; BIP-340; SLH-DSA (pure, context `MOR`); X-Wing (ML-KEM-768 + X25519, SHA3 combiner); act decode, open and shape (Envelopes Part three); running summary (MMR); written-for-people forms (bech32m); sealed container.
- Vector files fully agreeing: tagged-hash, cbor, canonical-text, lock, running-summary, sequence-three-acts, open-act, textual-forms, signatures, sealed (222 checks).

## Reading notes (where the verdict logic lives in the texts)

- Identity: genesis checks (l.111), everyday checks (l.169), receipt checks (l.289), rotation checks (l.367), which rotation counts (l.376), rules 33-35 kept ancestry (l.388-440), conflicting receipts (l.524), homeless/escape (l.570-618), audit (l.654), formats Part three (l.678+).
- Envelopes Part three (l.314+): act shape, sealed, running summary. Text rule 14 (l.59), text payload (l.127).
- Agreements formats Part four (l.2810+): terms 0, signature 1 ({0: act, ?1: signing-key}, objects [[signed, signed]]), keeper record 2 ({0: [act, signer]}), grant 9 / revocation 10 (l.3407), liveness 12, record 17 (l.3647), collectives in the identity chain (declaration kind 0, l.3787). Rules: 1 (exists when every party signed), 6, 33 (deadline, l.977), 36-42 (keepers, l.1057), 45-47 (clones, deal forks, l.1128-1305), 57-60 (done, two chains, areas, l.1527-1722), 65 (counting, l.1842), 72 (agreement in force, l.2104), 73-80 (grants, l.2131-2285), "Belongs to" (l.98).
- The verdict cases' acts can be dumped with `build/rel/morv pool vectors/verdicts.json`.

## Next

1. Verdict layers for `verdicts.json` (63 cases): Identity (status, chain), Text rule 14, Envelopes (sequence shape, invalid beats unknown), Development (specification decoding, MIR hashes), Money (disagreements, backing, consent, paid toward), Agreements (agreement exists, deal fork, consent counts, backing, status after keepers, deadline).
2. Unit tests, sanitizer run, fuzz targets and runs.
3. QUESTIONS.md, RESULTS.md, REPORT.md; pull request.
