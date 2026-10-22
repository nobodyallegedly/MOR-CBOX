# MIR: Development

*Draft 11, freeze-ready, for Nobody, allegedly, to approve.*

## How to read this MIR

Normal text is the protocol. Italic text is commentary: reasons, examples and plain summaries. A summary that starts *In short* repeats what the rule says; where the two seem to differ, the rule's own text decides.

The rules are numbered in reading order. A rule of another MIR is cited by that MIR's name and the rule's number ("Agreements rule 11").

This mark recurs:

- *Client conformance:* a duty of clients. The protocol cannot check it from acts alone, so it is stated for builders.

The MIR has four parts. Part one is cMIPs and Modules as they run most of the time: naming a specification, freezing it, the tasks, adopting and leaving, signing what a rule computes, earning, and discovery. Part two is the rare cases and the mechanics: xtypes and extensions, the machine verification rules run on, predecessors, and the trust root and successors. Part three is the specification format, with how it is decoded and what each kind must carry. Part four is the reasoning, the parameters and the freeze scenarios.

## Purpose

Development defines how the protocol treats everything built above the core: cMIPs and Modules. It answers:

- what a cMIP is, and what a Module is;
- how each is written, identified and published;
- how it plugs into the core, through the tasks the MICS defines;
- how people adopt it, and leave it;
- how its creator is paid when it is used.

The core is frozen; everything above it competes. Development is the grammar of that competition: zero democracy on the MICS, a free market on cMIPs and Modules, where adoption decides.

## Dependencies

Identity, Envelopes, Text, Money and Agreements. Fees and usage rewards are defined in Money and Agreements, so a Money-only client never depends on this MIR.

## Words used in this MIR

### Specifications

- **Specification.** A document that defines acts, rules or behaviour, identified by its spec hash. The MIRs are specifications; so are cMIPs, Modules, unit specifications, media types and signature schemes, and the MICS document, test suites and MIPs.
- **Spec hash.** `tagged_hash("MOR/spec", content)`, where `content` is the deterministic CBOR encoding of the specification's content map (Part three). It is what acts name in their `spec` field and what agreements name per task. It is the same wherever the specification is published.
- **cMIP.** A community specification for one task: what it accepts and produces in core terms, the act types it defines, and the rules a verifier runs. Many cMIPs may compete for the same task.
- **Module.** An implementation or extension under a MIR or cMIP: code, a verification rule, the code that plays a media type, a split plan template, a rail. *A unit, a media type and a signature scheme are specifications of their own (kinds 2, 3 and 5; Money, "Unit"), not Modules; the code that plays a media type is a Module.* Identified by its spec hash. *A Module is a specification: it signs nothing, holds nothing and is paid nothing by itself; the identities acting under it do.*
- **Verification rule.** An executable program, part of a cMIP or Module, that anyone can run to check its outputs, such as a rail Module's check of a rail proof. The rule decides validity; the text explains it.
- **Signature-scheme specification.** A specification of kind 5 that defines a signature scheme's key encoding, signature encoding and verification, so that Identity can name it as a scheme by its hash.
- **MIP.** A proposal toward a successor protocol, drafted layer by layer like the MIRs: a specification of kind 8, with a creator, naming the MIR or MIP it forks, or, for a MIP proposing a new layer, the MICS document. *MOR 2's starting drafts are the MICS and the six MIRs, each cloned and labelled a MIP; anyone may fork a MIP, and adoption, not authority, sorts the forks.*
- **Machine profile.** The memory layout, input and output encoding and entry point on which verification rules run (rule 22): a specification of its own, named by its hash, not a parameter fixed at the freeze. It is a Module whose field 5 names the Development MIR, and a rule names it by its spec hash.

### Tasks, extensions and xtypes

- **Task.** A point where a MIR hands work to a cMIP that an agreement may name by the task's number (Agreements, terms field 2). Tasks exist only where the MICS defines them. Tasks 1 to 5, whose cMIP a text act, a publication or a client usually chooses, are tasks: an agreement may name a cMIP for any of them by its number. A hand-off with no task number, such as the proof checker of a proven offer, is not a task.
- **Extension.** A cMIP an agreement names in terms field 15, outside the listed tasks, for work nobody foresaw. An extension may add rules, never relax the core's. The cMIP a proven offer names in its access form is not an extension, a lone seller's offer being its agreement included: it checks the offer's proof and adds no rule to any agreement.
- **Xtype** ("extra type"). A cMIP that defines new act types from core pieces for anything the MICS did not foresee, filling no task (rule 18). Clients that have not adopted it show its acts as unknown. *The three ways a cMIP plugs in read: a task, an extension, an xtype. They are names for what a cMIP does, not exclusive kinds: an agreement may name any cMIP among its extensions (rule 19), an xtype included, which is then an extension of that agreement too.*

### Creators and users

- **Creator.** The identity that publishes a cMIP or Module and signs it as its own. MIRs have no creator: they are the protocol itself. Nor have the MICS document and test suites, published like the MIRs; a MIP has one, since a proposal is somebody's.
- **Adoption.** A client's or user's opt-in to a specification, after which acts under it are no longer unknown to them.

## Part one: cMIPs and Modules as they run most of the time

*How it runs most of the time:* a creator publishes a cMIP or a Module, signed as its own; it is named by the hash of its content and never changes. An agreement names at most one cMIP per task, by the task's number; several Modules may work under it, such as one rail Module per rail. A client adopts a specification by the user's choice, and shows acts under any other as unknown. Before its user signs anything a rule computes, it shows the numbers on an example. A creator who charges states a fee; only an agreement's own fee entry makes it owed, and a Module's use is shown by a party's signature, never the Module's.

