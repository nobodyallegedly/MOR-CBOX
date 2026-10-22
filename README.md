# MOR: the texts and the vectors

This repository holds a fixed snapshot of MOR's core, and nothing else:

- `spec/`: the eight texts of the core, as they stand before the freeze: the MICS document (the map), the six MIRs (Identity, Text, Envelopes, Money, Agreements, Development) and the freeze test suite.
- `vectors/`: published test vectors for every layer, with their own README.
- `MANIFEST.sha256`: the SHA-256 fingerprint of every file above. `SNAPSHOT.md` says when the snapshot was taken.

The texts define the protocol. The vectors are values an implementation of the texts should reproduce. Where the two seem to disagree, that is worth recording, not resolving silently.
