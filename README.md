# MOR Agreements: a small box, second edition

This branch holds a fixed snapshot of one layer of a protocol, MOR, and nothing else:

- `spec/agreements-draft-19.md`: the Agreements layer, a grammar of blocks for agreements between people. It defines two acts (Terms and Statement), the grammar rules G1 to G14 a verifier applies, and the tasks: the sockets where plug-in specifications (cMIPs) are named.
- `spec/report-block-2.md`: the report, a block the Agreements text relies on. It is a proposal for the shared language of all layers, not yet written into one.
- `MANIFEST.sha256`: the SHA-256 fingerprint of every file above. `SNAPSHOT.md` says where and when the snapshot was taken.

There are no test vectors. The layers this text leans on (the act format, fingerprints, signatures, places, clocks, the envelope, the settlement, how an amount is written) are not here: the Agreements text lists what it needs from them (sections 8.1 and 9). Where you need one, choose the simplest stand-in, say what you chose, and go on.

This branch shares no history with any other branch of this repository, and nothing on the others is part of this box.