### 1. Naming a specification

#### Rule 1. By hash

A specification is identified by its spec hash, and labelled under its creator's identity, so names cannot collide. There is no registry.

#### Rule 2. Types by spec

Every act names its type within the specification that defines it (Envelopes). Two specifications can never define the same type.

The types a specification defines are those its text defines, as a verifier implementing it knows them; field 3 only lists them, for discovery (Part three).

#### Rule 3. Creator signature

A cMIP or Module counts as its creator's only when published in an act signed by the creator named inside it. A MIR names no creator.

A specification naming a creator that never published it is shown as unclaimed by that creator; fee terms and reputation displays MUST NOT assume the named creator consented.

A publication the creator's rotation disowned leaves the specification unclaimed, as if never published by it (Identity rule 34). It is unclaimed whether another identity relied on the publication or not; a publication relied on stays shown as disputed (Identity rule 34) and claims nothing for the creator: a special case beside rule 34. *Authorship is the creator's own claim, and the rotation withdrew it.* *So a thief who publishes in the creator's name with a stolen signing key gains nothing the creator's rotation does not take back: the specification keeps its hash, and anyone who adopted it chose to.*

### 2. Frozen at publication

#### Rule 4. Never updated

A changed specification is a new specification, with a new hash. A specification may name a predecessor, so users can migrate, but the predecessor stays exactly as it was. *Who may name a predecessor: rule 25.*

#### Rule 5. Data formats outlive modules

Modules format their data as the MICS defines, so users can move their activity off a fading branch.

### 3. Tasks: the contract between the core and the community

#### Rule 6. Task numbers

Every task has one global number, given in the table below. Agreements name their cMIPs by these numbers (Agreements, terms field 2), so every client counts tasks the same way. A successor protocol may add tasks with new numbers; the numbers here never change.

#### Rule 7. Task grammar

For each task, the MIR that defines it states what a cMIP must accept and produce, in core terms. A cMIP that fills a task MUST accept everything the task requires and produce only what it allows.

#### Rule 8. One per task, per agreement

An agreement names at most one cMIP per task (Agreements). A client chooses its cMIPs; choosing them is like choosing a sub-network.

#### Rule 9. Modules within cMIPs

Several Modules may operate at once under the one cMIP an agreement names for a task (for example several rail Modules under the payment cMIP, one per rail), each naming that cMIP in its field 5. A Module is a specification: it signs nothing; the identities acting under it sign.

#### The tasks the core defines

| Number | MIR | Task | A cMIP accepts | and produces |
| --- | --- | --- | --- | --- |
| 1 | Identity | Identity proofs and credentials | an identity hash and a claim | valid, invalid, pending or unknown |
| 2 | Identity | Naming | a name, or an identity hash | an identity hash, or its names |
| 3 | Identity | Outside link proofs | a link claim and its proof | valid, invalid or unknown |
| 4 | Text | Text format | a canonical text | a rendering that hides only its own declared markup, never what Text's floor protects, and adds no text (Text, task "Text format") |
| 5 | Envelopes | Media interpretation | a media object or manifest | its interpretation for display or play |
| 6 | Money | Payment | an act it follows (an obligation, offer or payee pointer, for instance), an amount, the flow rail or vault entry paid to, and, for a purchase, the claim it pays under | how rail Modules plug in; how a payment carries what its receipt and claim say; how the receiver's receipt and the payer's claim carry a rail's proof, and the rule that checks it; how a fresh receiving address is obtained under a vault entry; reads each rail Module's kind and binding from its specification (fields 11 and 13) |
| 7 | Money | Conversion | a conversion service's offer, and a hop received on one rail | how the service's receipt with a forward, and its onward hop, are evidenced |
| 8 | Agreements | Split | an amount received and a split plan; and, for a payment, the agreement and the attempts made | payouts summing exactly, with third-party evidence; and the receiver for this payment, one of the services the signed terms name, in their order, the next only where the attempt before it failed as the cMIP defines failure (Agreements rule 17) |
| 9 | Agreements | Condition evaluation | a condition and the acts it refers to | true, false, pending or unknown |
| 10 | Agreements | Time reference | a point on the reference and an act | before, after or undetermined |
| 11 | Envelopes | Anchoring | an act id | a proof it existed at a point on a named clock |
| 12 | Agreements | Grant limits | a grant and an act under it | within limits, or not |
| 13 | Agreements | Work claims | a work hash and a claim | a claim, optionally with a pre-publication commitment |
| 14 | Agreements | Absence proof | an abandonment declaration, the clause's parameters, and the acts the record's history holds | accepted, refused or unknown |

**Not a task: the proof checker of a proven offer** (Agreements rule 11, and "A service sold under a cMIP's terms"). A proven offer sells a delivery that leaves a proof any verifier can check from data alone, no one's word trusted. It uses the access form, `[ 2, [ cMIP, params ] ]`, under that cMIP's terms. A cMIP may produce the proof; the verifier checks the proof itself, never the cMIP's say-so. A delivery the verifier cannot check from data it holds is not proven, whatever the cMIP signs. A verifier asks the checker of the cMIP the offer names, among those it holds, for the proof's verdict; with none held, the delivery is not proven for it.

