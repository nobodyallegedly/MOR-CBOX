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

## Next

1. Verdict layers for `verdicts.json` (63 cases): Identity (status, chain), Text rule 14, Envelopes (sequence shape, invalid beats unknown), Development (specification decoding, MIR hashes), Money (disagreements, backing, consent, paid toward), Agreements (agreement exists, deal fork, consent counts, backing, status after keepers, deadline).
2. Unit tests, sanitizer run, fuzz targets and runs.
3. QUESTIONS.md, RESULTS.md, REPORT.md; pull request.
