# MIR: Envelopes

*Draft 11, freeze-ready, for Nobody, allegedly, to approve.*

## How to read this MIR

Normal text is the protocol. Italic text is commentary: reasons, examples and plain summaries. A plain summary in italics (*How it runs most of the time*) repeats what the rules say; where the two seem to differ, the rule's own text decides.

The rules are numbered in reading order. The numbered steps of "How it fits together" (1 to 3) and of the sealed container (1 to 5) are steps of a format, not rules. A rule of another MIR is cited by that MIR's name and the rule's number ("Agreements rule 58").

A retired rule or type keeps its number, which is never reused.

*Client conformance* marks a rule for the client that locks or signs, which no verifier checks and no verdict reads.

The MIR has four parts. Part one is an act as it runs most of the time: its shape, public and private acts, private delivery, sequences, chains, acknowledgements and references, how acts reach people, media, vows, withdrawal, relays, and what a client does with what it does not understand. Part two is the rare case: theft of the signing key. Part three is the act formats. Part four is the tasks, the reasoning, the parameters and the freeze scenarios.

## Purpose

The Envelopes MIR is the grammar every act is written in. It defines:

- the two parts of every act: a small **outside** that relays can see, and a locked **inside** that holds everything else;
- how content is locked, and how its key is made public or delivered privately;
- how acts name their type, link into chains, acknowledge and refer to each other, without a clock;
- how media is fingerprinted and published, and for whom;
- what relays do, and how acts reach the people they concern (a relay's statement of what it holds is the relay transport cMIP's);
- how an offer of media is withdrawn;
- what a client does with an act it does not understand.

## Dependencies

Identity (signers, keys, bindings, sequences, routes) and Text (canonical text in every text field). Every client implements this MIR.

## Words used in this MIR

### Acts

- **Act.** Any signed object. Every act has an outside, a locked inside, and a signature.
- **Outside.** The part of an act every relay can read: only what a relay needs to store, route and check it.
- **Inside.** Everything else: type, pointers, payload. Always locked with a content key.
- **Specification.** The MIR or cMIP that defines an act's type, named by its hash.
- **Chain.** The acts on one object, each naming the act or acts it follows. Each MIR that defines an object defines its chain rules; this MIR defines only what all chains share.
- **Fork.** Two acts naming the same predecessor in the same chain.
- **Acknowledgement.** An act naming another identity's act, as received. Only act types defined by the Identity, Money and Agreements MIRs may carry one.
- **Reference.** An act naming another act, or a web resource, without following or acknowledging it. A reference to an act may also name the act's signer, as a hint of where to look. A repost is a reference.

### Locking and delivery

- **Content key.** The key that locks one inside or one media object. A new one for every object.
- **Public act.** An act whose content key travels with it, on its outside. Anyone can open it.
- **Private act.** An act whose content key is delivered only to chosen recipients.
- **Key delivery.** An act that says which key opens which act or media object. Sent privately, it travels in a sealed container locked to the recipient's encryption key, or to a bare key the recipient supplied; published openly, it makes that act or media public.
- **Encryption key.** A public key an identity publishes so others can deliver content keys to it.
- **Sealed container.** A wrapper that carries a private act and its content key to its recipients, locked to their encryption keys, and hides the sender from relays. Not an act. Its outside is unauthenticated by design: all proof lives in the inner act.

### Media

- **Work hash.** The fingerprint of a work's complete plaintext.
- **Withdrawal.** An act marking a publication as no longer offered.

### Relays

- **Relay.** A server that publishes, fetches and mirrors acts and media, untrusted by default, operated by an identity.
- **Inbox.** A route of kind 1 (Identity) where others deliver acts addressed to an identity.

## Part one: an act as it runs most of the time

*How it runs most of the time: a signer writes an act's inside (its type, its pointers, its payload), locks it with a fresh content key, and signs an outside that commits to both the locked bytes and the unlocked inside. A public act carries its key on the outside, so anyone can open it; a private act reaches its recipients in a sealed container that carries its key, or has its key delivered by a key delivery that does. The act links into the signer's sequence and into the chains of the objects it touches. Relays store and pass on what they cannot read; a client shows as unknown what it does not implement.*

### 1. Every act

Every act names, inside, the specification that defines its type by hash, and a type number within it. *Since a specification's hash is unique, types can never collide, and no registry is needed.* A specification other than a MIR, the MICS document or a test suite names its creator (Development). Every tag used by MOR begins with `MOR/`; a successor protocol MUST use a different prefix, so that no act of one protocol can be replayed in the other (Development).

**What a relay sees:** *who signed (or nothing, inside a sealed container), which recipients a delivery is for, the size, and whether the key is attached. Never the type, the thread, the payload, or who it answers.*

*The act's format, and how its lock and commitments fit together, are in Part three ("The act").*

The rules below are numbered for reading. A verifier checks them in the order the bytes allow: the outside, then the locked bytes against the locked hash, then the inside once unlocked, against the inside commitment, then the inside's shape and its text. An act that breaks rule 1, 2, 3, 5 or 7 is invalid, whichever it finds first; a scheme it does not implement (rule 4) makes the act unknown only where no other rule it can apply fails (rule 30).

#### Rule 1. One shape, in deterministic CBOR

An act MUST decode as deterministic CBOR in the shape Part three gives ("The act"); an unknown key, outside or inside, makes it invalid.

Deterministic CBOR, and the CDDL conventions every format of the MICS follows (closed maps, optional fields, `uint`, `hash`, `tstr`), are written once, in Identity's Part three ("Encoding and conventions"). *Section 4.2.1 of RFC 8949 leaves floats to the protocol; Identity's encoding fixes them to their shortest form, so no verifier accepts a float in a longer form that another refuses.*

#### Rule 2. No deeper than 128 levels

