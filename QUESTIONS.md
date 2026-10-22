# Questions

Every place where the texts left me a choice: where it is, what the question was, the readings I weighed, what I chose and why, and which vectors (if any) decided it. "Line" numbers are those of the files in `spec/` as committed. Abbreviations: **Id** = MIR Identity d15, **Env** = Envelopes d11, **Txt** = Text d11, **Mon** = Money d11, **Agr** = Agreements d17, **Dev** = Development d11, **Suite** = freeze test suite v30.

The second edition of the box settled many of these. Each question it settled carries a **Second edition** note: settled by the text or by a vector, and whether that agrees with the reading I had chosen. Questions 50 onwards are new with the second edition.

A "decided by vectors" entry means a published vector fails under the other reading; "consistent with vectors" means the vectors pass under my reading but would also pass under the other one, or test nothing there.

---

## Encoding, text and the act

**1. The order in which canonical-text rules are checked.**
Source: Txt rules 7 to 12 ("canonical text"); `canonical-text.json` gives "the first rule it breaks".
Question: a string can break several rules at once (for instance invalid UTF-8 and a control character); which one is "first"?
Readings: (a) text order, rule 7 then 8 ... 12; (b) severity, UTF-8 validity before anything else.
Choice: (a), the rules in the order the text numbers them, checking each over the whole string before the next. It is the plain reading of "the first rule it breaks", and UTF-8 validity is rule 7 anyway, so both readings agree where it matters.
Vectors: decided by `canonical-text.json` (37 cases, each naming a first rule).

**Second edition: settled by the text, agreeing.** Text rule 13 now says a verifier names the lowest-numbered rule broken; `canonical-text.json` numbers the rules 7 to 12 and adds two cases for the order (I renumbered my rules 7 to 12 to match).

**2. Which error name an additional-information value of 28-30 gets, and what 31 is on majors 0, 1, 6 and 7.**
Source: Id Part three, "Encoding" (RFC 8949 4.2.1); Env rule 1; `cbor.json` reasons.
Question: RFC 8949 calls 28-30 reserved, and 31 on majors 0, 1 and 6 "not well-formed"; the vectors use names of their own.
Readings: one reason for every malformed head, or one per kind.
Choice: `ReservedAdditionalInfo` for 28-30 on every major and for 31 on majors 0, 1 and 6; `IndefiniteLength` for 31 on majors 2-5 and on major 7 (the break code standing alone); `InvalidSimpleValue` for simple values below 32 written in two bytes. Errors are reported for the first offending byte in encoding order.
Vectors: decided by `cbor.json` (46 cases, one reason each).

**3. Shortest floats and NaN.**
Source: RFC 8949 4.2.1 as Id cites it ("preferred serialization").
Question: is a NaN with a payload encoded in the shortest width that keeps its payload, or always as `f97e00`?
Readings: (a) shortest width preserving the exact bits (4.2.1); (b) one canonical NaN (4.2.2, the optional rule).
Choice: (a). Id cites 4.2.1 only; 4.2.2's NaN rule is an additional, optional profile. A NaN is accepted in the shortest width that represents its payload exactly and refused in any wider one.
Vectors: partly decided by `cbor.json` ("NaN as a double" refused, "NaN as a half" accepted); a NaN whose payload needs more than a half is accepted at the shortest width that holds it, which no vector tests.

**Second edition: settled by a vector, agreeing.** `cbor.json` now has a NaN whose payload needs single width: accepted as a single, refused as a double, as reading (a) says.

**4. The order in which an act's opening checks run.**
Source: Env rules 1-5, 7, 10; `open-act.json` ("the first check that fails").
Question: rule order in the text (1, 2, 3, ...) does not match the order a reader can check them in (the inside cannot be decoded before it is unlocked).
Choice: outside shape and CBOR, then locked hash, then key present (a private act stops here: NO_KEY), then unlock, then inside commitment, then the inside's CBOR, then its shape, then canonical text in every text string.
Vectors: decided by `open-act.json` (9 cases).

**Second edition: settled by the text, agreeing.** Envelopes now gives the order: the outside, the locked bytes against the locked hash, the inside once unlocked against its commitment, then its shape and text; a scheme the verifier lacks makes the act unknown only where nothing else fails.

**5. Which text strings must be canonical.**
Source: Env rule 5 ("Canonical text"), Txt rule 1.
Question: only fields a MIR's format calls text, or every CBOR text string anywhere in the act?
Readings: (a) per-field, by format; (b) every text string in the inside, at any depth, including map keys, in payloads of specifications a verifier does not implement.
Choice: (b). Rule 5 speaks of the act, not of fields; and (b) lets a verifier refuse a non-canonical string in an act of a specification it does not know, which (a) cannot.
Vectors: consistent with `open-act.json` (the TEXT case is in a known field).

**Second edition: settled by the text, agreeing.** Envelopes rule 5 now reads: every CBOR text string in the inside, map keys included, at any depth, whatever specification the act names.