**The checker is not a task.** Each proven offer names its checker: the cMIP in its access form. No agreement names a checker by a task number (terms field 2), so a collective sets no checker for all its offers; each offer chooses the one that suits what it sells. **A cMIP that checks proofs of delivery declares so in its specification, by field 12** (Part three), as a rail Module declares its kind and what its proof binds in fields 11 and 13, so checkers can be found. **Field 12 serves discovery only:** a verifier asks the checker of whatever cMIP the offer names, declared or not, and the proof decides. *Checkers will evolve, and different checkers suit different deliveries, so locking a whole collective on one makes little sense.*

### 4. Adopting and leaving

#### Rule 10. Unknown until adopted

A client that has not adopted a specification shows its acts as unknown and never signs, pays or accepts under it (Envelopes). An act signed under a signature scheme the client has not adopted is unknown in the same way (Identity).

A specification whose content is not in the format is no specification, and no client can have adopted it, so an act naming it is unknown to every client, unless a rule the verifier can apply shows the act invalid (Part three).

#### Rule 11. Adoption by opt-in

A client may fetch a specification by its hash, show it to the user, and adopt it on the user's choice.

#### Rule 12. Exit

Agreements are always in core format, so activity on a fading cMIP moves by cloning agreements onto another (Agreements).

### 5. Signing what a rule computes

#### Rule 13. Signing what it computes

*Client conformance.* Before signing anything that runs through a rule (a split plan, fee terms, a condition), a client MUST show what the rule actually computes on a concrete example, such as: "on a sale of 10, this split pays the singer 4, the co-writer 4, the publisher 1.80, the service 0.20."

*The signer approves the numbers the code produces, not the description. This extends the Text MIR's rule, what you sign is what you saw. No verifier can check that a client did this; it is conformance, not validity.*

### 6. Earning

#### Rule 14. A creator who charges states its fee in its specification

*In short: a creator who charges states its fee in its specification, for information only; only the agreement's own fee entry makes a fee owed, and nothing is forced.*

A creator who charges states its fee in its specification (field 7). *A specification carries as much as it can.*

The rate is informative: a client shows a plan paying less than the specification asks. **Only an agreement's fee entry, signed by the power that places fees (Agreements rule 28, terms field 28), makes a fee owed**, and nothing is forced.

Agreements' Module fee terms act (type 15) is retired, never reused.

#### Rule 15. Usage is evidenced, not tracked, and always by a party

*In short: a Module's use is shown by a party's signature, never by the Module: for a rail Module, the receipt or the payer's claim naming it; for a service, a use record signed by whoever runs it. That record alone pays nothing: a service is paid by a named share the owners chose in advance, or by a role share only where the payer the payment commits to acknowledges its record in its claim.*

Usage is evidenced, not tracked, and always by a party. Receipts and splits name the specifications they ran under (Money, Agreements).

The evidence that a Module was used is signed by a party, never by the Module, which is a specification and signs nothing:

- for a rail Module, the receipt or the payer's claim naming it in field 0 (Money rule 20);
- for a service someone runs on a Module, a use record signed by the identity running the service.

**That record alone pays nothing**:

- a service the owners chose in advance is paid by a named share, with no evidence;
- a service the payer chose at payment time is paid by a role share only when the payer the payment commits to acknowledges its use record in its claim (Agreements rule 22, the relay's rule);
- a service evidenced only by its own record is not paid.

*The record is the role-filler's own signature: paying on it alone would invite faking usage, as for relays. Whether a client carries a user's choice of service into the payment is good practice for client developers, legible to those who look.*

A plain tip carries no module fees (Money rule 22).

*The use record is defined by the cMIP the service runs under, as the relay transport cMIP defines the delivery record; it binds to the object served, and is spent for its payer's other claims naming the same object; Agreements rule 22 applies whatever cMIP defines it.*

#### Rule 16. Bounties

A client or collective may offer a share of future splits to whoever builds a specification it needs. A bounty is an agreement like any other.

### 7. Discovery

#### Rule 17. No core registry

Discovery happens through use. Indexes, catalogues and repositories of specifications are clients and services, competing like everything else above the core.

## Part two: rare cases and mechanics

### 8. Xtypes and extensions

#### Rule 18. Xtypes: new act types need no task

A cMIP may define new act types for anything the MICS did not foresee, built from core pieces (agreements, obligations, receipts, grants, text, media). **Such a cMIP is an xtype.** Clients that have not adopted it show those acts as unknown.

*In short: a task, an extension, an xtype ("xtype" for extra type).*

#### Rule 19. Extensions in agreements

Besides one cMIP per task, an agreement may name any number of extensions by spec hash. A client that does not implement every extension an agreement names MUST NOT sign it.

*Which layers an extension acts on, and so whose approval a collective needs to adopt or drop it: field 10 (Part three).*

#### Rule 20. Add, never subtract

An extension may add rules, never relax the core's. Conservation, the stake rule, required signatures, keeper and rotation rules, and fail-closed always hold, whatever an extension says. A client MUST reject an act that is valid under an extension but invalid under the core.

*This is not mechanically checkable across all clients: determinism is scoped to conforming clients, and an extension's new act types are unknown to non-adopters.*

#### Rule 21. The path to MOR 2

Extensions in wide use are the natural candidates for tasks in a successor protocol; a successor protocol, not MOR 1, makes them tasks (rule 6).

### 9. Verification rules

#### Rule 22. One frozen machine

*In short: where a cMIP or Module says anyone can run its rule, the rule is a program for one frozen machine, RV32IM, with fixed memory, an exact step budget, a fixed input and output encoding, and no clock, network or randomness. Its input is the act and the data it names, which may include a chain's headers the client supplies; without them, the rule answers as it says, "unknown" where it says so. Over budget, an environment call or a misaligned access answers "unknown", everywhere. The profile that fixes memory and encoding is a specification named by its hash, published after the freeze; until one is, no founding rule is published under this rule, and each founding cMIP and Module says its rule is a reference implementation.*

Where a cMIP or Module says "anyone can run the same rule", its verification rule MUST be published as a program for RV32IM, the RISC-V 32-bit base integer instruction set with the multiplication extension, with:

- a fixed memory size, stated in the profile;
- an exact step budget, stated in the rule's reference in the specification, every instruction counting as one step;
- no access to clocks, networks or randomness;
- the input (the act and the data it names) and the output (valid, invalid, pending or unknown) passed in the encoding the profile fixes; the data a rule names may include a chain's headers, which the client supplies (for instance a chain proof checked against the real chain's headers, handed to the frozen rule as input, never fetched by it); a client holding no such data gets the answer the rule gives without it, "unknown" where the rule says so;
- environment calls and misaligned accesses answering "unknown".