In an encoded act, and in an encoded inside, no data item is nested more than 128 levels below the outermost item (the outermost item is at level 0). An act that nests deeper is invalid, for every verifier. *An application that needs deeper data carries it in a byte string, or as a media object, which the act does not decode.*

#### Rule 3. The lock and the commitments match

The locked bytes MUST hash to the locked hash. Whoever unlocks the inside MUST check it against the inside commitment; an inside that does not match makes the act invalid.

#### Rule 4. The signature scheme

An act's signature scheme is checked as Identity defines: a scheme the client does not implement makes the act unknown, never valid.

#### Rule 5. Canonical text

Every text field MUST be canonical text (Text MIR): every CBOR text string in the inside, map keys included, at any depth, whatever specification the act names.

#### Rule 6. Hints never decide

A hint MUST NOT be used for any validity decision.

### 2. Public and private

#### Rule 7. A public act carries the key that opens it

**Public:** the content key sits on the outside (key 5). Anyone can open the act. The locked bytes and their key travel together.

A public act's content key MUST open its inside.

#### Rule 8. Identity acts are public

Identity acts are the public face of an identity: genesis, rotations, receipts, routes, names, links, log summaries, cosignatures, objections, absence statements, escape endorsements, witness acts, and this MIR's encryption keys (type 4). They MUST be public acts, save a private link (Identity, section 3). *They go through the same lock as everything else, with their key attached, so every act has one shape.*

#### Rule 9. A new key for every object

*Client conformance.* A content key MUST NOT be reused for two objects. *It binds whoever locks; no verdict reads it, and a verifier does not refuse an act for it.*

### 3. Private delivery

**Private:** no key on the outside. The key reaches each recipient in a sealed container: either with the act itself, or in a key delivery for an act the recipient fetches elsewhere.

- **Encryption key.** An identity that wants to receive private content publishes an encryption key: a public act (type 4), versioned in its own chain like routes, changed with the signing key. *Its forks are in Part two; its scheme, X-Wing, and its format are in Part three.*
- **Key delivery.** An act (type 1) whose payload names the act whose content key it carries, or the publication whose media key it carries, and the key itself. Sent privately, it is a private act, addressed (`to`) to its recipient, and travels in a sealed container locked to the recipient's current encryption key; sent to a bare key the recipient supplied (a device or application key), it carries no `to`, and its container is locked to that bare key. Published openly, addressed to no one, it makes the act or media it names public. Key release may be delegated to a service through a grant (Agreements). Group keys, subscriptions and rights management are access modules.

Following the privacy principle, public receiver and private sender, a private act reaches its recipients inside a sealed container, with its content key. A sealed container is not an act, and relays carry it as opaque bytes. Relays see only the recipients, which are none for a delivery to a bare key, and the size. *The inner act is signed by its sender as any act; its recipient may later show it to anyone, and a private inner act stays locked unless its key is shown too.* *The container's format, and how it is locked and opened, are in Part three ("Sealed container").*