**6. A content key used for two objects (Env rule 9).**
Source: Env rule 9, "A content key MUST NOT be reused for two objects" (line 115).
Question: should a verifier refuse an act whose content key it has seen on another act?
Observation: the published vectors reuse content keys and nonces across acts: in `verdicts.json`'s pool, the key `8005f02d…` with nonce `f554cc66…` locks at least three different acts (`0159cbda`, `0e9039a6`, `16f2ad0a`), and `8d85f846…` with nonce `d03524a9…` locks `0ccfec70` and `0f5926e2`.
Readings: (a) a check every verifier makes across the acts it holds; (b) a duty of the signer, not something a verdict reads.
Choice: (b). The rule binds whoever locks; no verdict rule names it as a check, and the vectors would fail under (a). I enforce nothing. (The vectors' keys are labelled test data derived from labels, which explains the reuse.)
Vectors: decided by `verdicts.json` (cases built on those acts would come out invalid under (a)).

**Second edition: settled by the text, agreeing.** Rule 9 is now marked client conformance: "no verdict reads it, and a verifier does not refuse an act for it". The new vectors also give every act its own key and nonce.

**7. X-Wing, whose draft is not in the box.**
Source: Env line 370 and line 502: "`XWING`, the SHA-256 of the X-Wing draft's revision 11 as published".
Question: the definition of X-Wing (key expansion from a 32-byte seed, the combiner) is in an external draft that I may not fetch (the task forbids looking for MOR elsewhere; the draft is not MOR, but I kept to the box).
Choice: I implemented X-Wing from my own knowledge of the draft: seed expanded with SHAKE-256 to 96 bytes (ML-KEM-768 seed d‖z, then the X25519 secret), and the combiner SHA3-256(ss_M ‖ ss_X ‖ ct_X ‖ pk_X ‖ label) with the six-byte label `\.//^\` last. `sealed.json`, which fixes every input, confirms the result byte for byte (the recipient key, capsule and opened contents all match), so my reading of revision 11 agrees with the one the vectors were made with. I did not compute `XWING` (the hash of the draft's text): I do not hold the text, and no rule I check reads it.
Vectors: decided by `sealed.json` (6 checks).

**8. The length limit of the written-for-people forms.**
Source: Id line 758: BIP-350's guarantee holds up to 90 characters "which every label" stays within; BIP-173's 90-character limit.
Question: refuse a string over 90 characters?
Choice: no length limit beyond what each label's data length gives (an `mkey` with a 1216-byte X-Wing key, or a long SLH-DSA key form, is far longer than 90). The text gives 90 as a property of the forms, not as a rule; and an `mkey` of an SLH-DSA or X-Wing key could not be written at all under a 90 limit.
Vectors: consistent with `textual-forms.json`.

**9. Which refusal a bad textual form gets first.**
Source: Id "Written for people"; `textual-forms.json` refused forms, each with why.
Choice: checks in this order: syntax (one separator, allowed characters), mixed case, checksum, label known, label is the kind asked for, data length/shape for the label.
Vectors: decided by `textual-forms.json` (refusals each name one reason).

**10. Bare hexadecimal: for which kinds, and hints for which labels.**
Source: Id "Written for people" (bare hex and the where-it-lives form).
Choice: 64 lowercase hex characters are accepted wherever a 32-byte hash is asked for (`mid`, `mact`, `mspec`, `mwork`), never for an `mkey` (a key is not a hash); upper-case hex refused. Where-it-lives hints (`?r=`) are accepted only on `mid`, outside the checksum, as the text shows them only for identities.
Vectors: decided by `textual-forms.json` (`bare`, `lives`, `upper`).

---

## Identity

**11. A private Identity act, or a private act a verifier cannot open.**
Source: Env rule 8 (Identity acts are public, save a private link), rule 10.
Question: what verdict for an act whose key the verifier does not hold?
Choice: its outside and signature are still checked; a bad signature or signature shape is **invalid**; otherwise **unknown** (nothing inside can be read). A public Identity act is required for every Identity type except link claims, confirmations and terminations (types 6-8); an Envelopes encryption key (type 4) must be public too.
Vectors: none (every vector act is public, as the README says).

**Second edition: settled by the text and vectors; my reading was incomplete.** A private link claim now also needs to be found at a home its signer's chain names (Identity rule 17, as the vectors read it): found nowhere, unknown, never invalid. See Q62.

**12. Inclusion proofs carry no receipt; how is a proof matched to its receipt?**
Source: Id "Audit" (line 654), RFC 9162 2.1.3.2; the audit case of `verdicts.json` (facts: inclusion proofs).
Question: a fact gives a leaf index and an audit path under a log summary, but not which receipt it proves.
Choice: the leaf is the plain RFC 9162 leaf hash, SHA-256(0x00 ‖ receipt's act id), and inner nodes SHA-256(0x01 ‖ left ‖ right). A proof is matched to a receipt when the proof's index is the receipt's log index (receipt field 3), its summary is a log summary (Id type 9) signed by the receipt's signer, and recomputing the root from the receipt's id gives the summary's root (if the fact does name a receipt, it must be that one). A summary counts as cosigned when at least the required number of distinct auditors the audit requirement names have valid cosignatures (Id type 10) naming it.
Vectors: decided by the Identity audit case (`verdicts.json`).

**Second edition: settled by the vectors, agreeing.** Each inclusion proof now names the receipt it proves; the code already required a named receipt to match.

**13. Dishonesty when there is no audit (conflicting receipts).**
Source: Id "Conflicting receipts" (line 524) and receipt checks (line 289): a receipt is "void unless cosigned"; a home that receipts two rotations for one position is dishonest.
Question: without an audit requirement, when is a conflict "proven" (marking the home dishonest from then on) rather than only a contest at that position?
Readings: (a) any two valid conflicting receipts prove it; (b) only receipts that survive the home's own rotations prove it: under a cosigned summary, or kept by a later rotation of the operator.
Choice: (b). Id says evidence counts only while its signer's chain keeps it; a receipt the home later disowns is no proof. Without proof, the position is **contested** and the home counts for nothing there, but later positions are not affected. With proof, the home counts for nothing from the next position on (`from` = the conflict position; positions after it are affected).
Vectors: consistent with the contested Identity case.

**14. The status of a chain act that lost (another rotation counts at its position).**
Source: Id "Which rotation counts" (line 376).
Choice: a rotation built on the same predecessor that passes its own checks but did not count is **void** (it was genuinely signed; it simply lost); one failing its checks is invalid; one built on an act that is not held is unknown; one built on a held act that does not count is void.
Vectors: consistent with Identity cases.

**Second edition: settled by the text, disagreeing.** "A rotation that does not count" is a new status word, **not counting**, neither invalid nor void; I had answered void. Acts bound to it fail everyday check 2 and are invalid, which my code already gave.

**15. A rotation receipted by no home yet; a rotation two homes split on.**
Source: Id "Which rotation counts", conflicting receipts.
Choice: at the chain's next position: no receipt yet → **pending**; a conflict that leaves no candidate meeting the rule → **contested**; a candidate whose checks need a scheme or act the verifier lacks → **unknown**. An everyday act bound to a pending rotation is pending.
Vectors: decided by the Identity pending, bound-to-pending and contested cases.

**Second edition: settled by the text, agreeing.** Rule 42 now says an act bound to a contested rotation is pending until the contest ends.

**16. Which acts must carry `prev`.**
Source: Id everyday checks (line 169); Env rule 13 (sequences); `verdicts.json` Envelopes cases.
Question: is `prev` required for every everyday act, or only for those in a sequence?
Choice: required for every everyday act, with zero or one entry; two entries is invalid; absent is invalid. Chain acts (genesis, rotation, chain signature) never carry it.
Vectors: decided by the Envelopes "no prev" and "two entries" cases.

**17. A non-fitting kept line (rule 33).**
Source: Id rules 33-35 (lines 388-440).
Question: a rotation names a kept tip `[act, n, summary]`; if the line rebuilt from that tip does not reach position `n` or its summary does not match, what then?
Readings: (a) the rotation is invalid; (b) that kept entry keeps nothing.
Choice: (b). The rotation's other checks are its own; a kept entry that does not fit simply keeps nothing, and acts only it would keep are judged as not kept. When an act a kept line needs is not held, the judgement is **unknown** rather than void.
Vectors: consistent with the Identity void/disputed cases.

**Second edition: settled by the text, agreeing, with one refinement.** Rotation check 7 says a tip that does not fit keeps nothing, itself included, and never makes the rotation invalid, as I had read it. The refinement: where the verifier does not hold the sequence, the tip itself is still kept by its name and only what lies below it is unknown. My code had answered unknown for the tip itself; it now keeps the tip.

**18. Disowned acts: void, disputed, and which acknowledgements count.**
Source: Id rule 34 ("Evidence counts only while its signer's chain keeps it").
Choice: an act signed with a key a later counting rotation replaced, not on a kept line (or named in the rotation's disowned list), is **void** — unless another identity acknowledged it (`acks`) by an act of a type that may acknowledge and that is itself valid (or scoped) now, which makes it **disputed**. An acknowledgement by the signer itself never counts. An escape endorsement naming the rotation it endorses is never judged by that rotation.
Vectors: decided by the Identity void and disputed (acknowledged) cases.

**Second edition: settled by the text, disagreeing in part.** An acknowledgement now counts for rule 34 only when signed with the acknowledging identity's signing key, never a scoped key. I had counted scoped acknowledgements too; the code now does not (vector identity[24]).

**19. "Status" versus "status after keepers".**
Source: `verdicts.json` questions; Agr rules 36-42 (lines 1057+), Id rule 34.
Question: Identity alone cannot see keeper records (they are Agreements acts); the vectors ask both.
Choice: the plain status is Identity's judgement; "status after keepers" adds Agr rule 41: an act its agreement's keepers recorded before the keepers' record of the voiding rotation, by the keepers' rule, is **disputed** instead of void.
Vectors: decided by the Identity keeper cases (disputed when recorded before, void when the keeper's record of the rotation comes first).

**20. Homeless rotations and the escape.**
Source: Id lines 570-618.
Choice: a homeless rotation never counts under the old rule by its own flag; it counts only through the homeless procedure (no valid objection from a home of the old set; enough old homes gone that the old rule cannot be met; a self-hosted home is never gone; the new homes' receipts meet the new rule) or the escape with both keys. A position that counts this way without the old audit is marked unaudited.
Vectors: none directly (consistent with all).

**21. Self-hosted homes and identities resolving into each other.**
Source: Id rule 2 (a home `null` served by the identity itself).
Choice: a self-hosted home's "receipt" is the rotation it serves; two different valid rotations at one position are a conflict for good. If resolving a home's chain leads back to an identity being resolved, that inner chain is taken on its own signatures (as a self-hosted one) to break the loop, bounded at a fixed depth (anything deeper is unknown).
Vectors: consistent with all.

**22. Duplicate copies of one act id.**
Source: Env (the act id depends only on the outside; the signature is outside the id).
Question: a verifier can hold two byte strings with the same id and different signatures.
Choice: keep one copy per id, preferring a copy whose signature verifies. Otherwise a bad copy could poison a good act.
Vectors: none.

**23. Which declaration of a collective's agreement counts.**
Source: Agr "collectives in the identity chain" (line 3787).
Choice: for each (specification, kind), the counting chain act with the highest position declaring it wins; a `null` value removes it.
Vectors: consistent with the collective cases.

---

## Text

**24. Text rule 14 for decisions.**
Source: Txt rule 14 (line 59), Agr record (type 17), grant (9), revocation (10).
Choice: a text act's objects: at most one entry; an entry on another identity's chain is invalid; an individual's text naming its own chain is invalid; a collective's text may name its own decision (a record, grant or revocation of the collective) on its own chain. Signed with a grant key, its one object must name the grant its binding names; signed with a holder's member key, it names `[collective, version]`, the founding or clone in force. Naming the member key's signature act instead is invalid.
Vectors: decided by the Text rule 14 cases (6).

**25. Rule 14 when Agreements is absent.**
Choice: rule 14 is applied by this verifier, which implements Agreements; where it would need an act the verifier does not hold, the result is unknown (never invalid).
Vectors: consistent.

---

## Envelopes

**26. Invalid beats unknown.**
Source: Env (the act's checks), `verdicts.json` Envelopes cases.
Choice: an act of a specification this verifier does not implement is **unknown** unless something it can check fails (signature, shape, canonical text, `prev`), which makes it **invalid**.
Vectors: decided by the two Envelopes cases.

---

## Money

**27. "Same payment".**
Source: Mon rules 10-11 (double entry), "One rail proof per payment or per named batch" (Suite).
Question: the core never reads a rail; how does a verifier know a receipt and a claim are of one payment?
Readings: (a) same rail Module and identical proof bytes; (b) same purpose and payee; (c) the rail answer fact says so.
Choice: (a). The proof is what the rail binds; two acts presenting the same proof on the same rail are the same payment. Purpose, payee, amount and previous hop are then what must *match* (a mismatch is a disagreement), so they cannot also decide sameness.
Vectors: decided by the Money agree/disagree cases.

**Second edition: settled by the text, disagreeing.** Money rule 12 says which payment a record is comes from the rail Module's answer, never from the proof's bytes, and gives the answer when it is not held (judged both ways, Q51-Q52). My reading (a) is withdrawn.

**28. What a disagreement counts.**
Choice: one per payer's claim that has at least one receipt of the same payment from the receiver, none of which matches it in amount, payee, purpose and previous hop. Receipts and claims count only when their rail answer (a fact) is valid and the act itself is valid or scoped (Mon rule 10).
Vectors: decided by the disagreement cases (0 and 1).

**Second edition:** disagreements may now answer unknown under rule 12 (Q52).

**29. "Paid toward" an obligation.**
Source: Mon "What a payment fulfils", rule 11 ("the greater amount counts").
Choice: sum, over distinct payments, of the greater of receipt and claim amounts, counting only receipts/claims whose field naming what they fulfil (receipt 5, claim 4) names the obligation, whose payee is the obligation's creditor, in the obligation's unit, with a valid rail answer.
Vectors: decided by the vault transfer case (0 paid toward an obligation it does not name).

**Second edition:** paid toward may now answer unknown under rule 12 (Q52). The vault case is relabelled "a payment to oneself naming one's own genesis".

**30. A transfer to one's vault naming the vault's declaration.**
Source: Mon "What a payment fulfils"; Suite (Money bullet: "a transfer to one's vault naming the vault's declaration among them, valid, fulfilling nothing").
Question: the case's identity declares no vault in its genesis, yet the payment names "the vault's declaration".
Choice: valid regardless; naming an act no Money or Agreements rule reads makes it fulfil nothing; I do not require the vault to exist for the receipt to be valid.
Vectors: decided by that case.

**Second edition: no longer asked.** The case is now a payment to oneself naming one's own genesis, which fulfils nothing; the answer is unchanged.

**31. A payee pointer at version 0.**
Source: Id versioned chains (versioned heads: version 0 is none).
Choice: version 0 has no previous and means "none"; version 1 has no previous; later versions name the one before.
Vectors: consistent.

---

## Agreements

**32. Whether founding terms exist: field 4 `[0]` and deal marks.**
Source: Agr rule 1, rule 47 (deal clones, line 1128-1305), terms field 4.
Choice: founding terms carry `[0]` in field 4 and exist once every party has a valid signature act (type 1) naming them. A deal's clone carries a mark naming the clone rule `[1]` and the parent's parties; it exists when every party the mark names and every party the clone adds signed it.
Vectors: decided by the deal signed/unsigned cases.

**33. Clone objects `[parent, parent]`.**
Source: Agr formats (terms objects).
Question: the vectors' clones put the parent in both places of the object entry; a strict reading of "[chain, predecessor]" would want the line's first act as chain.
Choice: accept `[parent, parent]` as written; the format says the chain an act is on is named by its line, and for an agreement the line is identified by the version it continues.
Vectors: decided by the clone/fork cases.

**Second edition: settled by the text, agreeing in effect.** Terms field 11 now says objects names the parent as chain and, as predecessor, any act of the parent's chain (the latest is client conformance). The code now checks the chain entry and accepts any predecessor.

**34. A deal fork and its reference.**
Source: Agr rule 47 ("tangled"), rule 46.
Choice: the versions that fork are the complete clones naming one parent; the reference is that parent (the version both sides share). The fork is tangled (and stays on its reference, rule 47) when three or more complete versions share the parent, or when a branch splits again further down.
Vectors: decided by the deal fork case.

**35. Belongs to: how far.**
Source: Agr "Belongs to" (line 98).
Choice: one level: an act belongs to an agreement it names in objects (or whose chain act it names), the agreement its Money purpose names, the agreement or offer its fulfilled act names (and one level further through an obligation), and an offer's agreement. Deeper chains of "belongs to" are not followed.
Vectors: consistent with the keeper cases.

**Second edition: settled by the text, disagreeing.** "Belongs to" is now followed at no fixed depth, with each objects entry or answered act one step; beyond an act not held, unknown. My one-level reading is replaced (Q55).

**36. Which keepers count for an act: the version in force.**
Source: Agr rules 36-42; Suite ("the keepers counting those of the version in force for the act").
Question: the version in force at the moment of the act may be a later clone than the one the act names.
Choice: the version the act names (the terms it belongs to). Following the line to the latest complete version before the act needs a time order the vectors do not give. This is a gap: a clone that changes keepers is not followed.
Vectors: consistent.

**Second edition: settled by the text, disagreeing (my gap).** The version in force is now defined: for a deal, the latest complete version on the named version's line that the act's history holds. Built (Q56).

**37. Keeper records after the keeper's rotation (rule 42).**
Source: Agr rule 42; Id "Evidence counts only while its signer's chain keeps it".
Choice: a keeper record that a rule reads to place an act keeps its placement even if the keeper's later rotation disowned it (valid, void or disputed records all place); an invalid record places nothing. A record counts only when the verifier holds the act it names and that act names the same signer.
Vectors: consistent with the keeper cases.

**38. "Before" in a keeper's sequence.**
Choice: record r lies before record s when s's `prev` chain, walked back within the keeper's own acts, reaches r. A different sequence of the same keeper orders nothing against it.
Vectors: decided by the keeper before/after cases.

**39. Deadlines (rule 33).**
Source: Agr rule 33 (line 977); Suite ("a deadline met shown by an anchor ... a missed deadline shown only by a named keeper's sequence anchored past the point holding no record").
Choices: (a) the keepers' rule `0` written in the question means `[0]` (every keeper); (b) an anchor at exactly the point counts as met ("at or before"); (c) anchors are compared only on the same clock (byte-identical reference); (d) where several anchors of one act exist, the earliest point counts; (e) met by keepers when keepers meeting the rule hold the act's record before one of their records anchored at or before the point; missed when keepers whose records anchored past the point do not hold it leave the rule unmeetable; otherwise **undetermined**.
Vectors: decided by the deadline cases.

**Second edition: settled by the vectors, agreeing.** The deadline question's keepers' rule is now written `[0]`, the format's own form; the runner now passes it on as CBOR.

**40. A collective's consent: areas, member keys, done.**
Source: Agr rules 57-60 (lines 1527-1722), 65 (line 1842), 72 (line 2104).
Choices: an act counts for the collective when it is "done" (sealed to every member or public, and on the collective's chain); the areas that reach it (by layer of its specification's task, or by kind naming its type) each need their holders' signatures meeting the area's number; when fewer voices remain than the number, all remaining meet it; an area with no holder is frozen; no area reaching it → the constitutional rule. A holder signs with its member key: a signature act of a founding or clone naming a key; a key named by two signatures of different signers is no member's.
Vectors: decided by the collective threshold cases and the Money member-key cases.

**41. Records (type 17) when counting consent.**
Source: Agr record (line 3647): an operational clone recorded "at once".
Choice: not applied; consent is judged on the agreement the collective's chain declares. This is a gap (no vector depends on it).

**Second edition: settled by vectors; my gap closed.** Records putting operational clones in force are now built (Q57).

**42. Grant backing: backed vs binds.**
Source: Agr rules 73-80 (lines 2131-2285).
Choices: an act signed with a grant key, citing the grant on the grantor's chain, within reach, with a valid public grant accepted by its grantee (a signature act), is **backed**. When the grantor's valid revocation exists: an act the revocation's history holds **binds**; one it misses (racing or after) is **void**, unless another identity (never the grantee) acknowledged it, or a keeper recorded it before the revocation, which makes it **disputed**; an act the grantor adopted (its own key acknowledging, citing in history, or receipting on it) **binds**. A grant with limits (fields 3, 4) is judged by a cMIP this verifier lacks → **unknown**.
Vectors: decided by the grant cases.

**43. A grant not accepted.**
Choice: an act under a grant its grantee never signed is **not backed**.
Vectors: consistent.

**44. A grant to oneself.**
Source: Agr rule 73; Suite ("a grant to oneself ... valid").
Choice: valid; the grantor's signature act is its acceptance.
Vectors: none.

**45. Terms checks that are written as prose.**
Source: Agr Part four.
Choices: fields 10 and 25 withdrawn (present → invalid); founding field 4 `[0]`; collective-only fields refused in a deal; stakes per object summing to one million; a stake holder never the collective itself; one time reference (field 6 and task 10 the same specification); a judge never also named for the task it judges; keepers' rule `[1, k]` with 1 ≤ k ≤ keepers.
Vectors: consistent (terms in the vectors pass).

---

## Development

**46. The MIR hash's text: the status line.**
Source: Dev rule 26; Suite line 378 ("each frozen text's header becomes the text's name alone").
Choice: the hash covers the file's text with at most one final line break removed, and with a draft's status line (the italic line after the title beginning `*Draft ` or `*Version `) and the blank line after it dropped. Two final line breaks or a trailing space: refused (not canonical text).
Vectors: decided by the five MIR-hash cases. For the record, `morv mir-hash` gives these draft hashes from the texts in `spec/` (status line dropped): Identity `51121e18…`, Envelopes `91730a7a…`, Text `ceab6c92…`, Money `2393e1bb…`, Agreements `b0c805e2…`, Development `e1273217…`. They are not the freeze hashes (the drafts are not frozen), and the vectors use stand-ins.

**Second edition: settled by the text and vectors, disagreeing.** Development rule 26 hashes the file's text less one final line break "and nothing else": the draft's status line is part of the text. I had dropped it. The code now keeps every line. The draft hashes I gave are withdrawn. From the second edition's texts, `morv mir-hash` gives (kind 4 for a MIR): Identity `a2711541…`, Envelopes `70ae6360…`, Text `c8d24025…`, Money `8e690bdd…`, Agreements `ba6489b3…`, Development `01e60e8b…`; the MICS (kind 6) `e283869b…`; the suite (kind 7) `f7832dc4…`. They are drafts' hashes, not the freeze's, and the vectors still use stand-ins.

**47. A Module that works under a MIR.**
Source: Dev ("A Module working under a MIR with no cMIP ... naming the MIR in field 5"), Suite.
Choice: a Module whose field 5 names a MIR needs no fields 11 and 13; only a Module under a *payment cMIP* (a cMIP whose field 2 lists Money's task 6) without 11 and 13 is no specification; if the cMIP named is not held → **unknown**.
Vectors: decided by the Development cases.

---

## Verdict questions in the vectors

**48. "All six MIRs, no cMIP".**
Question: the verifier implements the six MIRs and holds no cMIPs; acts needing a cMIP's judgement are unknown.
Choice: as stated; the vectors' facts supply rail answers and anchors that a cMIP would otherwise give.
Vectors: consistent with all 63.

**49. What stops a chain.**
Question: the "chain" question asks for the acts counting at each position and what stops the chain; the texts name no vocabulary for the stop.
Choice: the chain ends (no candidate held for the next position), or stops pending (candidates, none receipted enough yet), contested (a conflict leaves none counting), or unknown (a needed check cannot be made); with the homes proven dishonest from a position on.
Vectors: decided by the chain cases (the answers use these words).

**Second edition: settled by the text, agreeing in part.** Identity now names seven stop reasons and their order: no genesis, the genesis unknown, the genesis invalid, the end of what is held, pending, contested, cannot tell. The vectors write "end" and "contested". I added the rest and moved "cannot tell" ahead of a contest under the home rule (Q64).

---

## Second edition: new questions

**50. Rules the vectors cite that the texts do not number.**
Source: `verdicts.json` cites "Agreements rule 47a" (closings and forks), "rule 34a" (the chain of judgment) and "Money rule 10a" (refunds). Agreements numbers no 47a or 34a, and Money no 10a.
Readings: (a) rules missing from the texts; (b) labels from the vectors' generator for rules the texts do number.
Choice: (b). I read 47a as rule 91 with "Fork (type 19)" and "Closing (type 20)"; 34a as rules 35 and 47 ("The judge of forks"); 10a as Money rules 4 and 19 on money owed back. They say what the vectors test.
Vectors: decided nothing on its own; it guided which text I read for agreements[19]-[32] and money[11]-[12].

**51. What identifies "which payment" in the rail's answer.**
Source: Money rule 12; the facts `rail_payments` (`{payment, proof}`).
Question: the fact maps a proof's bytes to a payment; it names no rail Module, and no act.
Choice: a record's payment is the one whose proof bytes equal its field 1. Two records are one payment when both are mapped and the payments are equal. Records on different rail Modules (field 0) are never one payment. A record whose proof is not mapped leaves its sameness to any other record unknown, even when the proof bytes match (vector money[5]).
Vectors: decided by money[2] to money[6].

**52. Which pairs an answer "depends on".**
Source: Money rule 12, "Only the pairs an answer depends on count".
Question: the rule counts the pairs whose sameness the rules giving the answer ask; how does an implementation know which ones it asked?
Choice: the answer is computed once, recording every pair of unknown sameness it actually compares (records joined into payments one by one, each compared with the first record of each payment so far). With no such pair, it stands. With two or more, it is unknown. With one, it is computed again with that pair taken as one payment and as two; any further pair asked there (the rule's "asked only when another such pair is taken as one payment") makes it unknown; otherwise the two results are compared. For a list (a vow's sales) each entry found both ways stands, and each found one way only goes to `unknown`. This is the rule's "sound rather than complete".
Vectors: decided by money[4], [5], [8], [10], [12].

**53. A vow's sales: which records, which act, which state.**
Source: Agreements rule 13; Envelopes "Vows"; the vow question ("each sale's state, the payments signed by nobody, and those whose place is unknown").
Choices:
- the records are Money receipts and claims whose fulfils field names an offer selling the vow (`[3, vow]` in field 1), the offer and the record valid. The rail's verification answer is not required: the vectors' receipts have none stated and still count as payments (money[8] lists the receipt as unknown, money[10] counts it among three pairs);
- a payment with a payer's claim is a sale, its buyer the claim's signer;
- the act shown is the claim that signs on (the one no other claim of the payment follows), or, once confirmed, the confirming claim;
- confirmed: a claim of the buyer for that payment acknowledges an act about the vow. "About the vow" is the vow itself, an act naming it in `refs` or `objects`, or an offer selling it, held and valid;
- contested: a contest by the buyer whose field 0 names a record of the payment;
- a payment with receipts and no claim is "unsigned", listed by its records.
Vectors: decided by money[7] to money[10].

**54. What repays a refund.**
Source: Money rule 4 ("Money owed back follows this rule exactly ... the payer's own receipt counts wherever the money went"), rule 12; the refund-repaid question.
Choice: the receipts and claims paying the one owed (as payee), in the unit, with a valid rail answer, that name in their fulfils field a record of the payment the refund is owed on (the record asked about, or another of the same payment), each payment counted once at its greater amount. Not built: the bound on a refund paid to the payer's refund rail (the payer's vault limit), and refunds to a bare key.
Vectors: decided by money[11], [12].

**55. "Belongs to" at any depth: what a step is.**
Source: Agreements "Belongs to" (line 98), second edition.
Choices:
- a step is each objects entry (both chain and predecessor), a Money record's purpose (field 9) and fulfils (receipt 5, claim 4, obligation 4), and an offer's agreement (field 0);
- reaching terms (an agreement version) adds it and stops;
- a lone seller's offer is itself an agreement;
- Identity acts are not followed (an identity's acts belong to none), but a step to an id not held makes the answer beyond it unknown;
- every act of a collective (not its Identity acts) belongs to the agreement its chain declares.

In "status after keepers", an unknown "belongs to" makes a would-be void act unknown, never void.
Vectors: decided by agreements[13], [14].

**56. The version in force for keepers: what "history holds" means.**
Source: Agreements "Belongs to", "Its keepers are those of the version in force for the act".
Choice: the act's history is what its `prev` and its objects' predecessors name, followed back through held acts (the same walk as rule 76's history). A clone is held when its id appears there, for instance as the predecessor a signature act names. Of the complete versions on the named version's line held so, the deepest is in force; none held, the named version. For a collective, the agreement its chain declares (with records, Q57).
Vectors: decided by agreements[15], [16].

**57. A record putting an operational clone in force.**
Source: Agreements, record (type 17); rules 60 and 72.
Choice: for a collective's act signed with its own key, the agreement in force is the declared one, replaced by the clone of the deepest record that:
- the act's history holds;
- is valid, done (public or sealed to every member), and signed by the collective;
- names in field 2 signature acts that are held, valid and name the clone.

Not checked: that those signatures meet the powers the clone's mark names, counted as rule 65 says at the record. An act signed with a member key keeps the version its signature act names.
Vectors: decided by agreements[17], [18].

**58. A collective's fork and closing: what is checked.**
Source: Agreements rule 91, "Fork (type 19)", "Closing (type 20)".
Choice: complete when:
- the ending is valid;
- its chain act counts in the collective's chain, and every kept tip fits its line (Identity's rebuilding);
- the agreement it names is the one in force at that chain act, with records the tips' history holds;
- it is done;
- its signer and the members' chain signatures (Identity type 16 naming it, counting in each member's chain) meet the constitutional change rule (field 18; absent, every party);
- for a fork: every listed member is a party, on one side, and chain-signed. Each successor's genesis is held, and its declared founding terms have exactly that side's members as parties and name the original in field 23. Every obligation of the collective that binds it and lies in the cited history is handed out in field 6;
- for a closing: every such obligation is paid in full (paid toward), and no held terms of another agreement name the collective as a stake holder.

A history whose acts are not all held answers unknown, never "not complete".

Not checked, because no vector reaches them and each needs much more:
- voices that departures removed;
- share-counted rules (answered unknown);
- kept holders' pay compared with the original's;
- successors' chain signatures on the debts assigned to them;
- creditor releases and the acknowledgement rule for others' receipts;
- money owed back left open (field 4), a broken collective;
- finality and ties among several endings.
Vectors: decided by agreements[19], [20], [23], [24].

**59. An act after an ending's line.**
Source: rule 91, "The line".
Choice: an act of the collective is made before a complete ending when it is bound to an earlier link of the collective's identity chain than the ending's chain act, or bound to that chain act and in the history the ending's kept tips cite (a tip itself included). Every other act of the collective is after it, and its consent counts for nothing.
Vectors: decided by agreements[21], [22], [25], [26].

**60. The chain of judgment for a deal's fork.**
Source: Agreements rules 35 and 47 ("The judge of forks"); facts `judges_lapsed` and `settled_out_of_turn`.
Choices:
- the links are the judge of forks (field 27), then the identities its chain (field 21) lists for it;
- the active link is the first whose period on the fork (the reference, as the fact's `question`) has not lapsed;
- a settlement counts only when it is the active link's, names a request (type 22) for the reference by a party who signed it, keeps a version on the fork, and is not out of turn;
- a lapsed link's settlement counts for nothing (an answer after its period);
- a parties' settling version (a complete version with field 26) settles first.

Not built: a judge contradicting itself, a settlement that must name every tip it saw, and tangled forks among settlements.
Vectors: decided by agreements[29] to [32].

**61. An agreement as a party: which version signs.**
Source: Agreements rule 1, "Any agreement may be a party".
Choice: a party that is an agreement, named by its founding version, signs where every party of its version in force signs. That version is the deal's line followed while each version has exactly one complete clone. Nesting is bounded at eight layers; deeper, or an agreement reached again (a loop), answers unknown.
Vectors: decided by agreements[27], [28].

**62. A private link "found at a home".**
Source: Identity rule 17; facts `content_keys`, `found_at_home`.
Choice: the content key opens the private act. A private link act (Identity types 6 to 8, no key on its outside) is valid only when found at a home whose operator is one of its signer's homes at the latest position of its chain (a self-hosted home's operator being the signer). Otherwise it is unknown.
Vectors: decided by identity[25], [26].

**63. Succession.**
Source: Identity section 8; link confirmation (type 7); the succession question.
Choices:
- the successor field of the counting rotation with the highest position that carries one counts; null or none means none;
- a successor on another protocol is "another protocol's";
- confirmed and followed: a valid link confirmation by the successor naming, in objects, `[identity, rotation]`, or a valid witness act by the successor acknowledging the rotation;
- otherwise declared, or unknown where such a confirmation's status cannot be told.
Vectors: decided by identity[12], [13].

**64. What stops a chain: words and order.**
Source: Identity "What stops a chain" (second edition).
Choice: the words the vectors use for the two they show (`end`, `contested`), and for the rest `no genesis`, `genesis unknown`, `genesis invalid`, `pending`, `cannot tell`. Order at a position: the end of what is held; cannot tell; contested; pending. Not modelled: "contested where two or more homeless rotations there are final" ahead of cannot tell, since my homeless procedure does not keep provisional and final homeless candidates apart.
Vectors: consistent (identity[5], [15]).

**65. A rotation that does not count, and the acts bound to it.**
Source: Identity "Which rotation counts", second edition.
Choice: a rotation built on the predecessor that counts, passing the rotation checks, at a position where another counts, is **not counting**. An act bound to it is invalid (everyday check 2). A rotation built on a held act that does not count stays void, as before.
Vectors: decided by identity[16], [17].