A rule that exceeds its budget answers "unknown", everywhere.

**The profile** (memory layout, input and output encoding, entry point) **is a specification of its own, named by its hash, published with test vectors; it is not a parameter fixed at the freeze.** It can be tested, succeeded and adopted by choice, as any specification is, without touching the core; the instruction set, RV32IM, stays the one frozen machine. A rule names the profile it runs on by the profile's spec hash, in its reference (`rule-ref`, Part three). The profile is a Module whose field 5 names the Development MIR. The first profile is MOR 1's first technical specification after the freeze, open to the first wave.

**Until a profile is published, no rule is published under this rule.** A founding cMIP or Module with a verification rule says in its text that its rule is a reference implementation, not a program published under rule 22, so that no reader takes two clients to have been checked against one machine. *The promise stays in the core: "a rule can never be right on one client and wrong on another" holds for every rule published under this rule; a reference implementation is one client's code, and says so.* *Against the MICS's test (law where bytes can prove it, record where they cannot): until a profile is published and a rule published under it, "anyone can run the same rule" is a promise the core cannot check, so it is on the record's side; it moves to the law's side once a profile is published.*

#### Rule 23. The rule decides validity; the text explains it

Verifiers run the rule, and every verifier must reach the same answer, so the executable rule decides. The text should match it.

#### Rule 24. A mismatch is a bug

Since nothing is updated, the fix is a new specification with its own hash, naming the old one as predecessor; users migrate by choice. The faulty specification stays on record, attributed to its creator.

### 10. Predecessors

#### Rule 25. Predecessors are claims

Anyone can publish a specification naming another as predecessor; nothing ties a predecessor to the same creator. A client that suggests successors to its users MUST show when the successor's creator differs from the predecessor's.

### 11. The trust root and successors

#### Rule 26. The six hashes

Below the MICS there is one root: **the six frozen texts of its MIRs, published as canonical text; their spec hashes follow from them.** A client ships the texts, or their canonical bytes, and recomputes the hashes it accepts, so that two clients can differ only about a text, which anyone can read. Every act it accepts names one of them, or a specification whose text depends on them.

**How a MIR's spec hash is computed.** A MIR's content is the map `{ 0 => 4, 9 => text }`, kind 4 and its frozen text, with no other field; its spec hash is `tagged_hash("MOR/spec", content)` over that map's deterministic CBOR encoding. The MICS document's and a test suite's are computed the same way, with kind 6 or 7. These are the hashes the MIRs write as `IDENTITY`, `TEXT`, `ENVELOPES`, `MONEY`, `AGREEMENTS` and `DEVELOPMENT`.

At the freeze, each frozen text's first line is its name alone ("# MIR: Identity" and the like), with no draft number or approval line, since every line of the file is part of the hashed text. The frozen specifications are committed to the genesis repository as files of canonical text (Text), save that a file may end with exactly one line break, which is not part of the text: the text is the file's content as UTF-8 with that one final line break removed, if present. A check of the repository refuses a file whose text so read is not canonical text (a trailing space on any line, two final line breaks, or any other break of the Text rules), so that the file, the publication and the hash are one.

*A build and a list of hashes are signed by the same author, so checking one against the other checks nothing; a text anyone can read and hash is a root nobody has to take on trust.*

A fork presenting six other hashes is a different protocol, and its acts cannot replay here because every tag begins with `MOR/` and a successor MUST use a different prefix (Envelopes).

#### Rule 27. A successor changes the prefix

A protocol that succeeds MOR and reuses its formats MUST use a different tag prefix in every tagged hash, so that no act of one protocol is valid in the other. Migration of identities, agreements and history to a successor is by succession and cloning (Identity, Agreements), never by reinterpretation.

A successor's specifications have new hashes, since the tag prefix is part of the hash; agreements name them afresh by cloning. *The texts carry across unchanged; every hash is new.*

## Part three: the specification format

