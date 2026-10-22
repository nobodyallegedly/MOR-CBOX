# Test vectors for MOR's core

Published test vectors for every layer of MOR's core: the pieces every act is made of, signatures, and verdict vectors (complete acts with the verdict each must get, and the rule that decides it). An implementation of the eight texts in `spec/` should reproduce every value here.

**Stand-ins until the freeze.** The six MIR hashes the acts name (`IDENTITY`, `ENVELOPES`, `TEXT`, `MONEY`, `AGREEMENTS`, `DEVELOPMENT`) are fixed only at the freeze, from the frozen texts (Development rule 26). Until then every vector uses stand-ins, SHA-256 of a label, listed in each file that uses them (`verdicts.json`, `"mirs"`). At the freeze the files are generated again with the real hashes; every act id changes, no verdict does.

**Test data.** Every key, seed and secret here is derived from a label and is test data, labelled so. None may be used for anything else.

All byte strings are lowercase hexadecimal.

## The files

| File | What it pins | Defined in |
| --- | --- | --- |
| `tagged-hash.json` | `tagged_hash(tag, x)` for each tag the core uses. | Identity, "Hashes" |
| `cbor.json` | Encodings that must be accepted (and re-encode to the same bytes), and encodings that must be rejected, one reason each; the 128-level nesting limit. | Identity, Part three, "Encoding" (RFC 8949 §4.2.1); Envelopes rule 1 |
| `canonical-text.json` | Strings as code points, whether each is canonical text, and the first rule it breaks; a string using two marks new in Unicode 17.0 that only a verifier with the pinned tables rejects. | Text rules 7 to 13 |
| `lock.json` | XChaCha20-Poly1305 with no associated data, and the locked hash. | Envelopes, "How it fits together" |
| `running-summary.json` | The running summary over 0 to 11 stand-in act ids. | Envelopes, "The running summary" |
| `sequence-three-acts.json` | Three public text acts in one sequence: each inside, its commitment, the locked bytes, the outside, the act id, the running summary each carries, and the summary including the third. | Envelopes rule 13, "The running summary"; Text, "Text act (type 0)" |
| `open-act.json` | Sealed acts, and the first check that fails when each is opened, or `ok`. | Envelopes rules 1 to 5 |
| `textual-forms.json` | Each label's short form (`mid`, `mact`, `mkey`, `mspec`, `mwork`) and its data; one in capitals; the where-it-lives form; bare hex; strings refused, each with why. | Identity, "Written for people" |
| `sealed.json` | A sealed container, `[to, locked-act]`, every input fixed, and what the recipient opens. | Envelopes, "Sealed container" |
| `signatures.json` | Schnorr over secp256k1 (BIP-340, scheme 1): the key from a fixed secret, the act id signed, signatures with a given and a zero `aux`, and nine signatures that must fail, one reason each. SLH-DSA-SHA2-128s and -128f (FIPS 205, schemes 2 and 3, pure, context `MOR`): keys from fixed seeds, the chain-key commitment, a deterministic and a hedged signature, and eight that must fail. | Identity, "Signature schemes", "Chain-key commitment" |
| `verdicts.json` | A pool of complete acts by id, and 63 cases, each with the acts a verifier holds, the facts it holds beside them, the question, the verdict and the rule that decides it. Listed below. | Each case names its rule |

## The verdict vectors

Each case in `verdicts.json` names the acts a verifier holds (complete bytes, in the pool), the facts that are not acts (inclusion proofs a home served; anchors the verifier checked; the rail's answer for a receipt or claim, which the core never reads itself), a question and its verdict. The questions are explained in the file's `about`. Every act is public.

| Layer | Cases | What they pin |
| --- | --- | --- |
| Identity | 14 | A genesis valid, and one signed by a key it does not declare. A rotation counting on its home's receipt; one not yet receipted, pending, and an act bound to it, pending. Two genuine rotations receipted by one home: contested. A disowned act void; disputed when another identity acknowledged it; disputed when its deal's keeper recorded it before the rotation, and void when the keeper is one no agreement it belongs to names. An escape. A succession declared. A receipt from an operator that is no home changes nothing. A home proven dishonest at a position, audited. |
| Text | 8 | Canonical and not. Text rule 14: two objects, another's chain, an individual's own chain, invalid; a collective's text naming its own decision, valid; a member key's text naming its version, counting; naming the signature act, invalid. |
| Envelopes | 5 | A sequence of two, valid; an act with no `prev`, and one with two entries, invalid. Invalid beats unknown: an act under a specification the verifier does not implement, unknown; the same with a broken signature, invalid. |
| Money | 6 | A receipt and a claim of one payment agreeing, and disagreeing. A payee's receipt under its grant key, backed. A collective's receipt by its Money area's member key, counting; by a member of no Money area, not. A transfer to one's vault naming the vault's declaration, valid, paying no obligation. |
| Agreements | 18 | A deal not signed by every party, and signed; a fork of a deal and its reference. A collective's act under its threshold, and meeting it. A grant key's act, backed; a revocation: an act it holds binds; one it misses, void; acknowledged by another identity, disputed; acknowledged by its grantee alone, void; recorded by a keeper of its deal before the revocation, disputed. A keeper's record before the rotation, disputed; after it, void. Deadlines: met by an anchor of the act; met by the keepers' anchored records; missed by their anchored silence; undetermined with no anchor; an anchor of the act winning over the keepers' silence. |
| Development | 7 | A payment cMIP; a rail Module under it with fields 11 and 13; without them, no specification; the same at a verifier not holding the cMIP, unknown; a request rail lacking the purpose; a cMIP naming no creator; a key beyond 13. |
| MIR hashes | 5 | A MIR's spec hash from its file: no final line break, one (not part of the text), a draft's status line dropped from the header; two final line breaks and a trailing space refused. |

**What no vector covers**, and why: proofs of a home's log handed to a verifier beyond the audit case; a private act (every vector act is public; a private act's opening is `open-act.json`); a collective's fork and closing, succession plans, issuance and the owners' matters (their acts are long chains of chain signatures, megabytes of SLH-DSA signatures); a rail's own proof (a rail Module's, outside the core); anchors as proofs (an anchoring cMIP's: the vectors state the anchor as a checked fact).

The act id does not depend on the signature, so every id here is final for its outside.