**The outside of a sealed container is unauthenticated by design.** Nothing on it is signed, and nothing in the core relies on it: who wrote, to whom (the inner act's own `to`) and what, are proven inside, by the inner act, signed by its sender. What anyone handling the container can do to its outside, leave a recipient out with that recipient's capsule, is no more than withholding the container from that recipient, and delivery is the relay market's business (rule 29). *An outside that hides its sender cannot be authenticated at all: signed by the sender, it would reveal the sender; signed by nobody in particular, it proves nothing. Adding a recipient, opening the act or moving a capsule to another container stays impossible, by the capsules and the locks.*

#### Rule 10. A private act counts only once opened

Rules about an act's inside can only be checked by those who hold its key. A client MUST NOT treat a private act as valid for anything that depends on its inside unless it has opened and checked it.

#### Rule 11. A private act reaches its recipients with its key

A private act MUST reach its recipients in a sealed container that carries its content key, or have its content key delivered by a key delivery that does.

A key delivery MUST name the act or publication whose key it carries; a private one MUST be addressed to the recipient whose encryption key its container is locked to, or, for a bare key, carry no `to`.

A key delivery is valid as an act by its shape; its key is used only if it opens what it names (rule 7's test), and one that does not is ignored and shown so. Anyone holding a key may deliver it. *A recipient may already show what it was given to anyone (section 3); a buyer publishing a key it bought is the same fact, and nothing in the bytes can stop it.*

#### Rule 12. A container that fails delivers nothing

A sealed container MUST follow the format in Part three; a container whose capsule or contents fail the checks of "Opening" (step 4), or whose inner act fails "Addressing" (step 5), is invalid, and a client MUST NOT treat its inner act as delivered.

**Going public later.** The signer publishes a key delivery addressed to no one, with the key on its outside. *One act turns a private act public, for an embargo, a timed release, or disclosing a negotiation record (Agreements).*

### 4. Sequences

Every everyday act (Identity: every act other than genesis, a rotation and a chain signature) carries, inside, its position in the signer's sequence and a running summary of that sequence.

- **Single lines.** Within a sequence, every act names at most one predecessor. Parallel devices keep separate sequences.
- **Position.** The first act of a sequence (empty `prev`) has position 1; every later act has its predecessor's position plus one.
- **Running summary.** *How it is built, and its empty value, are in Part three ("The running summary").*

#### Rule 13. Position and summary

An everyday act MUST carry a position and a running summary consistent with the line before it (the empty summary for the first act of a sequence), and `prev` (empty for the first act of a sequence), with at most one entry. An everyday act that carries no `prev` is invalid. An everyday act whose `prev` carries more than one entry is invalid too, act by act, a shape rule like a missing `prev`. *The number of entries needs nothing but the act, so every verifier gives the same answer, in any order; the field's two bad shapes, absent and two entries, get one verdict.*

Position and summary are not checked act by act, and nor is their absence: an everyday act missing either is judged as one whose position or summary does not fit. They are checked when a line is rebuilt (Identity, kept ancestry): an act whose position or summary does not fit the line leading to a kept tip cannot be proven kept. *A verdict given act by act would change with which predecessors a verifier happened to hold. A bad position or summary comes only from a buggy client or a careless thief, never from honest use (parallel devices, a restored backup), and it changes nothing that counts until a rotation judges the line; the rotation is the remedy. No client is asked to warn about it.*

*These make it cheap to prove that an act lies on the line leading to a later one, which is how a rotation's kept ancestry is checked (Identity).* Proving it for a private act needs that act's key, which its recipients and its signer hold.

### 5. Chains

#### Rule 14. Chains

An act that belongs to an object's chain names, in `objects`, that chain and the act it follows in it: one entry normally, several where branches of one chain merge (same chain), or where one act belongs to several chains (different chains). A chain may branch and merge. *A collective keeps two chains (Agreements rule 58): its decisions, and its actions, each action citing, on the chain named by the collective's own identity, the decision it acts under and the heads it joins: several entries naming one chain, a merge of its branches, as this rule already reads them.*

#### Rule 15. Forks

Two valid acts naming the same predecessor in the same chain are a fork, provable from their signatures and insides. Whether a fork is normal or a conflict is defined by the MIR that defines the object.

#### Rule 16. Order without a clock

A chain's order records when acts were added to it, never when the things they refer to were created. Hints are never load-bearing. Every "before" in the core is judged inside one named chain, by reference to a specific act.

### 6. Acknowledgements and references

#### Rule 17. Acknowledgements

An act acknowledges another identity's act by naming it in `acks`. It says only "I received this". An acknowledgement counts, for any purpose in the core, only alongside the act it names: a verifier that does not hold that act, or holds it and finds it invalid as an act, ignores the acknowledgement. *It keeps an act visible as a dispute after a rotation (Identity), turns a regretted sale into a visible dispute (Money), proves a negotiation thread complete and places a member's signature on a collective's line (Agreements).*

#### Rule 18. Who may acknowledge

Only an act whose type is defined by the Identity, Money or Agreements MIR may carry `acks`. A text act, any other act of this MIR, and every act of a cMIP or Module MUST NOT carry `acks`; an act that does is invalid. *The rule limits which acts carry an acknowledgement, never which acts can be acknowledged: a post or a publication still becomes disputed, not void, after its signer's rotation when an Identity, Money or Agreements act acknowledges it (a witness act, a buyer's claim, a signature) or a keeper recorded it. To rely on a post or a message deliberately, an identity signs a witness act (Identity, type 15).*

Negotiation messages acknowledge the latest message received from the other side (Agreements rule 52). A negotiation message is an Agreements act type carrying text (Agreements type 18), so it may carry `acks` under this rule; a text act in a negotiation thread may not, and is no part of the record.

#### Rule 19. An act that may not acknowledge, and does, is invalid

An act carrying `acks` whose type is not defined by the Identity, Money or Agreements MIR is invalid. *A verifier needs the six MIR hashes to apply this, as it needs them for everything else; one that cannot tell which MIR an act's specification is shows the act as unknown, never as valid.*

#### Rule 20. References

An act names other acts, or web resources, in `refs`, meaning only "look at this". A web reference may carry the hash of what was there when it was referenced, so readers can check it has not changed. A repost is a reference to a publication, never a publication of its own: it pays nobody by itself, and a reposter earns through a role share where the owners' split plan offers one (Agreements).

**A reference may name the act's signer.** A reference to an act MAY carry, beside the act id, the identity hash of the act's signer (Part three, `ref`). A reader uses it only to know where to look: the signer's identity, its homes, its routes, then the act. It is never a validity rule: a reference that names no signer never makes an act invalid, and an act whose reference names a wrong signer stays valid; a wrong signer shows when the reference is followed, since the act fetched shows its own signer. The signer of a genesis is the identity it makes, whose hash is the genesis's own id.

*Client conformance:* when the act referenced is public, the client that writes the reference MUST hold that act and MUST name its signer. When the act referenced is private, naming its signer is the writer's choice. A web reference names no signer.

*Why: a reader holding only a reply knows, by id, which post it answers, but not whose it is nor where it is kept, so without the signer it could reach the post only by guessing at relays it knows. An act id names an act, never a place; its signer leads to a place, through identity, home and routes, as any two identities find each other (section 7). The signer is not required in every reference, nor checked by verifiers, because a verifier could check it only by fetching the act named, and forcing that fetch would make an act's validity hostage to whoever keeps the act it names. The writer, who already points at the act, can fetch it, so that much is asked of the client. A private act is set apart because naming the signer of a sealed message in a reference others can read would expose the sender the sealed container hides. The way back, from a post to the replies that name it, which relays do not sort by, is left to the relays' market and the relay transport.*

### 7. How acts reach people

An act counts for a verifier only once the verifier holds it. Nothing forces propagation; delivery is the signer's interest. *Four reaches, as a working picture: identity acts must reach the home, since their standing depends on it; acts that concern a counterparty (agreements, signatures, receipts, claims, key deliveries, obligations) should reach that counterparty; publications and posts are published on the signer's outbox routes for whoever is interested; drafts never leave.*

Clients SHOULD deliver an act that concerns a counterparty to that counterparty's inbox route where one is declared (Identity). Any two identities can find each other without already sharing a relay, through identity, home and routes. Whether a relay accepts deliveries, from whom, at what price and for how long, and whether it propagates to other relays, is the relay market's business; spam control, metadata privacy and gossip are cMIPs.

### 8. Media

**Work hash.** The work hash is `tagged_hash("MOR/work", plaintext)`, over the complete plaintext of the work. For a text work, the plaintext is its canonical text. It is the same wherever the work travels and however it is locked or cut, and it is what ownership points to (Agreements).

**Publication.** A publication is an act of type 0 of this MIR. Its payload describes a media object. *Its format is in Part three ("Publication").*

Media bytes are always stored locked, with their own content key, the same way as an inside: XChaCha20-Poly1305 with the nonce in the payload and no associated data. The size (field 3) is that of the media once unlocked: what a reader and a media module need to know; a relay measures the locked bytes it stores for itself. For public media, the key sits in the publication's payload. For media on sale, the publication is public (anyone can see the offer) but the media key is absent, and is delivered to each buyer. After unlocking, a buyer checks the plaintext against the work hash.

A publication is a neutral carrier: it never claims the work. A work is bound to its creators only by an explicit work claim (Agreements).

#### Rule 21. Check the media against the work hash

A client MUST check unlocked media against the work hash before treating a purchase as delivered.

#### Rule 22. A size that does not match is shown

A publication whose size (field 3) is not the size of the unlocked media is shown as inconsistent. *It cannot be judged without the media's key, so it is not a validity rule on the act; a client that unlocks the media checks it, as it checks the work hash.*

#### Rule 23. For whom

A payment for a publication is owed to the identity in `for` if present, otherwise to the signer (Money).

Payment for a publication goes to the payee pointer of the identity named in `for` if present, otherwise of the signer (Money). *A false `for` can only send money to the identity it names, never to whoever wrote it, so nobody gains by lying in it. Whether a grant backs a publication made for another identity is Agreements' business (a collective's grantee signs with a grant key, so its publication is the collective's own and names no `for`); an Agreements client may refuse a publication whose `for` is not backed by a grant it can check, and shows it so. A Money-only wallet needs nothing beyond this field.*

**Buying a work under an Agreements claim** is a purchase only when the payment names that claim (Agreements rule 12, Money rule 18): a Money-only wallet can pay a claimed work's publication, but its payment is no purchase and is owed back to it. *A plain publication of an unclaimed work is paid like any publication.*

**Parts, streams and live media.** Segmentation, chunk fingerprints, streaming and live media are defined by the cMIP the publication names; the fields of the publication then describe the whole, or a manifest of the parts. A live stream has no work hash until it ends; paid live access is sold through a standing offer (Agreements), not a publication. A stream that needs a name before it ends takes a vow's (below); everything else about it is the live media cMIP's.

### 9. Vows

*The core keeps the grammar of claiming and confirming apart from what is claimed and confirmed: it wants to know only whether a sale is pending, confirmed or contested.*

A vow is an act of type 5 of this MIR, naming in words **something MOR cannot hash, existing or to come**: a good in stock or to be made, a service, an hourly rate, storage, a seat, a work not yet made. **It is its own chain:** its first act is its **genesis**, **whose id is its name for good**; a later version (a new date, a changed description) is signed again on that chain by the genesis's signer, naming the genesis and the version it follows, as an offer's and an agreement's versions do. The name is the vow's fingerprint, never a work hash.

*Its payload's format is in Part three ("Vow").*

Its `objects`: none on the genesis; a later version names `[genesis, previous]`, one entry beside any a collective's actions chain needs (Agreements rule 58).

- **A version counts** only signed by the genesis's signer, naming the genesis and a version of the same chain. A stranger's act naming the chain is no version of it. **Two versions naming one previous are the signer's fork, shown**, both standing (rule 15), as two versions of an offer naming one previous are (Agreements).
- **A rotation voids the signer's own acts after the kept tip, the genesis among them where it lies there; never a name others wrote down**: what others signed naming the id (an offer, a stake, a claim) stands, pointing at a void act, shown so.
- **Offers naming it** sell it, with their words, and **each sale's state** (pending, confirmed or contested) are Agreements' (Agreements rule 13). **Nothing else is the core's**: what is claimed or delivered and how it is confirmed; what a vow became and how claims reach it (an ordinary work claim, dated by itself, Agreements rule 7); quantities and seats; delivery points; kinds; and streams, whose segments, signers, closing, branches and recording are the live media cMIP's.

### 10. Withdrawal

#### Rule 24. Who may withdraw, and what it does

A withdrawal is an act of type 3 of this MIR naming a publication in `objects`. It marks that publication as no longer offered.

Its payload is empty, and its `objects` carries the entry `[publication, publication]`, naming the publication as both chain and predecessor (Part three, "Withdrawal").

A withdrawal is valid only if signed by the publication's signer or by the identity named in its `for` field.

- **Authority.** The publication's signer, or the identity named in its `for` field. No other authority exists at this layer. *A creator who wants to withdraw a publisher's publication of their work has Agreements' tools: a contest, and their own publication.*
- **Effect.** Clients stop presenting the publication as available; offers attached to it close; no new content keys are released for it.
- **Past deals stand.** A payment arriving after the withdrawal creates no deal and an obligation to refund it (Agreements).
- **Relays are asked, not forced,** to stop serving it.

### 11. Relays

#### Rule 25. Untrusted by default

Relays publish, fetch and mirror acts, media and sealed containers. Nothing a relay says is trusted unless it is signed.

#### Rule 26. Operated by identities

A relay's statements are signed by its operator's identity. Homes and keepers are relays in particular roles; an inbox is a relay in another.

#### Rule 27. What relays check

A relay SHOULD check an act's signature and that its locked bytes match the locked hash. *It cannot, and need not, read a private act.*

A relay holding a copy of an act whose signature it could not verify SHOULD replace it with a copy of the same act id whose signature it can (Part three, "How it fits together", step 3). *Otherwise whoever sees an act's outside first could lodge it under a signature of a scheme the relay does not implement, and the genuine copy would be refused as already held: an honest relay turned into a censor.*

#### Rule 28. Commitments

*Retired.* Rule 28 and type 2 of this MIR are retired; their numbers are never reused. A relay's statement of the acts it holds is the relay transport cMIP's. *Relays are untrusted in the core, and what the core guarantees about storage it guarantees through homes' receipts and logs.*

#### Rule 29. Storage and delivery policy

Storage and delivery policy (what a relay keeps, whose deliveries it accepts, for how long, at what price) is the relay market's business.

### 12. Fail closed

#### Rule 30. Nothing under an unknown specification

A client MUST NOT sign, pay or accept anything whose specification it does not implement. It shows such an act as unknown.

**Outside its format, invalid; not implemented, unknown.** An act whose type its specification does not define, or whose payload is not in the format that specification gives for the type, is invalid, as the MICS's common conventions say for every MIR and cMIP; a verifier that does not implement the specification shows it as unknown. An act naming a specification whose content is not in Development's format names no specification: it is unknown to every client, since no client can have adopted it, and a verifier holding that content shows it as no specification. *Nothing in MOR is updated, so a later format is a new specification with a new hash, shown unknown by older clients, never invalid: strictness costs no flexibility. This MIR's own types are judged so too: a payload outside Part three's formats, or an act of a type this MIR does not define (type 2 among them, retired), is invalid.*

**Invalid beats unknown.** A verifier that can show an act invalid by any rule it can apply (its shape, its text, its acknowledgements) does so, whatever else in it the verifier cannot read; unknown is the verdict only where no rule it can apply fails. *An act of a specification the verifier does not implement that carries a text that is not canonical, or a text act signed under a scheme the verifier lacks that carries `acks`, is invalid, not unknown: two verifiers say the same word.*

#### Rule 31. Adopted by opt-in

A client MAY fetch an unknown specification by its hash, show it to the user, and adopt it by opt-in.

#### Rule 32. The one exception: a text's format

The one exception is a text act's format (Text MIR): its text is always shown as plain text.

## Part two: rare cases

### 13. Theft of the signing key

- **The hijack window.** A thief holding the signing key can publish a new encryption key, and every private delivery until the owner rotates goes to the thief. The rotation voids the thief's act. *The consequence for a purchase is stated plainly: a seller who delivered a bought key into that window did nothing wrong, and still owes the buyer a new delivery once the owner has rotated, since the buyer paid and did not receive (Agreements). The window is the price of changing the encryption key with the signing key, and the same window applies to the inbox route (Identity).*
- **Forks of the encryption key.** A fork in the encryption key's chain is contested from that point, as for routes; a fork behind an act the owner's later rotation kept is settled by that rotation.

## Part three: act formats

### The act

```cddl
act = [ outside, locked: bstr, signature ]

outside = {
  ? 0 => hash,          ; signer: identity hash (absent only in genesis)
  ? 1 => hash,          ; binding: identity-chain act that bound the signing key; for a scoped key, the act installing it (Identity rule 8)
  2 => hash,            ; inside commitment: tagged_hash("MOR/inside", inside)
  3 => hash,            ; locked hash: SHA-256 of the locked bytes
  4 => bstr .size 24,   ; nonce for the lock
  ? 5 => bstr .size 32, ; content key: present on a public act, absent on a private one
  ? 6 => [+ hash]       ; to: recipients' identity hashes (key deliveries, and acts addressed to someone); for a collective's act, every member among them, or the act public (Agreements rule 57)
}

inside = {
  0 => hash,            ; spec: the MIR or cMIP defining the type
  1 => uint,            ; type, numbered within that spec
  ? 2 => [* hash],      ; prev: previous act in the signer's sequence (at most one); required on every everyday act, empty on the first of a sequence (rule 13)
  ? 3 => [+ object],    ; objects: the chain(s) this act belongs to, and the act it follows in each
  4 => { * any => any },; payload, defined by the type
  ? 5 => uint,          ; position in the signer's sequence
  ? 6 => hash,          ; running summary of the signer's sequence before this act: over its acts up to and including the previous one
  ? 7 => [+ hash],      ; acks: acts by other identities this act acknowledges; only on act types the Identity, Money and Agreements MIRs define (rule 18)
  ? 8 => [+ ref],       ; refs: acts or web resources this act refers to
  ? 9 => tstr,          ; hint: a timestamp or other hint, never load-bearing
  10 => bstr .size 16   ; salt: random, so the inside commitment cannot be guessed
}

object = [ chain: hash, predecessor: hash ]   ; the chain's root act, and the act in that chain this one follows

ref = hash                           ; another act, by its id
    / [ act: hash, signer: hash ]    ; another act, and the identity that signed it (rule 20)
    / [ address: tstr, ? hash ]      ; a web resource, and optionally the hash of what was there

signature = [ scheme: 1 / 2 / 3 / hash, key: bstr, sig: bstr ]   ; scheme as Identity defines it
```

**How it fits together.**

1. The inside is encoded in deterministic CBOR, then locked with the content key using XChaCha20-Poly1305 and the nonce, with no associated data. The result is `locked`: the ciphertext followed by the 16-byte authentication tag.
2. The outside commits to both: the hash of the locked bytes, so relays can check what they store, and the hash of the unlocked inside, so a reader can check that what they opened is exactly what was signed.
3. The act id is `tagged_hash("MOR/act", outside)`. The signature signs the act id. The signature is not part of the id: a verifier that holds two copies of one act id, differing in their signature, keeps the one whose signature is valid (a relay's part is in rule 27).

*The two array forms of `ref` are told apart by their first element: a byte string names an act, a text string a web address.*

*No associated data is bound to the lock: the outside already commits to both the locked bytes and the unlocked inside, so binding more would add nothing.*

*The salt makes the inside commitment impossible to guess: without it, anyone could hash "yes" and "no" and learn a private one-word reply.*

*Each `objects` entry names its chain, so two entries with the same chain are a merge of two branches, and two entries with different chains are one act touching two objects, such as a stake transfer on a work's chain and on the agreement's chain. A reader never has to guess which.*

### Encryption key (type 4) and key delivery (type 1)

**The scheme: X-Wing.** An encryption key, and a bare key, is `[scheme, key]`, in the form Identity defines for keys, in the same number space: founding number 4 is X-Wing, the hybrid of ML-KEM-768 (FIPS 203) and X25519, as specified in draft-connolly-cfrg-xwing-kem-11 ("X-Wing Construction"): a 1216-byte public key, a 1120-byte ciphertext, a 32-byte shared secret. Numbers 1 to 3 are signature schemes and are never encryption keys. A scheme may also be named by the hash of a key-exchange specification (Development); a client that does not implement it cannot seal to that key. The version MOR names is that draft's revision 11, fixed by the SHA-256 of its published text, `XWING`, which is written here at the freeze (a step of the freeze procedure). *What is locked today stays locked if either half holds. The draft may change before it is final; revision 11, fixed by hash, does not.*

```cddl
enc-key = [ scheme: 4 / hash, key: bstr ]   ; 4: X-Wing, a 1216-byte public key

encryption-key-payload = {     ; type 4
  0 => uint,                   ; version: 1 for the first, then the previous version plus one
  ? 1 => hash,                 ; the encryption-key act it replaces (absent only for version 1)
  2 => enc-key
}
; versions as the routes act's (Identity, "Routes (type 3)"): version 0 invalid, version 1 without field 1, every later
; version with it; an X-Wing key (scheme 4) of another length than 1216 bytes makes the act invalid

key-delivery-payload = {       ; type 1
  0 => hash,                   ; the act whose content key this is; with 2, the publication whose media key it is
  1 => bstr .size 32,          ; the key
  ? 2 => true                  ; the key of the media the publication describes
}
```

*Why a key delivery cannot simply be a private act that carries its key locked inside: its payload is locked with its own content key, which would need a delivery of its own, and that one another, without end. The recipient could never open the first. The key exchange therefore lives in one place, the sealed container, which carries an act and the key that opens it; the key delivery's payload is then plain, and the same payload, published openly, is "going public later".*

### Sealed container

```cddl
sealed = [ to: [* hash], locked-act: bstr ]   ; no one-time key, no signature (section 3)

; locked-act holds, encoded:
locked-act-content = [ capsules: [+ capsule], nonce: bstr .size 24, locked: bstr ]
capsule = [ ct: bstr .size 1120, wrapped: bstr .size 48 ]
; locked opens to, encoded:
contents = [ act: bstr, ? key: bstr .size 32 ]   ; the inner act, encoded whole, and its content key if it is private
```

1. **Recipients.** `to` lists the recipients' identity hashes, and `capsules` holds one capsule per entry, in the same order. A container for a bare key has an empty `to` and exactly one capsule; a bare key is never combined with other recipients.
2. **Locking.** A fresh 32-byte container key locks `contents` with XChaCha20-Poly1305 under the nonce, with no associated data. For each recipient, X-Wing encapsulates to its key (the identity's current encryption key, or the bare key) with fresh randomness: `ct` is the X-Wing ciphertext, and `wrapped` is the container key locked with XChaCha20-Poly1305 under the key `tagged_hash("MOR/sealed/wrap", shared secret)` and a nonce of 24 zero bytes. *The zero nonce is safe because every shared secret is fresh.*
3. **No signature.** The container carries no one-time key and no signature: its outside is unauthenticated by design (section 3). *Who sent the act, and to whom, are proven by the inner act, signed by its sender, and checked at step 5. A signature by a one-time key belonging to no identity would prove nothing: anyone could drop a recipient and its capsule and sign the rest with a key of their own. The core's one tag for the container is `MOR/sealed/wrap` (step 2); nothing is hashed under `MOR/sealed`. A relay's own name for a container, a hash of its bytes, is the relay transport cMIP's business, under a tag of that cMIP's own.*
4. **Opening.** A recipient decapsulates its own capsule, unwraps the container key, opens `contents`, and opens the inner act with the key given (or its own, for a public act), checked against its outside as for any act. A key given for a public act makes the container invalid.
5. **Addressing (rule 11).** An inner act sent to an identity names that identity in its own `to`; one sent to a bare key names no one.

### The running summary

**The summary of n acts.** `S(n)`, the summary of a sequence's first `n` acts, is the root of a Merkle mountain range over their act ids, in order:

- a leaf is `tagged_hash("MOR/mmr-leaf", act id)` (`tagged_hash` as Identity, "Encoding and conventions", defines it), and a node `node(left, right) = tagged_hash("MOR/mmr-node", left || right)`;
- the range is a list of perfect binary trees, its peaks, highest on the left, one per binary digit 1 of `n`: adding an act adds its leaf as a peak of height 0, then, while the peak to its left has the same height, the two become one, `node(left peak, new peak)`;
- the peaks are bagged right to left into one root: start from the rightmost peak, and hash each peak to its left as `node(peak, bagged so far)`, so left stays left; a single peak is its own root, with no further hashing;
- `S(0)`, the empty summary, is 32 zero bytes.

**Which summary an act carries.** The act at position `p` of a sequence carries `S(p − 1)`: the summary over every act before it, up to and including the previous one; the first act, at position 1, carries `S(0)`. A rotation records, in its `kept-tip` entry for the act at position `p`, `S(p)`, the summary including that act: the one the act after it would carry (Identity, "Rotation (type 1)"). *So `S(3)` over three acts is the summary a fourth act carries, and the one a rotation keeping the third carries for it.*

Its test vectors, `S(0)` to `S(11)` and a three-act sequence among them, are published with this MIR's conformance vectors (Parameters); they are computed exactly as written here.

### Publication (type 0)

```cddl
media = {
  0 => hash,          ; spec of the media type (a media module), which interprets the bytes
  1 => hash,          ; work hash
  2 => hash,          ; locked bytes hash: SHA-256 of the media as stored
  3 => uint,          ; size of the media once unlocked, in bytes (rule 22)
  4 => bstr .size 24, ; nonce of the media lock
  ? 5 => bstr .size 32, ; the media's content key, for public media
  ? 6 => [+ tstr],    ; where the locked bytes can be fetched
  ? 7 => any,         ; price, in the form the Money MIR defines
  ? 8 => hash         ; for: the identity this publication is made for; payment goes to its payee pointer
}
```

### Vow (type 5)

```cddl
vow = {
  0 => tstr,          ; its words: what is vowed, existing or to come, canonical text
  ? 1 => [hash, any]  ; a cMIP and its parameters: what it is in that cMIP's terms (its kind, its terms)
}
```

### Withdrawal (type 3)

```cddl
withdrawal = { }      ; empty
; its objects carry the entry [publication, publication]: the publication as both chain and predecessor
```

### Types defined by this MIR

| Type | Name | Public or private |
| --- | --- | --- |
| 0 | Publication | public (the offer); its media may be locked |
| 1 | Key delivery | private, addressed to its recipient, in a sealed container; or public, addressed to no one, to make something public |
| 2 | *Retired*, never reused: an act of this MIR of type 2 is of a type it does not define (rule 30) | |
| 3 | Withdrawal | public |
| 4 | Encryption key | public (an Identity act in spirit, defined here) |
| 5 | Vow | as any act ("Public and private"); its offers are always public (Agreements) |

## Part four: tasks, reasoning, parameters and freeze scenarios

### Tasks

- **Media interpretation, per type.** A cMIP accepts a media object (bytes, or a manifest of parts) and produces its interpretation for display or play. It defines segmentation, chunk fingerprints, streaming and live media for its type.
- **Anchoring.** A cMIP accepts any act id and produces a proof that the act existed by a point on a named clock. Two anchors are compared only on the same clock; anchors on clocks that cannot be compared order nothing. Anyone may anchor any act. **One anchoring cMIP; the clocks are Modules under it**: the cMIP says what an anchor is, how two anchors on one clock compare, and what a clock Module must accept and produce; each clock (Bitcoin, a timestamp service, any other) is a Module under it, as rails are under the payment cMIP, and a clock is named by a clock Module and its parameters. An act may be anchored on any number of clocks. **On a clock that counts blocks, the point is the block**: two anchors in one block are at the same point. **An anchor may place a commitment naming its act**: a hash the act determines under the specification that defines it (a payment commitment, recomputed from a receipt or claim by the payment cMIP), so that a payment made on a clock's own chain anchors the receipt or claim of that payment at its block, for what the commitment binds. *The act itself is written later; what the anchor shows is that the payment it records was made by that point, which is what Money rule 29 compares.* **A clock's own data** (a chain's headers) is handed to its Module's rule by the client, as input (Development rule 22). *MOR has no clock, but anchoring to one is a task the core defines and accepts; it does not specify how. Every layer above may rely on it: Money for a lock change after a theft (Money rule 29), Agreements for agreements' deadlines and time references, absence-proof modules for presence. A client that relies on an anchor carries the anchoring cMIP it names, as it carries a rail or media Module.*

### Reasoning

- **Everything locked, so relays cannot harvest private content.** *A relay cannot read private acts at all, and learns only the signer, the size and any recipients. Public acts carry their key on the outside, so any relay can open them cheaply: harvesting public content is not prevented, only made no easier than reading it. Private content is where the protection lies.*
- **Direct messages do not stand out by kind.** *When encryption is the rule, a private message is not a special type that relays can spot. An addressed act does show its recipient, as the principle of a public receiver allows, but never its sender inside a sealed container, nor what it is.*
- **Commit to the inside, not only the lock.** *Common encryption lets a clever sender make one locked message open to different contents under different keys. Signing the hash of the unlocked inside makes that impossible: what you sign is what everyone opens.*
- **Salt the inside.** *Without a salt, a short private reply could be guessed by hashing candidates.*
- **One shape for every act.** *Identity acts are locked too, with their key attached. That costs a few bytes and saves every implementer a special case.*
- **Going public is one act.** *Publishing a key turns anything private public, for embargoes, timed releases and disclosure, with no copy and no new fingerprint.*
- **Order without a clock.** *Clocks can be faked and disagree; hash links cannot.*
- **Acknowledgements are cheap, on purpose, and never stand alone.** *A thief with an accomplice can turn their own acts into visible disputes, never into valid acts. In return, anyone who genuinely relied on an act is protected. An acknowledgement of an act nobody can produce changes nothing, so the noise a thief can make is bounded by the real acts the thief signed.*
- **A like is never an acknowledgement.** *An acknowledgement carries weight in three layers: it keeps an act alive as a dispute, turns a regretted sale into a dispute, and places signatures on a collective's line. A reaction module that let a like carry `acks` would turn every like on a thief's post into testimony. So the line is drawn by type, the simplest check there is: only the act types of the three layers that need acknowledgements may carry them, and a deliberate reliance on anything else is a witness act, signed on purpose. An accomplice can still witness a thief's acts, making a visible dispute and never a valid act; what disappears is the accidental case.*
- **One size, the one readers need.** *A field two conforming programs can read two ways is a flaw. The size of the unlocked media is what a reader shows and a media module needs; a relay counts what it stores itself.*
- **A layer's rules use only that layer's data.** *The `for` field exists so that a wallet with no Agreements can pay the right identity, and a client with no Agreements can judge a withdrawal. Pushing one small field down is always better than pulling a rule up.*
- **Where to look, never what counts.** *A reference's signer helps a reader find the act, through the signer's homes and routes, and decides nothing: validity never waits on fetching what an act points at. The client that writes it, which already points at the act, is the one asked to fetch it and name its signer rightly, and only for public acts, whose signer is no secret.*
- **Delivery is the signer's interest.** *An act counts only where it is held, so whoever wants an act to count carries it to those who must hold it. A reserved inbox gives that carrying a destination without making any relay the meeting point by default.*
- **One depth for everyone.** *A decoder must stop somewhere, or hostile input can crash it. If each stopped at its own depth, one verifier could accept an act that another cannot read. A fixed limit, far above any act the MICS defines, gives every verifier the same answer.*
- **Encryption for the long run.** *Content locked today may be stored and attacked for decades. A hybrid key exchange stays safe if either half holds. X-Wing combines the two halves with a combiner designed and reviewed for exactly this pair, rather than one made here.*
- **One place for the key exchange.** *A key that must be delivered privately cannot sit inside the act that delivers it. The sealed container is where every private act meets its recipient's key, so the key delivery can stay a plain statement, and the same statement, published, makes something public.*
- **All proof inside, none outside.** *A sealed container's outside hides its sender, so it cannot be authenticated: signed by the sender it would reveal the sender, signed by nobody in particular it proves nothing. Nothing needs it to be: the inner act, signed by its sender, proves who wrote, to whom and what; what a relay can do to the outside, leave a recipient out, it can do by withholding, and delivery is the relay market's. Dropping the one-time key and its signature saves 96 bytes and a verification per container. If anything later needs the outside bound, it comes as a cMIP's, never as a guard on the core's container.*
- **Judged where a whole line is held.** *A verdict on an act's position or summary given act by act would depend on which predecessors a verifier happened to fetch, and two verifiers would disagree until they held the same acts. Rebuilding a line to a kept tip is where every act's fit is known, so it is judged there, and nowhere else.*
- **A relay's store is the relay market's.** *Relays are untrusted in the core. What the core guarantees about storage it guarantees through homes' receipts and logs; a relay's statement of what it holds is a promise between a relay and its users, for the relay transport cMIP to give a form.*
- **Strict formats cost no flexibility.** *An act outside its format is invalid wherever its specification is implemented, so two honest verifiers never read one act two ways. Since nothing in MOR is updated, a new format is a new specification, shown unknown, never invalid, by clients that have not adopted it.*

### Parameters

- The running-summary test vectors (`S(0)` to `S(11)`, and a three-act sequence): published with this MIR's conformance vectors, alongside the texts; the summaries over stand-in act ids are final now, and the three-act sequence's at the freeze, when the spec hashes its acts name are fixed.

Left to the relay transport cMIP: how a client finds the key deliveries and sealed containers addressed to it (inbox routes and relay queries). The core fixes only what a client checks of what it finds: the act, the sealed container and the key delivery, in the formats of Part three.

Fixed at the freeze: `XWING`, the SHA-256 of the X-Wing draft's revision 11 as published, written into "Encryption key (type 4) and key delivery (type 1)".

### Freeze scenarios

*Each bullet names the scenarios of the freeze test suite v30 that cover it.*

#### Every act, public and private

- Envelopes, types, specification named: all scenarios.
- Key delivery to an identity and to a bare application key: 2, 6.
- Delegated key release at scale, stopping on lapse: 6, 7.
- A private message between two parties that a relay can route but not read, sender sealed; the two parties share no relay and meet through the inbox: 5.
- A private act made public later by publishing its key (disclosure): 5.
- A container with a recipient and its capsule stripped: the remaining recipients open it as before, the inner act's own `to` the only record of the sender's addressing; a recipient cannot be added: a component under "Envelopes and Text".
- A key delivery whose key does not open what it names: valid as an act, its key ignored and shown so: a component under "Envelopes and Text".
- Two copies of one act id differing in their signature, delivered in both orders: the copy with the valid signature counts at every verifier, and a relay holding the other replaces it: a component under "Envelopes and Text".

#### Sequences, chains, acknowledgements and references

- Hash-linked ordering; provable forks; a two-chain act (stake transfer) read correctly: 1, 3.
- Running summary from the test vector; kept-ancestry proofs: 1, 5, 8 (with Identity).
- An everyday act with key 2 (`prev`) absent is invalid, and so is one whose `prev` carries two entries; one whose position or summary does not fit its predecessor is not invalid act by act, and cannot be proven kept when its line is rebuilt: a component under "Envelopes and Text", and 5, 8 (with Identity).
- A repost as a reference; tips reach the original: 5.
- A stranger holding only a reply at one home follows it, by the signer its reference names, to the post at another home; a reference naming a wrong signer leaves both acts valid and fails when followed: 5.
- A text act carrying `acks` is invalid; a reaction under a cMIP carrying `acks` is invalid; a witness act keeps a disowned post visible as disputed; a buyer's claim does the same for a publication: 2, 5.

#### Media and withdrawal

- Work hash of plaintext; locked bytes fingerprinted per publication; segmentation by the named cMIP: 1, 2, 6.
- Publication with `for`; a Money-only wallet and a full client pay the same identity; withdrawal by signer or `for`: 2, 3.
- A publication whose size is not the unlocked media's, shown as inconsistent: 2.
- Withdrawal, including a payment racing it: 2.
- A withdrawal in its format, `{}` with the entry `[publication, publication]`: 2, 5.

#### Relays and fail closed

- Relays: 4, 5. *Scenario 4's "every mirror matches the commitment" rests on the voting cMIP's own commitment, not on this MIR.*
- Unknown act carried, shown, then adopted by opt-in: 2.
- An act of a type its specification does not define, or with a payload outside that specification's format, invalid where the specification is implemented and unknown where it is not; an act naming a specification whose content is not in Development's format, unknown to every client: 2, 8.
- An act of a specification a client does not implement, carrying a text that is not canonical: invalid, not unknown: a component under "Envelopes and Text".