A specification is a public act (Envelopes type 0, a publication) whose media is the specification's content, published and signed by its creator. Its publication names the Development MIR's spec hash as its media spec (field 0 of the publication's media), since this MIR defines the format of that content. The content is:

```cddl
spec = {
  0 => uint,                  ; kind: 0 cMIP, 1 Module, 2 unit, 3 media type, 4 MIR, 5 signature scheme,
                              ; 6 the MICS document, 7 a test suite, 8 a MIP
  ? 1 => hash,                ; creator: identity hash (required for every kind except MIR, the MICS document
                              ; and a test suite, which carry none; required for a MIP)
  ? 2 => [+ task-ref],        ; tasks it fills (required for a cMIP filling a task)
  ? 3 => [+ type-def],        ; act types it defines (discovery only)
  ? 4 => [+ hash],            ; dependencies: MIRs and specifications it relies on (discovery only)
  ? 5 => hash,                ; implements: the cMIP a Module implements, or the MIR it works under (required for a Module)
  ? 6 => [+ rule-ref],        ; verification rules: executable rules, as locked objects
  ? 7 => fee,                 ; the Module's own fee, if the creator charges: machine-readable, informative
  ? 8 => hash,                ; predecessor: an earlier specification this one replaces for its users;
                              ; for a MIP, required: the MIR or MIP it forks, or for a new layer the MICS document
  9 => tstr,                  ; the specification itself, as canonical text
  ? 10 => [+ uint],           ; layers: for an extension, the layers it acts on beyond Development
  ? 11 => uint,               ; a rail Module's kind: 0 a request rail, 1 a push rail (Money rules 11, 18)
  ? 12 => true,               ; a proof-of-delivery checker: this cMIP checks the proofs of delivery of proven offers
                              ; that name it (Agreements rule 11)
  ? 13 => [* uint]            ; what a rail Module's proof binds: ascending, without repeats, from 0 payee, 1 purpose,
                              ; 2 previous hop; empty, it binds none;
                              ; holding 0 and 1 wherever field 11 is 0
}

fee = [ rate, ? paid: hash ]   ; who is paid: absent, the creator
rate = [ 0, part: uint ]        ; millionths of each payment the Module runs on
     / [ 1, amount ]            ; a fixed amount per payment

task-ref = [ mir: hash, task: uint ]
type-def = [ type: uint, name: tstr, payload-schema: tstr ]   ; schema written in CDDL
rule-ref = [ bytes-hash: hash, size: uint, budget: uint, profile: hash ]     ; the rule's binary, by the hash of its bytes, carried as a separate locked object named by this specification; its step budget; the spec hash of the machine profile it runs on
```

### How the content is decoded

The content is decoded as an act's payload is, under the encoding and the CDDL conventions of Identity's Part three ("Encoding and conventions"), save that a failure makes the content **no specification** (below), not an act invalid. In full:

1. The content is deterministic CBOR, holding one map. Bytes that are not, or a map key twice, make it no specification.
2. Every key of the content map is an unsigned integer from 0 to 13 (arrays and maps inside it follow their own CDDL). Any other key, a key the format does not define, makes it no specification.
3. Every text string in it, field 9 and every `type-def` included, is canonical text (Text MIR, the pinned Unicode version).
4. Each field holds the type the CDDL gives it: `hash` exactly 32 bytes; `uint` an unsigned integer; `[+ x]` at least one item; `task-ref`, `type-def` and `rule-ref` arrays of exactly two, three and four items; `fee` an array of one or two items, its rate `[0, part]` or `[1, amount]`, `part` an unsigned integer counted in millionths, `amount` carried as given; field 12 the value `true` alone.
5. Field 0 is a kind from 0 to 8, and field 9 is present.
6. **By kind**:
   - field 1 is absent on kinds 4, 6 and 7 (a MIR, the MICS document, a test suite) and present on every other kind;
   - field 5 is present on a Module (kind 1) and absent on every other kind;
   - field 8 is present on a MIP (kind 8);
   - fields 11 and 13 are present together or not at all, and only on a Module; field 11 is 0 or 1; field 13 is ascending, without repeats, from 0, 1 and 2, holding 0 and 1 where field 11 is 0; a Module whose field 5 names a payment cMIP carries both. This one step needs another specification's bytes, those of the cMIP field 5 names ("A rail Module's kind and binding");
   - field 12 is present only on a cMIP (kind 0);
   - field 10 is ascending, without repeats, from 0, 1, 2 and 3, on any kind; it is read only where an agreement names the specification as an extension.
7. Fields 2, 3, 4, 6, 7 and 8 may sit on any kind; fields 3 and 4 serve discovery only, and field 2 is read where an agreement names the cMIP for a task, and, on a cMIP a Module's field 5 names, to tell whether it is a payment cMIP (step 6; "A rail Module's kind and binding").

A field the rules above do not forbid is never a reason to refuse the content. *A cMIP filling a task writes field 2 (the publishing tool's rule, below); no decoder can tell that a text fills a task, so a missing field 2 is never no specification.*

**A rule's bytes** (field 6). A rule's binary is a media object of its own, locked as every media object is (Envelopes, "Media"), and `bytes-hash` is its work hash: `tagged_hash("MOR/work", binary)` over the binary's complete bytes, `size` their length in bytes. *Until a machine profile is published, no rule is published under rule 22, so no verifier yet reads field 6.*

**Type definitions** (field 3). A `type-def`'s `payload-schema` is a CDDL rule (RFC 8610) for the type's payload map, written under the conventions of Identity's Part three ("Encoding and conventions"), naming the shared parts of the MIRs (`hash`, `amount`, `signing-key`…) by the names they carry there. It is read by people and tools, for discovery; the specification's text decides.

### What each kind must carry

*In short: a few fields are required, by kind, so that every honest tool publishing one text makes one specification; the rest are written only where the text fills them. The types and dependencies a specification has are those its text gives; fields 3 and 4 only help people find them.*

Every specification carries field 0 (its kind) and field 9 (its text). Besides them, by kind:

- **a cMIP** carries field 1, its creator; and, where it fills a task, field 2, naming each task it fills;
- **a Module** carries field 1, its creator, and field 5, the cMIP it implements or the MIR it works under; and, where field 5 names a payment cMIP, fields 11 and 13 (below). *A Module working under a MIR or one of its tasks with no cMIP, such as the founding seed Modules and the air-gapped signers, names that MIR.*
- **a unit, a media type and a signature scheme** carry field 1, their creator;
- **a MIR, the MICS document and a test suite** carry no field 1 (below);
- **a MIP** carries field 1, its creator, and field 8, the MIR or MIP it forks, or, for a MIP proposing a new layer, the MICS document.

**Fields 3 and 4 serve discovery only**, as field 12 does. The act types a specification defines are those its text defines, as a verifier that implements the specification knows them; the specifications it depends on are those its text depends on. No verifier judges an act against field 3, and an act whose type the text does not define is invalid where the verifier implements the specification, and is unknown where it does not (Envelopes rule 30).

**A publishing tool writes every required field, and no optional field it cannot fill from the specification's text.** Two publications of one text that differ in optional fields are two specifications, with two hashes; a listing shows them as versions of one text. *So the promise of one specification wherever published holds between any two honest tools: what they must write is fixed, and what they may write comes from the text, never from the tool.*

### A rail Module's kind and binding (fields 11 and 13)

*In short: a rail Module says, in its specification, whether it is a request rail or a push rail, and what its proof binds, so the payment cMIP reads them from the specification an act names, never from code.*

**A payment cMIP** is a cMIP (kind 0) whose field 2 lists Money's payment task, `[MONEY, 6]`, Money's spec hash with task 6. Whether a Module's field 5 names a payment cMIP is decided from bytes: by decoding the specification field 5 names. A verifier that does not hold that specification cannot tell whether a Module lacking fields 11 and 13 is a specification, and answers unknown for it, and so for every act whose verdict rests on it; one that holds it decides, the same way in every client. *No decoder can tell "the" payment cMIP from the bytes of a Module alone, and many payment cMIPs may exist; the cMIP's own field 2 is what says it fills the payment task.*

A Module whose field 5 names a payment cMIP carries field 11, the rail's kind (0 a request rail, 1 a push rail), and field 13, the set of what its proof binds, ascending and without repeats, from 0 the payee, 1 the purpose, 2 the previous hop; an empty set binds none of them. Without either, or with any other value, or a field 13 out of order, the specification is not in the format, and so is no specification (below). Wherever field 11 is 0 (a request rail), field 13 holds 0 and 1, the payee and the purpose; otherwise the specification is not in the format, and so is no specification: a request rail always binds payee and purpose. Where field 13 holds part of the set, Money rule 11 applies element by element. **A proof binding the purpose binds the payer the payment commits to** (Agreements rule 22): where field 13 holds 1, the payer is bound. **Fields 11 and 13 are a rail Module's**: on a specification of any other kind, or on a Module carrying one without the other, they are given against their rule, and the specification is not in the format.

*The purpose is bound only through the payment cMIP's commitment, one fingerprint that also carries the payer; a rail binding the one binds the other.*

The payment cMIP reads both from the specification the receipt or claim names (task 6): which rails are request rails decides whether a push-rail payment two holders received is a purchase or a refund (Money rule 18), and what the proof binds decides which act settles a disagreement in payee, purpose or previous hop (Money rule 11).


### A unit (kind 2)

*In short: a unit is named by its hash, and a verifier reads nothing else of it; its text says what one smallest part is, so that people and rail Modules agree on it.*

A unit specification carries field 0 (2), field 1 (its creator) and field 9. Its text MUST say:

- what one smallest, indivisible part of the unit is (one satoshi, one cent of a currency, one token of a named issuance), which every `amount` naming it counts (Money, "Act formats");
- how many smallest parts make the unit as people write it, and the symbol or code a client shows, for display only;
- what makes it distinct from any other unit (a test network's coin is a unit of its own).

A verifier reads only the unit's spec hash: two amounts are in one unit only where they name the same hash, and no rule of the MICS converts between units (Money: conversion is a party's act). *A unit's text is for the people and the rail Modules that carry it; nothing in it is computed by a verifier, so no field beyond those every specification carries is needed.*

### A signature scheme (kind 5)

*In short: a signature scheme is named by its hash, as Identity's founding schemes are by number; a client that adopts it implements it from its text.*

A signature-scheme specification carries field 0 (5), field 1 (its creator) and field 9, and MAY carry field 6, its verification rule, which a verifier runs only once a machine profile is published (rule 22). Its text MUST state:

- its public key's encoding and length, as the bytes Identity's `signing-key` and the signature's `key` carry;
- its signature's encoding and length, as the bytes the signature's `sig` carries;
- how a signature is verified over the 32-byte act id, the message every MOR signature signs (Identity, "Signature schemes"), with any context string it uses;
- which roles it may fill: a signing key, a chain key, or both; and, for a chain key, that its commitment is Identity's `tagged_hash("MOR/safety", scheme || key)`, `scheme` being this specification's 32-byte spec hash;
- test vectors: keys, act ids and signatures, valid and invalid.

A client that has not adopted the scheme answers unknown for every act signed under it, never valid or invalid, whatever the lengths (Identity); a client that has adopted it verifies as its text says. *The founding schemes 1 to 3 are named by number in Identity; a scheme named by hash is everything else, adopted by opt-in, as every specification is.*

### Not in the format: no specification

*In short: a malformed specification is not a bad specification; it is none. No client can adopt it, so every act naming it is unknown, unless a rule the verifier can apply shows the act invalid.*

A specification whose content is not in this format (a kind, a reference, a fee or a field given against its rule, fields 10 to 13 included, or a required field missing) is no specification. A verifier holding its content shows it so; an act naming it is unknown to every client, since no client can have adopted it (rule 10), unless a rule the verifier can apply shows the act invalid (invalid beats unknown, Envelopes rule 30); a verifier not holding the content answers unknown, as for any specification it has not adopted. The invalid of Envelopes rule 30 for an act outside its specification's format stays for specifications the verifier implements; its "invalid beats unknown" holds everywhere.

**A specification malformed and implemented is no specification.** A client may implement a text, its own founding cMIP for instance, whose published content is not in the format: it has not adopted that specification, since adoption is of a published specification, and a client's implementation of a text is not an adoption of malformed bytes. An act naming that spec hash is unknown to it, as to every client: Envelopes rule 30's invalid for an act outside its format does not apply, while its "invalid beats unknown" does. *A client decodes the format at adoption and in a listing, so it cannot implement around it.*

*Adoption is the verifier's own act, and a malformed specification is one it cannot have adopted; so "the specification is invalid" has one consequence a verifier can produce, the same in every client. A client decodes the whole format before adopting, and a listing before listing.*

### Layers an extension acts on (field 10)

*In short: an extension lists the other layers it adds rules to, so a collective's client knows whose approval adopting or dropping it needs. It is read only where an agreement names the specification as an extension; any other value, or a list out of order, makes the specification no specification. The list is not trusted for enforcement.*

An extension's own acts belong to Development. Field 10 lists, ascending and without repeats, each other layer whose acts it adds rules to, numbered as the core's layers: 0 Identity, 1 Envelopes and Text, 2 Money, 3 Agreements. A specification whose field 10 carries any other value, or is not ascending, is not in the format, and so is no specification.

The field is read only where an agreement names the specification as an extension (Agreements, terms field 15); a specification without it declares nothing beyond Development.

*The declaration tells a collective's client whose approval adopting or dropping the extension needs (Agreements, rule 60). It is not trusted for enforcement: in a collective, an extension's rules bind only the layers whose holders approved it, so an extension that under-declares binds nothing beyond them (Agreements, rule 60).*

### A proof-of-delivery checker (field 12)

*In short: a cMIP that checks the proofs of delivery of proven offers says so in one field, so anyone looking for a checker can find it.*

A cMIP whose verification rules check the proofs of delivery of proven offers declares so by field 12. A proven offer names such a cMIP in its access form, `[ 2, [ cMIP, params ] ]` (Agreements rule 11); the verifier checks the proof itself, never the cMIP's say-so. A specification without field 12 declares nothing. The field serves discovery only: a verifier does not read it, and asks the checker of whatever cMIP a proven offer names.

### The spec hash covers the content

The spec hash covers this content, not the act that publishes it, so the same specification can be published by anyone, anywhere, and remain the same specification.

A rule's binary is not canonical text, so it is carried as a locked media object of its own, and the specification names it by the hash of its bytes; the content field stays canonical text and the spec hash stays stable.

### MIRs have no creator

*Every identity's acts name the Identity MIR by its spec hash, so a MIR cannot name the hash of a creator whose own genesis needs the MIR's hash.*

A MIR's hash is computed over its content alone; the act that publishes it is signed by its publisher, which shows who published it without making them part of its identity. cMIPs and Modules keep the creator field, so their spec hash covers the creator's identity: a cMIP is always "this specification, by this identity".

### The MICS document, test suites and MIPs (kinds 6, 7 and 8)

The MICS document (kind 6) and a test suite (kind 7) are published like the MIRs, with no creator, so their hashes are fixed at the freeze. *The MICS is the map of the six MIRs, not a MIR, and the freeze test suite is the specification of what to test; neither fits kind 4, and published under it, a listing would call them MIRs.*

A MIP (kind 8) is a proposal toward a successor protocol. It names its creator, since a proposal is somebody's, and in field 8 the MIR or MIP it forks. A MIP proposing a new layer forks no MIR: it names the MICS document in field 8, where layers are added, and a listing shows it there. A listing shows MIPs in a part of their own, by the MIR they fork and how many fork each. *Anyone may fork a MIP; a fork that makes sense draws others to work from it. Under kind 4 a MIP would read as a MIR to every client and lose its creator; under its own kind a fork is seen as a fork.*


## Part four: reasoning and freeze scenarios

### Reasoning

- **A hash is a name nobody can take.** *Naming by content hash means no registry, no gatekeeper and no collision, and a specification means the same thing wherever it is copied.*
- **Frozen, so trusted.** *An agreement that names a cMIP must mean the same thing forever. If specifications could change, every signed deal could change under its signers.*
- **Tasks keep the core small and the market open.** *The core decides only where it hands work over and what must come back. How the work is done is where people compete.*
- **Extensions keep the frozen core open-ended.** *Nobody can foresee every task. An extension lets agreements use work nobody defined, while the core's promises stay out of any extension's reach.*
- **Leaving is always possible.** *Because data formats and agreements stay in core terms, no module can hold its users hostage.*
- **Paid for use, never for attention.** *A creator earns when their work takes part in a settlement someone chose to make, through terms the payer or owner signed. Nothing is collected silently.*
- **Rules anyone can run, on one machine that never changes.** *A verification answer is only as good as everyone's ability to check it. One instruction set, not two: "RV32IM or RV64IM" would have been two machines giving two answers. RV32IM is ratified and frozen, integer-only execution is deterministic by nature, metering is exact, and most zero-knowledge machines execute RISC-V, so a verifier could later check a proof that a rule ran instead of re-running it. Authors never write it by hand: common languages compile to it. Anything the profile does not define answers unknown, so a rule can never be right on one client and wrong on another. The instruction set is frozen; the profile around it is a specification, so a hurried profile is never fixed into the core for good, and a better one is adopted by choice. Until one is published, the founding rules say they are reference implementations, and the texts claim no more than the code keeps.*
- **The code decides, the signer sees the result.** *A misleading description can fool someone who only reads, but not someone signing through a client that shows what the rule computes. Anyone can compare a rule with its text on test cases, and indexers, reviewers and rival authors all have reason to.*
- **What must be written is fixed; the rest comes from the text.** *Two honest tools publishing one text must make one specification, or a hash names a tool's habits rather than a text. And a specification that is not in the format is none: no client can adopt it, so nothing it names is ever invalid, only unknown.*
- **Even cryptography is a specification.** *Naming a signature scheme by its specification's hash means the core never has to be reopened to add one. When a post-quantum signing-key scheme is ready, someone publishes it, clients adopt it, owners rotate to it.*
- **MIRs belong to no one, and the six hashes belong to everyone.** *A MIR is the protocol itself. Leaving the creator out also breaks the circle of a MIR needing an identity that needs the MIR. What holds the whole thing together is six texts anyone can read and hash, and a prefix no successor may reuse.*

### Parameters

- The RV32IM execution profile: memory size, input and output encoding, entry point, and the treatment of every instruction outside the base set; test vectors. Not a parameter of the freeze: a specification named by its hash, MOR 1's first technical specification after the freeze (rule 22).

- How the MIRs are published, so their spec hashes (`IDENTITY`, `TEXT`, `ENVELOPES`, `MONEY`, `AGREEMENTS`, `DEVELOPMENT`) are fixed at the freeze: rule 26. A MIR's frozen text contains only these placeholders, never its own or another MIR's hash, and MIRs do not list each other by hash as dependencies: their dependencies are circular, so they are stated by name. The six hashes are published together, alongside the texts.
- The CDDL conventions for type definitions: Part three ("How the content is decoded"), under Identity's.
- The signature-scheme specification: Part three ("A signature scheme (kind 5)"); the unit specification: Part three ("A unit (kind 2)").

### Freeze scenarios

*Each bullet names the scenarios of the freeze test suite v30 that cover it.*

#### Part one

- Module identity by hash; frozen at publication: 2, 8.
- Consent by whoever bears the cost, including creator-side fees: 2.
- Exit by cloning onto another cMIP: 1.
- Unknown act carried, shown, then adopted by opt-in: 2.
- A split plan shown with its computed payouts before signing: 2.
- A publication in the creator's name signed with a stolen key, disowned by the creator's rotation and relied on by another identity: shown as disputed, and the specification unclaimed by the creator: 8.
- A cMIP declaring itself a proof-of-delivery checker (field 12), found by it, and named by a proven offer whose proof it checks; a proven offer naming a cMIP without field 12, its checker asked just the same: 8.
- A Module working under a MIR with no cMIP (a seed Module, an air-gapped signer), naming the MIR in field 5: in the format: a component under "Development".
- A specification's text published by two tools, one writing field 2 and one not: a listing shows two specifications of one text, as versions; a client adopting by text finds both; an act of a type the text defines is judged by the text, never by field 3: 8.
- A publication carrying a specification with field 10 out of order: a listing shows it as no specification; an act naming it is unknown in every client, unless a rule the verifier can apply shows it invalid (a text that is not canonical, `acks` it may not carry): 8.
- A role share paid to the modules a payment ran through, evidenced by a party: the receipt or claim naming a rail Module, or a use record signed by the identity running a service and acknowledged by the payer the payment commits to; the use record alone pays nothing: 2.

#### Part two

- An extension declaring the layers it acts on (field 10); a collective adopting and dropping it with the approval of each layer declared: 3, 8.
- A rail Module's verification rule run by two different clients on the RV32IM profile, giving the same answer; a rule exceeding its budget, or making an environment call, answering unknown on both: 2, 8. **Deferred: no published rule in MOR 1**; the founding cMIPs and Modules say their rules are reference implementations.
- A verification rule's reference naming its machine profile by spec hash, the profile published as a Module whose field 5 names the Development MIR: 8, deferred with the line above.
- A rail Module published with field 11 and field 13; the payment cMIP reads its kind and binding from the specification the receipt names; a Module naming a payment cMIP (a cMIP whose field 2 lists Money's task 6) without them is no specification, and a receipt naming it unknown; so is a request rail (field 11 0) whose field 13 lacks the payee or the purpose; a verifier not holding the cMIP a Module lacking them names answers unknown for it: 2, 8.
- A signature-scheme specification published; an identity rotates to it; a client without it shows the identity's later acts as unknown and the rotation as valid: 8.
- A successor specification by a different creator shown with the creator mismatch: 8.
- The six MIR hashes stable wherever published; a read-only client recomputes them from the texts it ships, and shows them; two clients built by two people recompute the same six; a frozen file with a trailing space, or two final line breaks, fails the repository's check, one final line break being no part of the text: 8.
- The MICS document and a test suite published under kinds 6 and 7 with no creator; a MIP under kind 8, with its creator and the MIR it forks in field 8, shown by a listing in a part of its own, by the MIR it forks; a MIP proposing a new layer, naming the MICS document in field 8, shown under it: 8.