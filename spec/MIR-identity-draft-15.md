# MIR: Identity

*Draft 15, freeze-ready, for Nobody, allegedly, to approve.*

## How to read this MIR

Normal text is the protocol. Italic text is commentary: reasons, examples and plain summaries. A summary that starts *In short* repeats what the rule says; where the two seem to differ, the rule's own text decides.

The rules are numbered in reading order. The checks a verifier runs have names and numbered steps, cited as "everyday check 2", "receipt check 6" or "homeless procedure, step 4"; each stands beside the rules it serves. A rule of another MIR is cited by that MIR's name and the rule's number ("Agreements rule 73").

Some marks recur:

- *Client conformance:* a duty or permission of clients (the owner's client, a reader's client, a key Module). The protocol cannot check it from acts alone, so it is stated for builders.
- *Stated cost:* a weakness the rules accept knowingly, written down so nobody mistakes it for an oversight.
- **Re-homed without audit:** an answer that rests on the verifier's own failed attempt to reach a home. It is said, and nothing binding rests on it ("Every verifier gives the same answer", after rule 35).

The MIR has four parts. Part one is an identity as it runs most of the time: genesis, signing, everyday acts, homes and receipts, a rotation and what it keeps. Part two is the rare cases: chain signatures, succession, competing rotations, homeless rotations, escape with both keys, finality, and audit. Part three is the encoding and the act formats. Part four is the tasks, the reasoning, the parameters and the freeze scenarios.

## Purpose

Identity is the ground layer of MOR. It defines what an identity is, how it is born, how its keys are replaced, where others check it, how others reach it, and how it relates to other identities. Every act in MOR is signed in the name of an identity, so every client implements this MIR.

The identity hash is the identity. It never changes. Keys are only the pens that sign for it, and are replaced by rotation. The home is where anyone holding the hash learns which pen currently speaks for it, and where its content and its inbox can be found.

## Dependencies

None beneath it. Identity defines the envelope as far as it needs it; the Envelopes MIR completes it. Its rules depend upward in two places only: acknowledgements (Envelopes) and keeper records (Agreements, its exception for a keeper's record included) can give an act standing after a rotation. Nothing in this MIR requires a client to hold data from a higher layer: every rule here is decided from identity chains, receipts, summaries and the acts a verifier already holds.

## Words used in this MIR

### Acts and keys

- **Act.** Any signed object.
- **Envelopes.** The signed container of an act.
- **Identity hash.** The hash of an identity's genesis act. It is the identity.
- **Signing key.** The key that signs everyday acts. It cannot change the identity chain.
- **Chain key.** A key from a post-quantum family, committed in advance by its hash, revealed and used once, to sign a rotation or a chain signature.
- **Scoped key.** A key of an identity that an act of a higher MIR installs, limited to a scope that MIR defines: a grant key, a collective's or a person's (a payee's grant to a deal's split service among them), installed by its grant and removed by its revocation (Agreements rules 73 to 80), or, for a collective holder's member key, named by the holder's signature of the collective's founding or a clone and replaced by a later version (Agreements rule 73). It signs everyday acts in the identity's name, never an identity-chain act, and only that MIR says whether it counts. *It is added and removed by everyday acts (for a grant key, the grant and its revocation; for a member key, the holder's signature and a later version), never by a rotation: no chain key is spent on it. The act that installed it is judged by the next rotation like any act of the key it replaces (rules 33 to 35), so a scoped key installed by a thief who stole the signing key is void once the owner rotates without keeping it. A holder's member key is the exception Agreements makes for a signature a collective's line placed: its holder's later rotation leaves it as it is for the collective (Agreements rule 73).*

### Chains and sequences

- **Identity chain.** Genesis followed by the identity's rotations and chain signatures, each naming the one before. Only the chain key adds to it. Genesis is position 0.
- **Sequence.** A line of everyday acts by one identity, each naming the previous act in the line. An identity may keep one sequence or several; the split is the owner's choice.
- **Kept ancestry.** The acts a rotation names as the latest genuine act of each sequence, and every act they descend from. For each tip, the rotation carries its position and the running summary over the sequence up to and including the tip (the summary the act after it would carry: Envelopes, "The running summary"), so anyone can prove an act lies in the kept ancestry without opening any private act.
- **Everyday act.** Every act other than an identity-chain act (genesis, a rotation, a chain signature): it is signed with a signing key or a scoped key, carries `signer`, `binding` and `prev`, and sits in a sequence.
- **Chain signature.** An act on the identity chain, signed with the revealed chain key, that names another act and commits the next chain key, and changes nothing else. *The one line of an identity that never branches is its identity chain; a signature placed there is ordered against every other one of the same identity, whatever devices the identity uses. A higher MIR says which of its acts need one: Agreements' forks and closings.*

### Homes

- **Home.** A relay that focuses on the Identity MIR: it stores identity chains and serves them. A relay is not an identity; it is operated by one, a person or organisation with an ordinary MOR identity of its own, which signs everything the home states. A home is declared by its operator's identity hash plus the relay's address. It verifies what it stores and signs receipts for it, but never signs on the owner's behalf. "Home" names a role, not a size.
- **Operator.** The identity that runs a home or an auditing relay and signs its receipts, log summaries, objections and absence statements. Homes are counted per operator.
- **Self-hosted home.** A home operated by the identity itself. A self-hosted identity is trusted on its own signatures: its rotations count without receipts.
- **Receipt.** A home's signed statement that it holds a given identity-chain act, at a given position in that chain, recorded at a given position in the home's own log, and that it is the first valid act the home accepted at that chain position. The home's log position is a sequence number, never a time. A receipt counts only alongside the act it names.
- **Home rule.** The identity's declared rule for which homes count.
- **Log summary.** A home's signed summary of its whole log of receipts, as a Merkle tree root.
- **Audit.** Another home checking that a home's new log summary extends the previous one, and cosigning it. An identity may require that its receipts be audited.

### Reaching an identity and relating to others

- **Route.** Where the identity's content of a given type can be found (an outbox route), or where others deliver acts addressed to the identity (an inbox route).
- **Link.** A two-way statement, a claim by one side and a confirmation by the other, that two identities belong to the same entity. One side may be an account on another protocol. Its purpose is to let other protocols and services bind their accounts to a MOR identity, such as signing in with MOR, and to let anyone who wishes build bridges.
- **Succession.** A declaration, carried in a rotation, naming the identity that succeeds this one. A succession to a MOR identity is followed only once that identity confirms it (section 8).

### Rare cases

- **Homeless rotation.** A rotation signed with the current chain key that names new homes and counts without receipts from the old ones, when those homes are gone.
- **Closure.** A declaration, carried in a rotation of a home's operator, that the home stops serving every identity.
- **Contested.** The word names four things, each said with its own noun:
  - **a contested position** of an identity chain: no rotation counts there while a conflict stands, because competing rotations each miss the home rule (rule 40), because one home's two receipts there conflict (rule 44), or because two rotations are served there for a self-hosted identity (rule 4); later positions are not evaluated meanwhile ("Which rotation counts", step 5). It is never frozen: the conflict ends as rule 40 and "Conflicting receipts", step 2, say, except at a self-hosted identity, where nothing settles it and the owner leaves by the escape;
  - **a contested rotation:** one of the rotations competing at a contested position; a rule it changes waits (rule 42);
  - **a contesting receipt:** a receipt voided by its operator's rotation that another identity acknowledged; it is shown as a dispute at its chain position and decides nothing (receipt check 6);
  - **contested routes:** two routes acts naming one predecessor; clients use the last act before the fork ("Which value counts", 3).

  *Agreements' contest act is none of these: an objection shown beside what it names, which changes no status here.*

## Part one: an identity as it runs most of the time

*How it runs most of the time:* an owner's client makes a genesis, which names a first signing key, commits to a first chain key and declares one or more homes. The identity hash is the genesis act's id. The owner then signs everyday acts with the signing key, each bound to the identity-chain act that set that key. The homes store the identity chain and sign a receipt for each act on it. Rarely, the owner rotates: the chain key, kept offline, signs a rotation that installs a new signing key, commits the next chain key and names the latest genuine act of each sequence. The rotation counts once the homes the home rule requires have receipted it; acts it does not keep, or disowns, are void, unless another identity acknowledged them or a named keeper recorded them.

### 1. An identity is born

#### Rule 1. Genesis

Genesis MUST be signed by the signing key it declares, and MUST declare at least one home.

*Forging a genesis only creates a different identity, never a copy of an existing one, because the identity hash fixes the original.*

#### Rule 2. Several homes: the home rule, and its default

With several homes, genesis MAY declare a home rule, in one of two forms: one named authoritative home, or a threshold greater than half the distinct operators of the declared homes.

If it declares none, the default applies:

- the majority rule, with a threshold of the smallest number greater than half the distinct operators (n divided by 2, rounded down, plus 1, for n operators), computed from the home list; with a single operator, that operator's homes, counted as one;
- or, when one of the homes is self-hosted, that home is authoritative and the others are backups.

*One option is forced when none is selected; the default is always one of the declared forms, never a third, weaker rule.*

#### Rule 3. A single home's security is the owner's

An owner who relies on a single home or an authoritative home SHOULD be told plainly that the home's key security is effectively part of their own.

#### Rule 4. Self-hosting is a trust model

A self-hosted identity is trusted on its own signatures: its rotation counts with no receipt and no first-held-wins. A thief holding its chain key can serve a rotation of its own, and a verifier that holds only that rotation at its position counts it, as at a single lax home.

Two rotations served at one position of a self-hosted identity, by a thief and the owner, are a conflict nobody can settle: the position is contested for good, and the owner leaves it by the escape (section 11; rule 29's second exception).

*Self-hosting is a trust model, not a home like any other; the risk is the owner's choice, and other homes declared beside a self-host are backups by default.* *At a self-hosted home no third party fixes an order, and the ways a conflict ends elsewhere (a home's own rotation, an audit) do not exist there; the thief does not win where the owner has served a rotation, and the owner holding both keys leaves.*

#### The genesis checks

1. `signer`, `binding`, `prev` and `objects` are absent.
2. The key in the signature equals the signing key in the payload.
3. At least one home is declared.
4. If a home rule is present: the homes have at least two distinct operators; an index is lower than the number of homes; a threshold is greater than half the number of distinct operators and no greater than that number. Homes are counted per operator: several homes run by one operator count as one, since one stolen key signs for all of them.
5. If an audit requirement is present: at least one auditor is declared, and the threshold is at least one and no greater than the number of auditors.

### 2. Signing: keys and bindings

*In short: every act has one signer. Everyday acts are signed with the signing key and bound to the identity-chain act that set it; only the chain key adds to the identity chain; a scoped key, installed by a higher MIR, signs everyday acts whose standing that MIR decides.*

Every act has exactly one signer. An act that needs the agreement of two identities, such as a link, is made of two acts: one by each, the second naming the first.

#### The verification procedures

The verification procedures are: the checks on every act (this section), genesis (section 1), rotation (section 5), chain signature (section 7), everyday acts (this section), receipt (section 4), conflicting receipts (section 9), log summaries and audits (section 13), the homeless procedure (section 10), objection and absence statement (section 10), resolving an operator (section 4), which rotation counts (section 5), and which value counts: declarations, succession, routes, names and links (section 3). Each stands beside the rules it serves. A verifier runs the steps of each procedure in order. Where a procedure does not say what a failure means, the failure makes the act invalid. A check the verifier cannot run, because it does not hold an act the check needs, makes the act unknown, not invalid ("A verifier that cannot tell answers unknown", after rule 35).

#### Rule 5. The verifier runs the checks

A verifier MUST run the verification procedures of this MIR. In particular, it MUST check that the signature is valid for the key it names, and that the binding names an act in the signer's identity chain that bound that key, or, for a scoped key, what everyday check 2 says.


#### Rule 6. A signing key never adds to the identity chain

A signing key MUST NOT sign acts that add to the identity chain, save the genesis that declares it (rule 1).

#### Rule 7. A chain key signs only rotations and chain signatures

An act signed with a chain key MUST be a rotation or a chain signature. Any other act signed with a chain key is invalid.

#### Rule 8. Scoped keys

*In short: Identity checks only a scoped key's signature; the higher MIR that installed the key judges the act.*

An everyday act signed with a scoped key is valid under Identity only as far as its signature: its status is "scoped", and the higher MIR whose act its `binding` names judges it (for a grant key, Agreements rules 73 to 80).

A client that does not implement that MIR MUST show the act as unknown, never as valid (rule 9).

A scoped key never signs an Identity act, nor an act that adds to the identity chain; it never counts as the identity's signing key in a rotation's judgement (rules 33 to 35).

*Identity cannot read Agreements (a lower layer never depends on a higher one), so it carries the key's signature and leaves the key's standing to the MIR that installed it, as it carries declarations whose kinds higher MIRs define (rule 31). Stated cost: a client without Agreements cannot tell that a grant key is the collective's; it shows the act as unknown.*


#### Rule 9. A client implements every MIR it acts on

A client MUST implement every MIR that defines an act it acts on. A client that meets an act defined by a MIR it does not implement MUST show that act as unknown, never as valid. In particular, a client that acts on agreements MUST implement Agreements.

#### The checks on every act

These run on every act.

1. Decode the act; reject it unless it is deterministic CBOR in the Envelopes MIR's shape.
2. Check that it is public, save a private link (section 3): its content key is on the outside, and it opens the inside, which matches the inside commitment.
3. Compute the act id from the outside.
4. Check that the scheme is one this client implements and the signature is valid for the act id under the key in the signature. A scheme this client does not implement makes the act unknown, not invalid.


#### The everyday checks (types 2 to 15)

1. `signer` and `binding` are present, and `prev` is present.
2. `binding` names an act of the signer's identity chain, genesis or a rotation, never a chain signature, that counts, and the key in the signature equals the signing key that act set; where that act is a rotation that does not count yet (rule 28), the act is pending, never valid and not invalid, until that rotation counts (rule 28), and, once that rotation can no longer count, the binding names no act that counts; or, for an act signed with a scoped key, `binding` names a held act of a higher MIR, never an Identity act nor the act itself, and the signature is valid for the key it names: the act's status is then **scoped**, and the MIR that defines the binding act says whether that act installs that key for the signer, and whether the act counts (rule 8). A scoped act whose binding the verifier has not held is unknown, never invalid.
3. If a later rotation of the signer counts, the act's status follows rules 33 to 35. A rotation judges only acts signed with the key it replaces, never an act signed with a scoped key.


### 3. Everyday acts: witness, routes, names and links

*In short: an identity's everyday acts say where to find it (routes), what it is called (names), who else it is (links), and what it relies on (witness acts). All are public, except that a link may be private.*

#### Every act of this MIR is public, except a private link

Every act defined by this MIR is public, except a private link: its content key travels on its outside, so homes, auditors and anyone checking an identity can open it. They are the public face of an identity.

A link's claim, confirmation and termination (types 6 to 8) may instead be private, encrypted as the Envelopes MIR defines, their key delivered only to whoever should see them. For a private link act:

- It counts only if its sealed form is published at the homes its signer's chain names at the link act's binding (its place in that chain); a later move of homes does not void it.
- What a verifier found at those homes is an input like its own attempts: a link gives no authority (rule 16), so a link never decides anything binding, and its standing from one verifier's fetch is never needed for anything that does.
- A link nobody can fetch from those homes is unknown, never invalid.
- A link voided by a rotation is void whether or not it was fetched: the rotation is judged first.
- A private link act signed by a scoped key is invalid (rule 8).

Only key holders can tell a private act's type, so a verifier that opens a private Identity act of any other type treats it as invalid.

*A link carries no key event, so no home or auditor needs to open it to follow an identity's keys. Its content stays private, but its existence is public, so an owner whose stolen key confirmed a link sees an act they did not write and can rotate; a link shown only to one party would let a thief use the key and leave nothing to notice. Stated cost: anyone can see that a private link act exists, never with whom.*

#### Rule 10. Witness acts

A witness act MUST carry `acks`, naming at least one act, and MUST NOT carry `objects`; its payload is empty. Otherwise it is invalid. An act it names that is its own signer's is not acknowledged by it (Envelopes: an acknowledgement names another identity's act).

A witness act says only: **"I received this act and rely on it."** It is the one act whose whole purpose is acknowledgement.

*It brings back, deliberately, what a like must never do: relying on a post or a message that nobody paid for or signed around (a public promise, a threat kept as evidence, a statement someone relies on). It keeps the acts it names visible as disputes if their signer later disowns them (rule 34), for as long as its own signer's chain keeps it, and it is the natural carrier for the acknowledgement of a voided home receipt (receipt check 6). Like every act this MIR defines, it is public: it shows anyone which acts its signer relies on, never what they say.*

#### Rule 11. A witness act is never a side effect

*Client conformance.* A client MUST NOT sign a witness act as a side effect of another gesture (a like, a reply, opening or saving a message). Before signing one, it MUST say what it does, in plain words, for example: "This keeps the act visible as disputed even if its author later disowns it. It is public: anyone can see that you rely on this act, though not what it says."

*No verifier can check that a client did this; it is conformance, not validity, like the Text MIR's rule that what you sign is what you saw.*

#### Rule 12. Anyone can find anyone

Any two identities can find each other without already sharing a relay: an identity's home is known from its declaration, anyone can query the home, and the home serves the routes, which may include an inbox. An act counts for a verifier only once the verifier holds it; nothing forces propagation, and delivery is the signer's interest (Envelopes).


#### Rule 13. A stolen signing key and the inbox

A thief holding the signing key can redirect the inbox route until the owner rotates, as with the encryption key (Envelopes). Deliveries made into that window go where the thief pointed; the rotation voids the thief's routes act, and the sender re-delivers.

*The window is the same one the encryption key already has; no new exposure.*

#### Rule 14. A link needs both sides

A link counts only when both sides have signed: the claim and a confirmation, in MOR or through a proof from the other protocol. A claim without a confirmation is not a link, and clients MUST NOT display it next to the identity it names.

#### Rule 15. Either side may end a link

Either side MAY end a link alone. The ending applies to every act that holds the termination in its history; it does not erase that the link existed.

*There is no clock to say what comes after the ending; what an act cites says whether its signer had seen it.*

#### Rule 16. A link gives no authority

A link gives no authority. Delegated authority uses grants (Agreements).

#### Rule 17. Private links

Copies of a private link MUST be encrypted separately, so that stored copies cannot be matched to each other. Each half of a link counts once its sealed form is published at its signer's homes; an owner who holds back one half delays the link, which counts only once both are published (rule 14).

#### Which value counts: declarations, succession, routes, names and links

A disputed act never counts when choosing the latest value of anything below. *In this MIR an act is disputed only by its rotation rules (rules 33 to 35); the Agreements MIR gives the same status, in rule 34's words, to an act under a revoked grant (Agreements rule 76). No other act of a higher layer changes that status; a contest (Agreements) is an objection shown alongside what it names and changes nothing here.*

1. **Declarations:** for each spec and kind, the entry in the counting identity-chain act with the highest position counts. A null value means the kind is not declared.
2. **Succession:** the successor field in the counting rotation with the highest position that carries one counts; null means no succession. A succession it declares is followed only once confirmed (section 8).
3. **Routes:** follow the routes chain from version 1; each act must name the previous one and carry its version plus one. The latest act of an unbroken, unforked chain counts. If two valid routes acts name the same predecessor, or two are version 1, the routes are contested from that point (from the start, for two versions 1, so that no routes act counts): clients use the last act before the fork and warn the owner, who settles it by rotating. A fork behind an act the owner's later rotation kept is settled by that rotation; clients need not rewind past it.
4. **Names:** a name counts while its name act is valid and no valid withdrawal by the same identity names it.
5. **Links:** a link counts when a valid claim and a valid confirmation by the identity it names both exist, and no valid termination by either side names the claim.

### 4. Homes and receipts

*In short: a home verifies what it stores, keeps every valid rotation it accepted, and signs a receipt for each identity-chain act. Receipts, not a home's unsigned answers, are what clients should rely on. A home may set its own acceptance conditions; a condition tied to a device has a cost the owner must be told.*

#### Rule 18. A home checks before it stores

A home MUST check a rotation's validity before storing it, and MUST NOT store an invalid one.

#### Rule 19. A home keeps and serves

A home MUST NOT drop a valid rotation it has accepted, and MUST serve the full identity chain it holds.

#### Rule 20. A receipt for every chain act

A home MUST sign a receipt for every identity-chain act it accepts, and MUST serve its receipts with the chain.

#### Rule 21. Rely on receipts

Clients SHOULD rely on receipts, not on a home's unsigned answers.

#### Rule 22. What a home serves besides the chain

A home SHOULD store and serve the identity's latest routes act, name acts and public links, and MUST store and serve the sealed form of its private links, since a private link counts only where it is published.

#### Rule 23. Consult the homes

A client SHOULD consult the homes before relying on an identity's act. Keepers MUST (see Agreements).

#### Rule 24. A home's own acceptance conditions

A home MAY apply its own conditions before accepting a rotation (for example proof of a registered device). A home's acceptance conditions can refuse the owner as well as a thief.

Where a condition depends on a device the owner may lose, losing that device at a single or authoritative home, or at homes enough to block the home rule, can mean losing the identity: the normal rotation is refused, the homeless rotation is objected to, and the escape needs the signing key that was on the device.

*That risk is the owner's choice, and it is survivable exactly when the signing seed is backed up apart from the device (rule 32).*

#### Rule 25. Device-bound policies are never the default

*Client conformance.* A client MUST NOT choose a device-bound acceptance policy at a single or authoritative home, or at homes enough to block the home rule, by default. Where the owner chooses one, the client MUST say what it costs, and SHOULD require a backup of the signing seed first.

#### The receipt checks

1. **The receipt names a real act.** The verifier holds the identity-chain act the receipt names; that act passes the checks for its type (genesis, rotation or chain signature); its identity hash, or its signer, is the identity the receipt names; and its position is the position the receipt states. A receipt that fails this check is neither support nor conflict: it is ignored for every purpose below.
2. The receipt is valid as an everyday act of the home's operator.
3. The home is declared in the home set in effect for that chain position: the homes set by the identity-chain act at the position before; or, for a homeless rotation, a home in the new set it declares (homeless procedure, step 5). *A homeless rotation leaves homes that are gone, so its receipts can only come from the homes it names.*
4. If audit is required: an inclusion proof shows the receipt at its log position under a log summary of the home carrying at least the required number of valid cosignatures from the declared auditors. Otherwise the receipt does not count. The audit requirement that applies is the one the named rotation itself declares, or, if it declares none, the one in effect before it.
5. A receipt that lies outside the kept ancestry of a later counting rotation of the home's operator is void, unless it is included under a cosigned log summary: a receipt under a cosigned summary can never be voided by the operator's rotation.
6. A receipt voided by the operator's rotation that another identity acknowledged is shown as contesting that chain position. It never counts as support toward a home rule, and it blocks nothing: the home's other receipts at that position are judged as before. *An accomplice's acknowledgement turns a thief's receipt into a visible dispute, never into a veto.* *This is one case of the general rule beside rule 34: evidence counts only while its signer's chain keeps it.*

A self-hosted home signs no receipts: the rotation the identity itself serves stands in for one (see "Which rotation counts", section 5).


#### Resolving an operator

To check a receipt, a verifier needs the operator's identity chain, which it obtains from the operator's own homes, like any identity's. A self-hosted identity is resolved from its own home, on its own signatures. When resolving operators leads back to an identity already being resolved, the verifier accepts that identity's chain on its own signatures, as for a self-hosted identity.

#### A home's address

An address hint is where the home can currently be reached. It is not checked: the operator's identity is what counts, and the operator announces a new address through its own routes, so a home can move servers without any owner rotating. A null operator means self-hosting: the identity runs its own home. Several null entries count as one operator, the identity itself; an identity naming its own hash as operator in a rotation is the same as a null entry. *Genesis needs the null form because it cannot name its own identity hash, which does not exist yet.*

### 5. A rotation as it usually runs

*In short: the owner's chain key, revealed once, signs a rotation naming the identity-chain act before it. The rotation installs a new signing key, commits the next chain key, and names the latest genuine act of each sequence. The owner's client sends the same bytes to every home. A home that accepts a rotation holds the first valid one revealing that chain key, rejects any other, and receipts it; the rotation counts once the receipts the home rule requires exist.*

#### Rule 26. Why an owner rotates is the owner's business

The protocol does not restrict why an owner rotates. Rotation is expected to be rare.

#### Rule 27. A valid rotation

A rotation is valid only if:

- it names as its predecessor the identity-chain act at its own position minus one;
- the hash of the revealed chain key equals the hash committed by that predecessor;
- its signature is valid under the revealed key;
- and it commits the hash of a next chain key.

#### Rule 28. A rotation counts once the homes receipt it

A rotation counts once receipts from the homes required by the home rule show they hold it. Until then it is pending, and acts under its new key are pending too.

Receipts are proof, not permission: a home that holds a valid rotation cannot withhold its receipt without breaking rule 20. *A withheld receipt leaves no evidence, though, which is one reason to declare several homes.* *Anyone who accepts an act under a pending key does so at their own risk, since the rotation may never count.*

A self-hosted identity's rotation counts on the rotation alone.

#### Rule 29. One chain key, one rotation

*Client conformance.* A client MUST submit the exact same rotation bytes to every home, including when resubmitting after a refusal, and MUST NOT sign a second, different rotation or chain signature with the same chain key.

Two exceptions, each allowed once:

- after a homeless rotation is voided by an objection, the owner MAY sign one normal rotation with the same chain key;
- after a normal rotation is refused by the homes that count, or contested at a self-hosted home, the owner MAY sign one endorsed homeless rotation (an escape) with the same chain key, whose endorsement lists the refused rotation as abandoned, or, at a contested self-hosted position, both rotations served there.

*SLH-DSA is stateless and is not weakened by a few signatures; the single-use rule exists so that an owner never has two competing rotations in play. "Each allowed once" cannot be checked by a verifier; it is a client and Module rule, and the air-gapped chain key Module enforces it.*

*Client conformance:* the air-gapped chain key Module signs the escape for a position contested at a self-hosted home only on presentation of the competing rotation, which it checks offline under the chain key that rotation revealed: its signature verifies, its previous act is the same as that of the rotation the Module signed, and it is not the rotation the Module signed. The online side's word alone does not open it. A refusal by the homes that count (the first case of the exception) stays unprovable from bytes: the Module tells the human what it is asked to sign, and why, before signing.

*A contest at a self-hosted home is two valid rotations revealing one chain key, which the Module can see in bytes; a refusal is the absence of receipts, which no bytes prove. An honest owner holds the thief's rotation, since it is what made the contest, so the check costs the owner nothing.*

A rotation that fails the rotation checks was never a rotation: it can count nowhere, so the owner MAY sign again with the same chain key, and that signature is no second rotation under this rule. *Validity is fixed by the bytes, so an invalid rotation can never later become a competing one; without this clause, a client that changed the homes and forgot to send a rule that fits (rotation check 5) would spend the owner's chain key on a rotation no home stores.*

*Client conformance:* the air-gapped chain key Module accepts as showing that a rotation it signed was invalid only bytes: the identity chain from genesis to the rotation's predecessor, against which the Module runs the rotation checks on the rotation it signed, offline, itself. The online side's word alone opens no exception, as for the escape above. *The Module cannot run the home-rule check without the identity's history; given that history in bytes, it runs the check itself, so a compromised online side cannot get a second valid rotation signed by calling the first one invalid.*

#### Rule 30. First held wins, at each home

A home that holds a valid rotation or chain signature MUST reject any other rotation or chain signature revealing the same chain key. The first valid rotation a home holds wins at that home, and its receipt fixes that order.

#### Rule 31. Declarations ride on the chain key

An entry in the declarations slot is valid only in genesis or in a rotation that counts. The latest such entry of each kind counts; a null value removes the kind. Higher MIRs define the kinds and what they mean.

#### Rule 32. Keep the two keys apart, and back both up

Clients and key Modules SHOULD keep the signing key and the chain key separate, from separate seeds and on separate devices where possible, and SHOULD back up both seeds.

*Holding both keys is the strongest proof of ownership the protocol sees, so a thief who steals both can also leave the owner's careful homes; and a backed-up signing seed is what makes a lost device survivable at a strict home.*

#### The rotation checks

1. `signer` is present; `binding` and `prev` are absent.
2. The previous act named is the signer's identity-chain act at position (this position minus one).
3. The signature's scheme and key hash to the chain-key commitment of that previous act: `tagged_hash("MOR/safety", scheme || key)` equals it.
4. A new signing key and a next chain-key commitment are present; the position (field 1) is at least 1; a homeless rotation (field 11) declares a new set of homes (field 6).
5. Any new home rule or audit requirement meets the conditions for genesis, counted against the new set of homes if one is given. A home rule the rotation leaves in place must meet them too, against the new set of homes; if it does not, the rotation is invalid. *The owner's client sends a rule that fits, or null for the default, whenever it changes the homes; it never quietly changes a protection the owner chose.*
6. A closure flag is valid on any rotation; it has effect only for identities whose declared homes name the signer as operator.

#### Which rotation counts

For each position n of an identity chain, starting at 1 (a chain signature counts at its position exactly as a rotation does, and "rotation" below includes it, save where it says homeless):

1. Take the home set and home rule in effect: those set by the act that counts at position n minus one. If several homes are declared and no rule, the default rule of rule 2 applies.
2. Collect the receipts for position n from those homes that pass the receipt checks. Leave out receipts that are void, receipts in a standing conflict, and every receipt from a home proven dishonest for this identity at an earlier position. For a self-hosted home, the rotation the identity itself serves at position n stands in for a receipt; two different rotations served at one position are a conflict, which nothing settles: the position is contested for good, and the owner leaves it by the escape (rule 4).
3. Apply the rule:
   - **One home, or an authoritative home:** the rotation that home has a receipt for counts.
   - **Threshold k:** a rotation counts if homes of at least k distinct operators have receipts for it.
4. If no rotation counts at position n under the rule, a homeless rotation at n may count under the homeless procedure (section 10).
5. If nothing counts at position n, later positions are not evaluated: the latest act that counts is the one at n minus one.

### 6. What a rotation keeps

*In short: a rotation judges the acts signed with the key it replaces. Acts in its kept ancestry stay valid, however old. Acts outside it, or disowned inside it, are void, unless someone else relied on them in a way the protocol can see: then they stay valid and are shown as disputed. Evidence another identity signed counts only while that identity's own chain keeps it. A verifier that does not hold what a rule needs says unknown.*

Each rotation judges only the acts signed with the key it replaces (a chain signature replaces none, and judges nothing), except an escape endorsement naming that rotation, which it never judges. For such an act:

#### Rule 33. Kept acts stay valid

If it lies within the rotation's kept ancestry and is not listed as disowned, it is valid, however old.

#### Rule 34. Acts left out or disowned are void, unless relied on

If it lies outside the kept ancestry, or within it but listed as disowned, it is void, unless:

- another identity acknowledged it (Envelopes; by an act of a type the Identity, Money or Agreements MIR defines, such as a witness act);
- or a named keeper recorded it before recording the rotation (Agreements: the keeper record, type 2, and Agreements rules 41 and 42);

then it is valid and shown as disputed. A keeper record or acknowledgement counts only alongside the act it names (Envelopes, Agreements), and only while its own signer's chain keeps it valid (below), save the exception stated there for keeper records.

*Disowning an act does not cost the owner the genuine acts that followed it: later acts in the line stay valid, because their validity rests on being inside the kept ancestry, not on the disowned act's.*

#### Evidence counts only while its signer's chain keeps it

An act of another identity that a MIR uses as evidence (a receipt, an objection, an absence statement, a cosignature, an acknowledgement) counts only while its signer's chain keeps it valid. One voided by its signer's rotation is shown, as a dispute, and decides nothing, acknowledged or not.

**Agreements' exception for a keeper's record.** A keeper's record that an Agreements rule reads to place an act, or that a relied-on version's completeness rests on, keeps the effect that rule gave it, whatever the keeper's later rotation disowned, shown as a dispute (Agreements rule 42). *A witness chosen in advance cannot un-witness.* Every other use of a keeper record, and every other kind of evidence, follows the rule above, save what a collective's own line placed (the pair, below).

A signer's own acts keep rule 34 unchanged: disowned but relied on, they stay visible as disputed.

*Rule 34 protects those who relied on a signer's own acts; it was never meant to keep alive the evidence others rely on. Read that way, a thief holding a home operator's signing key signs an objection to the owner's homeless rotation, an accomplice witnesses it, the operator rotates and disowns it, and the owner who lost the signing key, the very owner the homeless path exists for, never leaves. The same would keep a forged log summary "audited", and a thief's post disputed for ever. Receipt check 6 says this of receipts; this rule says it of all five kinds. Honest evidence loses nothing: a rotation keeps everything its signer does not disown.*

*Its pair is in Agreements (Agreements rule 6, and "Made before, made after", 4): an act of another identity that a collective's line placed keeps, for that collective, the effect the line gave it, whatever its signer's later rotation kept or disowned; its status here is unchanged, and clients show it as a dispute on the line. That rule, and the keeper's exception above, are Agreements'.*

#### Rule 35. Sequences a rotation does not name

An act signed with the old key in a sequence the rotation does not name is outside the kept ancestry. A disowned hash that is not in the kept ancestry has no effect. Clients SHOULD track every sequence the owner has started, on any device, and warn before a rotation that omits one.

#### Every verifier gives the same answer; nothing binding rests on one's own attempts

These rules are part of validity and MUST give the same answer to every conforming verifier holding the same identity chain, the same acts of the signer's sequences up to the kept tips, acknowledgements, keeper records, inclusion proofs and content keys, having made the same attempts to reach homes, and having found the same acts at the signer's homes (section 3, private links).

An answer that rests on the verifier's own failed attempts is marked as such ("re-homed without audit"), and nothing binding may rely on it (MUST NOT): no keeper record, payment, discharge of a debt, agreement, fork or closing. For any of these, such an answer is shown as unknown, never as valid or invalid, until it no longer rests on the verifier's own attempts. Reading and following an identity MAY rest on it.

*A reader's own attempts are the escape from a censor; they are also the one input another reader cannot hold, so what rests on them is said, and nothing binding rests on it.*

#### A verifier that cannot tell answers unknown

A verifier that holds the acts a rule needs answers as the rule says. One that does not (a kept tip whose line it cannot rebuild, a scoped key whose installing act it has not held, a chain it cannot resolve) answers unknown, never valid, void or invalid, unless a rule it can apply shows the act invalid (invalid beats unknown, Envelopes rule 30).

Void needs every kept line of the rotation rebuilt and the act on none of them. Invalid is only for a rule the verifier can apply and sees fail. *What follows from this paragraph: an act the verifier does not hold is unknown to it; a rotation built on a rotation it does not hold, and every act bound to it, is unknown; and where an objection that would void a homeless rotation has a status the verifier cannot tell, whether that rotation counts cannot be told either, and the position is unknown (not pending, and not voided).*

*A verdict that flips from void to valid, or from invalid to scoped, as acts arrive is a verdict given with part of the picture; a reader that trusts it goes dark. Unknown is the word this MIR already uses for a link nobody can fetch and for a scheme a client lacks. Disputed is void but relied on, so it waits on void as well. An act bound to a rotation the verifier can check, of a chain whose genesis is signed under a scheme it lacks, is unknown, as is every act under it that no rule the verifier can apply shows invalid. A shorter proof that an act lies within a kept ancestry (the peaks of the running summary and a path) can follow after MOR 1 without changing any act; until then its absence costs a reader unknown, never void.*

#### Rule 36. Old disputes may be collapsed

*Client conformance.* Clients MAY collapse disputed acts older than the rotation that voided them, showing a count rather than each act. The status itself never changes.

## Part two: rare cases

*Most identities never meet these. A chain signature is spent only where a higher MIR asks for one (in MOR 1, Agreements' forks and closings). Succession is declared when an identity moves on. A competing rotation needs one chain key used twice. A homeless rotation needs homes that are gone, an escape a home that refuses, and audit an owner who asked for it.*

### 7. Chain signatures

*In short: a chain signature is an identity-chain act, signed with the chain key like a rotation, that signs another act and changes nothing else but the next chain-key commitment. Agreements uses it so that a member's signatures on forks and closings are ordered.*

A chain signature competes for its position with any rotation or chain signature revealing the same chain key, and counts as a rotation counts ("Which rotation counts", section 5): by the receipts of the homes the rule names, or on its own signatures for a self-hosted identity; it is never homeless.

Once it counts, the identity chain's state is the one before it with only the next chain-key commitment changed: the same signing key, homes, home rule, audit requirement, declarations and succession.

It judges no act: rules 33 to 35 belong to rotations alone.

The act it signs need not be held, nor be the signer's; what the signature means is defined by the MIR of the act it signs (Agreements: a member's signature on a fork or closing).

*Stated cost:* every chain signature spends a chain key, so it goes through the same ceremony as a rotation (the air-gapped device), and is as slow and heavy.

#### The chain signature checks

1. `signer` is present; `binding` and `prev` are absent; the position is at least 1.
2. The previous act named is the signer's identity-chain act at position (this position minus one).
3. The signature's scheme and key hash to the chain-key commitment of that previous act.
4. A next chain-key commitment and the act signed are present.

### 8. Succession

*In short: an identity may name the identity that succeeds it, only in a rotation; the latest counting rotation that says something about succession decides it; the successor confirms it before anyone follows it; and the old identity goes on living.*

#### Rule 37. Only in a rotation that counts

A succession is valid only as part of a valid rotation that counts. A succession in any other act is invalid.

#### Rule 38. The latest rotation decides

The latest rotation that counts decides the succession. A rotation without a succession field leaves the previous succession in place; ending a succession requires a rotation that says so.

#### Rule 39. Succession does not end the identity

Declaring a succession does not end the identity. Both may coexist while followers and agreements move across.

#### A succession is declared by the rotation and confirmed by the successor

A succession to a MOR identity is declared by the rotation and confirmed by the successor: by a link confirmation of the successor naming the rotation, or by a witness act of the successor naming it. Clients show a declared succession as declared until it is confirmed, and follow it only confirmed.

A succession to another protocol is confirmed as that protocol's link cMIP says.

*A link needs both sides, "otherwise anyone could claim to be anyone" (rule 14, and the reasoning on links); a succession declared by one side alone would let a stranger's rotation name a well-known identity as its successor, and pass a collective's seat, through Agreements' fallback (Agreements rule 84), to an identity that never agreed to take it. Agreements' fallback passes a seat only to a confirmed successor, as a nomination.* *The successor confirms after the rotation, since the confirmation names it: at any time before the succession is used, usually at once, the two arranging it together, or later, even after the owner is gone, since it needs nothing from the owner.* *Stated cost: confirmation does not stop a thief holding the chain key from naming, and confirming, an identity of the thief's own; that is the cost of a lost chain key.*

*Presentation, for clients:* an owner's client tells the owner when the successor they named has not yet confirmed.

### 9. Competing rotations and several homes

*In short: two rotations at one position can only come from one chain key used twice: by a thief, by an owner's mistake, or in one of rule 29's two exceptions. The home rule decides which counts; at most one can. Two receipts from one home for different acts at one position are a conflict: the identity is contested there, never frozen. A home is proven dishonest at a position only as rule 45 says, and the verdict never reaches backwards.*

#### Rule 40. The home rule decides between competing rotations

A declared home rule decides which rotation counts when homes disagree. Both declared forms ensure that at most one of two competing rotations can ever count. If none meets the rule, the identity is contested until one does; no rotation counts meanwhile.

#### Rule 41. With no declared rule, the default

With several homes and no declared rule, the default of rule 2 applies.

#### Rule 42. A contested rotation's rule waits

A rule changed in a contested rotation takes effect only once the contest is settled. An invalid rotation never counts.

#### Rule 43. An outvoted home

A home left holding a losing rotation is outvoted. The owner may drop it in a later rotation.

#### Rule 44. Two receipts from one home: a conflict

Two receipts that pass receipt check 1, from the same home, for different acts at the same position of the same identity chain are a conflict. While it stands, the identity is contested at that position, not frozen. Anyone may publish the pair, and clients SHOULD flag the home to the owner.

#### Rule 45. Dishonesty, proven at a position

A home is proven dishonest for an identity at a position only when a conflict there survives the home's own rotations and, where the identity requires audit, both receipts sit under cosigned log summaries. Its receipts then count for nothing for that identity at later positions.

#### Conflicting receipts

Two receipts that pass receipt check 1 from the same operator, for the same identity and chain position but different acts, are a conflict, even if they come from different relays of that operator. *After check 1, a conflict needs two genuine rotations at one position, which needs the chain key. A stolen operator key alone can no longer touch any identity's chain.*

1. While a conflict stands, neither receipt counts, and that home counts for nothing at that position. The identity is contested there, not frozen.
2. The conflict ends when one of the two receipts becomes void: through the home's own rotation, or, for an identity that requires audit, because only one of them is included under a cosigned log summary.
3. The home is proven dishonest for that identity **at that position** only as rule 45 says: both receipts remain valid after the home's rotations, and, where the identity requires audit, both are included under cosigned log summaries. *For an identity that requires audit, step 2 already ends the conflict where only one of them is included.* From then on, its receipts count for nothing for that identity at positions after that one. Its receipts at earlier positions keep counting. *A thief holding the chain key and a bad home may contest the future; they cannot erase the past.*

### 10. Homeless rotations

*In short: when an identity's homes are gone, the owner holding the current chain key can leave them with a homeless rotation that names new homes and counts without the old homes' receipts. A valid objection from a live home of the old set voids it (rule 49; the exceptions are in the homeless procedure and section 11), and the objection proves the home was alive after the rotation was made. A home is shown to be gone by its operator's closing rotation, by the owner's declared auditors, or, weakest, by the reader's own failed attempt.*

#### Rule 46. The way out of homes that are gone

A homeless rotation lets an owner holding the current chain key leave homes that are gone, with no receipt from them. It counts only under the homeless procedure below.

#### Rule 47. A client first tries the old homes

A client meeting a homeless rotation MUST first try to reach the old homes and ask for an objection or a normal rotation.

#### Rule 48. An old home objects

A home of the old set that sees a homeless rotation for an identity it serves MUST NOT hold it as that identity's rotation. It SHOULD sign an objection naming it, and it remains open to a normal rotation from the owner.

#### Rule 49. One objection voids it

A single valid objection from a live home of the old set voids a homeless rotation.

*This is what stops a thief holding the chain key from using the homeless path to get past careful homes.* *An objection voided by its signer's rotation voids nothing, acknowledged or not.* *Stated cost: until the operator notices and rotates, an objection signed with a stolen operator key still voids the owner's homeless rotation; an owner holding both keys leaves at once by the escape (section 11).*

#### Rule 50. Closure, by the operator's rotation

A rotation MAY declare closure. Closure ends the signer's role as a home operator for good; it does not affect the signer's own sequences or identity.

*Closing a home is a rare, force-majeure case; spending a chain key on it is warranted, and it means a stolen signing key can never close a home.*

#### Rule 51. A home that closes says so

A home that closes SHOULD do so by a rotation of its operator declaring closure, so that its identities are provably homeless. An operator who has lost the chain key cannot close gracefully; its identities rely on auditors' absence statements or the verifier's own attempt.

#### Rule 52. Auditors for a single home

Clients SHOULD recommend declared auditors to owners with a single home.

*Without auditors, an owner whose home vanishes without closing is re-homed only on each client's own failed attempt to reach it, and a thief able to cut a victim off from the home could exploit that until the home's objection gets through; under rule 55 the thief can never make it final.*

#### The homeless procedure

A homeless rotation at position n counts only if all of the following hold:

1. It is a valid rotation with the homeless flag, and it declares a new set of homes.
2. No rotation at position n counts under the old home rule. A rotation that does, whenever its receipts appear, beats the homeless rotation.
3. No valid objection naming it exists from any home of the old set, except a home proven dishonest for that identity at an earlier position.
4. Enough old homes are gone that the old rule can no longer be met: the single or authoritative home, or homes of more than (number of distinct operators minus threshold) operators under a threshold. A home counts as gone when:
   - a rotation of its operator that counts declares closure; or
   - the identity requires audit, and at least the required number of its declared auditors have signed absence statements naming the rotation and that home; or
   - the identity does not require audit, and the verifier itself has tried and failed to reach the home.

   The audit requirement and the auditors here are those in effect before the homeless rotation, while the old homes served, never the ones it declares. *A thief holding the chain key can neither drop auditing to fall back on a reader's failed attempt, nor name auditors of its own to sign absence statements.*

   A self-hosted home is never gone: the owner is the home, and re-homes by serving a rotation from anywhere.
5. Receipts from the new homes meet the new home rule, judged under the audit requirement this rotation declares or inherits.

*Section 11 says when steps 3 and 4 do not apply (escape with both keys); section 12 says when a homeless rotation becomes final, and when it never does by the next rotation.*

#### The objection and absence statement checks

1. Each is valid as an everyday act of the home's operator or the auditor, and counts only while its signer's chain keeps it valid: one voided by its signer's rotation, acknowledged or not, is shown, as a dispute, and voids nothing and proves nothing gone.
2. An objection counts only from a home in the old home set of the identity it names, and not from a home whose operator's counting rotation has declared closure. An absence statement counts only from an auditor that identity declares.

*Because nobody can know an act id before the act exists, an objection naming a homeless rotation proves the home was alive after that rotation was made, and an absence statement proves the auditor looked for the home after it was made. No clock is needed.*

### 11. Escape with both keys

*In short: a home that is alive but hostile can refuse the owner's normal rotation and object to a homeless one. An owner who proves both keys, the chain key in the rotation and the signing key in an endorsement, leaves anyway; so does the owner of a self-hosted identity whose position a thief contests.*

#### Rule 53. Both keys always get out

A homeless rotation endorsed with the owner's current signing key cannot be voided by an objection. An owner holding both keys can always leave homes that refuse them.

#### Escape with both keys

A homeless rotation that has a valid escape endorsement counts without steps 3 and 4 of the homeless procedure: objections do not void it, and the old homes need not be gone. Steps 1, 2 and 5 still apply, so a rotation that counts under the old home rule at the same position still beats it, except a rotation the endorsement lists as abandoned.

*This is the way out from a home that is alive but hostile: it can refuse the owner, but it cannot hold an owner who proves both keys.*

It is also the way out of a position contested at a self-hosted home (rule 4): the endorsement lists both rotations served there as abandoned, and the air-gapped Module signs it only on the competing rotation's bytes (rule 29).

#### The escape endorsement

An escape endorsement names a homeless rotation in `objects`. Its payload may list, under key 0, earlier rotations at the same position that the owner abandons (for example one a home refused); an abandoned rotation can never count afterwards.

It is signed by the owner's current signing key: the one bound by the identity-chain act just before the homeless rotation. Its `binding` names that act, or, where that act is a chain signature (which sets no key), the identity-chain act that set the key. A homeless rotation with an endorsement carries both of the owner's keys: the chain key in the rotation, the signing key in the endorsement.

An escape endorsement is never judged by the rotation it endorses (rules 33 to 35): it cannot lie in that rotation's kept ancestry, since it names the rotation and so is made after it.

### 12. Finality

*In short: a homeless rotation is final once the next rotation counts under the home rule it declared; after that, no objection voids it. Two limits: a rotation the old homes held at the same position still beats it, final or not; and one accepted only on the reader's own failed attempt is never made final by the next rotation.*

#### Finality

A homeless rotation at position n is final once a rotation at position n plus one counts under the home rule the homeless rotation declared, unless it counts only through the verifier's own failed attempt (below). After that, no objection can void it. Until then it is provisional.

*Nobody can name an act before it exists, so anything surfacing after the next rotation is late by construction. No clock, no anchor, no higher layer.*

Finality never reaches past step 2 of the homeless procedure: a rotation that counts under the old home rule at the same position beats the homeless rotation, final or not (rule 54).

*A verifier sees only the acts it holds, never when each arrived. Were finality to beat the old homes' receipts, a thief who found a used chain key on an old backup could, once the home closed, make a homeless rotation at that old position, rotate again at once, and replace every rotation the owner made since. A rotation the old homes held is first held, however late its receipts surface.*

#### Rule 54. The old homes' rotation always wins

A rotation that counts under the old home rule always beats a homeless rotation at the same position, even one made final by the next rotation.

#### Re-homed without audit

A homeless rotation that counts only through the verifier's own failed attempt (the last case of step 4) MUST be shown as "re-homed without audit", and nothing binding may rely on it, as the list after rule 35 says ("Every verifier gives the same answer"): no keeper record, payment, discharge of a debt, agreement, fork or closing. Reading and following the identity, and its everyday acts, may.

It is never made final by the next rotation: steps 2 and 3 keep applying, so a valid objection from a home of the old set voids it whenever it surfaces, and with it every rotation built on it. The latest act that counts is then the one at position n minus one.

It is final like any other homeless rotation only once it counts through something other than the verifier's attempt: a closure by the old home's operator (step 4, first case), or an escape endorsement (section 11).

*The finality reasoning above holds for acts that did not exist yet, not for an objection that existed but was kept from arriving. A thief holding the chain key controls the next rotation and chose the homes it counts under; if that rotation made the first final, a censor who blocks the real home could make a theft final inside its borders.*

*Stated cost: an honest owner whose single home vanished without closing, with no auditors, stays re-homed without audit unless they endorse the homeless rotation with their current signing key. Only an owner who has also lost the signing key stays so for good: a rare case.*

#### Rule 55. Re-homed without audit is never final by the next rotation

A homeless rotation that counts only through the verifier's own failed attempt to reach an old home is never made final by the next rotation: a valid objection voids it, and every rotation built on it, whenever the objection surfaces.

### 13. Audit

*In short: an owner may require that their receipts be audited: other operators, declared by the owner, cosign each home's log summaries after checking that each extends the last. A receipt under a cosigned summary is shared history: the home can no longer erase it, and a forged one stands out. Auditing is optional, and the chain key alone can change it.*

#### Rule 56. Audit is the owner's choice

An identity MAY require audited receipts, declaring its auditors and how many cosignatures a log summary needs. The requirement is declared in genesis or a rotation, so only the chain key can change it. Auditing is additional security, never compulsory.

Receipts for a rotation are judged under the requirement that rotation declares, or inherits if it declares none, so an owner can always drop or replace auditors who have stopped cosigning.

#### Log summaries and audits

1. A home MUST NOT sign two log summaries of which neither extends the other. Two such summaries carry no verdict on any receipt: auditors MUST NOT cosign further summaries of that home, and clients SHOULD flag the home to its owners. Receipts already under a cosigned summary stay protected; the rest are judged as before.
2. An auditor MUST check a consistency proof from the last summary of that home it cosigned before cosigning a new one, and MUST NOT cosign two summaries of the same home of which neither extends the other.
3. A cosignature counts only if its signer is declared as an auditor by the identity whose receipt is being checked, and only while its signer's chain keeps it valid: one voided by the auditor's rotation, acknowledged or not, protects no receipt.

#### Inclusion proofs travel

Proofs are not acts: a home serves them on request, alongside the summary, and anyone may carry them, since each is checked against a signed summary. An owner's client SHOULD keep the inclusion proofs of its own receipts under cosigned summaries, and hand them to its new homes when it re-homes.

*Otherwise, once a home vanishes, a reader who never read it before cannot prove any receipt it signed, and an audited identity's chain would stop at genesis for that reader, homeless rotation included.* *Stated cost: proofs the owner never kept are lost with the home.*

How often homes publish summaries, and which homes audit which, is left to the homes and the market. The core defines only the acts, and the yes-or-no checks.

## Part three: encoding and act formats

The encoding and the formats below bind as the rules do: an act outside them is invalid. *The rules above say what these acts mean and when they count.*

### Encoding and conventions

*The envelope itself is defined by the Envelopes MIR. This section repeats only what Identity needs.*

**Encoding.** Every act, and every structure the MICS hashes or signs, is encoded in deterministic CBOR. A verifier MUST reject any act whose bytes are not in that encoding. Deterministic CBOR is RFC 8949's core deterministic encoding (section 4.2.1), written out here once for every MIR:

1. Every integer, length and tag number is in the shortest form that holds it.
2. Every string, array and map has a definite length.
3. The keys of each map are distinct and ordered by the bytewise lexicographic order of their encodings.
4. A floating-point value is in the shortest of the half, single and double forms that preserves its value exactly, NaN payloads included (RFC 8949, section 4.1, preferred serialization).
5. A text string is valid UTF-8.
6. No data item is nested more than 128 levels below the outermost one (Envelopes rule 2): an array's items, a map's keys and values, and a tag's content each sit one level below what holds them.
7. The bytes hold exactly one data item, with nothing after it.
8. A simple value is written in one byte where it can be (0 to 23, `false`, `true`, `null`, `undefined`); a two-byte simple value below 32 is not well-formed.

Tags and simple values are allowed by the encoding and carried as they are, a tag's content decoded and checked as any item (its text strings are text fields, Text) and its meaning never checked; whether a field may hold one is its format's to say, and no format of the MICS allows a tag, a float or a simple value other than `true`, `false` and `null` where it does not write `any`.

**Formats.** Formats are written in CDDL (RFC 8610), with these conventions, which every MIR follows:

- Every map a format defines is closed. Its keys are the small unsigned integers it lists. A key it does not list, a key that is not an unsigned integer, a required key missing, or a value of another type than the one written makes the act invalid. (Duplicate keys never reach this point: the encoding refuses them.)
- `? n => x` is optional: absent, the field is not given. It is never written as `null` unless the format writes `/ null`. Where a format says a value is written as absent (a route's kind 0), writing it makes the act invalid.
- `uint` is an unsigned integer (major type 0), from 0 to 2^64 − 1; a negative integer is never a `uint`. `bstr` is a byte string; `bstr .size n` one of exactly `n` bytes; `hash` is `bstr .size 32`. `tstr` is a text string, and every `tstr` is canonical text (Text MIR), including address hints: a genesis with a trailing space in a hint is invalid. `true` is the value true alone. `any` is any data item the encoding allows.
- A map written `{ * any => any }` (an act's payload in the envelope) takes any keys at the encoding's level; the format of the act's type closes it.
- `[ a, b ]` is an array of exactly those items, in that order, a trailing `? c` being one item more or none; `[+ x]` is an array of one or more `x`; `[* x]` of zero or more.

The core sets no length bound beyond these; homes and relays set their own policy.

**Hashes.** Every hash is SHA-256, 32 bytes. A tagged hash is computed as in BIP-340:
`tagged_hash(tag, x) = SHA-256(SHA-256(tag) || SHA-256(tag) || x)`, the tag hashed as its UTF-8 bytes. Every tag the core uses begins with `MOR/`: `MOR/act`, `MOR/inside`, `MOR/mmr-leaf`, `MOR/mmr-node`, `MOR/work`, `MOR/spec`, `MOR/safety`, `MOR/sealed/wrap`, `MOR/finance/anonymous-claim` (Money) and `MOR/law/leftover` (Agreements). A plain SHA-256, untagged, is used only where a text says so: the locked hash (Envelopes), the Merkle tree of a home's log (RFC 9162, below), and `XWING` (Envelopes).
*Tags keep the hashes of different kinds of object apart, so one can never be passed off as another. A successor protocol MUST use a different tag prefix, so its acts can never replay here (Development).*

**Act id.** The act id is `tagged_hash("MOR/act", outside)`, where `outside` is the encoded outside of the act, which commits to its locked inside (Envelopes MIR). The signature is not part of the act id, so a signature can never change which act is meant.

**Signature schemes.** A scheme is named by a founding number, or by the hash of a signature-scheme specification (Development) that defines its key encoding, signature encoding and verification.

| Scheme | Name | Public key | Signature | Used by |
| --- | --- | --- | --- | --- |
| 1 | Schnorr over secp256k1 (BIP-340) | 32 bytes (x-only) | 64 bytes | signing keys |
| 2 | SLH-DSA-SHA2-128s (FIPS 205) | 32 bytes | 7,856 bytes | chain keys |
| 3 | SLH-DSA-SHA2-128f (FIPS 205) | 32 bytes | 17,088 bytes | chain keys |
| hash | as the specification defines | as defined | as defined | either, as the specification says |

In a signature, a signing key or a chain-key commitment, the scheme is written as the unsigned integer 1, 2 or 3, or as a 32-byte spec hash; any other value makes the act invalid. A key or a signature of a founding scheme whose length is not the one in the table makes the act invalid. A scheme named by hash is a signature-scheme specification (Development, kind 5): a client that has not adopted it answers unknown, whatever the lengths. An act's signature always signs the 32-byte act id; a signature a format carries inside a payload signs what that format says (Money's anonymous claim, key 8). SLH-DSA signatures are pure FIPS 205 signatures (no pre-hash) with the context string `MOR`, its three ASCII bytes. Both SLH-DSA variants are allowed; the owner's client chooses. The small variant (2) is recommended. A client that meets a scheme it does not implement MUST treat the act as unknown, never as valid. *A rotation's own validity never depends on the kind of signing key it installs, so every client verifies the rotation; only acts signed under the new scheme are unknown to a client that lacks it. This is how signing keys migrate to a post-quantum scheme before a break: by adoption, with no change to the core.*

**Chain key commitment.** A chain key is committed as `tagged_hash("MOR/safety", scheme || key)`, where `scheme` is the one-byte number or the 32-byte specification hash, and `key` the public key.

**Spec hash.** Every act names the specification that defines its type by that specification's spec hash. For the acts below it is this MIR's spec hash, written `IDENTITY` here: `tagged_hash("MOR/spec", content)`, where `content` is the deterministic CBOR map `{ 0 => 4, 9 => text }` holding this MIR's frozen text, and nothing else (Development rule 26).

### Written for people

*In short: every identity, act, key, specification and work a person copies is written with a short label saying what it is and a checksum that refuses a mistyped or cut string; the longer form adds where an identity lives; plain hexadecimal still works.*

**The form.** An object a person copies is written in bech32m (BIP-350), in small letters: its label, the separator `1`, its data in bech32's 32 characters, and a six-character checksum. A reader also accepts the same string written all in capitals (BIP-350; a QR code's), and refuses one mixing capitals and small letters.

**The labels**, one per kind of thing hashed, never per meaning:

| Label | What it names | Its data |
| --- | --- | --- |
| `mid` | an identity: a person's, a collective's, an agent's | its 32-byte identity hash |
| `mact` | an act (a vow, an offer, an agreement and a publication are acts) | its 32-byte act id |
| `mkey` | a public key, signing or encryption | Identity's `[scheme, key]`, or for an encryption key the Envelopes MIR's `enc-key`, in deterministic CBOR |
| `mspec` | a specification: a MIR, a cMIP, a Module or a MIP | its 32-byte spec hash |
| `mwork` | a work: the hash of a file's bytes | its 32-byte work hash |

*A label says what was hashed, not what the thing means: a label per meaning would grow with every type, and would claim what the encoding cannot check. What an object is beyond its kind, a reader learns by looking it up, and the client shows it.*

**A reader refuses, saying why:** a checksum that does not hold (a character mistyped, swapped, added or missing: the string was cut or mistyped); mixed capitals and small letters; a label neither this section nor a specification the reader implements defines; data of the wrong length or shape for its label; and **a string of another kind than the one asked**, naming both ("this is an act (mact), not an identity (mid)").

**No form for any secret.** No label writes a private key, a seed or a content key: signing words stay words, in the form the Module that carries an identity defines, and nothing secret can look like something safe to paste.

**Where an identity lives.** The where-it-lives form is the short form followed by `?r=` and an address hint, percent-encoded where it must be (RFC 3986), repeated with `&r=` for each further hint: `mid1…?r=https://home.example.org`. The hints lie outside the checksum: **hints guide, never decide** (Envelopes rule 6). A reader may look there first; what it finds counts only as the signed acts it gets back.

**Bare hexadecimal stays accepted** for a 32-byte hash: 64 characters, 0 to 9 and a to f, read as the kind the reader asked for. *Every identity, act and specification written in hexadecimal stays readable.*

**A kind a cMIP defines** may write its own objects in the same form under a label of its own, written in its text, never one of the five above.

*Stated costs: the form is about as long as hexadecimal for a hash (62 characters for an identity, against 64), not shorter; BIP-350's guarantee to catch any four errors holds for strings up to 90 characters, which every label but an encryption key's (`mkey` of an X-Wing key, about 1,950 characters) stays within; past it, an error is still caught but for one string in about a billion. A label is a short word in English letters, chosen to read plainly in code.*

### The envelope

Every act is an Envelopes MIR act: an outside (signer, binding, commitments, and for a public act its content key), a locked inside (spec, type, prev, objects, payload, position, summary, acks, refs, hint, salt), and a signature. Below, "signer" and "binding" are outside fields; "prev", "objects" and "payload" are inside fields.

Every act defined by this MIR is public, except a private link, as section 3 says.

Every act has exactly one signer (section 2).

`prev` holds one act id in the ordinary case, and none (an empty array) when the act starts a new sequence. Genesis, rotations and chain signatures carry neither `binding` nor `prev`: they belong to the identity chain, not to a sequence. Their position is the one in their payload (field 1; genesis is position 0); an inside position or running summary (inside keys 5 and 6) on an identity-chain act is not read, and nor are its `acks`, `refs`, `hint` and outside `to`. Genesis carries no `objects`; a rotation's or chain signature's `objects`, if any, are not read.

**What an Identity act names in `objects`.** Where this MIR says an act names another in `objects`, the act carries the entry `[ chain, named ]`, `named` being the act named, and `chain` the identity whose chain or sequence it concerns:

| Act | Its entry | What a verifier reads |
| --- | --- | --- |
| Name withdrawal (5) | `[ signer, name act ]` | an entry whose second item is the name act |
| Link confirmation (7), of a claim | `[ claim's signer, claim ]` | an entry whose second item is the claim |
| Link confirmation (7), of a succession | `[ rotation's signer, rotation ]` | an entry whose second item is the rotation |
| Link termination (8) | `[ claim's signer, claim ]` | an entry whose second item is the claim |
| Cosignature (10) | `[ home's operator, log summary ]` | an entry whose second item is the log summary |
| Objection (12), absence statement (13), escape endorsement (14) | `[ identity, homeless rotation ]`, `identity` the one the rotation is for | exactly that entry: the chain MUST be that identity |

*An entry's first item is where a reader looks; for the three acts of the homeless path it is also checked, since what they say is about one identity's chain.* For the acts of the first five rows, one entry is enough, and an act carrying more names each act an entry names. Such an entry names an act; it is no fork of that act's chain (Envelopes rule 15).

```cddl
hash   = bstr .size 32
scheme = 1 / 2 / 3 / hash
```

### Shared parts

```cddl
signing-key   = [ scheme, key: bstr ]
chain-key-commit = [ scheme, commit: hash ]

home      = [ operator: hash / null, hint: tstr ]   ; null: self-hosted, operated by this identity
home-rule = [ 0, index: uint ]                      ; one authoritative home
          / [ 1, threshold: uint ]                  ; a majority of homes

audit = [ threshold: uint, auditors: [+ hash] ]     ; cosignatures required, and from whom

declaration = [ spec: hash, kind: uint, value: any / null ]   ; null removes the kind
successor   = [ protocol: tstr, identifier: bstr ]  ; protocol "mor" for a MOR identity, its identifier the 32-byte identity hash;
                                                    ; a "mor" identifier (the three small letters exactly) of another length names no MOR identity, and no client follows it
```

How an address hint and a null operator are read is in section 4 ("A home's address").

### Types defined by this MIR

| Type | Name | Signed by | Adds to |
| --- | --- | --- | --- |
| 0 | Genesis | first signing key | identity chain |
| 1 | Rotation | revealed chain key | identity chain |
| 2 | Receipt | the operator's signing key | the operator's sequence |
| 3 | Routes | signing key | a sequence |
| 4 | Name | signing key | a sequence |
| 5 | Name withdrawal | signing key | a sequence |
| 6 | Link claim | signing key | a sequence |
| 7 | Link confirmation | signing key | a sequence |
| 8 | Link termination | signing key | a sequence |
| 9 | Log summary | the operator's signing key | the operator's sequence |
| 10 | Cosignature | the auditor's signing key | the auditor's sequence |
| 12 | Objection | the operator's signing key | the operator's sequence |
| 13 | Absence statement | the auditor's signing key | the auditor's sequence |
| 14 | Escape endorsement | the owner's current signing key | a sequence |
| 15 | Witness | signing key | a sequence |
| 16 | Chain signature | revealed chain key | identity chain |

*Type 11 is retired: a home closes by a rotation of its operator (rule 50). The number is never reused.*

### Genesis (type 0)

```cddl
genesis-payload = {
  0 => signing-key,       ; the first signing key
  1 => chain-key-commit,     ; commitment to the first chain key
  2 => [+ home],          ; one or more homes
  ? 3 => home-rule,       ; only with several homes; absent means the default (rule 2)
  ? 4 => [+ declaration], ; high-risk settings defined by higher MIRs
  ? 5 => audit            ; require audited receipts; absent means not required
}
```

Outside: no `signer` or `binding`. Inside: `spec`, `type` 0 and `payload`, with no `prev` or `objects`. The signature is by the first signing key. The identity hash is the act id of the genesis.

### Rotation (type 1)

```cddl
rotation-payload = {
  0 => hash,               ; the previous identity-chain act
  1 => uint,               ; position in the identity chain: 1 for the first rotation
  2 => signing-key,        ; the new signing key
  3 => chain-key-commit,      ; commitment to the next chain key
  4 => [* kept-tip],       ; kept: the latest genuine act of each kept sequence
  ? 5 => [+ hash],         ; disowned: acts within the kept ancestry the owner voids
  ? 6 => [+ home],         ; a new set of homes
  ? 7 => home-rule / null, ; a new home rule; null returns to the default
  ? 8 => [+ declaration],  ; new high-risk settings, replacing those of the same kind
  ? 9 => successor / null, ; a succession; null ends the current one
  ? 10 => audit / null,    ; a new audit requirement; null removes it
  ? 11 => true,            ; homeless: counts without the old homes (field 6 required)
  ? 12 => true             ; closure: the signer's home stops serving every identity
}

kept-tip = [ act: hash, position: uint, summary: hash ]   ; the tip's id, its position, and the running summary including it
```

Outside: `signer`, no `binding`. Inside: `spec`, `type` 1 and `payload`, with no `prev`. The signature is by the chain key committed in the previous identity-chain act; its public key is revealed in the signature itself. Field 1 is at least 1. A rotation carrying field 11 carries field 6; one that does not is invalid (rotation check 4). Fields 11 and 12 are written only as `true`, absent otherwise.

### Receipt (type 2)

```cddl
receipt-payload = {
  0 => hash,   ; the identity whose chain act is held
  1 => hash,   ; the identity-chain act held
  2 => uint,   ; its position in that identity chain
  3 => uint    ; its position in the home's own log
}
```

A receipt is an everyday act of the home's operator. Its log position is a counter kept by the home, starting at 0 for the first receipt it signs and increasing by one with every receipt, for any identity; it is the receipt's index in the home's Merkle tree (below). It is never a time.

### Routes (type 3)

```cddl
routes-payload = {
  0 => uint,               ; version: 1 for the first routes act, then the previous version plus one
  ? 1 => hash,             ; the previous routes act (absent only for version 1)
  2 => [* route]
}
route = [ scope: hash / null, hints: [+ tstr], ? kind: 0 / 1 ]
; scope: a spec hash, or null for everything else
; kind 0 (default): outbox, where this identity's content of that scope is found; written as absent, never as 0
; kind 1: inbox, where others deliver acts of that scope addressed to this identity
```

Version 0 is no version: a routes act carrying it is invalid, as is version 1 carrying field 1, or a later version without it.

Routes acts form their own chain: each names the one it replaces, and its version is exactly one more. *Nobody can jump ahead with a very high number, and two routes acts naming the same predecessor are a visible fork.* An identity MAY declare no inbox. *The reserved inbox kind is shared language, not a mandate: it exists so that any two identities can find each other without already sharing a relay, through identity, home and routes.*

### Name (type 4) and name withdrawal (type 5)

```cddl
name-payload = {
  0 => tstr,   ; the name
  1 => hash    ; the naming cMIP that defines its form and resolution
}
```

A name withdrawal names the name act in `objects` and has an empty payload.

### Link claim (type 6), confirmation (type 7) and termination (type 8)

```cddl
link-claim-payload = {
  0 => successor,   ; the other side: protocol and identifier (same shape as a successor)
  ? 1 => bstr       ; proof from the other protocol, as defined by a cMIP
}
```

A confirmation names the claim in `objects` and has an empty payload; it is signed by the MOR identity the claim names. When the other side is an account on another protocol, it confirms instead by signing the claim's act id in its own protocol; that signature is carried in the claim's proof field, and a cMIP for that protocol defines how to check it. Either side may start: an account elsewhere can sign first, and the MOR identity then publishes the claim carrying that proof. A termination names the claim in `objects`, has an empty payload, and is signed by the MOR side; an account elsewhere ends a link by the means its own protocol offers. A private link is stored as encrypted acts, in the form the Envelopes MIR defines.

A confirmation of a succession names, in `objects`, the rotation that declares it, has an empty payload, and is signed by the MOR identity the rotation names as successor; a witness act of that identity naming the rotation in its `acks` confirms it as well. What a confirmation of a succession means is in section 8.

### Log summary (type 9) and cosignature (type 10)

```cddl
log-summary-payload = {
  0 => uint,          ; size: number of receipts in the home's log
  1 => hash,          ; root: Merkle tree root over the receipts' act ids, in log order
  ? 2 => hash         ; the home's previous log summary; absent for the first
}
```

The Merkle tree is built as in RFC 9162, section 2.1, with plain SHA-256, over the receipts' act ids in log order: a leaf is `SHA-256(0x00 || act id)`, a node `SHA-256(0x01 || left || right)`, the empty tree's root `SHA-256()` of nothing. A receipt's log position is its index in that tree, from 0. An inclusion proof shows that a receipt sits at its position under a root; a consistency proof shows that a later summary extends an earlier one. Who serves and carries proofs is in section 13 ("Inclusion proofs travel").

**Proofs, as a verifier is handed them.** Proofs are not acts. How a client asks a home for them is the relay transport cMIP's; what a verifier checks of a proof it is handed, in this encoding, is the core's:

```cddl
inclusion-proof   = [ summary: hash, receipt: hash, index: uint, path: [* hash] ]
consistency-proof = [ old: hash, new: hash, path: [* hash] ]
```

- In an inclusion proof, `summary` is the act id of a log summary, `receipt` the act id of a receipt signed by the same operator, `index` its log position (equal to the receipt's field 3; otherwise the proof proves nothing), and `path` the audit path of RFC 9162, section 2.1.3.1, from the leaf upward. The verifier holds both acts and checks the path as RFC 9162, section 2.1.3.2, against the summary's size (field 0) and root (field 1); `index` lower than the size.
- In a consistency proof, `old` and `new` are the act ids of two log summaries of one operator, `path` the proof of RFC 9162, section 2.1.4.1. The verifier holds both and checks it as section 2.1.4.2, against their sizes and roots; an old size of 0, or larger than the new, proves nothing.

A proof that fails its check proves nothing, and makes no act invalid.

A cosignature names the log summary in `objects` and has an empty payload. It is signed by the auditing identity.

### Objection (type 12) and absence statement (type 13)

```cddl
objection-payload = {
  0 => hash                       ; the identity the homeless rotation claims
}

absence-payload = {
  0 => hash,                      ; the home found not answering (its operator)
  ? 1 => hash                     ; the last log summary of that home the auditor has seen
}
```

An objection and an absence statement name the homeless rotation in `objects` (section 10).

### Escape endorsement (type 14)

```cddl
escape-endorsement-payload = {
  ? 0 => [+ hash]       ; earlier rotations at the same position the owner abandons
}
```

An escape endorsement names a homeless rotation in `objects` (`[ identity, homeless rotation ]`). What it abandons and who signs it are in section 11 ("The escape endorsement").

### Chain signature (type 16)

```cddl
chain-signature-payload = {
  0 => hash,               ; the previous identity-chain act
  1 => uint,               ; position in the identity chain
  2 => chain-key-commit,      ; commitment to the next chain key
  3 => hash                ; the act it signs
}
```

Outside: `signer`, no `binding`. Inside: `spec`, `type` 16 and `payload`, with no `prev`. The signature is by the chain key committed in the previous identity-chain act, revealed in the signature itself, as for a rotation. How it counts, and what it changes, is in section 7.

### Witness (type 15)

```cddl
witness-payload = { }             ; empty: the type says it all
```

The acts a witness act witnesses are named in its `acks` (Envelopes), at least one; it carries no `objects`. What it says, and its rules, are in section 3 (rules 10 and 11).

## Part four: tasks, reasoning and freeze scenarios

### Tasks

- **Identity proofs and credentials.** A cMIP accepts an identity hash and a claim, and produces a verification answer (valid, invalid, pending, unknown) with the hash of the module that computed it. Sign-in flows are built on this task.
- **Naming.** A cMIP accepts a name and produces an identity hash, or accepts an identity hash and produces its names. A name resolves only if the identity signs it.
- **Outside link proofs.** For a given protocol, a cMIP accepts a link claim and its proof, and produces a verification answer (valid, invalid, unknown): whether the account named really signed the claim's act id. One cMIP per protocol; clients opt in to the protocols they support.

Bridges themselves, the software that carries content, messages or payments between MOR and another protocol, are Modules built on these cMIPs and on grants.

A collective is an identity like any other. Its keys may be held under its founding agreement's key grammar (Agreements): by one holder, by a threshold of members producing one ordinary signature, or by a custodian. Every grammar leaves a way to rotate that needs less than every member (Agreements). Identity sees only the resulting signatures.

Key storage, backup and recovery are Modules built on this vocabulary; they need no task because they create no act the core must read. Signature-scheme specifications are published through Development; a new signing-key scheme is adopted by clients implementing it, and installed by rotation.

### Reasoning

- **The hash is the identity, not a key.** *Keys can be lost or stolen; an identity that is a key dies with it. A hash of genesis lets keys change while everything pointing to the identity stays valid.*
- **Two keys with different jobs.** *The signing key is exposed daily, so its loss must be survivable. The chain key is used once and kept offline, so it can afford heavy, post-quantum cryptography. Committing each chain key by hash before it is needed means a replacement always exists before the key it replaces is used.*
- **Schemes are named, not fixed.** *A scheme can be a founding number or the hash of a specification. Nothing in a rotation's validity depends on what kind of signing key it installs, so when a post-quantum signing-key scheme is standardised, owners migrate by rotating to it and clients by implementing it. Migration must come before a break: after one, everything signed with the old scheme can be forged, homes and auditors included, and only owners who hold both keys still leave.*
- **Genesis is signed with the signing key, and that is safe.** *Forging a genesis only creates a different identity, never a copy of an existing one, because the identity hash fixes the original.*
- **Rotations are large by design.** *A post-quantum signature makes a rotation several kilobytes. For an act this rare, that is a deliberate trade, and implementers should not try to shrink it.*
- **The identity chain holds keys only.** *The home then needs to know nothing about what the identity does: messages, media and money live on whatever relays the owner chooses. Private acts never reach the home.*
- **Sequences, not one history.** *Hash links work across relays, so the owner may keep acts in any number of lines, per layer or per device. A rotation needs only the latest genuine act of each line to say what survives.*
- **Nothing relied on is silently erased.** *An act outside the kept ancestry, or disowned inside it, is void only if nobody else relied on it in a way the protocol can see. Otherwise it stays visible as a dispute. Disowning costs the owner nothing else: the genuine acts after a thief's act stay valid.*
- **A home verifies but does not vouch.** *Everything it serves can be re-checked, so a dishonest home can hide or delay a rotation, never fake one. Homes guarding acceptance are a market service, not a protocol duty.*
- **Homes are relays, run by operators.** *A relay is infrastructure, not an identity. Its operator's ordinary identity signs what the home states, so the operator's keys can rotate like anyone's, and a stolen key's forged receipts die with the rotation. Counting homes per operator makes centralisation visible under honest labelling; it does not detect one company behind three operators, and the core does not try to.*
- **Receipts make homes answerable, and name real acts.** *What passes between a dishonest home and a thief is private and cannot be made public. A receipt makes it provable whenever it surfaces, so a home cannot tell different clients different stories without leaving evidence. A receipt counts only alongside the act it names, so a stolen operator key can produce nothing that changes any identity's chain: a conflict needs two genuine rotations, which needs the chain key. Because a home signs, it has its own identity, which lets it move servers without the owner spending a chain key.*
- **Homes audit each other.** *A stolen home key can sign false receipts, but it cannot get them into a log summary other homes have already cosigned. Auditing turns forgeries into self-evident ones without waiting for the home to rotate, and makes it impossible for a home to show different logs to different people without other homes cosigning both. Two non-extending summaries are a reason to stop auditing the home, never a verdict on its past receipts. The core defines the acts and the yes-or-no checks; how often and by whom is the market's business.*
- **Contested, not frozen; and never backwards.** *A conflict between two receipts can only come from two genuine rotations. Treating it as a contest that the home's rotation or an audit can settle means nothing freezes; and a dishonesty verdict is bound to the position where it was proven, so a thief with the chain key and a bad home can contest the future but never erase what came before.*
- **The log counter covers every identity.** *A home's log position increases across all the identities it serves, so a summary reveals the home's total volume. That is the price of one log that can be audited as a whole.*
- **Two forms of home rule, one default.** *With one authoritative home, the others are simple backups: copies to fall back on. With a majority threshold, they form a safety network: a thief must win over most of them. Both forms allow at most one competing rotation to count. An owner who declares neither gets the majority rule, computed from the home list, or, beside a self-hosted home, the self-host as authoritative. A single home stays possible at genesis: forcing several is too much friction for a new user; hygiene is recommended, never forced.*
- **Self-hosting is a trust model.** *An identity that is its own home has chosen to be trusted on its own signatures. Its rotation counts with no receipt, which is the only way it can rotate at all; a thief holding its chain key can serve a rotation of its own, which counts where the owner serves none. That is the same risk as a single lax home, chosen knowingly, and the most sovereign setup is no longer the one that cannot rotate. Where the owner serves a rotation too, nobody can settle the contest, since no third party fixes an order; the owner holding both keys leaves it by the escape, which the air-gapped Module signs on the thief's rotation's bytes.*
- **First held wins.** *Without a third key, the only order the protocol can see is the order in which homes accepted rotations. A stolen chain key is like a stolen bitcoin key, unless the owner chose homes that guard against it.*
- **Links exist for signing in and for bridges.** *Signing in with MOR is essential, so the core gives every protocol and service one shared, two-way statement to read: this MOR identity and that account are the same entity. Anyone can build a bridge on it. A link needs both sides, otherwise anyone could claim to be anyone, and either side can leave, because nobody should be kept linked against their will. It confers no authority: a bridge that acts for someone needs a grant.*
- **High-risk settings ride on the chain key.** *Anything whose theft would cause lasting harm, such as where money is protected, who succeeds the identity, or that a home has closed, is declared in genesis or a rotation. A thief holding only the signing key cannot change it. Higher MIRs decide what counts as high-risk; Identity only carries the slot.*
- **Every protection the owner chooses is removable by the chain key alone.** *An audit requirement is judged under the rotation that changes it, so auditors who have gone cannot lock the owner in. A device policy is the one protection the core cannot make removable, because a refusal leaves no evidence; so the text says what it costs, and clients never choose it by default.*
- **Succession rides on rotation.** *The chain key keeps its single use. Tying succession to it means a thief holding only the signing key can never redirect an identity's followers, agreements or money. Leaving is as protected as a rotation, and as rare.*
- **A succession needs both sides.** *Like a link, a succession is a statement about two identities, so the second signs too: the rotation declares, the successor confirms. Nobody can be made a successor, of followers or of a collective's seat, without agreeing to it.*
- **Evidence stands only with its signer.** *What someone relied on stands, even when its signer disowns it: that protects those who relied on the signer's own acts. Evidence is different: an objection, an absence statement, a cosignature, an acknowledgement or a receipt decides something about someone else, so it must still be its signer's word. Voided by its signer's rotation, it is shown and decides nothing; otherwise a stolen key and one accomplice could block a way out for good.*
- **Unknown when the picture is partial.** *A verifier that lacks an act cannot see what that act would show; saying void or invalid then is a guess that flips as acts arrive. Unknown is honest, and every verifier converges on the same final answer once the acts are held.*
- **Reliance is a deliberate act.** *An acknowledgement carries weight: it can keep a thief's act alive as a dispute. A gesture as light as a like must never carry that weight by accident, so acknowledgements live only on the act types of the three layers that need them, and reliance on anything else is a witness act, signed on purpose and explained before signing. A thief's accomplice can still witness the thief's acts, making a visible dispute, never a valid act; what no like or reply can do is keep a thief's words alive.*
- **One signer per act.** *Every act is signed by exactly one identity, so every act sits cleanly in one sequence and has one binding to check. Agreement between two identities is expressed as two acts, one naming the other, which is also how the second side stays free to refuse.*
- **The signature is outside the act id.** *A signature can be recomputed in more than one valid way. Keeping it out of the act id means the id never changes, so every pointer to an act stays valid.*
- **Nobody dies with their home.** *In a one-home setup, a vanished home would otherwise mean the death of the identity. Silence cannot be proven, but life can: a home that names the homeless rotation in an objection proves it was alive after the rotation was made. So the homeless path is closed whenever the home can answer, and open when it cannot. Three layers set how sure the rest of the network can be that a home is gone: its operator's closing rotation, the owner's declared auditors, or, weakest, each client's own attempt. And a homeless rotation is final once the owner's next rotation counts, which every Identity client can check; except one accepted only on a client's own failed attempt, since a thief holding the chain key controls that next rotation and a censor can keep the objection from arriving. That one stays open to the objection for good, unless the owner endorses it with the signing key.*
- **Anyone can find anyone.** *Without a standard place to deliver, the largest relays become the only meeting points, which is centralisation by default. A reserved inbox route is the right of exit applied to communication: it is shared language, an identity may declare none, and relay operators decide whom they accept deliveries from and at what price.*
- **Losing the chain key is permanent.** *An owner who loses the chain key, and its seed, can never rotate, change homes or leave. The identity keeps working with its signing key until that is lost too. Back up the chain-key seed as carefully as the key itself.*
- **Protected history.** *Once other operators have cosigned a log summary, the receipts under it are part of shared history: the operator can no longer erase them, by mistake or on purpose. Identities that do not require audits trust their home for this.*
- **Both keys always get out.** *A careful home's refusal and a hostile home's refusal look identical from outside, so the protocol cannot judge the home. It judges the owner instead: an owner who proves both keys can leave any home. Keeping the keys separate, and backed up, is what makes that proof strong and that exit available.*
- **The way out.** *The post-quantum chain key keeps continuity through a break of elliptic-curve signatures. It secures the exit, not what is left behind: history signed with the signing key relies on anchoring before a break, and signing keys should migrate to a post-quantum scheme before one.*

### Parameters

Settled in this MIR: hash function (SHA-256), encoding (deterministic CBOR), founding signature schemes and the specification-hash form, chain key commitment, receipt format, home address changes (through the home's own routes), routes with an inbox kind, closure by rotation.

Left to the relay transport cMIP: how a home is queried (the requests and answers for an identity's chain, receipts, routes, names, public links, log summaries and proofs). The core fixes only what a verifier checks of what it is handed: the acts, and the proofs in the encoding Part three gives ("Proofs, as a verifier is handed them").

Not in MOR 1: a shorter proof that an act lies within a kept ancestry than rebuilding the line (Envelopes: sequence position and running summary). A verifier that cannot rebuild a kept line answers unknown, never void; such a proof (the running summary's peaks and a path) can follow after MOR 1 without changing any act.

Fixed at the freeze: this MIR's spec hash, computed from its frozen text as Development rule 26 says, used as `IDENTITY` in every act it defines.

### Freeze scenarios

Components exercised (freeze test suite v30; scenario and step numbers are the suite's):

- Genesis, homes and routes, including an inbox route: all scenarios.
- Rotation by chain key, delivered at the home: 1, 5; pending until the home rule is met: a component under "Identity".
- Receipts naming real acts; a forged receipt changes nothing; conflicts only between genuine rotations; dishonesty bound to a position: 5.
- Competing rotation: first held by the homes that count wins; home acceptance policies; the lost device with a backed-up signing seed: 5.
- Home rules: authoritative, majority, majority by default; self-hosted default: 5.
- Self-hosted identity rotating with no receipt; the stated cost: 5.
- Identity chain of key events; rotation keeps the named acts' ancestry in each sequence; excluded or disowned acts void, or disputed if acknowledged or recorded; later genuine acts kept: 1, 2, 5.
- Routine rotation keeps unacknowledged history: 5.
- A witness act keeps a disowned post visible as disputed; a like-style reaction carrying `acks` keeps nothing, being invalid: 5.
- Audit requirement dropped by the rotation that drops it: 5.
- Closure by rotation; a stolen operator key cannot close a home: 5.
- Homeless rotation: closure, objection, auditor absence statements judged by the auditors in force before it, the vanished home's audited history proved by the proofs the owner carried; receipts from the new homes; final once the next rotation counts, yet still beaten by a rotation the old homes held, so a used chain key found after a closure rewrites nothing; re-homed without audit never final by the next rotation, a censored reader, a thief's second rotation voided with the first: 5.
- Escape with both keys past a hostile home, including after a refused normal rotation: 5.
- Several unlinked identities; linking signed by both, ended by either; public or encrypted: 5.
- Succession: 5.
- Linking an outside identifier; signing in after a rotation: 5.
- Rotation to a signing key of a new scheme; unknown to a client without it: 8.
- Two identities on disjoint relays reaching each other through homes and routes: 5.
- An act signed with a collective's grant key: scoped under Identity, judged by Agreements; signed with another key than the one its binding installs, or bound to nothing held, not the collective's: 3.
- A chain signature: pending until its home receipts it; same key and binding afterwards, no act judged; a later rotation naming it as predecessor; a binding naming it invalid; a spent chain key revealed again never counting: 3.9aa.
- Evidence voided by its signer's rotation: an objection to the journalist's homeless rotation signed with a stolen operator key and witnessed by an accomplice, disowned by the operator's rotation, voids nothing, and the exit counts; a cosignature of a forged log summary disowned by the auditor protects no receipt; a witness act disowned by its signer keeps no post disputed: 5.7e.
- An objection to the journalist's homeless rotation signed with a stolen operator key, before the operator rotates: the homeless rotation voided, the stated cost; the journalist, holding both keys, leaves at once by the escape: 5.7e.
- A position contested at a self-hosted home: a thief's rotation and the owner's served at one position; contested, nobody wins; the air-gapped Module signs the owner's escape only on the competing rotation's bytes, never on the online side's word; the escape counts: 5.7f.
- A rotation invalid for a home rule left in place that no longer fits, and the owner's next signature with the same chain key; the air-gapped Module signs it only on the identity chain from genesis to the rotation's predecessor, checked offline by the Module itself, never on the online side's report: 5.7g.
- A reader holding the rotation and the kept tip but not the whole line: the first post unknown, never void, until the line arrives; a scoped act whose binding is not held, unknown, never invalid: 5.5c.
- An identity born under a signing scheme a client lacks: its rotation the client can verify, and every act under it, unknown, never invalid: 8.4.
- A succession declared to the journalist by a stranger's rotation: shown as declared, never followed, until the journalist confirms by a link confirmation or a witness act naming the rotation: 5.8b.