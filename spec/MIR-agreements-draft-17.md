# MIR: Agreements

*Draft 17, freeze-ready, for Nobody, allegedly, to approve.*

## How to read this MIR

Normal text is the protocol. Italic text is commentary: reasons, examples and plain summaries. A summary that starts *In short* repeats what the rule says; where the two seem to differ, the rule's own text decides.

The rules are numbered in reading order, save rules 94 to 97, which stand beside rule 48, whose protection they complete. A rule of another MIR is cited by that MIR's name and the rule's number ("Money rule 19"). A retired rule, type or field keeps its number, which is never reused.

Some marks recur:

- *Client conformance:* a duty of clients (wallets, the collective's client, a split service's software). The protocol cannot check it from acts alone, so it is stated for builders.
- *Stated cost:* a weakness the rules accept knowingly, written down so nobody mistakes it for an oversight.

*Working rule:* where the core gives participants the means to choose, the choice is theirs; the core keeps only what no choice may break.

The MIR has five parts. Part one is agreements as they run most of the time: making and signing, works and stakes, selling, splits, judges, keepers, changing a deal, defaults and the public domain. Part two is collectives: an agreement with its own identity, its tiers, members, grants, succession and abandonment. Part three is the rare cases: forks of records, how a collective ends, a broken collective, and how "before" and "after" are read in a collective. Part four is the act formats. Part five is the tasks, the reasoning and the freeze scenarios.

## Purpose

Agreements is the language of verifiable agreements between identities. It defines:

- how an agreement is proposed, signed, recorded and changed (by cloning, never by editing);
- keepers: the notaries that record deals as they happen, so that a rotation cannot erase what someone relied on;
- stakes in works and publications: how they are bound, transferred and released;
- splits: how money that reached the owners' payee pointer is divided, exactly, including module fees and shares assigned to roles;
- offers accepted by payment, conditions, and time;
- grants and collectives: acting for others, within limits;
- abandonment, and what happens when someone disappears from a deal;
- non-performance: the core never enforces, but it makes default impossible to hide.

Money ends when money reaches a payee pointer. Agreements say who owns what, how it is shared, and who failed when something breaks.

## Dependencies

Agreements builds on Identity, Envelopes, Text and Money.

Agreements are optional per client, but all or nothing: a client implements it fully, or shows its acts as unknown (core, client conformance). A client that acts on agreements implements Agreements.

No rule of a lower layer depends on Agreements. Where Agreements need a lower layer to carry something, it is a field there (the publication's `for`, Envelopes).

**Only acts valid in their own format count.** Every act a rule of this MIR counts, its own or another specification's (a receipt, a claim, a publication, a rotation, a cMIP's act), counts only where it is valid under the specification that defines it. An act whose type its specification does not define, or whose payload is outside that specification's format, is invalid (Envelopes rule 30), and no rule of this MIR counts it; an act under a specification the verifier does not implement is unknown to it. The rule reaches the acts of the core's own types; a cMIP's act (a delivery, use or metric record) counts where the verifier reads its format, as the cMIP defines it. A format rule is one decided from the act's own bytes; a bound that needs other acts (a transfer's field 3, at most what the seller still holds) is a status rule, judged where this MIR says, never a format rule: an over-sale is a valid act that confers nothing (rule 10). *One rule for every layer, as the MICS's common conventions say: a verifier never counts here what the act's own MIR or cMIP refuses.*

## Words used in this MIR

### Agreements and signing

- **Terms.** An act proposing an agreement: its parties, its text, its cMIPs, its keepers and its rules.
- **Signature.** An act by one party naming terms (or a clone, a transfer, a release) as agreed. Each act has one signer, so every agreement is its terms plus one signature act per party.
- **Agreement.** Terms together with the signatures its rules require. Its id is the id of its terms act. An agreement commits identities, not keys: a later rotation changes the pen, never the promise. A party is bound only through its own signature act.
- **Agreement chain.** The acts on one agreement: signatures, clones, records, transfers, payouts, contests. Each names, in `objects` (Envelopes), the chain and the act it follows.
- **Deal.** An agreement that founds no collective: its terms carry no key grammar. A deal exists when every party has signed it, and changes only with every party's signature.
- **Position.** An agreement that confers income without ownership.
- **Standing offer.** Published terms that anyone accepts by paying.
- **Proven offer, promised offer.** Offers are of two classes.
  - **Proven:** its delivery leaves a proof that any verifier can check from data alone, no one's word trusted: a hash in an anchoring batch; a key that decrypts to the promised hash; a rail swap's payment proof; a deterministic computation re-run to the same hash; a bounty whose answer checks itself. A proven offer uses the access form, `[ 2, [ cMIP, params ] ]` (rule 11).
  - **Promised:** its delivery rests on someone's word, so it names a vow, and each sale is confirmed or contested by those who signed onto it (rule 13). A service's signed word alone (counts, checks, quality) and anything ongoing (hosting over time) stay promised.
  - A cMIP may produce the proof; **the verifier checks the proof itself, never the cMIP's say-so.**
- **Negotiation message.** An Agreements act carrying text, by which two sides negotiate (type 18, rule 52). Each message names the previous message of its thread, and acknowledges the latest one received from the other side. Like every Agreements act it may carry acknowledgements (Envelopes rule 18); a text act never can.

### Works, stakes and selling

- **Work claim.** An act binding a work's hash to its creators.
- **Stake.** A share in a work or a publication. An agreement defines it, and it is identified by that agreement and its index in the agreement's stakes list. The shares in each always add up to 100%. The current holder of a stake is whoever the transfer chain on the agreement names.
  A stake whose object is the collective whose agreement defines it is a share of all that collective's income, whoever earns it and whenever. It decides what every holder is paid, member or departed.
  **A stake whose object is a vow is held by the vow's signer: a deal made on a vow has an owner** (rule 14).
- **Holders' rule.** For a stake of a collective, how its holders consent together to the owners' matters: every holder (the default), or owners holding together at least a stated share of the stake. It reaches only changes that hit every owner of the stake alike, binds only holders who accepted it by signing or buying, and changes only with every holder (rule 94, terms field 30). The release rule (field 24) is its first instance.
- **Owners' matters.** What a stake's owners consent to by their holders' rule, or each signing: what the stake is paid, an issuance in it, a collective's sale or release of a work, and a fork's successors' founding terms paying a kept holder less (rule 95).
- **Issuance.** A version adding new holders to a stake, every existing share scaled by the same factor, decided by the holders' rule and pre-emptive: each existing holder first offered its proportion on the same terms by an issuance offer (type 26), waived only by every holder (rule 96).
- **Proxy.** A holder's act handing its vote on the owners' matters of one stake to another identity, for one version or until revoked; never its money or property, nor anything needing every holder (rule 97, type 27).
- **Purchase.** A payment for a work under an Agreements claim, naming the claim it pays under: the claiming agreement, and the line at which the payer's client read it current (Money receipt and claim field 9, Money rule 18).
  - On a request rail, the claim it names is the one the seller's request committed to.
  - On a push rail, each holder the claim names settles it on its own chain (rule 12).

  Where the seller is a collective, the purchase becomes a **sale** once the collective's actions chain records it (rule 12).
- **Vow.** An act naming in words something to come, something MOR cannot hash or cannot hash yet: its own chain, its genesis id its name for good (Envelopes type 5). Offers naming it sell it, and each sale of it is pending, confirmed or contested (rule 13). A vow covers anything MOR cannot hash, existing or to come, and every sale of a physical good or of a service names one, unless its delivery is proven; it holds one or more promises, which may be bundled; it names what is promised to the buyer, never the event itself; two vows on one event stay separate, each referencing the event where it exists on MOR, not each other.
- **Request rail, push rail.** A request rail is one on which the payee's side commits to each payment before it is made (the payment cMIP's commitment, which an invoice carries). A push rail is one on which the payer pays an address with no such request. The rail Module's specification says which it is (payment cMIP).

### Money and splits

- **Split plan.** The part of an agreement that says how incoming payments are divided: fixed shares on stakes, role shares, a maximum fee per payout where it states one, and what happens to unfilled roles. Fees are their own terms field, 28, beside it. **Every share paying anyone but a stake is paid off the top, as overhead; what is left goes to the stakes alone.**
- **Split service.** An identity that receives payments on behalf of a work's owners, divides each one according to their agreement, and pays every party out. It can be a company, a collective, a friend running software, or the owners themselves; a sole creator is simply their own split service.
  Where an agreement names one, the payee pointer of the identity its publications are made for counts, for Agreements, only if both hold (rule 16):
  - every address in it also appears in that service's own signed pointer in force;
  - every entry of its vault in force also appears in the service's own signed vault.
- **Payer-side splitting.** The other split model. Where an agreement names no split service, the payer's wallet, reading Agreements, pays each holder's own pointer by the stakes of the work's claim: one flow, with the holders' identities as destinations.
- **Split.** A split service's signed act dividing one incoming payment into payouts (type 8, rule 20). It is delivered to every holder it pays, and names each fee and who received it.
- **Payout.** One receiver's part of a split, paid as a simple Money payment with its own receipt.
- **Role share.** A share assigned to a role rather than to a named party, filled when the payment happens. For example: the reposter who led to the sale, the relay that served the file, a service the payer chose, the rail Modules the payment ran through.
  Its evidence is an act signed by a party other than the split service and the payee. **That evidence counts only when the payer the payment commits to acknowledges it in its claim** (rule 22).
  The exception is a rail Module's role share: its evidence is the receipt or claim naming that rail Module, where the payee's own pointer or vault names it (rule 22).
  A helper the owners chose in advance takes a named share instead, and needs no evidence.

### Judges, keepers and time

- **Keeper.** A relay, operated by an identity, that an agreement names to record the acts around a deal as they arrive, and to sign what it recorded: a notary stamping sealed envelopes. A keeper records act ids and signers, never content. A record counts only alongside the act it names, and only for an act that belongs to an agreement naming that keeper (rule 42).
- **Belongs to.** An act belongs to an agreement when it names, in `objects`, that agreement or an act of its chain; or through the act it answers:
  - a Money receipt or payment claim, through the claim it pays under (its field 9, the claiming agreement), and through what it fulfils or followed (the receipt's field 5, the claim's field 4): the agreement itself, or an obligation, offer or act that belongs to it;
  - an obligation, through the agreement or offer it arises from (its field 4);
  - a standing offer, through the agreement it is made under (its field 0); a lone seller's offer is itself the agreement its payments belong to.

  An act may belong to several agreements. An act that belongs to none (a post, a work claim alone, a rotation) has no keeper of its own: only an anchor places it. A verifier follows "belongs to" from act to act, each `objects` entry or act answered being one step, as far as the acts it holds allow, at no fixed depth. What it finds through acts it holds stands; where a step names an act it does not hold, whether the act belongs to anything beyond that step is unknown to it.

  **Its keepers are those of the version in force for the act.** For a deal, the latest complete version, on the line of the version the act names, that the act's history holds (what its `prev` and `objects` name, followed back), or the version it names where its history holds none on that line; for a collective, the agreement in force at the act (rule 60). Every action and decision of a collective belongs to the collective's agreement, whatever it names: an act of the collective's own key, of a grant key the collective issued, or of a holder's member key (rule 73). Its genesis, its rotations and its Identity everyday acts belong to none, as every identity's. *A keeper a clone removed no longer speaks for acts under the clone; a keeper a clone added speaks from the version naming it.* *Every party's acts on an agreement, the payer's and the payee's alike, fall under its keepers; a keeper chosen for one deal never speaks for acts outside it.*
- **Arbitrator or verifier.** A third party named by an agreement to judge content (escrow and milestones, arbitration, regulated checks). Unlike a keeper, it receives the keys of the acts it must judge, by ordinary key delivery.
- **Judge.** Either a specification named for a judicial task (9 condition evaluation, 10 time reference, 11 anchoring, 14 absence proof), or an identity named to judge or testify: a keeper's operator, an arbitrator or verifier, the abandonment authority. *The split service is not a judge: its backups are attempts per payment (rule 17).*
- **Chain of judgment.** For a judge the terms name: the ones that take over from it, in order, when it answers "unknown" or does not act within its period. The chain is named in the terms and signed by everyone, so that it passes with no new signature. Each link names its period, measured on the agreement's time reference (terms field 6; the chain itself is terms field 21; rule 35).
- **Time reference.** A source of time an agreement names, such as a block height. For that agreement, only it decides whether an act is before or after a point on its reference, and whether something is past its terms. *Before and after among acts done in a collective's name are judged on the collective's own sequence ("Made before, made after"); the time reference judges deadlines and absence.*
- **Concurrency rule.** Retired: terms field 10 is withdrawn, never reused. A fork of records is settled by rule 90's default, the parent staying in force until a clone recorded after both lines resolves it.
- **Contest.** A signed objection to another act, by a party with standing. It changes nothing about any act's validity, status or selection; it makes disagreement visible. *"Dispute" is a different word: it is reserved for the status Identity gives an act after a rotation.*
- **Standing.** Who may contest an act: a party to its agreement, a holder of a stake it affects, or a keeper, arbitrator or verifier the agreement names.

### Collectives

- **Collective.** Several identities acting together as one identity. A collective is a full identity (Identity MIR), with its own genesis, homes, signing key and chain key. Its founding agreement says how its members hold and use those keys.
- **This collective.** In a collective's own terms, null written as a stake's object or holder, or as a grant's grantor, names the collective whose agreement it is. This is the same device by which Identity lets a genesis name itself as its home's operator by null. Founding terms can so name a collective that does not exist yet.
- **Key grammar.** The part of a collective's founding agreement that says how its keys are held and activated, and the way to rotate that needs less than every member.
- **Tier.** In a collective's founding agreement and its clones, each field of the terms belongs to one of three tiers ("Tiers"):
  - Constitutional: who decides. The parties (membership), the change rules, the key grammar, the areas, the constitution's words, the departed members entry.
  - Judicial: who judges, and by what. The protected clauses, changed only with every member's signature (rule 67).
  - Operational: everything else.

  Deals have no tiers (scope).
- **Protected clause.** In a collective, a clause that can move a party's stake or voice, or decide who wins a dispute. The protected clauses are: the abandonment clause, the succession plan, the keepers, the split service and its backups (field 29), the time reference, the arbitrators or verifiers, the condition and anchoring cMIPs, and the chain of judgment.
  Together they are the judicial tier, which changes only with every member's signature, one version for everyone (rule 67). The split service and its backups are operational in kind, and protected as a stated exception, since they can move money. A deal needs no protected clauses: no clone of it completes without every party.
- **Change rules.** The rules a collective's clone must meet, each covering part of the terms:
  - the **constitutional change rule** (terms field 18; every party when none was agreed);
  - the **judicial tier's rule**: every party whose voice remains, fixed by this MIR and never by the terms (rule 67);
  - the **clone rule** (field 5), for operational matters outside every area;
  - the **power** over each area.

  In a deal, the signing rule and the clone rule are every party.
- **Power.** The right to bring a change into force: the constitutional change rule, the judicial tier's rule, the clone rule, or an area's holders meeting its threshold.
- **Mark.** A clone's terms field 4: which of the parent's change rules it claims to meet, and the parties whose signatures meet it.
- **Area.** A part of a collective's operational tier that the constitution gives to some of its members, the area's **holders.** An area covers acts of the collective, by whole layer or by act type, and operational fields of its terms.
  - Its holders alone decide there, and may grant within it.
  - Its holders, and the rule they act by, change only by a constitutional change; a holder may step down from it at once (rule 71).
  - An area with no holder whose voice remains stands frozen; a clone may list none at all.

  An area reaching a whole layer also decides which cMIPs the collective adopts for that layer's tasks, and an area reaching Development decides which extensions it adopts. Below, "the area reaching Money" (or another layer) is such an area.
- **Layer.** One of MOR's layers (core, "Structure"): Identity; Envelopes and Text; Money; Agreements; Development. Which layer an act belongs to depends on its `spec`:
  - An act whose `spec` is a MIR belongs to that MIR's layer.
  - An act whose `spec` is a cMIP belongs to the layer of each task that the collective's agreement in force (rule 72) names that cMIP for (terms field 2). A task's layer is that of the MIR defining it (Development, "The tasks the core defines").
  - An act whose `spec` is an extension the agreement names (field 15) belongs to Development.
  - An act whose `spec` is a specification the agreement in force names neither for a task nor as an extension belongs to no layer. In a collective with areas it counts for nothing, unless an area reaches it by a kind naming its type (rule 60).

  Development defines no tasks and no act types of its own: specifications and releases are Envelopes publications.
- **Extension.** A cMIP an agreement names outside the listed tasks (field 15), which adds rules and never relaxes the core's (Development). Its specification declares the layers it acts on (Development, specification field 10); its own acts belong to Development.
- **Voice.** A party's say in an agreement: it counts toward the agreement's change rules and, in a collective, toward the areas it holds.
  - A party loses its voice only by its own resignation (rule 70), by an abandonment declaration (rule 89), or by a constitutional clone that removes it (rule 69).
  - It loses its voice in one area by stepping down from it (rule 71), or by a constitutional clone that replaces it there.
  - It never loses its stake that way.

  In a collective, a resignation, a stepping down and a declaration take effect at the collective's line registering them ("Made before, made after"), and keep that effect for the collective whatever their signer's later rotation disowns.
- **Departed holder.** An identity that left a collective, by resigning or by being removed, and kept a stake in it. Its stake is in the collective's stakes (field 7), which decide what it is paid; it is consulted on the owners' matters (rule 95), and may be declared absent though no party (rule 85). The collective's **departed members entry** records only that the identity is departed, without voice (terms field 22, rule 68).

### A collective's acts, in order

- **Record.** The collective's everyday line (type 17, rule 72). It is the act by which the collective:
  - writes a complete clone of its agreement in its own sequence, naming the signature acts that complete it, so that the collective's later acts are judged under it;
  - registers its members' departures and rotations.
- **Line.** An act of a collective, in its own sequence, at which an event that changes who may act for it takes effect for its acts. A line is a record (type 17) or a rotation of the collective, each naming the latest act of every other sequence the collective keeps. What was done in the collective's name counts as made before or after a line by its place in the collective's own sequence, that is, by whether the history the line cites holds it ("Made before, made after").
- **Decisions chain.** A collective's decisions, in order:
  - its genesis (founding);
  - its rotations (members joining or leaving, every constitutional change);
  - its records (its other clones, the release rule among them, and the departures they register);
  - its grants and revocations;
  - its fork or closing.

  Each decision cites the previous decision and the action head it saw:
  - a record, by its own sequence and the tips it names;
  - a rotation, by its kept tips (Identity);
  - a grant or revocation, by its own sequence and its citations in `objects`, as an action cites;
  - a fork or closing, by its chain act and tips.

  Records, rotations, forks and closings are the lines.
- **Actions chain.** Every other act in a collective's name: debts, payments, publications, releases, sales, the acts signed with a grant key. Each cites, in `objects`, on the collective's chain (the collective's identity as chain), the decision it acts under and the heads it joins. Its own sequence's previous act counts as cited (rule 58). Identity's own everyday acts carry no objects and are on neither chain; nor are negotiation messages, which are talk.
- **History.** What a decision or an action cites, transitively: the previous acts of its own sequence, the tips it names, what each act reached cites on the collective's chain, and the tips of each record reached. An act is before a line when the line's history holds it.
- **Tie rule.** When a decision ending powers (a fork, a closing, a departure, a grant's revocation, a departure emptying an area) and an action using them cite neither each other, the ending wins: the action is void, or, against a grant's revocation, void or disputed under rule 76. *A counterparty protects itself by waiting until the actions chain visibly moves past its act before delivering.*
- **Done.** An act in a collective's name (signed with its key, or with a grant key of its) is done once both hold (rule 57):
  - it is sealed to every member of the agreement in force for it (or public);
  - it is on the chain, citing the collective's head as its signer knew it.

  Where the collective's stake in itself has holders who are no members, a Money act in its name is done only once sealed to every holder of that stake as well, or public (rule 57).

  Before that, even signed, it is planning, and binds no one, its signer included. Where it is held is never a condition: relays are transport. This is so for records too, and for forks and closings: one rule for every act in the collective's name. *One thing is to act, another to read.*

### Grants

- **Grant.** A delegation of authority, with limits, from an identity or collective to another identity. A grant never reaches beyond the power of whoever issued it. A collective's grant is a decision naming the grantee's own identity, which signs to accept it, and hands the grantee a grant key. In a deal, each payee's grant to the split service is written in the deal's terms (field 14), naming "this agreement" by null.
- **Grant key.** A key of an identity, a person or a collective (extended), scoped to one grant's area and reach. The grant names it (field 9); the grantee makes and holds it. It is like a device of the collective limited to that scope. What it signs is the grantor's own act (for a collective, a strand of its actions chain), citing the grant. A revocation removes it, as a decision ending powers. Every grant key ends at a fork's line. *In the end, it is an extra key tied to both the collective's and the grantee's identities.* *Stated cost:* *a stolen grant key acts within its scope until revoked.*
- **This agreement.** In a payee's grant to a deal's split service, null written as the agreement concerned (grant field 2) names the deal whose terms list that grant in field 14, with the versions it clones. It works as null in a collective's terms names "this collective". A grant can so name a deal that does not exist yet.
- **Payee.** In a deal naming a split service, a party whose own payee pointer receives the deal's incoming money, and who grants the service a grant key for it (field 14).

### Ending

- **Release to the public domain.** An act ending a work's claim, at once or at a point it names on the time reference (type 5, rule 55). After it:
  - nobody earns from the work as its owner any more;
  - its history, who made it and who owned it, stays named;
  - its content key is published, or delivered at that point by the identity the act names.
- **Fork of a collective.** The end of a collective whose members split into sides that will not agree (type 19, terms field 23, rule 91).
  - Each side first founds its successor.
  - A fork act names the successors, signed by the members on its sides with their own identities under the constitutional change rule.
  - The fork act closes the original collective in Agreements and passes its ownership to the successors.
  - Each successor names the original in "forked from", a back-link only.

  Not to be confused with a fork of records or of an agreement chain (rules 6, 72, 90), which rule 90 settles.
- **Closing.** The end of a collective that holds nothing and owes nothing: every work sold or released, every debt paid or released by its creditor. It is a closing act, signed under the constitutional change rule. After its line, the collective's keys count for nothing in Agreements (type 20, rule 91). The selling and releasing are ordinary acts before the closing. An absentee's money share follows the collective's own rules, and stays visible as an open obligation wherever they sent it; the closing names nothing for it.
- **A deal's end.** A deal has no closing act. It ends, or winds down, by a clone every party signs, stating its new conditions: where its stakes went, what is still owed, and to whom its outer agreement now pays (rules 15, 47).
- **Creditor's release.** An act by which a creditor ends an obligation owed to it without full payment, for instance against stakes or a partial payment. Only the creditor signs it (rule 92). It is a Money act (Money type 4; Agreements type 21 retired). A collective creditor signs it by the area reaching Money alone. A release traded for future terms is also an Agreements agreement, signed by the area reaching Agreements, which the release names. *MOR has no court and cannot force a creditor to let go: a debt ends by payment or by its creditor's word.*

## Part one: agreements as they run most of the time

### 1. Making an agreement

#### Rule 1. Proposal and signatures

An agreement begins as terms. Each party signs with a signature act naming the terms.

Founding terms exist once every party has signed them. This holds in a deal and in a collective's founding agreement alike: their signing rule (field 4) is every party. A clone comes into force by its mark (rule 45).

**A party is bound by an agreement only through its own signature act.** A rule decides when an agreement or a clone comes into force among those who signed, never who owes. An obligation in the terms of a party who has not signed is shown as unsigned, never as open.

#### Rule 2. cMIPs

Every agreement carries the hashes of the cMIPs it uses, at most one per task. An agreement that does not is invalid.

It may also name extensions (Development). A client that does not implement all of them cannot sign it.

#### Rule 3. Commitments of identities

An agreement names identity hashes, not keys. After a rotation, the party's new key signs for the same agreement.

#### Rule 4. What you sign is what you saw

*In short: before anything is signed, the client can show exactly what it says, as plain text.*

*Client conformance.* Before any terms, clone or grant is signed, the client:

- MUST be able to show its text as plain text;
- SHOULD do so by default;
- MUST show every bidirectional control visibly in that view (Text rule 6).

#### Rule 5. Positions

Positions are agreements that confer income without ownership.

#### Rule 6. Agreement chains

Every act on an agreement names the chain and the act it follows.

*In short: two acts following the same act are a fork. Most forks are settled by the agreement's own rule for acting independently, or by keeping the status quo.*

Two acts naming the same predecessor are a fork. The exception: signatures naming the act they sign are parallel consents, not a fork.

A fork is settled this way:

- where the agreement lets parties act independently, rule 90 settles it (the concurrency rule, terms field 10, is withdrawn);
- otherwise the status quo stands;
- save that two complete versions of one deal follow rule 47, as below.

Two complete versions of one deal:

- the version before the split is the reference;
- a new act may follow either branch;
- a tangled fork stays on the reference;
- a settlement settles it, final for everything its signers could see.

The settlement is one of:

- a version naming every tip it drops;
- or the judge of forks the reference names, or the link of its chain of judgment that took over, on a party's request.

*The rare case, in detail.* An act on an agreement chain that is void or disputed under Identity's rotation rules, or disputed under rule 76 as an act a grant's revocation does not hold, confers nothing: no stake, no obligation, no consent.

The one exception: a member's signature act that a collective placed before its line registering that member's rotation stays valid for the collective, as "Made before, made after" says. Its status under Identity is unchanged.

**What a collective's line placed stays placed for that collective.** The exception reaches every act of another identity that a line of a collective names, registers, or relies on to count what it registers: a resignation or stepping down, a declaration of absence and the signature acts completing it, the signature acts a declaration's field 1 is checked against, and every other act "Made before, made after", 4, lists. Such an act keeps, for that collective, the effect the line gave it, whatever its signer's later rotation kept or disowned; its status under Identity is unchanged, and every client shows it as a dispute on the line. *With rule 42's for a keeper's record, this is the exception Agreements makes to Identity's rule that another identity's evidence counts only while its signer's chain keeps it: what a collective's own line placed stays placed.*

**A deal's version, once relied on, stays complete**, as rule 47 says, the disowned signature shown as a dispute.

### 2. Works and stakes

#### Rule 7. Binding a work

A work is bound to its creators only by an explicit work claim:

- a sole creator, by its claim alone;
- several creators, by one creator's claim completed by signature acts from all the others, and bound only when all are present.

A publication is a neutral carrier: carrying or quoting a work, or pointing to a web resource, claims nothing.

The core records claims and their order, never legitimacy. Order is not evidence of authorship; a claim cMIP MAY carry a pre-publication commitment.

Where claims from different identities conflict, priority can only be decided where they meet: by anchoring, or by a keeper of an agreement both claims belong to that recorded both. Otherwise they remain openly contested.

**A work claim counts only public or addressed to every creator it names.** It names no roles.

In MOR's eyes the owners are who the recorded work claim names, with the shares its agreement gives. **Where no agreement writes shares, the default holder is the creator who opened the work claim.** A client shows it "as recorded", any contest beside it, never as legitimacy.

**A manifest neither claims its parts nor is reached by their claims.** A manifest work (an album naming its songs) is a work of its own: its claim binds the manifest's hash, never the parts it names, and a claim on a part never reaches the manifest. The parts' owners take part in it by agreement: the manifest's agreement has the parts' agreements as parties (rule 15).

#### Rule 8. Stake rule

Any act that changes the size of a stake requires its holder's signature.

**The one exception is an issuance**: new holders added to a stake, every existing holder's share scaled by the same factor, decided by the stake's holders' rule and pre-emptive, as rule 96 says. A share added any other way still needs every holder whose share it lowers. **A holder is whoever field 7 and the transfers placed say** (rule 10): a buyer who completed a transfer is the holder of what it bought, for this rule as for rule 48.

A signature on an agreement is consent to everything it defines, including pre-authorised changes such as abandonment outcomes. Clients SHOULD make such terms prominent.

#### Rule 9. Stakes are written in millionths

Stakes are written in millionths, summing to 1,000,000.

*In short: when a payment does not divide exactly, the few leftover units go to the holders whose exact shares were cut the most, and ties take turns. The holders keep the chain of receipts that proves it.*

Leftover units from dividing a stake go one each to the holders whose exact shares have the largest fractional remainders.

Holders with equal remainders take turns. A tied unit goes to the tied holder that has received the fewest leftover units from this stake so far, counted along the dividing split service's own receipts for the stake. Where that count is equal, it goes to the holder whose identity hash is smallest.

**The count runs along the service's receipts across every version of the deal, the same stake being the same holder's share in the same work whatever its place in the list, and a new holder starting at zero.**

The order in which holders are listed decides nothing.

**The chain of receipts.** Each receipt of a split service carries, per stake, the running count of leftover units each holder has received, and cites the service's previous receipt for that stake. So a tie is checked from two acts.

- The count is split key 4, `[+ [ stake, [+ [ holder, count ]] ]]`: the count after this split, a holder not named counting zero.
- "Receipt" in this rule means the split act.

A verifier that does not hold the previous receipt shows that unit as unknown, never the rest of the payment.

**The split service delivers every receipt for a stake to every holder of it, paid or not** (client conformance). So each holder keeps the whole chain and checks each receipt as it arrives.

**A receipt that does not cite the latest receipt for its stake, or two receipts citing the same one, is a deviation that breaks the plan** (rule 68). Then:

- the break is shown on every split involved, and each holder's client names the one that arrived second;
- after a deviation, the reference stays the last split that continued the chain, so a deviating split never becomes the baseline.

Splits made from a payer's claim are in the chain. A new holder starts keeping the chain from its first split, with a warning.

*The holders build the chain's reference copy themselves: whoever has money at stake holds the record of how it was divided, and checks each new link against it, so a lie is caught at its first step and a reset or fork is shown.*

*The rare cases, in detail.*

**Where the payer divides** (a payer-side split, or a split from a payer's claim with no receipt), the payer decides each tied unit, at most one per tie (no hash, since the receipts do not exist when the payer divides). This is a stated cost, since a payer who is not a holder gains nothing by it.

**A fork's division** keeps ties among sides by the hash of the fork act with each side, which every member signs and any may refuse. Among members counted alike, it gives leftovers to the first as listed in the agreement in force, an order every member signed.

**While a deal stands forked, the turns stay per branch.** Each split is judged under its branch's latest version, and its stake's holders there take their turns along that branch's receipts.

**One numbering runs across every split the service makes under the deal, on any branch.** Each split shows its number (split key 5), from 1, each one more than the service's split under the deal before it.

*Client conformance:* **a holder's client MUST raise the alarm when the numbers on the splits it receives skip.** Splits are then being made where it is not shown, and a gap reveals a hidden branch even to the party a bait branch was hiding from. *One count across both branches acts as an alarm.*

**A deal's split that carries no number, or a number another split of the same service under the deal carries, is a deviation that breaks the plan**, as two receipts citing the same one are: shown on every split carrying it.

A gap stays the holder's alarm, not a deviation: the split missing may only not have reached that holder.

#### Rule 10. Transfer

By default, a stake is sold or given by a transfer signed by the seller and completed by the buyer's signature. Other holders are notified, not asked.

**For a collective's owners' matters, a transfer counts only where the collective's line placed it** (rule 94): until then the seller holds, for the collective, what field 7 and the transfers already placed say. Money follows transfers as this rule says, unchanged. **The buyer accepts the holders' rule of the version in force at the placement** (rule 94), never of the version the transfer names in its field 0, which may be a stale one. **A collective's transfer of a stake it holds in a work is an owners' matter** (rule 95).

The new holder inherits the full position. The transfers name the current holder of each stake, and a split service MUST follow them.

**A sale takes effect on the seller's own line.** The seller signs the transfer by a chain signature too (Identity type 16), which fixes its place.

**Once the seller has signed it, a payout receipt the seller signs for that stake after it on the seller's own line is a wrong receipt, counting for nothing.** That payout is owed to the buyer, in the share sold, as the split service's open obligation (rule 30), the service settling with the seller.

*Stated cost:* between the transfer and the service's first split following it (split key 8, the service's statement, never the moment), a colluding seller and service can keep paying the seller. The buyer's protection is to anchor the transfer. Otherwise the trail shows it, and the service is liable once it is shown.

*Client conformance:* a co-owner's and the buyer's client show a split paying a seller for a stake the seller has transferred, beside the transfer.

*The rare cases, in detail.*

**"After" on the seller's own line.** Identity binds an everyday act to genesis or a rotation, never to a chain signature. So **a seller's payout receipt for a stake it transferred is after the transfer, and wrong, once it is bound to a rotation later than the transfer's chain signature**, on the seller's identity line.

**Until then, what moves the money is the service.** From its first split citing the transfer (split key 8), every payout to the seller for that stake is the service's debt to the buyer, receipted or not.

"First" and "from" are read on the service's split numbers under the deal: the citing split, and every split of the service under the same lineage numbered above the lowest citing one. What is owed is what the payout gives the seller above its due once the transfer and those before it on the seller's line are followed, at most the share sold of what the split pays the stake.

*Stated cost, the window's second edge:* the window accepted as legible closes at the earlier of the service's first split citing the transfer and the seller's next rotation. Within it, a colluding seller and service can keep paying the seller, the trail showing it.

**A stake sold twice is settled on the seller's own line**:

- a stake's transfers are ordered on the seller's own line, by their chain signatures' places;
- each is checked against what the seller still held at that point;
- transfers that fit are all valid, so two partial sales are not a fork;
- **an over-sale confers nothing, and its payment is owed back by the seller** (money received for nothing), paid as a refund (Money rule 19; stated here, not in Money rule 18).

What the seller still held is its share in the version the transfer names, less each transfer of the same stake (by its object, across the lineage) placed before it on its line that fitted what was left; an over-sale before it consumes nothing. An earlier transfer counts from the seller's chain signature, whether its buyer has completed it yet or not, so a later buyer's client sees it before paying.

The payment for a transfer is a Money receipt or claim fulfilling it (the receipt's field 5, the claim's field 4), or one the transfer's field 4 names. The over-sale's payment is owed back by the seller. *A transfer cannot name by hash a payment made after it; such a payment names the transfer it fulfils (field 5).*

*Client conformance*: **a buyer's client pays only once the seller's transfer is receipted by the seller's homes** (its chain signature counted on the seller's resolved line) **and checked against what the seller still holds.** Nobody polices it: it is prevented where it can be and undeniable where it cannot, a refund debt any wallet sees, with signed proof of the double sale.

### 3. Selling

#### Rule 11. Standing offers, accepted by performance

*In short: a seller publishes a standing offer, a buyer accepts it by paying, and the deal is complete when the seller's side delivers.*

*How it runs most of the time:* a seller publishes an offer. The offer is always public. It names:

- what it sells: publications; a vow (a promised offer); or a delivery proven under a cMIP's terms (a proven offer, the access form);
- its price;
- who is paid.

A buyer's client finds the latest version of the offer and pays. The seller's side delivers the content key.

*Owners publish terms; anyone accepts by paying them.*

**A lone seller's offer, with no co-owners behind it, is itself the agreement between seller and buyer.** It carries its price, its words, its deadline and, if the seller sets any, its own refund terms on its own time reference. The buyer's payment accepts them and names no claim.

Co-owners' offer works differently. It:

- names their agreement (field 0);
- is signed under its rule (in a deal, every party);
- pays a payee its field 14 lists or, where it names no split service, the stakes.

A purchase following it names that agreement. Its refund terms are the agreement's (field 17).

An offer changes by versions on its own chain. A version selling nothing withdraws the offer. A purchase under any version counts. Two versions of one previous are the seller's fork, shown.

A payment the time reference places past the offer's deadline is owed back. A payment whose place is undetermined is a purchase: the seller who chose a push rail bears it.

The deal is complete when the offer's side either:

- delivers the content key; or
- for a publication whose key is public, **receipts the payment naming the offer** (type 7 is retired).

That proves payment was verified and honoured.

A payment that receives neither, such as one arriving after a withdrawal, creates an obligation to refund it.

A refund owed on a payment whose payer is not named is owed to whoever signs with the key the payment committed to as its payer. They claim it by a payment claim carrying that key's signature and naming where to be paid (Money rule 19). It is never owed to whoever presents the rail proof, which others on the route may hold too. A payment that committed to no key leaves its refund unclaimable by anyone.

The obligation stays open and visible until claimed and discharged. A seller who refuses a valid claim is visibly in the wrong.

**The vault's limit protects refunds too.** A refund paid to the refund rail a payer's claim names counts as repaid only up to the payer's vault limit for its unit; above it, only once it reaches the payer's vault (Money rule 17, with rule 3's limit). A payer that declares no vault has no vault rule (Money rule 3). *Why: a thief holding the payer's signing key during a theft window could sign a claim naming their own refund rail; the vault exists partly to protect large debts.* A refund owed to a bare key (Money rule 19) has no vault, so the limit does not reach it (Money rule 17).

The offer's terms MAY say how long a refund stays claimable, on the time reference. The core sets no lapse. **A refund past the terms the buyer accepted by paying is ended**: the obligation closes there, the owners keep the money, and it does not block a closing. The terms are a fixed point on the time reference (terms field 17, or a lone seller's offer's own). Terms written in bad faith are not refused: they stay public, for anyone to judge.

*Client conformance:* the buyer's client shows the deadline before paying.

Paid access to a live stream, which has no work hash until it ends, is sold by a standing offer, not a publication. **The class follows what is sold.** A seller who promises the stream sells a vow, a promised offer (rule 13; the live media cMIP): the key delivered in keeping the vow is a proof that sits in the sale as evidence (rule 13), and the buyer still confirms or contests. *A stream cut short: the key worked, the promise did not.* A seller selling only a key to what already exists, such as the recording, sells a proven offer, under the access form. *An offer selling a stream's access under the access form with no vow sells the key only, and is read so.*

**Two classes of offers: proven and promised.** What an offer sells is delivered either with a proof or on someone's word.

- **A proven offer** sells a delivery that leaves a proof any verifier can check from data alone, no one's word trusted. It uses the access form, `[ 2, [ cMIP, params ] ]`, under that cMIP's terms ("A service sold under a cMIP's terms"). The form is **widened from access to bytes to any delivery proven this way**: access to bytes (a key that decrypts to the promised hash, such as a subscription's keys), anchoring (a hash in a batch), a rail swap (the payment's proof), a deterministic computation (re-run to the same hash), a bounty whose answer checks itself.
- **A promised offer** names a vow (`[ 3, vow ]`, rule 13). Each sale is confirmed or contested by those who signed onto the vow. A service's signed word alone (counts, checks, quality) and anything ongoing (hosting over time) are promised.
- **A cMIP may produce the proof; the verifier checks the proof itself, never the cMIP's say-so.** A delivery the verifier cannot check from data it holds is not proven, whatever the cMIP signs.
- **A pooled anchoring service is a proven offer:** the hash is in the batch or it is not.
- **Each proven offer names its checker**: the cMIP in its access form. The checker is not a task (Development, "The checker is not a task"): no agreement names one by a task number, so a collective sets no checker for all its offers. A cMIP that checks proofs of delivery declares so in its specification, by Development's field 12, for discovery only: a verifier asks the checker of whatever cMIP the offer names.

*A proven offer needs no one's confirmation, since its proof is the confirmation; a promised one needs its signer's.*

*The widening changes no format. A verifier tells the classes apart by what an offer sells, and asks the checker of the cMIP a proven offer names, among those it holds, for a proof's verdict; with none held, the delivery is not proven for it.*

#### Rule 12. A purchase names the claim it pays under

*In short: a payment for a standing offer, or for a publication carrying a claimed work, is a purchase only if it names the claim it pays under.*

Buying a work under an Agreements claim is not pure money, since the work's claim and offer are Agreements. So a payment is a purchase only if its receipt or claim names the claim it pays under (Money receipt and claim field 9). This holds for:

- a payment for a standing offer;
- a payment for a publication carrying a work some agreement claims.

The claim named is two things:

- the claiming agreement; and
- the line at which the payer's client read it current: a version of that agreement, or the fork, closing or release that last changed who is paid for the work.

The payer's client therefore reads the work's Agreements claim. A Money-only wallet cannot make a valid purchase.

**A payment naming no claim, or a line that carries none of that agreement's claims, is no purchase: money received for nothing, owed back to the payer as a refund** (Money rule 19: the payer named, or whoever signs with the key the payment committed to; with neither, an open, visible debt nobody can claim).

*The claim a purchase may name is read as the current version of the claiming agreement the payment names. So every agreement holding a stake in the work is a claim a purchase may name, a sale under another shown outside the first by rule 14 (Money rule 18).*

*Stated here alone: Money carries field 9 and the payment commitment, and pays the refund (rule 19); whether a payment is a purchase, and what is owed when it is not, is this rule's.*

**Which claim the payment buys under is settled where the payment truly takes place.**


**On a request rail**, the payee's side commits to each payment before it is made (the payment cMIP's commitment, which an invoice carries). There, **the claim a purchase names is the one the seller's request committed to**, so a buyer cannot pay a version the seller has left. The payment is a purchase under it, whatever the claim became after.

**On a push rail**, the payer pays an address with no request. There, **each holder of the work's stake in the claim named settles on its own chain**:

- a receipt that holder records before its own signature on the act superseding the claim is a sale under the old claim. The superseding act is a version of the claiming agreement changing who is paid, a release, a fork or a closing;
- a receipt recorded after that signature, its own history holding it, is not.

**The payment is a purchase only if every holder's receipt is a sale; otherwise every holder refunds what it received** (paid by Money rule 19), and the buyer buys again under the current claim. Until every holder has signed its receipt, the payment is **pending.**

A push payment to several holders is one rail payment: their receipts carry its one rail proof.

*The clock is when each holder records, the only one MOR has, and it belongs to the one answering for it. A receipt on another of the holder's sequences than its signature is not after it: each holder answers for its own devices.*

**Where the seller is a collective, a payment becomes a sale once the collective's actions chain records it.** It is recorded by an act that:

- is in the collective's name and counts (done, on its chain, not void at an ending);
- is signed with the collective's own key or with a grant key of its, its split service's among them;
- is the payment's receipt, acknowledges it, or is a receipt for the same rail proof.

And the claim must match:

- on a request rail, the claim is the one the request committed to;
- on a push rail, the claim named was current as that act's agreement in force reads it, and before the collective's own release where the release is current.

Until the seller's chain records it, the payment is **pending**: a sale once recorded.

*The payment is the buyer's act; the sale is the collective's, on its chain, where the fork's line can place it.* *No payment is undetermined.* *"Records it" and "the claim current at an act": the latest version, in force for it, that changed who is paid for the work.*

*The rare cases, in detail.*

*Wrong receipts.* **The payment decides.** **The claim is the one the payment's commitment names** (the payment cMIP: a purchase names its claim). **A receipt naming another claim is a wrong receipt, shown as such, counting for nothing**: neither a sale nor a refund. Its commitment, recomputed from it, is not the one the rail proof carries, so the rail shows it. The push-rail rule above then applies to the holders' receipts naming the payment's claim, each settled on its own holder's chain.

Until a verifier has the rail's answer, receipts of one payment naming different claims are judged none of them a sale or a refund: the payment is **pending.**

*One payment shown as both a purchase and a refund cannot arise: of two receipts naming different claims, at most one carries the payment's commitment.*

*Payments the chain did not record in time.* **A payment the original's chain never recorded before the fork or closing that superseded its claim is no purchase, and is refunded** (paid by Money rule 19). A sale in the fork's history stays a purchase. A payment recorded concurrently with the fork falls under the tie rule: the fork wins, and it is refunded.

*Client conformance:* a split service whose grant has ended issues no payment request. So on rails where the payee's side issues each request (invoices), a payment to a superseded claim cannot happen.

*Stated cost: a wallet that reads only Money can pay a claimed work's publication, but cannot tell whether it bought it: its payment is no purchase, and the money is owed back to it; it can still receive the refund (Money rule 19).*

*Client conformance, a strong SHOULD:* **before paying, a buyer's client finds the offer and verifies that it is truly the latest in its chain**, fetching the seller's latest version from its homes or relays, **and warns where what it shows is outdated.**

*What is already in place stands: a purchase names the version and the point at which the client read it current; on a request rail the seller commits to the version sold; on a push rail each holder settles on its own chain. Guidance, not rule: sellers prefer request rails for sales; an offer may carry its own expiry on a named time reference.*


#### Rule 13. Vows: the state of each sale

*In short: for each sale of a vow, the core records only whether it is pending, confirmed or contested.*


The core keeps only the grammar:

- a vow (Envelopes type 5);
- offers naming it with their words (the offer format);
- **the state of each sale of it**: was what the offered deal promised respected.

**Each sale is pending, confirmed or contested.**

- **Confirmed:** the buyer acknowledges an act about the vow. For a payment, this is done in a payer's claim of that payment by the payer its commitment names (rule 22: never the payer a receipt names), its `acks` naming an act about the vow, held and valid (Envelopes rule 17).
- **Contested:** the buyer signs a contest naming the sale (type 14).
- Otherwise, **pending.**

**Only someone who signed onto the vow may confirm or contest it**, **at any price, including none.** They sign on:

- for a payment that is a purchase under an offer naming the vow (rule 12), by their own payer's claim;
- for an offer at no price, by a signature act naming the offer.

**A vow nobody signed onto carries no state.**

**What a vow covers.** A vow covers anything MOR cannot hash, existing or to come: a good in stock or to be made, a service, an hourly rate, storage, a seat. **Every sale of a physical good or of a service names a vow** (an offer's `[ 3, vow ]`), **unless its delivery is proven** (a proven offer, under the access form, rule 11), so each such sale is pending, confirmed or contested. One form for one task. *MOR needs a structured way of selling something that has no bytes, or no bytes yet.*

**A proof may sit inside a vow as evidence.** A promised sale may carry a proof of delivery beside the buyer's confirmation or contest: it is evidence, shown, and replaces neither. The sale's state is still the signer's confirmation or contest alone. How a proof is carried and shown is the cMIP's.

**Promises, and bundles.** A vow holds one or more promises. A seller bundles them as it chooses, in public: a season, a series, a single match. What each promise is, and how promises are listed, is the vow's words or a cMIP's.

- **A contest names the sale it contests**, never the whole vow ("Contest (type 14)"). **Naming the promise that failed is optional** (field 1): a buyer may know only that something went wrong. One failed match marks one buyer's sale contested, not the season's every sale.
- **A confirmation likewise names the sale** it confirms. A buyer's claim of a payment is that sale, so a confirmation made there names it; for a free sale, the buyer's act acknowledges both the act about the vow and its own signature act on the offer.
- **What a buyer is owed back when part of a bundle fails is set by the bundle's terms**: the offer's words, its agreement's, or a cMIP's. The core fixes no partial refund.


**A vow names what is promised to the buyer, never the event itself.** A place in the stand to watch a match is one vow; access to a stream of the match is another; neither is "the match". **Two vows on one event stay separate**: neither references the other. This is how they are made, not a check a verifier runs: a vow naming another stays valid (below). **Each may reference the event, where the event exists on MOR as an act of its own** (in its `refs`, Envelopes), never as a vow, since a vow names what is promised. A reference carries no claim, no stake and no marking under rule 14. Each vow sits under its own agreement; whether an agreement claims a vow, and how its rights are managed, is its signer's business.

**A vow's reference to another vow is not an act about it.** Any vow may still name another in its `refs`; that reference moves none of the other vow's sales' states: acknowledging the referring vow confirms no sale of the vow it names.


These hold too:

- "an act about the vow" is one of its versions, or a valid act naming its name in `objects` or `refs`, except another vow naming it in `refs` (above);
- a contest prevails over a confirmation and voids nothing;
- a free sale is confirmed by any valid act of the buyer's acknowledging such an act;
- a sign-on by signature is by anyone other than the offer's own side (its signer, its payee, the parties of its agreement);
- a payment whose buyer signed nothing the verifier holds, or committed only a bare key, is a purchase nobody signed onto, listed, with no state (a stated cost of rule 22).

**Everything about the substance is the cMIPs'**:

- what is claimed or delivered and how it is confirmed;
- what a vow became and how claims reach it (an ordinary work claim, dated by itself under rule 7, so nothing is backdated);
- quantities and seats;
- delivery points;
- kinds;
- and streams (the live media cMIP).

*By keeping the grammar lean, the same tools serve every kind of event or service; the cMIPs a specialized fork uses decide the rest.*

**Rule 7 speaks of works only:** a claim on a vow's name covers nothing it became.

#### Rule 14. A sale is tied to the seller's agreement, not to the work's stakes

The core cannot tie a sale of a work to that work's stakes. It ties a sale to the agreement of whoever sells, and makes a sale outside the claiming agreement visible to those it concerns.

A publication or standing offer for a work with an existing claim, not signed under that claim's agreement, is a valid act that visibly contradicts a signed agreement:

- the claim's holders contest it;
- a keeper has the claim on record;
- the seller's own split shows what they took.

*The bits and bolts are all in the core: work claims with order, contests with standing, keeper records, the published split, the publication's `for` field, and the payer's claim. A client that refuses to sell a claimed work under a plan that does not pay the claim's holders, or a reputation service that reads contests, can be built on them, and adoption decides.*

*Client conformance.* A client SHOULD show such a publication or offer as outside the claiming agreement.

**A vow's name is read as a work's.** A standing offer naming a vow (`[ 3, vow ]`, rule 13) in which an agreement writes a stake, not signed under that agreement, is the same valid act visibly contradicting it, shown as outside it. One rule for anything sold under a claim, bytes or not: a buyer sees that the seller is not the agreement claiming the vow.

"Signed under that claim's agreement" is read as made under a version of that agreement's line (the offer's field 0). A lone seller's offer is under none. Whether every party of a deal signed the offer is checked as rule 11 says (in a deal, every party).

**A deal made on a vow has an owner: the vow's signer, who holds the stake.** A stake an agreement writes in a vow, its object the vow's name, is held by the vow's signer; in a collective's terms, the collective's own vow is held by the collective, written null. The overhead (a platform's fee, a helper's share, a fee) comes off the top (rule 22), and what is left is the owner's. A version that pays the owner less needs the owner's signature where the owner has no voice in the power making it (rule 48): in a deal the owner who is a party signs every version anyway (rule 47). Terms writing a stake in a vow held otherwise are invalid, signed by every party or not.

*The deal is based on a vow, and somebody owns the vow.* *A worker's vow, a job booked through a platform: the job's agreement writes the stake in her vow, held by her; the platform's fee comes off the top; what is left is hers, and the platform raising its fee needs her signature.*

- The vow's signer holds the whole stake in the version that first writes it (the founding terms, or a clone whose parent writes no stake in that vow). From then on the stake moves as any stake moves: by its holder's transfer (rule 10), by succession (rule 89), or by a version its holder signs (rule 48), so the owner may sell or share what she owns.
- **A deal selling a vow it writes no stake in.** An offer made under an agreement and naming a vow counts whether or not that version writes a stake in the vow: how a platform or a marketplace sells others' vows is a cMIP's. The core only shows such a sale where another agreement claims the vow (above); the owner's protection is her own signature on the deal, and what her client shows before she signs (her client flags terms dividing money with no stake: "No owner").
- The vow's signer is read from the vow the verifier holds, by its name (its genesis). A verifier that does not hold the vow cannot tell its name from a work's hash, and judges nothing, as for an offer's `[ 3, vow ]` ("Standing offer (type 6)").

*Budgets are not vows: the vote that decides where a common budget goes is a cMIP's; the projects answering the need make vows, each a deal with its owner, which the budget pays.*

**After the fork of a collective, the original's stake passes to the successors in the shares the fork act gives them (rule 91): a successor's sale is under the claiming agreement, as a holder of that stake, never outside it.**

*Stated cost:* a resale under a fresh vow escapes this marking. A holder of a claimed piece who signs a fresh vow for the same object and sells under it is marked by nothing here: this rule reads only offers naming the claimed vow, and the core cannot tell that two vows describe one object, since the object is what MOR cannot hash. Catching it is the provenance cMIP's, with registries and reputation. The point of cheating stays easy to find: it is the fresh vow, signed by the reseller.

#### Rule 15. A publication carries two agreements

A publication carries two agreements: the publication's own, and the work's.

*How it runs most of the time:* the owners sell through their own publication, and only the work's agreement divides.

Where a publisher carries the work:

- **the publication's agreement** sets the terms of use and divides the publication's income among its participants, a publisher's share among them;
- **the work's agreement** is one participant of it, consenting by its own rule (in a deal, every party signing the publication's agreement), and divides whatever reaches it.

A sale through the publication passes outside in. The publication's split gives the work's agreement its line, which nobody on the publication's side changes alone (rule 27 at each envelope). Then the work's split divides it.

A publisher holds a stake in the publication's agreement, never in the work, by construction. No stake is marked as a publisher's. Recoupment and every other shape are the publication's split cMIP's.


**Money passes layer by layer**: a pool (a subscription service's, splitting by its stated rules; freeze scenario 7), then each publication's agreement, then the work's agreement, as many layers as the agreements chain. *Money is split envelope by envelope, deal by deal, and can reward more participants than the work and the publisher: the pipe too.*

**The receiver of a layer's share is the one pointer the next layer's agreement names for money arriving from above.** The layer above never divides the next layer's share. Behind that pointer is the next agreement's own business:

- a lone owner's payee pointer (nothing to split);
- a split service;
- or an owner acting as treasury for the joint venture.


**The work's agreement designates the one pointer at which it accepts payments, the same whoever pays**: a buyer directly, or a layer above. The publisher points there and divides nothing. Its obligation for the work's share is discharged once that share reaches the designated pointer (Money's receipt shows it). What follows is the work's agreement's alone. *The work's agreement states the pointer, the publishing agreement the cut; the publisher delivers the cut and its responsibility ends there.*

*The rare cases, in detail.*

**The designation is field 14's one grantee**, reusing what the terms already write, read from the holding agreement's version in force:

- in a deal, every grant field 14 lists names the same service, or the deal carries none of them ("A payee's grant in a deal");
- in a collective, its one grant;
- an owner acting as treasury being an owner so named.

**Where it names no split service, a deal of one party holding every stake it writes designates that party's own payee pointer**, the one pointer its terms can mean.

**A backup of the split cMIP's fields (field 29) receives a payment only once the attempt before it failed for that payment, as the cMIP defines failure. The next payment starts again from the designated pointer** (rule 17). *A backup is no link of a chain of judgment: it acts only where the attempt before it failed.* So the designated pointer is always the first. A layer's obligation for the share ends when it reaches the first service in order whose attempt succeeds, Money's receipt the proof.

**An agreement cannot be paid through a layer until it designates one pointer.** This covers:

- an agreement designating none (several owners and no split service: payer-side splitting, rule 16);
- an agreement naming several split services without designating one (field 14's grants naming different services, which the deal then does not carry).

A verifier reads a payout to the designated pointer as paid to that agreement's holding. It reads a payout to anyone else on that holding as a mismatch (rule 27): an owner it does not designate, or a backup whose earlier attempts did not fail for that payment.

Who receipts the incoming share depends on the rail:

- **where the pointer is a split service and the rail pays the service itself** (Lightning to its own node), **that service receipts the share in its own name**, never with a grant key, its receipt naming the split as Money's routes chain receipts (rule 23);
- **where the rail pays an account an owner holds** (a bank transfer to the owner), **the owner may grant the service to sign for that incoming share on its behalf, the rail's proof required on that receipt** ("What a split service's grant key may sign").

Either way the service **splits it under its own agreement** (rules 20, 30: until a split of its names that receipt, it is the service's open obligation). A lone owner receipts its share as any payee does, with nothing to split.

Each layer may pay more participants than the work and the publisher (relays and other pipes, by role shares, rule 22), as its agreement says.

**Rule 16's limits on what a split service's grant key signs stand** everywhere else.

A deal that is a party of another deal signs it where every party of its version in force signs it, a party that is itself a deal signing the same way, at most eight layers deep.

**Any agreement may be a party to another.** The two agreements of a publication are one case of a general rule.

- **Any agreement may be a party of any agreement**, consenting by its own rule. A collective signs with its own keys. A deal, having none, consents when its own parties sign under its own rule: every party (rule 47).
- **As a party it holds stakes**, paid at the one pointer it designates.
- **It is named by its line**: its founding version, never a single later version. Every later version of the inner agreement carries what it signed outward, and cannot escape it by re-versioning. Money goes to the pointer the inner agreement's version in force designates. *What happens in that envelope is separate from the publishing deal.*
- **No loops.** An agreement is never a party of itself, directly or through others: terms making one so are refused; a verifier that cannot follow every layer of it answers unknown.
- **Nesting is bounded at 8 layers**, a freeze parameter ("Parameters").
- **When an inner agreement ends, its stakes move by ordinary acts, and what is still owed stays visible:**
  - **A collective is liquidated by ordinary sales and transfers before its closing**: each stake sold or moved by a stake transfer (rule 10), or released (rule 55). The closing (type 20) still counts only once the collective holds nothing (rule 91). **An absentee's money share follows the collective's own rules**: its split plan, its absence clause, its terms, written before anyone left. What those rules could not deliver stays visible as an open obligation wherever they sent it, until a valid claim appears: never lost, never taken ("Closing (type 20)"). The closing needs no entry for it; its field 4 stays for money owed back. No new field. *Batching those acts together is a client's or a cMIP's convenience, never a second way in the core.*
  - **A deal ends, or winds down, by a clone every party signs** (rule 47), **stating its new conditions**: where its stakes went, what is still owed, as open obligations, and to whom its outer agreement now pays (its designated pointer). What those conditions are depends on the deal. **A deal has no closing act.** A party who is absent is handled by the absence rules: in a deal, one identity judges absence (rules 85 to 89).
  - Either way, the outer agreement keeps paying the one pointer the inner agreement's version in force designates, its own terms unchanged.
- **A manifest neither claims its parts nor is reached by their claims** (rule 7).

*A deal's last clone needs nothing new: it is an ordinary clone of a deal.*

**Responsibility ends at the designated pointer, in every chain of agreements.** Any agreement paying another agreement pays the pointer that agreement designates, and its responsibility for that payment ends there, Money's receipt the proof: a publisher paying a work, an agency paying a production company paying its crew, a compilation paying its parts. What follows is the receiving agreement's alone. *The network is segmented so that the point of failure or cheating is easily found.*

**A compilation is a layer with its parts' consent, a derivative without it.** With the parts' agreements as its parties, a compilation (a course of lessons by different teachers, an anthology) is a layer: it pays each part's designated pointer, and its responsibility ends there. Without their consent it is a derivative: its own work, paying its sources by its own agreement, its published split showing whether it honours them.

### 4. Splits

How a split service runs is defined by the split cMIP its owners' agreement names. Agreements keep only the vocabulary and the guarantees below.

*In short: who names the split service, what it may sign, and what it is handed.*

#### Rule 16. Named by grant key

The owners' agreement names the split service through a grant, with any limits the owners choose (for example how long it may hold money, or which rails it may use), handing the service a grant key.

The grant:

- **in a collective, one grant of the collective** (field 14, one hash). A collective's field 14 stays one hash, its one payee being the collective;
- **in a deal, one grant per payee**, each naming "this agreement" by null and signed by the payee, the service holding one grant key per payee (field 14, a list; "Grant (type 9)").

**Every split service's grant key, in a deal or a collective, signs only receipts for money coming in under the grantor's own claims and offers.** It never signs one whose payer is the service, nor a payout the grantor is owed ("What a split service's grant key may sign"). The one exception: a layer's incoming share on the owner's behalf, with the rail's proof behind it (rule 15).

The grant is revocable at any time under the agreement's rule. In a deal, each payee revokes its own.

**Two models, chosen per collective**:

- a split service: two flows (payer to owners' pointer, service to holders);
- or payer-side splitting: one flow, the payer's wallet paying each holder's own pointer by the stakes of the work's claim, with no intermediary to bypass, each holder's own vault protecting its share.

The model is chosen by naming a split service (field 14) or not; no field says which.

The pointer check, in detail. **Where an agreement names a split service, the payee pointer of the identity its publications are made for counts, for Agreements, only if:**

- every address in it also appears in the split service's own signed pointer in force;
- and every entry of its vault in force appears in the service's own signed vault;
- or both appear in those of a backup the split cMIP's fields name (field 29).

In each case one service carries every address and every vault entry. The identity its publications are made for is the owners' collective, the sole owner, or, in a deal, each payee whose own grant field 14 lists.

An address is a rail, a rail Module and the address bytes it reads (Money, payee pointer). That is how money reaches the service (Money).

*Client conformance:* an Agreements client, and any wallet reading Agreements, checks the pointer before paying, and shows a pointer that bypasses the split service as such, never as an ordinary pointer.

*Stated cost:* a wallet that reads only Money cannot check it, and pays the pointer as it stands; payers who want the protection use wallets that read Agreements.

*Make it legible: a bypass needs the service to sign, publicly, for an address that is not its own: collusion in plain view. No format changes. Set aside: a "receiver" field (anyone can write any name next to their own address); a pointer delegating to another identity's pointer (cleanest, but a change to Money and every wallet); making every pointer change need every stake holder (every new rail would need everyone); sending every buyer's claim to every owner (it reveals buyers).*

*Stated cost, for payer-side splitting: several payments, partial payment if one fails, owners' pointers visible to the buyer, and a wallet that reads Agreements.*

#### Rule 17. The split task, handed to the split cMIP the agreement names

The split task is handed to the split cMIP the agreement names.

*How it runs most of the time:* one split service, field 14's, receives every payment.

**The split service, its backups in order and what counts as a failed attempt are fields of the split cMIP**, signed with the terms (field 14, then field 29).

**Attempts run per payment, not by stages:**

- each payment is tried at the first service (the designated pointer);
- only if that attempt fails, as the cMIP defines failure, is the next tried, and so on;
- **the next payment starts again from the first.**

*A chain of judgment has stages; a chain of backups has attempts, each only if the one before failed, and every payment starts again at the first.*

A payer's obligation ends when the payment reaches the first service in order whose attempt succeeds, Money's receipt the proof.

**Every payer tries the services in order.** A buyer paying the owners directly follows the same rule as a publisher or a layer above: it tries the services the agreement names, field 14's then field 29's, in order, attempt by attempt, each payment starting again from the first. **A backup paid directly receipts in the owners' name, with the grants it already holds** (field 29's, one per payee in a deal; "What a split service's grant key may sign"). In Money's pointer format, the owners' payee pointer leads to the first service, and a backup's address is the one its own signed pointer carries, which the pointer check accepts (rule 16).

**The task:** the cMIP accepts the agreement and the attempts made for a payment, and produces the receiver for this payment.

**The guarantee, the core's: money goes only to an address the owners signed.**

- A cMIP chooses among the services the signed terms name (field 14's and field 29's), never invents one.
- A payout to anyone else on the agreement's holding is a mismatch (rule 27), whatever any cMIP answers.
- Where every attempt fails, the money stays where it is, an open obligation of whoever holds it (rules 24, 30).

*What counts as failure depends on rails and services, so it is the cMIP's arena. A verifier that does not hold the cMIP, or its answer for a payment, reads a payout to the designated pointer as paid on the holding, and a payout to a backup as unknown, never as a mismatch, as a client not holding a judge's specification shows its question as unknown (rule 35).*


#### Rule 18. Each participant's check

Once a purchase triggers the agreement, each participant checks: **is my cut at my flow address, or at my vault where needed?**


Its cut is what the splits that follow the purchase assign it, layer by layer (rule 15):

- the payment itself, where it is the payee (payer-side splitting, or a lone owner);
- otherwise, its payouts in each split naming the payment, signed by a service the agreement names, field 14's or a backup's, whoever paid, a direct buyer included.

It has arrived when its own receipts, valid, each with the rail's answer, place that much at its payee pointer or at an entry of its vault (Money rules 1, 3).

Where the trail stops short (no split yet, a split that breaks the plan, a payout with no receipt of the participant), the check says where, and the shortfall is the service's open obligation (rule 30).

#### Rule 19. What it receives

The split service receives:

- the incoming payment (an ordinary Money payment, for which it signs the receipt);
- any payer's claim for it;
- the split plan from the agreement;
- the evidence for role shares: the referral the payer signed in its claim, and any act offered as a helper's evidence (a relay's delivery record, a service's use record) once the claim of the payer the payment commits to acknowledges it;
- and, for a rail Module, the receipt or claim naming it.

*A Module is a specification: it signs nothing.*

*In short: what the split service must do with every payment it receives.*

#### Rule 20. Publish a split

For every incoming receipt, and for every payer's claim that shows money arrived without one, the service publishes a split. The split is signed with its own key, names that receipt or claim, and lists every payout.

**It delivers the split to every holder it pays, departed or not, and names each fee and who received it** ("Split plan and split"). A split not delivered to a holder it pays is shown so.

#### Rule 21. Conservation

A split's payouts MUST sum exactly to the amount received, in the unit's smallest part. **A split that does not sum exactly is invalid.**

Under the default rule, every leftover unit goes exactly where rule 9 sends it, so a unit sent elsewhere is a deviation.

The split cMIP declares its remainder rule; where it declares none, leftovers follow rule 9. A declared remainder rule MUST NOT depend on the order in which holders are listed.

#### Rule 22. Role shares with evidence

**Every share paying anyone but a stake, a role share, a named receiver or a metric share, is paid off the top only, as overhead; what is left goes to the stakes alone, and within each stake by its holders' shares. A plan putting a non-owner in the rest is invalid.** *Subtle ways to dilute are stopped: shared ownership means shared fruits. Weights between several stakes stay a business choice.*

**A collective may keep money for itself, off the top of its own plan.** Its split plan may name the collective itself as a named receiver, written null ("this collective"). That part is the collective's own money, kept, and never split again: it is **paid to the collective's vault** (Money rule 2, "The vault"), never to its designated pointer, which is its split service's address (field 14) and would split it again. A plan naming the collective off the top is invalid where the collective declares no vault; a verifier that does not hold the collective's identity chain answers unknown. Raising that part pays the stakes less, so it is an owners' matter (rules 48, 95); **spending it is the collective's business**, done under its areas. *A rule that "the reserve pays no member without the owners' agreement" is not one a verifier can decide, money at the collective's pointer being fungible; what the owners get instead is sight (rule 57).*

*In short: a role share pays a helper known only at payment time, and, save a rail Module's, it counts only when the payer's own claim vouches for the helper.*

*How it runs most of the time:* the owners name in advance the helpers they pay (a host, a mastering service), by a named share in the plan, and no evidence is needed. A role share pays for help known only at payment time, and **its evidence counts only when the payer the payment commits to acknowledges it**, in its claim for the payment: the payer chose the helper, and the payer's own signature says so.

Each role payout names who filled the role and the proof. The proof MUST be an act signed by someone other than the split service and the payee.

The evidence:

- **a referral**: the referral the payer signs in its claim (Money, the claim's referral field);
- **any other act offered as a helper's evidence**, a relay's delivery record (relay transport cMIP, "Delivery records") or a service's use record (Development rule 15), whatever cMIP defines it, **counts only when the payer's claim acknowledges it** (Envelopes rule 18: a Money act may carry acknowledgements). So no verifier needs telling which specification defines it, and every verifier gives the same answer.

For a relay's delivery record, **the object it names is the one the payment was for**, where the verifier reads the record's format.

One no claim acknowledges is evidence of nothing. A claim may acknowledge several, so several helpers can share one payment.

**The payer is the one the payment's own commitment names** (payment cMIP: an identity, an anonymous payer's bare key, or nobody): a claim of the same rail payment whose rail proof carries the commitment recomputed from it, its signer, or the bare key it carries, as payer. **It is never the payer a receipt names**, which the split service writes. Where the commitment names nobody, or the rail binds no payer, nothing can be acknowledged, and the share is one nobody fills.

*Stated cost: a payer who stays wholly anonymous, committing no key, cannot reward a relay or a service.*

**No service is paid on its own record alone**:

- a service the owners chose in advance is paid by a named share;
- a service the payer chose at payment time is paid by a role share only when the payer acknowledges its use record, as for a relay;
- a service evidenced only by its own record is not paid.

A service that worked unnamed is paid by whoever chose it: the owners (by naming it in a new version), the payer (through a client that carries the choice into the payment), or the relay or service that used it (by its own agreement with it).

*Whether a client carries a user's choices into the payment is good practice for client developers, the builders' choice, legible to those who look: a payer may pay the pipe if it wishes to.*

*A helper's own signature alone would let it claim help it never gave, and paying for usage invites faking usage; the confirming signature comes from the other side of the deal, as for a referral.* *Stated cost: a payer and a helper together can acknowledge help that never happened, which the owners bound by the size of the share they offer, as for a self-referral.*

A referral the service names on its own earns nothing.

*The exception: a rail Module.* **A rail Module's role share is the one exception**:

- its proof is the receipt or claim naming that rail Module in its field 0 (Money rule 20);
- it counts even when the split service signs it, provided that rail Module is one the payee's own payee pointer or vault names, where the payment was paid (Money rule 4);
- the payee is the identity the payment was made to, whose pointer leads to the split service (rule 16).

A receipt or claim naming a rail Module the payee's pointer and vault never named is evidence of nothing, and counts for nothing as a receipt either (Money rule 4).

*The service cannot invent a Module: what it may name is fixed by the payee, publicly, and the Module's own rule checks the rail's proof. Requiring a third party's signature here would leave only the payer's claim, which payers publish by choice, so most uses would go unrecorded.*

*Stated cost: where the payee's pointer or vault names two rail Modules for one rail, whoever issues the invoice, here the split service, chooses which of them earns; the pointer shows it publicly, and the payee's agreement with the service can constrain it.*

Anyone can check the evidence afterwards.

A role share nobody fills follows the option the plan chose: to the owners pro rata, held open as an obligation, or to a named party. **Absent a choice, it goes to the stakes, in proportion to their parts**, shown before signing (Development rule 13).

**A role share is named by its role, never by a cMIP's hash**: a new transport cMIP needs no clone of every plan.

**Evidence is acknowledged in the claim written at payment.** *Client conformance:* a buyer's client fetches, asks the relay for its delivery record, and acknowledges it in the claim it writes at payment.

*Stated costs:* a record acknowledged after the split fills nothing. One record acknowledged by the same payer's claims for several payments counts for the first on the payer's sequence and is spent for the others (two at one position, on two devices, leave it spent for both).

Several fillers of one role divide it equally.

#### Rule 23. Pay out

Each payout is paid as a simple Money payment to the current holder of the stake, or the named receiver, with its own receipt naming the split.

**A stake held by another agreement is paid to the one pointer that agreement designates for payments** (rule 15). That pointer is:

- its split service, field 14's one grantee, which receipts it in its own name and splits it under that agreement, an owner acting as treasury being named so;
- or a lone owner, with nothing to split;
- or, for one payment, a backup of that agreement's field 29 once the attempts before it failed (rule 17).

An agreement designating none cannot be paid through a layer until it designates one.

#### Rule 24. Keep what it cannot deliver

The service keeps what it cannot deliver as open obligations, visible until paid. Amounts below a rail's minimum are held until they add up to enough.

#### Rule 25. Payout fees

The split plan says who bears each payout's rail fee: each receiver by default, or the split service as part of its cut. **It may state a maximum fee per payout** (plan key 6); **where it states none, there is no cap.**

A payout is discharged when its receipt shows the amount minus a fee no greater than that maximum, where the plan states one (Money rule 21). A shortfall beyond it is an open obligation of the service (stated here, not in Money).

*Client conformance*: **before a party signs terms whose split plan states no maximum fee per payout, its client MUST flag the absence**: where a receiver bears its payout's rail fee, any fee may then come off its payout, and no shortfall is the service's obligation.

*Flag the risk: whether to cap the fee is the parties' choice; the core keeps only that the choice is seen before signing.*

#### Rule 26. Batching

The grant may allow the service to accumulate and pay out periodically or above a threshold, measured on the agreement's time reference.

Every incoming receipt must still be matched by a published split, so accumulation is visible. Payouts paid with one rail payment name the same batch in their receipts (Money).

*In short: what the split service may never do.*

#### Rule 27. Only the plan

A split service MUST NOT pay anyone outside the plan's stakes as currently held, roles as evidenced, and named receivers.

It MUST NOT change the plan: only the owners can, by cloning their agreement.

#### Rule 28. Fees

*In short: a Module states its fee, but only the agreement's own fee entry makes a fee owed.*

A Module states its fee in its own specification (Development field 7), machine-readable and informative. **Only a fee entry of the agreement (terms field 28) makes a fee owed**, and nothing is forced.

Fees are their own field, so a constitution can place them in an area apart from the split plan, or leave them to the clone rule. The bearers (every stake, fees coming off the top) consent by the power the field falls under, save a holder with no voice in it, a deal's holder who is no party or a collective's departed holder: a version adding or raising a fee its stake bears needs that holder's signature (rule 48), or, in a collective, the stake's holders' rule where that holder accepted one (rule 94).

A fee entry says when it applies: every payment, payments for a named publication, or payments by a named rail. **A split leaving out a fee its entry makes owed is a wrong split, and the fee is the service's open obligation.** The split's list of Modules (key 6) is the service's statement, never what decides.


#### Rule 29. Attention

A split may follow plays, views or any metric a module reports.

**A metric share is paid off the top only, as overhead, like every share paying anyone but a stake** (rule 22).

The metric, its module and the resulting payouts are named (the metric and its share in the plan, the records in split key 7), so it is legible like any other.

The metric's evidence is a record signed by the identity running the service that measured it: never by the split service, and never by a Module, which signs nothing.

*In short: what the service owes when something is not paid, and how owners change services.*

#### Rule 30. Open obligations

Each of these is an open obligation of the service, naming exactly one receiver and one agreement:

- every incoming receipt, and every payer's claim showing money arrived, without a matching split;
- every payout without a receipt;
- **every fee a split left out though its entry made it owed**;
- **every payout a seller receipted after transferring the stake, owed to the buyer.**

**A metric's measurer is none of the identities the metric divides among, nor the split service** (rule 29). A metric pays only identities the plan already pays.

#### Rule 31. Switching services

The owners switch services by revoking the grant, naming a new one, and having the identity publications are made for publish a new version of its payee pointer.

In a deal, each payee revokes its own grant, and the parties clone the deal, every party signing, its field 14 listing the payees' grants to the new service.

**A backup the split cMIP's fields already name (field 29), with one grant per payee in a deal, needs no clone.** It receives a payment whenever the attempts before it fail, payment by payment, and the next payment is tried at field 14's service again (rule 17).

Switching for good, so that field 14 names another service, is this rule's revocation and clone.

The old service still owes payouts on everything it received before.

#### Rule 32. The trust that remains

*Between receiving and paying out, the service holds other people's money. MOR cannot prevent theft, but makes it provable from the receipts and the payers' claims: hiding income requires the payer's silence or collusion. Owners who want less exposure choose services that pay out immediately, or payer-side splitting cMIPs, where capable clients pay each owner's pointer directly and no service's honesty is needed.*

### 5. Time, conditions and judges

#### Rule 33. Time

An agreement that needs deadlines names its time reference. "Past its terms" is judged against that reference only.

**An anchor shows a deadline met, never missed.** An anchor of the act itself (Envelopes, task "Anchoring"), at or before the deadline's point on the agreement's time reference, shows the act made in time. An anchor shows only that something existed by a point, never that it did not exist before.

**A keeper's anchored record shows a deadline met too.** An anchor of a keeper's record shows that record, and every record before it in the keeper's sequence, to have existed by the anchor's point (rule 41); so a record of the act lying, in the keeper's sequence, at or before a record anchored at or before the deadline's point shows the act made in time.

**A deadline is shown missed only by a keeper's anchored sequence**: the sequence of a keeper the agreement names, holding a record anchored at a point past the deadline's point, and no record of the act in its records up to and including that anchored one (rule 41). A keeper's word alone never says late: a keeper's sequence orders acts, it does not time them.

**Where the agreement names several keepers**, the agreement's rule for what counts as recorded (terms field 3, rule 38) decides, counted the same way for a deadline as for a rotation: the deadline is shown met when the keepers showing it met, as above, meet that rule; it is shown missed when the keepers whose sequences show it missed, as above, are enough that the rule can no longer be met by the others.

**An anchor of the act itself at or before the deadline's point shows the deadline met, whatever any keeper's sequence lacks**, with one keeper or several. *An anchor shows that the act existed; a keeper's silence rests on the trust rule 42 states.*

Where nothing shows it met or missed, the answer is undetermined.

*Only anchors place anything on the clock. What an anchored keeper's sequence shows about an act it did not record by that point rests on the keeper recording acts as they arrive: the trust rule 42 states. An act proving its own lateness, by naming something that exists only after the deadline, is not a mechanism of MOR 1.*

#### Rule 34. Conditions

An agreement may make an obligation or a payout depend on a condition, evaluated by the condition cMIP it names.

#### Rule 35. The chain of judgment

*In short: an agreement may name, for each judge, who takes over, in order, when that judge answers "unknown" or does not act in time; so a dead judge traps nobody.*

For each judge it names, an agreement may name, in its terms (field 21), the ones that take over, in order, when it answers "unknown" or does not act within its period. The ones that take over are:

- for a judicial task, other specifications;
- for a keeper's operator, an arbitrator or verifier, or the abandonment authority, other identities.

*The split service is no judge: its backups are attempts per payment, not stages (rule 17, field 29), and this rule's periods and deadlines do not apply to them.*

Everyone signs the chain with the terms: every party in a deal, every member in a collective, as for the whole judicial tier (rule 67).

*How it runs most of the time.*

**When a judge answers "unknown", the next in its chain is asked. The first answer that is not "unknown" is the chain's answer. No new signature is needed when it passes.**

- A condition cMIP's "unknown" and a time reference's "undetermined" are such answers.
- "pending" is not (Tasks).
- Where every one answers "unknown", the chain's answer is "unknown".

*A client that does not hold a judge's specification shows the question as unknown, as for any act it cannot judge. That is the client's ignorance, not the judge's answer, and it never passes the question down the chain: otherwise two clients holding different specifications would reach different answers.*

**A judge that does not act passes the question on too.** Each link names a period. Once the judge has been asked and has not acted within it, as the agreement's time reference judges it, the next may act. Where the time reference answers "undetermined", so is the chain's answer.

Within one link, a judge contradicting itself has answered "unknown", and the next link is active with no period to wait.

An arbitrator's answer is an act of the condition cMIP the agreement names (task 9).

*The rare cases, in detail:* periods and stages.

**The chain runs on the agreement's deadlines, stage by stage, per question, never per party.**

*There is no one party's deadline; there is an agreed deadline.*

A question is a disputed fork (rule 47) or any question put to the chain.

- **The first valid request with standing** (rule 54) **starts the active link's period**, measured on the agreement's time reference (rule 33).
- **Later requests on the same question join that stage** and start no clock of their own.
- **When the period passes with no answer, the next link is active, and every earlier link is void on that question.**
- **Only the active link can settle.** **A settlement signed outside its link's active window counts for nothing**: before the window opened, or after it closed.

*When the next judge is triggered, the previous one is void; the active judge settles.*

So two links never both settle one question. Where an earlier link settles after a later one took over, having seen its settlement or not, the later link's stands.

In detail:

- the question of a fork is its reference, the version the settlement requests name (type 22);
- the question of a request to a judge is its field 2 (type 25);
- each next link's period runs from the end of the one before.

The rule is the chain's, so it holds for every judge the chain serves, one active link per question.

**A request counts, and the period runs, only where it is public or sealed to the judge it names.** This holds for the request to a judge (type 25), and for the settlement request (type 22) likewise. A link of the chain the request never reached was never asked.

*It is a chain of judgment: a judge that fails is replaced by the one everyone already signed, so a dead judge traps nobody.*

### 6. Keepers

*In short: a keeper is a signed ordering mechanism serving proofs. It signs a sequence, this act, then this, then this rotation, and anyone holding a copy can check that order. A keeper orders; only anchors time.*

#### Rule 36. Named by the agreement

Keepers are chosen by the parties when they sign, so neither side can later substitute a friendly one.

- In a collective, the keepers change only with every member's signature (rule 67).
- A deal's keepers change only with every party (rule 47).

#### Rule 37. Records

A keeper records the deal and the acts around it as they arrive, one keeper record (type 2) per act, signed by its operator. Its records are everyday acts of its operator in one sequence, each naming the one before (Envelopes rule 13): the **keeper's sequence.** One act is recorded before another when its record lies before the other's record in the keeper's sequence.

**A keeper MUST keep one sequence for its records.** Records in two sequences of one operator are never ordered against each other, so a keeper's second sequence protects nothing against its first: a record counts against a rotation only in the sequence that holds the keeper's record of that rotation (rule 41). Each record is public, names the act's signer, which must be the act's own, and names the operator's previous record in `prev` (type 2).

A keeper's sequence is a signed order and nothing more: it places no act on a time reference.

#### Rule 38. Several keepers

An agreement may name several keepers and a rule for what counts as recorded: any one, a threshold, or all. Only the named keepers' records count as testimony.

**Each keeper's record is judged alone**, against that keeper's own record of the rotation (rule 41). The agreement's rule then counts how many keepers' records stand before their own keeper's record of it: under `[0]`, every keeper named; under `[1, k]`, at least `k` of them (`[1, 1]` is any one); under `[2, operators]`, each of the keepers listed (terms field 3).

#### Rule 39. Copies

Records are signed and hash-linked, so they can be copied anywhere and verified wherever they sit. If a keeper disappears, its records survive in every copy.

#### Rule 40. Keepers check homes

Before recording an act, a keeper checks the signer's identity chain at its homes, and records any rotation it sees.

#### Rule 41. What a rotation cannot erase, and what an anchor adds

A record counts against a rotation only if it sits before the keeper's own record of that rotation, in the keeper's sequence.

The act so recorded, even outside the kept ancestry or disowned, becomes a visible dispute rather than void (Identity rule 34). The keeper's record of the rotation, or of a revocation (rule 76), serves only as that point in the keeper's sequence; rule 42's conditions are those of the act recorded before it.

**A keeper MAY anchor its own sequence** (Envelopes, task "Anchoring"). An anchor of one of its records shows that record, and every record before it in the keeper's sequence, to have existed by the anchor's point. *Anchoring is optional; serious keepers will, and an agreement that needs deadlines may require it of its keepers in its terms.*

#### Rule 42. A record counts only alongside the act it names

A keeper record counts, for any rule that reads it, only if:

- the verifier holds the act it names;
- that act is valid as an act, and its signer is the identity the record names beside it;
- and the keeper is one named by the version, in force for the act, of an agreement the act belongs to ("Belongs to"), or, for an act of a collective placed against the collective's own line, one named by the collective's agreement, as "Made before, made after" says.

A record of an act nobody can produce changes no verdict.

**A keeper's own later rotation never undoes what its records placed.** A keeper's record that a rule of this MIR reads to place an act (such as rules 7, 33, 41 and 76, a deal's declaration line, and "Made before, made after", 5), or that a relied-on version's completeness rests on (rule 47), keeps the effect that rule gave it, whatever the keeper's later rotation disowned, shown as a dispute. *A witness chosen in advance cannot un-witness.* This is Identity's stated exception to its rule that evidence counts only while its signer's chain keeps it (Identity, "Evidence counts only while its signer's chain keeps it").

*What remains to trust is that a keeper records acts as they arrive. A keeper can delay recording a rotation while recording a thief's acts first, or record an act later than it arrived; nothing in the bytes shows it. That is trust in keepers, accepted and stated: choose keepers as carefully as homes. A keeper whose signing key is stolen is such a keeper until the agreement names another: the thief's records keep their effect as the keeper's own do, since a witness chosen in advance cannot un-witness; what the keeper's sequence held before its last anchor cannot be rewritten, and a second sequence orders nothing against the first.*

#### Rule 43. Keepers are always sealed

*In short: a keeper never needs an act's content, and is never given it for being a keeper.*

*A keeper only has to prove that an act existed before a rotation. For that it needs the act id and the signer, both on the act's outside, and a check of the signer's home, never the content.*

A client MUST NOT deliver an act's content key to a party because it is a named keeper. A keeper that must also judge content is named as an arbitrator or verifier as well.

*This is a rule for clients: key deliveries are private, so no one else could check it.*

#### Rule 44. Judging content is someone else's job

Where a third party must judge content (escrow and milestones, arbitration, regulated verification), the agreement names an arbitrator or verifier, and the parties deliver it the keys by ordinary key delivery.

### 7. Changing an agreement

#### Rule 45. Clone, never modify

*In short: an agreement is never edited. A change is a new version, a clone, that names the version before it and comes into force only when everyone it needs has signed.*

A clone is new terms naming the parent and the parent's latest act. *Client conformance:* the latest act is the latest the client holds; a verifier accepts any act of the parent's chain. Its mark (field 4) names each power its changes require (rule 64, or rule 47 for a deal). For each power, it names the parties whose signatures meet it (rule 65, or rule 47 for a deal).

A clone is a draft until all of these hold:
- every party its mark names has a valid signature act naming it, and every holder it names under a power counted among holders (`[5, stake]`, or a constitutional change rule counted among a stake's holders);
- every party it adds, and every party it makes a holder of an area it did not hold, has one too;
- every holder whose share of a stake it drops or lowers has one too, member or not (rule 48);
- every holder it pays less, by lowering the stake's part of the split plan or adding or raising a fee the stake bears, who has no voice in the power making that change (a deal's holder who is no party, a collective's departed holder), has one too (rule 48); **or, in a collective, where the stake's holders' rule reaches the change and binds that holder, holders meeting the rule have signed, its mark naming them** (`[5, stake]`, rules 64, 94);
- for an issuance, the stake's holders' rule is met, every new holder has one, and the offer's checks hold, or every holder of the stake has one (rule 96): pro rata issuance under the holders' rule is excepted from the bullet on shares dropped or lowered;
- for an owners' matter of rule 95, the owners' consent that rule asks for is given;
- every departed holder whose own stake plan it changes has one too;
- where it changes the release rule, every holder of every stake the parent defines has one too;
- where it changes a stake's holders' rule (field 30), every holder of that stake has one too, as the roster at the line counts them (rules 94, 65);
- the parent exists.

**A draft changes nothing.** *Nobody is added to a collective, or given an area, without their approval: a party added is bound only by its own signature (rule 1), and until it signs, the clone that adds it changes nothing.*

A complete clone closes the parent. Obligations before that point settle under the parent.

Where two complete clones of one deal share a parent, rule 47 says which version is in force. For a collective's agreement, rules 72 and 90 settle it.

#### Rule 46. A false mark makes a clone invalid

A false mark makes a clone invalid.

A clone is invalid, whatever signatures it gathers, if its mark:
- names other powers than those its changes require; or
- names, for a power, a signer who is not among the parties that power is counted among (for `[5, stake]`, and a constitutional change rule counted among a stake's holders, among that stake's holders at the line, rule 94); or
- names too few of them to meet it (as rule 65 counts them, by share where the rule is by share).

A mark naming a succession plan, `[3, party]`, is checked as rule 83 says.

Signature acts by parties the mark does not name bind those parties (rule 1), but do not bring the clone into force.

*The mark names exactly the required powers, never "one at least as strong": powers are not ranked against each other (an area held by one member is neither stronger nor weaker than a clone rule of two of three), so "stronger" could not be computed. It names identities, not signature acts: the signature acts name the clone, so the clone cannot name them. Every version thus says on its face who brought it into force and under which power, and any verifier checks it from the parent and the clone alone. Founding terms' field 4 already said the same thing: which signatures make them exist.*

#### Rule 47. A deal changes only with everyone's signature

A deal changes only with everyone's signature.

In a deal, the signing rule and the clone rule are every party (terms fields 4 and 5). A deal exists when all its parties have signed it. A clone of it completes only when all the parent's parties whose voice remains have signed it. Its mark names the clone rule, `[1]`, and those parties.

Majorities, areas and tiers belong to collectives. Rules 62 to 66, 67, 68, 90 and 91 apply to collectives only. So do the holders' rule and proxies (rules 94 and 97).

**What others relied on stands: a version relied on stays complete.** A deal has no line of its own, so nothing places its versions' signatures as a collective's line places them. Instead, once any party's or keeper's later act relies on a complete version, the version keeps its completeness for everyone, whatever a signer's later rotation disowns; the disowned signature is shown as a dispute, the relying act its place. An act relies on a version when it names it:

- a purchase naming it (rule 12);
- a split under it (rule 20);
- a keeper's record of it (rule 37);
- a later version cloning it (rule 45).

*Checkable: the relying act names the version. Without it, a party who rotates and disowns its signature on a completed version would uncomplete it for everyone, the parent coming back into force under purchases and splits already made under the version.* A version nothing relies on yet is judged by its signatures as Identity judges them.

**A deal ends, or winds down, by such a clone.** A deal has no closing act. Its parties end it, or wind it down, by a clone every party signs, stating its new conditions: where its stakes went, what is still owed (open obligations), and, where the deal is a party of another agreement, to whom that outer agreement now pays (its designated pointer, rule 15). What those conditions are depends on the deal. A party who is absent is handled by the absence rules (rules 85 to 89; in a deal one identity judges absence): where the deal provides it, the declaration removes the absent party's voice (rule 89), and the clone completes with the parties whose voice remains; the absent party's stake is never reduced without it (rule 48).

*Stated cost: one silent party blocks every change to a deal, however large: one person can hold everything until all agree, as in deal making in everyday life. The ways out: the abandonment clause, whose declaration is not a clone, can remove an absent party's voice where the deal provides it (rule 89); succession plans; and, for many parties or a long relationship, a collective.*

*How it runs most of the time:* a deal has one line of versions, each a complete clone of the one before, and the latest is in force. A deal cannot break: it has no keys of its own, and a clone that is not complete, or whose mark is false, is a draft or invalid and changes nothing (rules 45, 46).

**Rarely: two complete versions of one deal.**

**The fork.** Rarely, every party whose voice remains signs two clones of the same version, neither descending from the other (two devices out of step, a careless client, one party in bad faith and another mistaken). Both are complete, and that version has two branches: a fork of the deal.

Then, in short: the reference holds while the deal has two branches; a new act may follow either branch, and counts; the parties settle the fork by one version that names every tip it drops; a settlement is final for everything its signers could see; and where the parties cannot agree, the judge of forks the reference names may settle it on a party's request. Each part follows.

**The reference.** While the deal has two branches, the version before the split is the reference. It is the last version every party agrees on, what a verifier reports in force, and the point the settlement names.

**Acts on either branch.** A new act may follow either branch, and counts. A payment, a split, a sale or a release naming a version of a branch is judged under that branch's latest version. One naming the reference is judged under the reference. Every party signed both branches, so what is done under either counts, in Agreements and in Money alike, which cannot be separated here.

*The buyer is protected from the parties' human error: the version before the split cannot always count, given the decentralized nature of the network.*

**The settling version.** The parties settle the fork by one version that names every tip it drops, signed by the parties of every branch. From it on, its line is in force and the dropped branches are not. A branch that grows settles nothing.

Only complete versions of the deal count here. A version is read as settling only once it is complete and is a version of this deal. A draft, or an act whose parent is no version of the deal, is nobody's version, and changes nothing.

The settling version is a complete clone:
- whose parent (field 11) is the latest version of one branch; and
- which names in field 26 every tip it drops: the latest version of each other branch, ascending, each cited in `objects` in that order after its parent.

It is signed by the parties of all the branches involved. Besides its mark, which names the clone rule and the parent's parties as for any clone of a deal (this rule), every party of each tip it drops whose voice remains signs it too. So no one on a dropped side is dropped without consent: everyone sees every option, everyone signs the choice.

**What a settlement could see.** MOR has no clock, so a settlement could see exactly what its history holds:
- its own line (its parent and the versions before it);
- each tip it names, with that tip's history;
- what it cites, each with its own history;
- for a judge's settlement, the histories of the version it keeps and of the tips it drops.

Nothing else counts as seen.

A settlement counts only where it names every tip it saw. Where its history holds a version of the fork that is neither on the line it keeps nor at or below a tip it names (a newer version of a branch whose older version it names, or a branch it cites and does not name), it settles nothing, and is a plain version.

**Finality.** A settlement that counts is final for everything its signers could see, and nothing they could not see reopens it. These reopen nothing:
- a version on a dropped branch the settlement does not hold (published late, kept back, or grown there by a party left alone on it);
- a whole branch the settlement never saw;
- a second settlement whose history holds it (made after it), whoever signs it, the judge of forks included.

Only a new version on the line the settlement put in force, complete as this rule asks (signed by every party whose voice remains in the version in force), changes the deal. A client may show such a late version as a leftover of a dropped branch; it changes nothing.

*Stated cost: whoever signed a version on a dropped branch that the settlement's signers could not see loses it, honestly forgotten on an out-of-step device or kept hidden on purpose; nobody who settled in good faith loses anything.*

Finality stands: a settlement is final against a complete branch its signers never saw. *A forked deal is not read as broken, as a collective can be.*

**Client conformance against hidden branches.** Every hidden branch passes through one point: a party signing two different successors of one version.

*Client conformance*: **before signing a successor of a version, a client checks, across all its owner's devices, whether its owner has already signed another successor of the same version; if so, it shows the first and does not sign silently; and a settling version the client signs names every successor of the reference its owner signed** (as a member's client does for a collective's endings, rule 91).

Proposing a version is signing it; the reference is the latest version both the settling version's line and every tip it names descend from; a successor is named where it is on the settling version's line or at or above a tip it names.

Accidental double signing disappears; what remains is deliberate, its trail public for good.

*Stated cost:* a newcomer on a branch the settlers hid is dropped by a settlement that does not name it. This is reached only by settlers who knowingly double-signed past their clients' warnings, their signatures public for good.

*Client conformance*: **before a newcomer signs onto a deal, her client looks for another successor of the version she joins, at the parties' relays and keepers, and warns her if it finds one.**

Every version on the line she joins is looked at; a successor a settling version on that line names (at or above a tip it names) was dropped for good and is not warned of.

*For deals that want more:* a keeper; nothing new in the core.

**Two settlements of one fork, neither holding the other.**
- Where both are the parties' settling versions: every party signed both, and the deal is tangled, on its reference, until a settlement holding both, and naming every tip, settles it.
- Where both are one judge's: a judge's two settlements of one fork, neither holding the other, both count for nothing. They are read as the judge answering "unknown" (rule 35): the deal stays on its last agreed version, and the next link of the chain of judgment settles, with no period to wait. The double signature stays visible, and a client shows it beside the fork.
- Where one is signed by every party and the other is a judge's: the parties' holds, and the judge's counts for nothing. The judge is a fallback a request activates, and a version every party signed is the deal changing by its own rule.
- Two links' settlements of one fork never both count: only the link active on the fork settles it (rule 35). A link whose stage on the fork passed with no answer is void on it. So where an earlier link settles after a later link took over, the later link's stands, whether or not the earlier one saw it. A settlement signed before its link's stage opened counts for nothing too.

Within one link, the rules above apply; and a settlement holding another is after it, the first final.

A judge has contradicted itself where its first settlements of the fork, read by history as any two settlements are read ("What a settlement could see"), keep different versions; one that settles the same way twice has not; a judge that contradicted itself speaks for nothing on that fork afterwards.

**A settling version naming a version this verifier does not hold** is read as a plain version until the verifier holds it. From then on it counts as a settlement. The deal stays readable meanwhile, forked or on its reference, and the parties may sign a correct settlement.

**A tangled fork stays on its reference.** A fork is tangled where, with no settlement that counts:
- a branch splits again; or
- three or more complete versions share one version; or
- two settlements of the parties compete; or
- a settling version is made from below its own branch's latest version, which it holds (one that does not hold it could not see it, and that later version loses).

The deal then stays on its last agreed version, the reference, until one settlement naming every tip it drops. The parties lose what the tangled versions changed, and must settle cleanly. Any buyer who paid under a version every party signed stays protected, as for a simple fork, the payment judged under the version it names. *A stated cost, borne by those who kept signing two versions.*

**The judge of forks.** Where the parties cannot agree, the judge of forks the reference names may settle the fork. The reference names which of its judges settles forks: terms field 27, one identity among its arbitrators or verifiers (field 13). Where it names none, none does, and the deal waits on its reference. *A grant to act under specific conditions, here the condition being the existence of a fork.* Every party consented in advance by signing the reference, or a branch that inherits it.

The judge of forks acts only once activated by one of the signing parties: a settlement request (type 22), signed by a party who signed the reference, naming it.

Its settlement (type 23) names that request, the version of the fork it keeps, and every tip it drops. From it on, the kept version's line is in force, what grows on it included, as after a settling version. It is final as a settling version is, for what it could see, and counts only where it names every tip it saw.

It follows the chain of judgment:
- Where the judge of forks does not act within its period, the next link of its chain (field 21) takes over, with no new signature, as everyone signed the chain. The period is counted on the deal's time reference from the first request on the fork, later requests joining that stage (rule 35).
- An answer a link gives after its period counts for nothing in that question.
- Settling a fork is signing a fork settlement, so only an identity in the chain can settle. A link that is a specification signs nothing and passes the question on once its period passes.


Any judge speaks once per fork: a second settlement, whose history holds the first, counts for nothing.

What the request shows: that a party who signed the reference knew of the fork when it signed the request; never on the judge's own initiative. It does not show that every party knew, the party losing a branch included.

*Stated cost: on a bait branch, the party who made the bait knows of the fork and may request at once, and a judge chosen badly may settle before the other party has seen it; every party chose the judge, by signing the reference.*

**The keepers' record of who signed what, and in what order, is the evidence the judge weighs**, never an automatic "first wins". With no judge of forks named, the deal stays on its reference. The amicable path comes first: a version naming every tip it drops, signed by the parties of all the branches.

*A keeper or arbitrator settling a fork no party has asked about would reopen the bait branch through a colluding keeper; the request is what MOR can check, having no clock.*

*Settled only by a version naming the other branch's tip, so every signer saw the choice: under "the next complete version on one branch settles it", a party in bad faith could steer an unaware partner into settling the fork with a harmless change, and a verifier holding one branch only would see no fork at all; one act naming the tips makes the choice knowing and the settlement checkable. Its cost: field 26. Tips are what matter on the forks.*

*Why every party of a dropped tip signs: by accident the case almost never arises; on purpose it is a bait branch, a fork only one side knows of, used to take a newcomer's money, work, rights or name, then settle on the other side and drop them. Stated cost: a party of either branch can refuse every settlement and keep the deal on its reference; that is all it can do.*

*Why finality: treating a version nobody could see as possibly made before the settlement would reopen a settled deal, and break "only what a party can read can be used against them".*

**The alarms.** *Client conformance:*
- **a seller's client, and a split service, MUST raise the alarm when a payment names a version of the deal that does not descend from the version they hold, showing both branches and pointing to settling the fork;**
- a version they do not hold raises it too: an unknown version is the hidden fork the alarm exists for, to be fetched from the payer's relays;
- a payment naming an older version of the same line, with no fork, raises a plain notice, not the alarm.

*A fork and an outdated offer are not the same issue; alarm fatigue would teach sellers to ignore the alarm the day a bait branch shows up.*

*A payment under an agreement names the version it followed (Money, the claim's field 9, and the receipt's), so a purchase under the other branch reaches the sellers as signed evidence of the fork: they are the participants who can look for the full picture once made aware of the need to look.*

#### Rule 48. No stake reduced without its holder

A clone can never reduce a stake without its holder's signature, unless the agreement pre-authorised it.

**A stake moved by succession is not a reduction, and the only advance permissions that move a stake without its holder are rule 89's**: a declaration of absence passes the stake to the stake successors the holder's succession plan names, or, where it names none, outcome 1 or 2 redistributes or transfers it.

**Nor is what a stake is paid lowered without its holder, where that holder has no voice in the power making the change.** A version that lowers a stake's part of the split plan (field 8), or adds or raises a fee the stake bears (field 28), needs the signature of every holder of that stake it pays less who has no voice in the power making that change:
- in a deal, a holder who is no party (every party signs every version of a deal, rule 47);
- in a collective, **every holder who is no member: a departed holder (rule 68), a stake successor who received a stake by succession (rule 83), and a holder who bought part of a stake (rule 10).** *A change paying such a holder less is a new version, born from a clone, and every holder it pays less who has no voice in the power making it signs it.*

A member, who holds the power or chose it by signing the constitution, is left to the collective's own rules (rules 64, 65).

**In a collective, the stake's holders' rule may stand in for each signature** (rule 94): where the stake carries a holders' rule by share, and the change hits every holder of the stake alike, holders meeting that rule consent for every holder it binds. A holder it does not bind, one that never accepted it, still signs. What such holders consent to are the owners' matters (rule 95).

**No advance permission reaches what one holder is paid unlike the others.** A holders' rule the holder accepted, by signing or by buying, reaches what every holder of the stake is paid alike (rule 94); nothing reaches what one holder is paid unlike the others of the stake, which that holder alone signs. Abandonment never lowers what a stake is paid while the stake stays with its holder. A departed holder who vanishes does not block the collective's fees, because its stake moves: declared absent, which a declaration may do although the holder is no party (rule 85), to its stake successor, or, with none named, by outcome 1 or 2 (rule 89); whoever then holds it signs, or already holds the say. For a deal's holder who is no party, no advance permission reaches its pay: a deal carries no holders' rule (rule 47).

*Abandonment protects operational flow; it is not meant to change stakes or pay. What it can trigger in that regard is succession, the stake and its pay going to the successor.*

**A stake is paid less by any change that lowers what it receives from a payment or can expect from the plan as written**: another share added or raised, a role share added, unfilled roles sent elsewhere, the rail fee moved onto receivers, the maximum fee raised or removed, the split cMIP or its parameters changed, as well as a fee added or raised. **What a cMIP pays cannot be read from the plan alone, so changing the split cMIP or its parameters counts.** **A share added within a stake by an issuance under its holders' rule (rule 96) is not "paid less" in this sense; one added any other way still is.** **The collective's own part off the top (rule 22) is overhead like any other: raising it pays the stakes less.**

*What money sees and money expects is what matters. Judged by effect: what reaches the stake from a payment, and what it can expect from the plan as written.*

**Since a plan pays non-owners off the top only, every share in the rest is a stake's**: a stake is paid less by anything taken off the top added or raised (a role share, a named receiver, a metric share), by a role nobody fills sent away from the stakes, or by its part lowered against the other stakes' parts; never by a non-owner's share in the rest, which no valid plan has.

*The voices chose the power over fields 8 and 28; those without one did not, and what a stake is paid is what no choice of theirs may break.*

#### Rule 94. The holders' rule

*In short: a stake's owners may agree together, by share, on the changes that hit every one of them alike, where each of them accepted that rule; by default every holder signs.*

**A holders' rule per stake.** A collective's terms may say, for each stake they define, how its holders consent together (terms field 30). There are two forms only:

- **every holder**: each holder with no voice in the power making the change signs for itself, as rule 48 asks, a member consenting through the collective's own rules. This is the default, where field 30 says nothing for the stake (the forced default, one of the declared options);
- **owners holding together at least this share of the stake**, in millionths, counted by share.

**It reaches only the changes that hit every owner of the stake alike**: the owners' matters (rule 95). A change that treats one holder of the stake unlike another of the same size, or lowers one holder's share, still needs that holder's own signature, whatever the holders' rule says (rules 8, 48).

**It binds only the holders who accepted it**: by signing a version carrying it (a member, who may later leave; a stake successor, whose stake is born in a clone it signs), or by buying under it (a buyer, who accepts the holders' rule of the version in force where the collective's line placed its transfer, rule 10). A holder it does not bind signs for itself, as rule 48 asks.

**It is changed only by every holder of the stake** (rule 45), as the release rule is (field 24), and a holders' rule is added to a stake only so: the rule binds only those who chose it.

**How it is counted.** The holders' rule is counted among the stake's holders, members and holders who are no members alike, each by its share; the share asked for is of the whole stake, since no holder loses its say on the owners' matters by leaving the membership. A holder whose share is gone for good still counts against the rule until its stake moves (rule 85). The holders whose signatures meet it are named in the clone's mark under `[5, stake]`, and a false mark sinks the clone (rule 46). A proxy may cast a holder's vote as rule 97 says.

**Who holds what at the vote.** For the owners' matters, a transfer counts only where the collective's line placed it: an act of the collective on its chain acknowledges it (Envelopes, `acks`), or a record names it in field 3. Until then the seller holds, for the collective, what field 7 and the transfers already placed say. A vote counts **the roster at the line that puts the version in force**: the holders the version in force there and the transfers placed before that line name. So a seller and a buyer never both count: the line moves the share once. A transfer the line placed that the verifier does not hold leaves the vote **unknown**, never silently short. Money follows transfers as rule 10 says, unchanged: this is a vote, not money.

**A holders' vote registered by the collective's line is kept.** The signature acts completing an owners' matter are named by the record or rotation putting it in force (record field 2), and placed there; a holder's later rotation shows a dispute on the line and changes nothing. Owner votes are signed by signature acts, never by chain key.

**A deal carries no holders' rule.** A deal has no line, so nothing could keep a passed change from unpassing when one holder rotates: the holders' rule is a collective's only, and terms of a deal carrying field 30 are invalid. **In a deal, the version carrying an owners' matter names the transfers it counts** (terms field 31), as a settling version names the tips it settles; a transfer it does not name counts nothing toward it, and its seller holds for that version. *Agreements serve simple partnerships; more complex structures need collectives.*

*Stated cost:* *an owner bound by a holders' rule can be paid less without signing, where owners holding enough of the stake agree and the change hits every owner of the stake alike. What it keeps: no singling out (the change hits every owner alike), a vote in proportion to its share, its proportion in any issuance (rule 96), and the exit (rule 10). What it loses: the personal veto, and it bears its share of the majority's judgment. The majority paying a service it owns outside MOR is fixed by no rule, a stated cost judged by reputation (rule 68's disclosure). An owner to whom the veto mattered would not have accepted a rule by share.*

*Stated cost:* *a thief holding a holder's signing key can vote the holder's share once; once the line registers it, the holder's rotation does not undo the vote for the collective. The vote can pass only what hits every holder alike, the thief's victim included, so there is nothing to steal by it but the vote.*

*Stated cost:* *a buyer has no vote until the collective places its transfer; members can delay a hostile buyer's vote by not placing it, visibly, bounded by the next record any member draws.*

*Example: an owner holds 10% of a stake under a 60% rule; owners holding 70% take 5% off the top for a service; the owner voted no. Under every holder the no blocks it; under the rule the owner accepted it passes, and the owner is paid a little less, exactly as every other owner of the stake is.*

#### Rule 95. The owners' matters

*In short: what a stake's owners consent to together: what it is paid, new shares in it, a sale or release of the works its income comes from, and what a fork's successors pay the owners they keep.*

**A stake's owners' matters** are the changes its holders consent to by its holders' rule (rule 94), or, where it is every holder, each holder with no voice in the power making the change by its own signature:

1. **what the stake is paid** (rule 48), the collective's own part off the top among it (rule 22);
2. **an issuance in it** (rule 96);
3. **a collective's transfer (type 4) or release (type 5) of a stake it holds in a work**, for the holders of the collective's stake in itself;
4. **a fork's completeness, where a successor's founding terms pay a kept holder less** than the original's agreement in force at the fork's line, for the kept holders of the original's stake in itself (rule 91).

*Every one of them hits every holder of the stake alike, so a holders' rule can reach it (rule 94). The core cannot tell "substantially all" the works from one song, nor a fair price from a token one; what a rule can say is who consents.*

**A sale or release of a work** (3). A transfer or release of a stake a collective holds in a work counts only once the owners of the collective's stake in itself who have no say consent: holders of that stake meeting its holders' rule, or, where it is every holder, each holder who is no member, by a signature act naming the transfer or the release. Those signature acts count where the collective's line placed them (an act of the collective acknowledging them, "Made before, made after", 2), the roster at that line (rule 94), and are kept there. *Without it, the members sell the works for a token sum to a collective of their own, and a departed holder's 25% of all the collective's income is "25% of nothing", unsigned.*

**A fork's successors** (4). For each successor, its founding terms must pay no kept holder (a departed holder of the original, or a member whose voice remains on no side) less than the original's agreement in force at the fork's line pays it, as rule 48 reads "paid less". The comparison runs between the original's stake in itself and the successor's, each written null in its own terms and read across the two collectives as the kept holders' stake: its part of the split plan against the other stakes', what each plan takes off the top, the fees each stake bears, and the split cMIP and its parameters. Where a successor's founding terms pay a kept holder less, the fork completes only once the kept holders consent: holders of the original's stake in itself meeting its holders' rule, or, where it is every holder, each kept holder paid less, by a signature act naming the successor's founding terms, as a holder who is no party signs a deal's version. **A kept holder's consent counts only where an act of the original in the fork's history acknowledges it**, as a closing relies on a release: placed by the original's line and kept there, so a kept holder's later rotation never unmakes a complete fork; one nobody acknowledged is shown, and the fork waits. *A chain-key ceremony per kept holder per fork would weigh on passive owners.* *With the same plan the check is met at once; with no kept holders there is nothing to check.*

**The way out against an owner who vanished** is the declaration of absence, which may name a holder who is no party (rule 85): its stake moves to its stake successor, who consents or not, or by outcome 1 or 2.

*Stated cost:* *a members' collective with departed holders and no holders' rule waits for their signatures on its sales and releases of works, and on a fork whose successors' plans pay them less, as its fee changes already do. Shared ownership means shared fruits.*

**Prices are no owners' matter.** The price of what a collective sells, and whether it publishes at all, are the collective's business, decided under its areas (rule 60). Owners who have no say are protected by sight (rule 57) and by the exit (rule 10), never by a vote on prices. *Stated cost:* *the members can offer every work at a token price, or at none; offers are public, so the owners see it, and can leave. A vote on every price is not one owners could cast.*

#### Rule 96. Issuance, with pre-emption

*In short: a stake may take new owners, every existing share shrinking in the same proportion, if its owners agree by their holders' rule; each existing owner is first offered its own proportion on the same terms.*

**An issuance is a version of a collective's agreement that adds new holders to one stake and scales every existing holder's share by the same factor**, within rule 9's rounding, so no existing holder's proportion to another changes. It names the issuance offer it follows (terms field 32), changes no other stake, and changes no holder of that stake otherwise. It is an owners' matter (rule 95), decided by the stake's holders' rule (rule 94), and, where the stake has none, by every holder of it, members included, as rule 8 asks.

**It is pre-emptive.** Each existing holder of the stake is first offered its own proportion of the new shares, on the same terms as everyone else, by an **issuance offer** (type 26), an act of the collective naming the stake, the size of the issuance, its terms, its new holders and its period. What the existing holders do not take up goes to the new holders the offer names, in its shares.

**The pre-emption is waived only by every holder of the stake**: an issuance every holder signs needs no offer. **The holders may also waive it standing**: a standing waiver in the stake's entry of the holders' rules (field 30), set or removed only by every holder of the stake, as the entry is changed (rule 94); a buyer accepts it by buying, as it accepts the holders' rule. Under it an issuance names no offer (field 32 written `[0]`), is decided by the stake's holders' rule, and is complete, besides the powers its mark names and that rule, once every new holder it adds signs it. *A standing waiver changed only by every holder is still "only by every holder"; a venture issuing often need not wait out an offer period each time.*

**What the core checks**:

- **the offer reached each holder**: it is public, or sealed to that holder among its recipients (Envelopes, `to`), as a declaration must reach the member it names; where a holder is itself an agreement, it is public (rule 57). **An offer that did not reach a holder starts no period for that holder**;
- **the offer names its period on the agreement's time reference** (terms field 6, or field 2's task 10); terms naming none carry no issuance offer, which counts for nothing under them;
- **the period's lapse is the time reference's answer**, a fact the caller states, as for every time reference (rule 33): the core reads no clock.

**The issuance is complete** (rule 45) only when, besides the powers its mark names and the stake's holders' rule:

- every new holder it adds signs it;
- for every holder of the stake the offer had to reach (the roster where the offer is placed on the collective's chain, rule 94), the offer reached it, and it either took up its proportion within its period, or its period lapsed;
- each holder that took up receives exactly its proportion of the new shares, and the rest goes to the offer's new holders in the offer's shares.

A holder the offer did not reach had no period: an issuance naming that offer stays a draft, and the collective makes a new offer, reaching every holder, and an issuance naming it. **A holder takes up** by a signature act naming the offer (type 1), within its period, with its payment where the offer names a price, a Money receipt or claim naming the take-up (the receipt's field 5, the claim's field 4), as a transfer's payment fulfils it, together at least its exact proportion of the price. **A take-up the agreement's time reference places within the period counts, placed by the collective's line or not**: a holder's protection is to anchor it, as a buyer anchors a transfer (rule 10); without an anchor, the time reference judges it where the collective's line placed it (an act of the collective acknowledging it, or a record naming it in field 3). Whether it lies within the period is the time reference's answer, which the caller states (rule 33). *Without it, the members issuing would decide whether an owner's paid take-up was placed in time.*

**The same terms are the offer's own**: one offer, its price and its words (fields 5 and 6), for every holder and every new holder; the core compares nothing beyond the offer. A transfer placed after the offer passes the seller's place in it to the buyer, the new holder inheriting the full position (rule 10).

**Any other form of issuance is a cMIP's, within this one**: staged issuance, vesting, other offer terms. A cMIP may offer more as a term of the issuance, never less: it is built within the pre-emption, never around it.

**Collectives only**: a deal carries no holders' rule (rule 94), so an issuance in a deal is signed by every holder, as rule 8 asks.

*Stated cost, as in law:* *an owner that does not take up its proportion within the period, or has no money for it, is diluted. The core cannot judge a price, so a token price dilutes an owner who cannot or does not pay. An owner who vanished for good is reached by the declaration of absence (rule 85). "Issuance below the last price needs every holder" and "a right that stays open" are not rules: the first no verifier can decide, and it restores the veto; the second restores the veto, or leaves a floating claim nothing can value or close.*


#### Rule 97. Proxies

*In short: a passive owner may hand its vote on the owners' matters to someone else, for one vote or until revoked; never its money, its property, or anything that needs every owner.*

**A proxy** (type 27) is an act of a holder of a stake naming a proxy identity and the stake, scoped either to one owners' matter, a version it names, or standing over the stake until revoked: the owner's choice when signing.

**The proxy's signature act on a version counts as the holder's vote under the stake's holders' rule** where all of these hold:

- the collective's line placed the proxy (an act of the collective acknowledging it, or a record naming it in field 3) before the line that puts the version in force;
- no revocation of it was placed before that line;
- it is the holder's latest proxy on the stake so placed: one proxy per owner per stake at a time, a new one replacing the old;
- **the holder placed no signature act of its own on that version at or before that line: the owner's own vote always beats the proxy's.** A signature of the holder's the line does not reach is no vote there, and leaves the proxy's standing.

**A proxy is revocable by the holder at any time**, by a revocation of the proxy (type 28), placed as the proxy was.

**The fence.** A proxy only votes, and only on matters a majority of the stake decides under a holders' rule: an owners' matter under the stake's holders' rule by share (rules 94 to 96), or a constitutional change counted among the stake's holders (rule 69). Never:

- on anything that needs every holder: changing the holders' rule (field 30), waiving pre-emption (rule 96), or a change that needs the holder's own signature (rules 8, 48);
- on the owner's money or property: taking up an issuance, selling or transferring the stake, signing a deal;
- through another: **no sub-proxies**, as a grant key signs no grant (rule 73); a proxy act counts only signed by the holder.

**Owners' votes only**: a member's voice is not handed on; members vote in person. **No instructions**: an instructed proxy is the owner's own signed vote, which anyone may deliver. **Collectives only**, as holders' rules are.

*A proxy can add a voice to a majority decision, never give away what is the owner's alone. On MOR no attendance is needed, so proxies have little use, and they are fenced.*

#### Rule 49. Exit

Agreements are always in core format, so activity on a fading cMIP moves by cloning the agreement onto another.

### 8. When something goes wrong

*The core never enforces; it makes default impossible to hide.*

#### Rule 50. Atomic where possible

Payment is acceptance. Delivery released by payment cannot be defaulted on.

#### Rule 51. Failure attribution

*In short: when a payment or a payout fails, the record names who and what failed.*

Money's receipts and payment claims name the hop and agreement that failed. A split's missing payout names the receiver and agreement.

Agreements define what follows: refund, redirection, contest.

#### Rule 52. Negotiation record

Two sides negotiate by negotiation messages (type 18). Each message is locked and signed, names the previous message in the thread, and acknowledges the latest one received from the other side.

The record is provably complete up to the last acknowledged message. A message acknowledged by the other side fixes every message of the thread before it, by the previous messages each names. It also shows the other side received them all.

Only negotiation messages make the record:

- a text act, a reaction or any other act naming the thread is no part of it, carries no acknowledgement (Envelopes rule 18), and proves nothing about its completeness;
- a negotiation message whose previous is not a message of the same thread is no part of the record either.

Two messages naming the same previous are a fork, visible and provable (Envelopes).

#### Rule 53. Disclosure by choice

Either party may publish the record, by publishing the keys of its messages (Envelopes: going public later).

#### Rule 54. Standing

A contest counts only when signed by someone with standing:

- a party to the agreement;
- a holder of a stake the contested act affects;
- a keeper, arbitrator or verifier the agreement names.

A contest never changes any act's validity, status or selection. It is shown alongside what it names.

*Reputation, arbitration and courts build on this record from outside the core.*

### 9. Public domain

#### Rule 55. Public domain

The owners may release a work by a release act (type 5). It takes effect at once, or at a future point on the time reference, its content key delivered then by an identity the act names.

A release does three things:

- **the claim ends**: nobody earns from the work as its owner any more, and no sale of it is outside a claim;
- **its history stays named**: the stakes it ends and the claims naming its creators;
- **its content key is published**: anyone may carry or sell copies.

It needs every direct owner's signature, departed holders of a stake in the work included, a collective owner signing by its own rules across the areas of every layer a release touches. This holds unless the release rule says otherwise (field 24, set at founding, changed only by a clone every owner signs).

**The release rule is the holders' rule's first instance, and its threshold is counted by share**: "holders together holding at least this share of the stake", never a number of heads, so a holder splitting its share among identities it controls gains nothing. **A collective's release of a stake it holds in a work is an owners' matter** (rule 95): it counts only once the owners of the collective's stake in itself who have no say consent.

**A release is permanent.**

**A claim the release does not name in its history is shown beside it**: made after it, or a competing claim, openly contested (rule 7), which the release cannot end.

*A work that belongs to no one, but traces its history.*

Its legal effect lies outside the core.

## Part two: collectives

### 10. A collective

#### Rule 56. A collective is a full identity

A collective is a full identity, with its own genesis, homes, signing key and chain key.

It uses the Identity MIR unchanged: it signs its own receipts, sets its own payee pointers, and rotates. Its genesis declares its founding agreement (an Agreements declaration in the declarations slot), and the agreement carries its key grammar.

#### Rule 57. An act is done once sealed to every member and on the chain

A collective's act is done only once it is sealed to every member and on its chain.

*In short: what a collective does, every member can see, and it is written on the collective's own chain. Until both are true, the act is only a plan.*

An act in a collective's name, signed with its key or with a grant key of its, is done, and binds the collective, only once both of these hold:

- it is sealed to every member: every party of the agreement in force for it is among its recipients (Envelopes, `to`), or the act is public; and
- it is on the collective's chain, citing its head as its signer knew it (rule 58).

Before that, even signed, it is planning. It binds no one, its signer included. It counts for nothing in Agreements: an area's signatures on it complete nothing, and an obligation it carries binds no one.

*A counterparty is safe once a later act of the collective cites its act, against everything but the collusion of an ending's signers, which stays visible* (rules 76, 79).

*Client conformance:* the collective's client drafts the act, signs it, seals it to every member, and publishes it where the collective's clients look.

*Powers can be segmented, but information runs free: a continuous assembly where matters are exposed, which only bothers cheaters.*

*Stated cost: a collective keeps no secret from its own members. What a collective plans can stay with some members for a while; once done, it is a fact that touches everyone in it.*

**Owners without a say see the collective's Money acts.** For a collective whose stake in itself has holders who are no members, a Money act in its name (a receipt, an obligation, a payout, a payee pointer) is done only once it is sealed to every holder of that stake, members and others, or public, besides being on its chain; until then it binds no one. The holders are those the agreement in force for the act and the transfers its line placed name (rule 94). *Client conformance:* the collective's client makes those acts public by default.

*Stated cost: more recipients per sealed act, or publicity; such a collective loses privacy from its owners only. The right of exit means little to an owner who cannot see why to leave.*

**Where a holder of the stake is itself an agreement, the act is public; for a collective holder, it is sealed to the collective.** A deal has no line, so no verifier could tell which of its versions was in force when the act was sealed; publicity reaches every party of every version. Sealed to an agreement's own hash (its pointer, its split service's address), or to the parties of one of its versions, an act reaches no holder that is an agreement. Where a collective seals to the parties of an agreement it names elsewhere, "the version in force" is the version the act itself names. This holds wherever an act must reach a holder: a collective's Money acts here, and an issuance offer (rule 96).

**Where an act is held is never a condition.** An act is valid by its signatures, seals and citations wherever a verifier holds it. Relays are transport: they are where a collective's clients publish and look first, a matter of client conformance or of a cMIP (the relay transport cMIP), never of the core. A verifier states where it found each act as information only; no validity rests on it.

One rule covers every act in the collective's name, with no exception by type. That includes records, and forks and closings:

- a record not done is no line ("Record (type 17)");
- a fork or closing not done takes no effect (rule 91).

*The rare exceptions, in detail:*

- **Negotiation messages are the one kind not held to this rule.** They are talk, binding nothing, and on neither chain. They need not be sealed to every member, since planning is allowed to stay secret. The deal they lead to is signed, and that signature is an action, which must be done.
- **The collective's identity-chain acts (genesis, rotations) are done by Identity's own rules**: public and at its homes.

#### Rule 58. Two chains

A collective keeps two chains: a decisions chain and an actions chain.

**The decisions chain** holds its genesis, rotations and records (which write its clones and register who joins and leaves), its grants and revocations, and its fork or closing. Each decision cites the previous decision and the action head it saw, by what it already names:

- a record, by its own sequence and kept tips;
- a rotation, by its kept tips (Identity);
- a grant or revocation, by its own sequence and its citations in `objects`, as an action cites;
- a fork or closing, by its chain act and tips.

Decisions keep their formats.

**The actions chain** holds every other act signed with the collective's key or a grant key of its: debts, payments, publications, releases, sales, and the acts of its grantees' strands.

On neither chain are:

- negotiation messages;
- Identity's own everyday acts, which count for nothing in Agreements. A witness act of the collective is one of them: it adopts nothing and places nothing.

**How an action cites.** An action names, in its inside's `objects`, the collective's identity as chain, once for each act it cites:

- the decision it acts under: a record of the collective, or a link of its identity chain, under the action's own key or an earlier one; for an action signed with a holder's member key, the founding or clone that named the key, its grant (rule 73), stands as such a decision; and
- each head of another device it knows, so that the next act joins the heads where several devices signed at once.

Its own sequence's previous act counts as cited. These entries come after those the act's type defines, all on that one chain. Agreements' formats allow them there: signature, release, resignation, declaration, terms.

**An act within its signer's powers counts for the collective as soon as it is done and cites the collective's head as the signer knew it.** It is judged under what its decisions leave in force: the chain's declarations, records carried forward under earlier keys, and, under its own key, the records its history holds and those before them (rule 72). A record it never cites decides nothing for it, save by the tie rule ("Made before, made after", 7). An action is judged under the decisions its history holds.

**An action citing no decision, or naming on the collective's chain an act that is not on it, is on no chain of the collective, and counts for nothing.** **An action citing a decision is unknown while the decision it cites is not held, and counts for nothing where it is held and is no decision of the collective.**

**An act signed with a grant key cites, among its decisions, its grant.** The grantee's strand is like a device of the collective, limited to the grant's scope.

*An act within one's powers in a collective is an act for the collective. The chain's role is to make acts unmissable, not to confirm them.*

*Client conformance: the collective's client cites its latest decision and every head it holds.*

*Identity's own everyday acts (a witness act, routes, an encryption key) carry no objects (Identity) and are on neither chain.*

#### Rule 59. Key grammar

The founding agreement says how the collective's keys are held and activated, according to the members' taste.

**The signing key** is held in one of these ways:

- one holder;
- a threshold: any k of n members jointly produce one ordinary signature, for example with FROST threshold schnorr;
- a custodian holding the key under the collective's grant.

**The chain key** is held by one holder, by a custodian, or as shares: split so that any k of n members can rebuild it.

**Rotating with a shared chain key.** At rotation, k members bring their shares to one offline device, which rebuilds the key, signs once and forgets it.

*For that moment one device holds the whole key; that is the price, and it holds the next key too, before dealing its shares. Nothing can prove a device forgot a key.*

The Module that does it MUST deal the shares of the next key verifiably, and have them checked against the committed key before that key is relied on, so that the members' shares rebuild it. Otherwise the device's holder could end up the only one able to rotate the collective.

*Verifiable dealing stops sole control, not a copy; a copy kept is the price above, stated.*

**A key grammar MUST leave a way to rotate that survives the loss of any one key holder.** That way is either:

- a threshold below the member count; or
- a named recovery path: a custodian holding a share under grant, or an escrowed share released by the abandonment authority, which the abandonment clause names.

A key holder is anyone holding the whole key or a share of it: a member, a custodian, the holder of an escrowed share.

**Where one person holds the chain key, the agreement MUST name a successor (a seat successor in that holder's succession plan) and an escrowed share released to them under its succession or abandonment clause.** A single custodian likewise needs a recovery path held by another.

A grammar without such a way is invalid.

Two partners may choose "both", provided they name where a further share sits and who releases it. Whether the path works is the members' risk, stated.

*For a single holder, one construction: the key is split so that any two of four shares rebuild it; the holder keeps two and signs alone, the successor and a keeper hold one each, useless alone. When the named authority declares the holder absent, the keeper releases its share and the successor rotates the collective to a new key; the succession clone passes the seat.*

**That succession clone also carries a succession plan for the successor, naming their own seat successor, signed by them in the same clone** (rules 64, 83). So the key always has a named successor, and the clone meets this rule.

*The successor signs the clone to join anyway, so the plan they add is signed by them; the escrowed share stays where the key grammar's recovery path puts it, released by the same clause.*

Where members want individual consent to show, the constitution gives acts of the collective to areas (rule 60).

#### Rule 60. Areas: rules by layer, powers of members

A collective's founding agreement may divide its acts and its operational fields into areas, each held by some of its members.

*In short: an area is a part of the collective's business that some members decide on. An act in that area counts only when enough of them sign it.*

An area is drawn either:

- by whole layer (Identity, Envelopes and Text, Money, Agreements, Development); or
- by act type within a specification.

Genesis and rotations lie outside any area: the key grammar governs them (terms field 19, "Areas"). An area over the Identity layer reaches the collective's other Identity acts. The collective's everyday Identity acts count for nothing in Agreements.

**When an act counts.** An act of the collective that an area reaches counts only with valid signature acts naming it (rule 6), meeting the area's threshold as rule 65 counts it, by holders of that area who:

- are holders of that area in the agreement in force;
- each signed it, or a version it descends from by complete clones; and
- still have a voice there for that act ("Made before, made after", rules 70, 71).

**Members' own keys count as their signatures.** An act of the collective signed with a holder's own key (rule 73), and a signature act of the collective naming an act, signed with a holder's own key, each count as that holder's signature there, in every area reaching the act that the agreement in force for it gives that holder. So an area of one takes one member's act; an area of two takes one member's act and another member's signature, each with their own key. The Money acts a member key may sign for the collective are those Money rule 7 lists. *Where an area needs two signatures, it needs two keys within that area.* The holders' signature acts with their own identity keys count too.

**An area's limits are a cMIP's.** An area may name a cMIP judging limits on its holders' keys, with its parameters (area key 6). It adds to this rule, never relaxes it: the act counts at the area's number only where the cMIP judges it within them, and as that cMIP asks beyond them. A verifier that does not hold the cMIP, or holds no verdict of it for the act, answers unknown for the area, and the act does not count there. *Capping an area is left to cMIPs; what the core keeps is this rule. A limits cMIP may add a second signature and make the first conditional: within its cap, the area's number; above it, one more holder's key.*

**The agreement in force for that act** is the one the collective's identity chain declared at the chain act its signing key is bound to (its binding). Where the act comes after a record made since that declaration, it is instead the clone such a record put in force, as rule 72 picks it. **For an act signed with a holder's member key**, whose binding is the holder's signature act (rule 73), it is the one rule 72 picks for the act, starting from the version that named the key, from the decisions its history holds: the clones the collective's identity chain declares there, and its records.

An act of an area that stands frozen counts for nothing (rule 71).

An act no area reaches counts on the collective's own signature, as any identity's act does, unless its specification is unadopted (below).

*So when members change, the rotation that declares the clone also fences off the old rules: whatever the old key signs afterwards is void under Identity, whoever signs it. Members' own identities do not rotate when they leave a collective; without this, former members could sign under the old rules at any later time, and no one could tell.*

*An area says whose consent counts; the key grammar says who can physically produce the collective's signature. A holder who should act alone without holding the key acts under a grant issued within the area (rule 74), with the grant key it hands them.*

*Layers and areas are one construct: a rule for a whole layer is an area that reaches that layer, held by the members who decide on it; an area may also reach single act types, and operational fields, with one set of rules for holding it, granting within it and changing it.*

**What an area reaching a layer reaches.** It reaches every act of that layer ("Layer"):

- the MIR's own acts;
- the acts of the cMIPs the agreement in force names for that layer's tasks;
- for Development, the acts of the extensions it names.

An act that belongs to two layers or more, of a specification named for tasks of each, is reached by the area reaching each. It counts only when each such area's holders meet its threshold, as a clone across two areas does.

**An area reaching a layer decides what the collective adopts for that layer**:

- which cMIP fills each of that layer's tasks. So the holders of the area reaching Money adopt a new payment cMIP alone; the rails beneath it are named by the collective's payee pointer, an act of that area, or its vault;
- for the area reaching Development, which extensions the collective adopts.

**Specifications the collective has not adopted.** In a collective whose agreement in force has any area, an act whose `spec` is a specification that agreement names neither for a task (field 2) nor as an extension (field 15) counts for nothing. The one exception: an area reaches it by a kind naming its type, `[1, spec, type]`; then it counts as that area decides.

To count, the specification must first be adopted, under the powers its adoption needs (rule 64):

- for an operational task, the area reaching that task's layer;
- as an extension, the area reaching Development with the area reaching each layer it declares.

An act of a MIR is never such an act.

*Otherwise an unlisted specification would be a way around every area: whoever holds the collective's key could act under a specification nobody adopted, around the area's holders. A rail is not adopted this way: it is named by the collective's own pointer or vault (Money rule 4). With adoption in the area's own hands, waiting costs little.*

**The judicial tasks.** The tasks of the judicial tier (9 condition evaluation, 10 time reference, 11 anchoring, 14 absence proof) stay judicial, outside the area reaching Agreements. Neither the holders of the area reaching Agreements nor any other area changes them. They change only with every member's signature (rule 67). That includes a version that also changes the constitution, which needs the constitutional change rule too (rule 64.1).

The area reaching Agreements adopts the cMIPs for Agreements' other tasks (8 split, 12 grant limits, 13 work claims).

**A specification adopted for a judicial task is adopted for no other task, and is no extension** ("Areas"). So a judge is never joined to an area.

*Each judicial task can decide who wins a dispute, and one area's holder picking a friendlier judge after the fact is worse than a majority doing it.*

**Extensions.** An extension acting on another layer needs the area reaching that layer too, in the same clone, all at once. Its specification declares the layers it acts on (Development, specification field 10), so a client knows whose approval to ask (rule 63).

**Dropping an extension needs the same approvals as adopting it**: the area reaching Development and the area reaching each layer it declares.

*Modules and cMIPs are governed on the layer they sit in.*

The declaration is not trusted for enforcement. In a collective, a verifier applies an extension's rules only to acts of Development and of the layers whose power its adoption met. So an extension that under-declares binds nothing beyond the areas whose holders approved it.

Nothing an extension does relaxes the core (Development rule 20): the MIRs' own acts stay in their own layers, whatever an extension adds around them.

#### Rule 61. Every constitutional voice is covered

Every party whose signature a collective's constitutional change rule is counted among MUST be covered by the agreement's abandonment clause.

Those parties are: every party under the default or a threshold; the named parties under a rule naming parties.

A party is covered when the clause allows outcome 0, removing a voice, and its authority can declare that party absent. So a named authority does not cover itself.

Coverage is judged under the clause in force, one version for everyone (rule 67).

**Under a rule by share** (`[3, stake, share]`), those parties are every party holding part of that stake, so that every member whose share alone can block the rule is covered; under "all of", the parties of each rule in it.

**Under a constitutional change rule counted among a stake's holders**, holders who are no parties are counted too. A holder has no voice for the clause to remove; its stake moves by a declaration naming it (rule 85). **Such a collective's abandonment clause MUST allow moving a vanished holder's stake, outcome 1 or 2, and its authority must be able to declare each holder of that stake who is no party absent; otherwise the terms are invalid, as for a party.** *Better done upstream: the promise that a lost member never freezes the constitution, carried to owners.*

A founding agreement or clone of a collective that leaves such a party uncovered is invalid, as a key grammar without a way to rotate is.

*The constitution is the tier that can repair every other (reassign an area, revoke a grant, refit the key grammar), so it alone must never freeze; it freezes only through the loss of someone whose signature it needs. A member without constitutional power needs no clause: their loss is repaired from above. This is the promise that a dead member never freezes the collective, carried from the keys (rule 59) to the rules.*

### 11. Tiers and clones

*Nobody loses their say by default. Each part of a collective's agreement is changed by the power that owns it. Which power that is follows mechanically from the fields a clone changes, so every verifier reaches the same answer and the mark (rule 46) can be checked.*

**Tiers and areas apply to collectives only** (scope): to a founding agreement carrying a key grammar, and its clones. A deal has none: every clone of it needs every party (rule 47).

*Deals stay as simple as possible while remaining rich; collectives are for when things get complicated. Two people who end up doing many deals together can found a collective.*

#### Rule 62. Every field belongs to one tier

Every field belongs to one tier.

| Field | Content | Tier |
| --- | --- | --- |
| 0 | Parties | Constitutional (membership) |
| 1 | The words: the constitution's and general words | Constitutional |
| 2 | cMIPs, per task | Tasks 9 (condition evaluation), 10 (time reference), 11 (anchoring) and 14 (absence proof; its cMIP named in the abandonment clause's key 3): judicial, outside the area reaching Agreements. Every other task: operational, in the area reaching the task's layer where an area holds it, and of each other layer the same specification serves. A specification named for a judicial task, or in field 6, is named for no other task and as no extension |
| 3 | Keepers | Judicial |
| 4 | Signing rule, or a clone's mark | None: each version states its own |
| 5 | Clone rule | Constitutional |
| 6 | Time reference | Judicial |
| 7 | Stakes | Operational |
| 8 | Split plan | Operational |
| 9 | Abandonment clause | Judicial |
| 10 | *Withdrawn*: once the concurrency rule; rule 90's default stands; never reused | — |
| 11 | Parent | None: every clone names its own |
| 12 | Key grammar | Constitutional |
| 13 | Arbitrators or verifiers | Judicial |
| 14 | The split service's grant: the designated pointer, the first of the split cMIP's addresses | Judicial (the stated exception: operational in kind, able to move money) |
| 15 | Extensions | Operational, per extension, added or dropped: in the area reaching Development, and in the area reaching each layer the extension declares in its specification (Development field 10) |
| 16 | Succession plans | Judicial |
| 17 | Refund terms | Operational |
| 18 | Constitutional change rule | Constitutional |
| 19 | Areas | Constitutional |
| 20 | Area words, per area | Operational, in the area whose id it carries |
| 21 | Chain of judgment | Judicial |
| 22 | Departed members entry | Constitutional; it records only who is departed, the stake being in field 7 |
| 23 | Forked from | None: founding terms only |
| 24 | Release rule | Operational, in no area: the clone rule, and every owner's signature |
| 25 | *Withdrawn*: once the relays; never reused | — |
| 26 | Settles: the tips a deal's settling version drops | None: a deal's clone only; deals have no tiers |
| 27 | The judge of forks | None: a deal only; deals have no tiers |
| 28 | Fees | Operational: an area may reach it apart from field 8 (`field-ref` `[0, 28]`); otherwise the clone rule |
| 29 | The split service's backups, and what counts as a failed attempt | Judicial, as field 14: able to move money |
| 30 | Holders' rules, per stake | Operational, in no area, as field 24: the clone rule, and every holder of the stake whose entry changes; changed by nobody's majority (rules 45, 94) |
| 31 | Transfers counted | None: a deal's clone states its own; deals have no tiers |
| 32 | Issues under | None: each issuance names its own offer; its change of field 7 is an owners' matter (rules 95, 96) |

An area's field references (area key 4) name only operational fields and tasks: fields 7, 8, 17 and 28, and tasks of field 2 other than 9, 10, 11 and 14 (fields 10 and 25 are withdrawn).


*Field 4 belongs to no tier: in a clone it is a mark of fact, written fresh each time; founding terms are signed by everyone, and the tiers govern only later changes. Who signs what for the collective is its areas, which are constitutional. Each area carries its own words, changed by its holders with it, so the words never go stale and no area's holder rewrites the constitution's.*

#### Rule 63. What a clone changes

*In short: a clone's changes are found by comparing it with its parent, field by field; each change belongs to its field's tier and, when operational, to an area or to none.*

A clone changes a field when the field's deterministic encoding in the clone differs from the parent's, or the field is present in one and absent in the other.

Fields 2, 15 and 20 are compared entry by entry: by task number, by extension and by area id. Each differing entry (added, removed or replaced) is one change.

Fields 4 and 11 are never compared, nor field 23: a clone never carries it ("Terms"). Nor are fields 31 and 32, which each version states for itself; field 30 is compared entry by entry, by stake, each differing entry one change.

**A change of field 16 that only takes, for each party the clone takes out of the parties, the seat part of its succession plan (keys 2 and 3), is no change of its own.** This holds where the plan stays for its stake part, or goes where it has none, and every other plan stays as it was. It goes with the removal, as the party's areas do.

Each change is in the tier of its field or entry (rule 62). An operational change lies:
- for an entry of field 2: in the parent's area reaching that task's layer as a whole, the area reaching that layer; where no area reaches that layer, in the parent's area whose field references name that task, `[1, task]`; where none does, in no area, under the clone rule. Where the specification the clone names for that task is also named in the clone for a task of another layer, it lies in the area reaching that layer too. A clone naming one specification for a judicial task and for another task is invalid, whatever its mark;
- for an entry of field 15: in the parent's area reaching Development, and in the area reaching each layer that the extension added or dropped declares in its specification (Development, field 10). Dropping an extension needs the same powers as adopting it. A clone naming as an extension a specification it names as a judge is invalid, whatever its mark;
- for an entry of field 20: in the area whose id it carries;
- otherwise: in the parent's area whose field references name that field.

The part of a change lying in a layer that no area of the parent reaches, and any other operational change, lies in no area.

#### Rule 64. Which powers a clone needs

*In short: the powers a clone needs follow from its changes alone.*

Which powers a clone needs, from its changes alone:

1. If any change is constitutional: the constitutional change rule, `[0]`. Where some change is also judicial, the judicial tier's rule, `[4]`, is needed too: the version stays a draft until both are met, its mark naming both. *Without this, two members of three could change a judge by adding a one-word change to the constitution. The constitutional change rule still changes every other tier alone; a judge needs every member as well.*
2. Otherwise:
   - the power of each area in which some change lies, `[2, area]`, one for each such area;
   - the clone rule, `[1]`, if some operational change, or part of one, lies in no area, or if the clone changes nothing;
   - and the judicial tier's rule, `[4]`, if some change is judicial (rule 67).

   So a clone that changes an area and something outside every area needs both, its mark naming both.
3. A clone that executes a party's succession plan may instead name that plan, `[3, party]`, alone, where rule 83 allows a seat to pass automatically. It may do so provided it changes nothing but what the plan gives:
   - the party out of the parties (field 0), its seat successors in;
   - the party out of the holders of every area it held, nothing else in the areas changed;
   - **the seat successors in the party's place in every holding of the key grammar (field 12), every threshold unchanged**;
   - the executed plan dropped from field 16, every other plan as it was;
   - where the party held the chain key alone (`[0, party]`), a succession plan for the seat successor in the executed plan's place, naming at least one seat successor of theirs, as rule 59 requires. The seat successor signs the clone as the plan's signer, so that plan is signed by them in the same clone.

   *A key grammar's holdings name only parties, and in most collectives every member holds a share of the chain key, so a seat could never pass without the keys; a plan left in field 16 for someone no longer a party would make the clone invalid. The thresholds stay as the members chose them; the rotation declaring the clone re-deals the keys (rule 83). Where this leaves a case undecided, a verifier answers unknown rather than guessing.*

4. **The owners' matters**: where a change is an owners' matter of a stake (rule 95) whose holders' rule is by share (field 30), and that rule is how its holders consent, the clone needs that stake's holders' rule, `[5, stake]`, beside the members' powers above, its mark naming it with the holders whose signatures meet it, so that a false claim sinks the clone (rule 46). Where the stake's holders' rule is every holder, its holders sign as rule 45 asks, outside the mark. *No declared tier is needed: the owners' matters are counted among a stake's holders, not by moving a field among the members' powers.*

A clone needing several powers comes into force only when each is met, all at once or not at all.

*The clone rule does not reach into an area, and one area's holders do not reach into another: an area's power is exclusive, held by its holders or by the narrower rule that governs it. Its holders, and the rule they act by, change only by a constitutional change.*

#### Rule 65. Counting a power

*In short: each power is counted among the parties whose voice remains; a party that has left still counts for what it signed before it left; and when fewer voices remain than a number asks for, all of them together meet it.*

**Who each power is counted among:**
- the constitutional change rule: among the parent's parties, under the parent's field 18, or every party when the parent has none;
- the judicial tier's rule: among the parent's parties, every one of them;
- the clone rule: among the parent's parties, under its field 5;
- an area's power: among that area's holders in the parent, at least its threshold of them;
- a stake's holders' rule, `[5, stake]`: among that stake's holders at the line, by share (rule 94); and a constitutional change rule counted among a stake's holders, likewise.

In each, only parties whose voice remains are counted, a stake's holders aside, who are counted by what they hold. This is judged on the collective's own sequence at the act, or, for a clone, at the record or rotation that puts it in force ("Made before, made after"). Left out are:
- a party whose resignation (rule 70) or declaration with outcome 0 (rule 89) the collective registered on a line before it;
- for one area, a holder whose stepping down from it (rule 71) the collective registered on such a line.

They stay left out under every parent descending from the agreement in which they lost that voice.

**A party is named again only by a constitutional clone** made after the version in which it lost that voice, naming it as a party or as that area's holder, which it signed by a signature act the collective did not place before the line registering its departure. It counts again under that clone and its descendants. *An ordinary clone that only copies the list of parties or of an area's holders restores nobody: otherwise a holder who stepped down would count again by signing a change of the area's words.*

**A party that lost its voice still counts, for a clone or an act, through a valid signature act on it placed before the collective's line registering its departure** ("Made before, made after"):
- for that clone or act alone, it counts as a voice that remains, so the rule is met or not as written, its number counted with that voice among the others, never lowered by the departure;
- it counts so even where the record or rotation putting the clone in force does not name that signature among those completing it, and is then a voice that did not sign there;
- a declaration never undoes a clone that a record or rotation put in force before the line registering it, whatever version of the clause or of the agreement it names.

*Her position at the time of signing is valid: she was still acting, and whether the clone is approved now depends on the collective's rules. Under three of three, a member who signed a clone and then resigned still leaves it needing both others: three voices count for it, and three signatures are needed; a rotation naming only the two others' signatures does not meet it. Under every party, it needs everyone who has not left, the early signature standing among them. "Before she left" reads "placed before the collective's line": the record came first, or the collective acknowledged her signature as it arrived.*

**When fewer voices remain than a rule's number asks for, all the remaining voices together meet it; where enough remain, the number stands as written.** This holds for every number an agreement states, and for the judicial tier's "every party":
- the constitutional change rule;
- the clone rule;
- an area's threshold, one a clone left above the area's holders included;
- an abandonment authority that is a threshold of the other parties (rule 85).

When no voice remains, nothing meets the rule. An area left without a holder, or listing none, stands frozen, as in rule 71, until a constitutional clone gives it a holder.

**Counting by share.** A rule may count shares of a stake instead of heads: "those counted, holding together at least this share of this stake". One voice per party stays the default: a rule written without a share counts one voice per party.

- **Among the parties** (a change rule by share, `[3, stake, share]`): each party whose voice remains counts by what it holds of that stake, read from the parent's field 7 and the transfers the collective's line placed before the line that puts the clone in force (rule 94). The share asked for is of what **the parties whose voice remains hold together** at that line, never of the whole stake, so departed holders' shares never keep the members from meeting it. **The fewer-voices clause, restated in shares:** where the voices that remain hold together less than the share the rule asks for, all of them together meet it; where none holds anything of the stake, all the voices that remain together meet it.
- **Among a stake's holders** (the holders' rule, rule 94; a constitutional change rule counted among holders, `[5, stake, share]`): every holder of the stake at that line counts by its share, members and others, and the share asked for is of **the whole stake**: no holder loses its say there by leaving the membership. A holder that vanished is moved by the declaration of absence (rule 85), not by this rule.
- **"All of these rules"** (`[4, rules]`): met when each rule in it is met, each counted as it would be alone, the fewer-voices clause included.

A holder's share counts once, whoever signs: a seller and its buyer never both count, the line moving the share once. An area's threshold stays a number of its holders (area key 2), the forms by share being forms of a change rule. **Areas keep counting heads in MOR 1**: a cMIP may put a by-share decision behind an area's single holder. *An area is not ownership; it splits operations.*

*Counting by share is optional. A member with a majority of the stake decides alone under a rule by share, and that is the point; weights are read from the stakes, never written as free numbers.*

*A number can outlast the voices it counts: "three of three" would still ask for three after one voice is removed, and the abandonment clause (rule 61) would unfreeze nothing. Read this way, a number never freezes a rule, it matches what "every party" already does, and it keeps the founders' number wherever enough voices remain. A lone survivor still holds the keys and the constitution only under rules 59 and 61: the key grammar's thresholds are cryptographic, and this rule never lowers them.*

#### Rule 66. Protections still hold

Protections still hold. Whatever power brings a clone into force, it cannot:
- reduce a stake without its holder's signature (rule 48), a departed holder's included (rule 68), nor lower what it pays a departed holder without that holder's signature (rule 48), or the stake's holders' rule where that holder accepted one (rule 94);
- nor change the judicial tier without every member, a version changing the constitution included (rules 64.1, 67).

*Stated cost: by default a member who is present but unwanted cannot be removed; the others can leave, each alone (rule 70), or use the abandonment clause where it applies. A group that wants expulsion writes a constitutional change rule below everyone at founding, where every founder signs it; even then, removal takes the voice, never the stake. A member who dies or vanishes never freezes the constitution: the abandonment clause every collective carries can remove their voice (rule 61).*

#### Rule 67. Protected clauses: the judicial tier

*In short: the clauses that decide disputes change only with every member, and there is one version of them for everyone.*

Collectives only. **The judicial tier changes only with the signature of every member whose voice remains, and there is one version of it for everyone.** The judicial tier is:
- the abandonment clause;
- the succession plans;
- the keepers;
- the split service and its backups (fields 14 and 29);
- the time reference;
- the arbitrators or verifiers (field 13);
- the condition, time reference and anchoring cMIPs (field 2, tasks 9, 10 and 11), and the absence-proof cMIP (task 14, named in the abandonment clause's key 3);
- the chain of judgment (field 21).

*The concurrency rule (field 10) is withdrawn: rule 90's default stands. Task 14 is judicial in its own entry ("Tasks").*

A clone changing any of them names the judicial tier's rule, `[4]`, in its mark, with every party of the parent whose voice remains, counted as rule 65 says. A version that also changes the constitution names the constitutional change rule beside it, and needs both (rule 64.1).

A clause naming only the member a version removes goes with the removal. This is only the seat part of their succession plan, which needs no judicial power (rule 63). The plan for their stake stays with the stake, as a departed holder's plan (field 16 may carry it; it names stake successors only). No other judicial clause changes that way.

A departed holder's stake plan changes only with that holder's signature too.

Every member, and every departed holder (rule 68), is judged by the version in force.

*It is the joint signature that matters. If a member who did not sign a change kept the version they signed, two members could hold different judges, and where one joined after a change the other never signed, no judge would be signed by both: nothing would say who decides between them. With one version for everyone, every dispute has one judge both signed. Arbitrators, the condition cMIP and the anchoring cMIP can each decide who wins a dispute, so a majority still cannot pick a friendlier judge after the fact.*

*Stated cost: one member can hold a change back, leverage being part of life; a judge that dies cannot trap the collective, the chain of judgment taking over (rule 35); and a holdout makes a standoff, which is good: communication becomes the only channel to solve it, with a collective fork as an option to settle it (rule 91).*

In a deal no clone completes without everyone, so the question never arises.

#### Rule 68. Departed holders

*In short: a member who leaves keeps their stake but loses all control; their stake is treated exactly like everyone else's.*

Collectives only. A collective's departed members entry (terms field 22) records only who left, without voice. Their stake is in the collective's stakes (field 7), which decide what every holder is paid.

**A departed holder has no control and no veto:** it is counted in no change rule and no area, and the collective's current rules and judges apply to it. Its one signature is on what lowers its own stake or its pay (rule 48), never on anything else. **It is paid, and consulted on the owners' matters** (rules 94, 95): what its stake is paid, an issuance in it, a sale or release of the collective's works, a fork's successors' plans, by the stake's holders' rule where it accepted one, else by its own signature; and it sees the collective's Money acts (rule 57). A constitution may also count it among the holders choosing members (rule 69).

Its stake never shrinks without its signature (rule 48). A clone that lowers its share in field 7, or drops it, is a draft until that holder signs it (rule 45), as for every holder. **Nor is what its stake is paid lowered without it:** a clone that lowers its stake's part of the split plan, or adds or raises a fee its stake bears, is a draft until it signs (rule 48). *This protects what it is paid; it gives it no say in anything else.*

The stake is a share of all the collective's income, not only of what was made while it was a member.

Every term applies equally to every stake, member or departed:
- a term may not treat a departed holder's stake worse than a member's of the same size;
- a fee raised for the departed is raised for the members;
- a fee paid to a service the members own is public. *Client conformance*: a verifier cannot see who owns a service, so the collective's client MUST disclose it to the members.

The stakes decide every holder's money (field 7), and every payout matches its stake exactly, every fee alike for every stake ("Split plan and split", rule 27). Equal treatment is what following the stakes with uniform fees produces, not a separate test, and any deviation, either way, breaks the plan. *A member who wants to give income to another transfers part of a stake.*

Every split is delivered to every holder it pays and names each fee and who received it (rule 20), so a fee to a service the members own is seen by everyone it is taken from.

Unpaid shares stay open debts (Money rule 5) until the holder can receive, for instance after setting up on a new rail the collective chose.

*A person who keeps a stake in a collective accepts having no control over its decisions; one who thinks the collective will not survive has the incentive to sell. The stake defines a share of income. A rail swap is the collective's decision: to get paid, past members accept it and set up accordingly. Equal treatment makes any creative accounting legible, and public.*

*The core checks each split; equal treatment follows from the stakes and uniform fees.*

### 12. Members joining and leaving

#### Rule 69. Changing members

*In short: changing who is in a collective takes a rotation of its keys plus a clone of its agreement that changes the constitutional tier. A rotation that gets this wrong breaks the collective until a rollback brings it back.*

Changing members means a rotation to new keys plus a clone of the founding agreement. The rotation declares the complete clone as the agreement the collective lives under. It names the signature acts that complete the clone, and places each at its own position. The clone counts as complete there only with those ("Collectives in the identity chain", "Made before, made after").

Membership is constitutional, so the clone needs the constitutional change rule (rule 64): by default, every party whose voice remains at the rotation. A member whose resignation (rule 70) or declaration with outcome 0 (rule 89) the collective registered on a line before the rotation is no longer counted.

The rotation is a line by its kept tips: a change of members, or of an area's holders, that it declares takes effect for the collective's acts there.

A clone that takes a party out of the parties takes it out of every area's holders too. An area it leaves with no holder is written with an empty list of holders, and stands frozen until a constitutional clone gives it a holder (rule 71).

A constitutional clone that removes a party who did not sign it, where the constitutional change rule allows that, takes that party's voice only. The party keeps every stake it holds (rule 48), as a departed holder, under the collective's current rules and judges (rule 68).

Any other change to the constitutional tier (the change rules, the key grammar, the areas) is likewise declared by a rotation before it applies to the collective's acts.

Each agreement the chain declares after the founding one MUST:

- be a clone of the agreement in force at the rotation (the one declared before it, or recorded since, rule 72), its parent (field 11) naming it;
- be complete with the signature acts the declaration names;
- change the constitutional tier.

Otherwise the collective's acts that need member signatures count for nothing: **the collective is broken** from that rotation on, until a rollback brings it back (rule 93). So it is where a rotation removes the collective's Agreements declaration or carries one that cannot be read.

A rollback is the one exception to this rule: it is a clone of the agreement in force just before the broken act, and need not change the constitutional tier (rule 93).

*A rotation declares only a constitutional clone: a clone changing only the judicial or operational tier is written by a record. And it declares only a direct clone of the agreement in force: every other clone is recorded at once (rule 72), so there is never a clone in between, and one that nothing recorded would have no line at which its completeness was judged.*

A rotation declaring nothing new carries the agreement in force forward ("Collectives in the identity chain").

*Clones that change only the judicial or operational tier need no rotation: they change neither who holds the collective's keys nor who decides, so there is no old key to fence off. They are written on the collective's record by a record act instead (rule 72). The collective's key resembles a chain key more than a signing key in that regard.*

A departing member hands over nothing: the remaining members rotate to keys the departing member never held, and re-split the chain key among the members who stay or join.

**Owners may choose who runs the collective, as a choice at founding.** A collective may, in its constitution, count the power over membership among a stake's holders by share: its constitutional change rule (field 18) written as a rule among the holders of a stake, members or not (`[5, stake, share]`), alone or with the members' own rule ("all of"). The default stays members choosing members. The holders sign the clone as its mark names them (rules 45, 46), counted as rule 65 says.

**How it is kept: as every constitution is, by the broken collective** (rule 93). A rotation that moves the keys without the signatures this constitution requires breaks the collective, its acts counting for nothing until a rollback, visibly: the operators cannot rotate around the owners' rule.

**It binds only through the keys**, a stated cost. Rule 93 punishes a rotation made; it cannot compel a rotation refused. A clone removing operator-members needs a rotation (above), which needs the chain key they hold. Where the key holders refuse it, the owners' vote stands complete and unexecuted, visible to every reader of the chain; the collective is not broken, and runs on under the old members. The owners keep the owners' matters (they can refuse every fee, issuance and sale, rule 95) and the exit, not replacement.

**A constitution that counts membership among a stake's holders SHOULD also put a share of the chain key, or the recovery path's custodian** (rule 59, `recovery = [0, custodian, grant]`), **under the owners' control**, so that the holders' rule can produce the rotation. *Client conformance*: the founding client warns, before the founders sign, where it does not.

*As long as it is a choice: it depends on balance, some people protecting their work, some their investment.* *Stated cost: a majority of capital can remove those doing the work, where the founders chose it.*

#### Rule 70. A member can always leave alone

A member can always leave alone. A party to a collective's agreement gives up its voice by a resignation act (type 16), without anyone else's signature.

**The resignation takes effect for the collective at the collective's line registering it** (record field 3). The collective's client draws that line in its next act. **Once registered, it stays taken for the collective, whatever its signer's later rotation disowns** ("Made before, made after", 4): the member comes back only by a constitutional clone naming them.

From that line on, the party's signature acts count toward no change rule and no area of that agreement, or of any agreement descending from it. A signature of the party placed before the line stays valid, and still counts toward what it signed. For that clone or act alone, its signer still counts among the voices that remain, so the departure never lowers the number the rule asks for ("Made before, made after", rule 65).

The party keeps its stake, and stays bound by what it signed; it is then a departed holder (rule 68).

The remaining members then rotate the collective's keys (rule 69) and clone it without the departed party, which takes the party out of every area it held too. They record the departed party, with its stake in the collective itself (field 7), in the departed members entry (field 22).

Meanwhile, from the line on, each rule, and each area the party held, is counted among the voices that remain (rule 65). An area left with no holder stands frozen from that line until a constitutional clone gives it a holder, as in rule 71. Every grant in it ends at that line, to be reinstated, where wanted, by cloning it at the refit.

A resignation counts like a loss: where the key grammar's recovery path is an escrowed share released by the abandonment authority, the authority may release it on a resignation as on a declaration of absence.

*Leaving needs no permission, and lets nobody remove anybody else; the collective that delays drawing the line only delays itself. Absence remains the abandonment clause's business, under the clause the member signed.*

**Leaving a broken collective.** A member can leave a broken collective too (rule 93). During the broken stretch, a resignation, or a stepping down (rule 71):

- names **the agreement in force just before the broken act**;
- takes effect at the collective's next valid line: normally the rollback, which registers it ("Collectives in the identity chain"), or else the first record after the rollback that registers it (record field 3).

A record drawn in the broken stretch is no line and registers nothing (rule 93). A resignation naming the clone the broken act declared names an agreement never in force, and no line can register it.

**Stepping down during the broken stretch is treated as resigning**: a client allows it, and the rollback registers it. *A rollback whose clone names that member again as a holder of the area, and which they sign, gives the area back to them, as any constitutional clone they sign naming them does (rule 71), so a client leaves them out of it.*

*A resignation cannot make the constitutional change rule impossible: a number is counted among the voices that remain, and all of them meet it where fewer remain than it names (rule 65). Where a departure takes away a share of the collective's key that the rollback's rotation needs, the key grammar's own way through applies: a resignation counts like a loss (above). Neither case is new to a broken collective.*

**The last voice.** *Client conformance:* where the member resigning is **the last voice that remains** in the collective, the client MUST say so in plain words before the resignation is signed. The member is the last voice that remains where no other party of the agreement in force (during a broken stretch, of the agreement in force just before the broken act) keeps a voice there as rule 65 counts it. The client says that the collective's works will be frozen as they stand, and nobody will be able to change, release or move them. Leaving stays the member's alone: the client says it, and never blocks it. *It is the sign at the entry of a danger zone.*

**The last constitutional voice.** Where the constitutional change rule names some parties only, and the member resigning is the last of them whose voice remains while other parties keep voices in their areas, the client MUST likewise say so before signing, in these words:

> **"You are about to break the collective's constitutional layer. Once you leave, nobody will be able to change its rules again. The other members keep acting in their areas."**

Its constitution freezes as it stands, and nobody can add or remove a member, redraw an area or change a rule of the constitution again (rule 65). It is not a broken collective (rule 93), and has no rollback. *This may be deliberate, in rare cases: a collective may choose to freeze its own rules for good, and the warning makes sure it is chosen.*

**Counting the voices that remain for these warnings, the client reads too the resignations the other parties have published that no line has registered yet.** Agreements count a resignation only once a line registers it (rule 65), and during a broken stretch nothing registers, so voices leaving from separate devices would otherwise each be told the others remain.

*A client names in the resignation the agreement Agreements find in force. Where it cannot read Agreements (its relays and homes away), it says that the resignation names its own copy, which may not be the one in force, and that no last-voice warning can be given.*

*Stated cost: when the last voice has left, the collective's works freeze as they stand.* Its works stay published and claimed, and every former member keeps its stake as a departed holder (rule 68). Nothing about them can be decided any more, a release to the public domain included, since a collective owner releases only by its own rules and no voice remains to meet them (rule 65). A broken collective can no longer be rolled back. No rule prevents it: *someone with stakes in works worth keeping does not resign.*

Money under offers already published keeps flowing through a split service the founding terms name by a grant tied to no area while that grant stands, as through any grant.

#### Rule 71. Leaving an area

Leaving an area: giving up an area changes the constitution at that level. It is a constitutional clone (rule 64), declared by a rotation (rule 69). It comes about in one of three ways:

1. the holder leaving nominates a replacement, and the constitutional change rule approves it or not;
2. the constitutional change rule proposes a replacement, and the holder leaving approves it;
3. the constitutional change rule redraws that area's rules entirely.

The replacement signs the clone too (rule 45). Until one of them completes, the holder keeps the area.

*Mechanically the first two are one clone, signed by the leaving holder and meeting the constitutional change rule, started from either side; the third needs no signature of the holder, since an area's holders and rules change by a constitutional change.*

**Stepping down at once.** A holder may instead give up an area at once, alone, by a resignation naming that area (type 16, field 1). *It says: I am done, and will do nothing more in this area.*

**The resignation names the drafts its signer had signed and leaves behind** (field 2). A version it names never gives the signer the area back, whichever line puts it in force. A draft it fails to name does: the signer's stated cost. *Client conformance:* the holder's client names them, as for a resignation.

The stepping down takes effect for the collective at the collective's line registering it (record field 3). From that line on, the holder's signature acts count toward nothing in that area, in that agreement or any descending from it. A signature of the holder placed before the line stays valid, and counts as rule 65 says. The holder keeps the rest of its voice, and its stake. The collective then makes the constitutional change for that area by one of the three ways.

**Meanwhile, where other holders of the area remain, they carry on**, counted as rule 65 says. The area's threshold stands where enough of them remain, and all of them together meet it where fewer remain than it asks for. Grants issued within the area keep backing acts. A departure that leaves an area held leaves its grants in force; only a revocation ends them.

*Rarely, no holder of the area remains.*

**An area with no holder remaining is frozen** until that change. Its acts made after the line that froze it count for nothing, and every grant issued within it ends for good. The line that freezes the area is:

- the collective's line registering the departure of its last holder whose voice there remained (a resignation, a stepping down, or a declaration removing that holder's voice, rule 89); or
- the rotation declaring a clone that leaves the area with no holder.

**A departure that empties an area ends every grant in it: a decision ending powers.**

- An act signed with such a grant's key that the line's history holds was done within the grant's powers, and binds.
- One racing it (neither citing the other) is void, the ending winning (the tie rule).
- Every act after it is void.

No suspension, and no undetermined act. *An empty area is an emergency: end it. The copy still exists, and cloning it reinstates what is needed.*

A void act binds the collective only where the collective itself, by an act of its own key that counts, acknowledges or pays on it, adopting it (rule 76; the import act, type 11, is retired).

*Client conformance:* a member's client shows an empty area as an emergency.

The change ends the freeze once a constitutional clone is declared that changes that area's entry of field 19, the entry with its id, so that it has a holder whose voice remains.

**At the refit, the new holders may reinstate any ended grant by cloning it.** A reinstatement is a grant (type 9):

- whose field 7 names the ended grant, and whose fields 0 to 6 are that grant's;
- with a grant key of its own (field 9);
- signed by the collective and completed by signature acts of the refitted area's holders, meeting the area's own rule, its threshold as for any grant within it (rule 74);
- accepted by the grantee.

It is a new decision, judged at its own place like any act the area reaches (rule 60). It counts only when made after the line at which the new holders took the area, since before it the area has no holder to complete it. One reinstatement for each grant, never one for several, and nothing is reinstated automatically.

A signature act naming the ended grant itself reinstates nothing: it is placed at that grant, before the freeze, where the new holders held nothing.

A reinstated grant backs acts as a grant within the refitted area, and only within that area's reach (rule 80), from its own place on. **It takes nothing on**: the acts under the ended grant stay as the ending left them.

*With grant keys, the grantee's acts are on the collective's own chain, so the line that emptied the area places them as it places every act, and nothing is left to decide later.*

*Stepping down thus works as resigning does (rule 70): neither can be played against the other, and one holder cannot stop the co-holders' work at will, nor keep an area frozen by withholding the signature its refit needs. A grant is the area's, not one holder's: a grant made within the area before the stepping down, even one to the holder who stepped down, keeps backing acts while the area runs, until its remaining holders revoke it (rule 74).*

*If a departure freezes a layer, the collective organizes quickly to fix it. Payments to the collective's payee pointers still arrive on their rails; only the collective's own acts in that area wait.*

#### Rule 72. Every complete clone is written on the record at once

**Every complete clone is written on the collective's record at once.** A clone that changes the constitutional tier is declared by a rotation (rule 69). Every other complete clone of a collective's agreement is written, at once, by a record act (type 17) that the collective signs in its own sequence.

For an act the collective's key signs, the agreement in force is the one the chain declares at the act's binding. For an act a holder's member key signs, it is the one the last bullet below picks. Where a valid record made since that declaration counts for the act, the agreement in force is instead the clone named by the latest such record, the one furthest along the agreement chain. Such a record may be under the same binding, or under an earlier one that a rotation declaring nothing new carried forward. Which records count:

- for a record itself, only the records before it;
- for an action, the records under an earlier binding carried forward, and, under its own, the records its history holds and those before them (rule 58). A record it never cites decides nothing for it, a concurrent one included, save by the tie rule ("Made before, made after", 7);
- for an action signed with a holder's member key, whose binding is no link of the collective's chain: starting from the version that named the key, the clones declared by the links of the collective's identity chain its history holds, and the records its history holds and those before them, the latest furthest along the agreement chain.

**Records on concurrent lines are a fork of the agreement chain, settled as rule 90 says.** Where two valid records of different clones of the same parent, neither before the other, both count for an act (a record after both, or an action whose history holds both), it is judged under the parent itself, the earlier version staying in force, until a clone of either branch, recorded after both lines, resolves it, as rule 90 says. *The concurrency rule (field 10) is withdrawn.*

*Where the collective's records were drawn concurrently, which of them its key holders drew first does not decide. Records drawn one after the other are no fork: the second names a parent no longer in force, and puts nothing in force.*

A record of a clone counts only if:

- that clone is complete, judged with the signature acts it names; and
- its parent is the agreement in force for the record itself, or one of the clones a fork leaves for it.

So records follow the agreement chain one clone at a time. A rotation's declaration of a new agreement supersedes every record before it, and a rotation declaring nothing new supersedes none.

*MOR has no clock, so an operational clone that nothing writes on the collective's record could not tell which of the collective's acts it governs: a payment cMIP the area reaching Money adopted would not govern the collective's acts until the next rotation, and acts of a cMIP adopted for a task would fall in no area, counting on the collective's own signature, without the area's holders. The record fixes the moment in the collective's own sequence, with the signing key; only clones that change who decides need the chain key's ceremony.*

*Until the record exists, the collective's acts are judged under the agreement before the clone: a delay, visible, which the key grammar's holders could already cause by withholding the collective's signature from any act.*

### 13. Grants

#### Rule 73. Grants

Grants delegate authority with limits: signing new deals, managing specific existing ones, or acting for an identity or collective (posting, publishing, spending up to a limit).

A grant is authority to act in the grantor's name without a stake, the employer-and-employee kind. It confers no voice in the agreement.

**A collective's grant hands its grantee a grant key.** An agent acting for a collective works under a grant, holding a key of the collective scoped to it, never the collective's own keys. What it signs with the grant key is the collective's own act, within the grant's reach, on the grantee's strand of the collective's actions chain, citing the grant (grant field 9, rule 58).

The grant is a decision naming the grantee's own identity, which signs to accept it.

**A grant whose grantee is its own grantor is valid, its acceptance the grantor's own signature act naming it (type 1).** *A grant to oneself: a person grants itself a key that an application makes and holds, so the application signs within the grant's reach and limits, and every reader sees the act signed under that grant, not with the person's own key. It ends with one revocation, as any grant.*

**A holder's signature of a founding or clone may name a key; it is a grant key within the areas the agreement in force for each of its acts gives the holder; the founding or clone is its grant, the signature its naming and acceptance; it counts from the moment the founding or clone naming it is in force (rule 72), until a later version names another key for that holder or the holder leaves.** *A member's own key: their app makes it as they sign, and keeps its private half sealed to the member themself, so any of their apps holding their identity can act with it.*

- An act signed with it names that signature act as its `binding` (a scoped key, judged here), its signer the collective. It is the collective's own act, on the key's strand of the collective's actions chain, citing the decisions it acts under as every action does (rule 58).
- It counts as the holder's signature within the areas the agreement in force for the act gives that holder (rule 60); no area reaching it, it counts for nothing. It never signs a decision: no record, grant, revocation, nor any act of the collective's identity chain. *The collective's own key is needed only for those.*
- It counts only under the version it was named on, once complete, and the versions descending from it, and only while it is its holder's latest: a later version of that line on which the holder's signature names a key replaces it. *So each member change, which every member signs, names fresh keys.*
- Its key is the holder's alone: a key named by two signatures of different signers is no member's.
- The holder's own change of keys leaves it as it is. A rotation of the holder that disowns the very signature that named it is judged as any member's signature the collective's line placed: it stays valid for the collective ("Made before, made after", 4).

*Stated cost: where a member never signs, the keys others' apps made for that founding are left unused. The key sits encrypted at its holder's homes.*

Grants are revocable at any time, by a revocation: a decision ending powers, which removes the key (type 10).

During a collective's broken stretch its grants are quarantined: a grantee's act of the stretch counts for nothing. The grants work again after the rollback (rule 93), for an act citing the rollback or a later decision.

*Stated cost: a stolen grant key acts within its scope until revoked; the collective's identity gains a scoped kind of key (Identity, "Scoped keys").*

**A grant's dials.** A grant has scope (fields 1, 6), limits by a cMIP (fields 3, 4), revocation with the tie rule, the grantee's acceptance and own key, and never reaches beyond its issuer's power (rule 74). Besides these:

- **Expiry** is one more limit of the grant's limits cMIP, judged against a time reference: an act of the grant key past it is not backed (rule 80). The grant goes quiet rather than ending, and stays revocable.
- **Sub-granting stays excluded**: a grant key signs no grant ("Grant (type 9)").
- **Exclusivity is not in the core**: no verifier proves the absence of another grant. It is a promise written as a deal term, its breach visible on the record. *MOR is not the law; on these matters it keeps the record.*
- **Conditions** (an escrow's "only once X") are a cMIP's.
- **Licences, leases and trusts are not grants**: they are offers, stakes and deals.
- **A proxy is not a grant**: a grant key signs no decision; a holder's vote is handed on only by a proxy (rule 97).

**Every identity grants keys the same way.** A person's grant hands its grantee a key of the person. What the grantee signs with it is the person's own act, citing the grant on the person's chain (the person's identity as chain).

**A person's grant and its revocation are everyday acts, signed with the person's own signing key: adding or removing a grant key needs no rotation and no chain key.**

They are public (scoped and visible: a person has no members to seal them to, and everyone who checks the grantee's acts must read them). A private one backs nothing.

A rotation of the grantor judges its grant like any act signed with the key it replaces (Identity rules 33 to 35). A grant the rotation does not keep is void, and its key with it. So a thief who stole the signing key and granted itself a key loses it at the owner's next rotation.

A person's grant reaches the kinds it names (a MIR's layer, or a type of a specification). Naming none, it reaches every act but a decision.

*Stated cost: as with every act of the signing key, a thief holding it can also revoke the owner's grants until the owner rotates and grants again.*

**Grants to a split service.** In a deal, each payee grants the split service its own grant key, scoped to that deal:

- the grant names "this agreement" by null;
- the deal lists it in field 14, one per payee;
- it counts once the deal exists;
- its key signs only receipts for money coming into the deal, never one whose payer is the service, nor a split's payout ("Grant (type 9)").

The same holds for every grant to a service the deal's chain names to take over, one per payee, and, under the same limit, for a collective's grant to its split service.

#### Rule 74. A grant never reaches beyond its issuer's power

A grant never reaches beyond the power of whoever issued it.

The holders of an area may grant within it. The collective signs a grant naming the area (field 5) and the acts it reaches (field 6), within the area's reach. It counts only with valid signature acts naming it by the area's holders, meeting its threshold as rule 65 counts it, as for any act the area reaches (rule 60). It is judged by the area it names alone, whichever area reaches grants as acts.

**So is a revocation of such a grant**: it counts only with valid signature acts naming it by the holders of the area the grant names, meeting its threshold as rule 65 counts it, whichever area reaches revocations as acts. So is a reinstatement (rule 71).

A collective's grant naming no area is judged as any other act of the collective, and reaches only acts that no area of the agreement in force reaches. **A collective's grant to its split service is judged under the area that issued it**: the area its field 5 names, or, where it names none and an area reaches Money, that area; the receipts its key signs are that area's acts (Money rule 7).

A grant whose field 6 reaches beyond its area is invalid, a reinstatement aside (rule 71).

*So a member holding an area can act alone there, under a grant to their own identity, and can hire others for it; and neither they nor the one who holds the collective's signing key can give anyone more than that area.*

#### Rule 75. Strands

A collective's actions chain has a strand for each of its devices and for each grant key.

A grantee's acts are its strand, each citing its previous act there by `prev` and its grant on the collective's chain.

*A grant key's strand is the collective's own, which every member's chain reading follows.*

#### Rule 76. Revocation

Revoking a grant removes its key. The revocation names the grant, never a place on its strand.

**An act signed with the grant key that the revocation's history holds was done within the grant's powers, and binds.** An act signed with the grant key that the revocation's history does not hold, racing it (neither citing the other) or after it, is void, the ending winning (the tie rule), unless:

- another identity acknowledged it (Envelopes rule 17; by an act of a type the Identity, Money or Agreements MIR defines), the grantee aside: an acknowledgement whose signer is the grantee does not count here;
- or a keeper recorded it before recording the revocation, the record counting as rule 42 says (rule 41);

then it is valid and shown as disputed. A keeper record or acknowledgement counts only alongside the act it names, and only while its own signer's chain keeps it valid (Identity, "Evidence counts only while its signer's chain keeps it"), save rule 42's exception for a keeper's record.

**What such a disputed act confers: nothing, unless adopted.** As an act a rotation disowned (rule 6), an act a revocation's history does not hold, shown disputed, confers nothing on an agreement chain (no stake, no obligation, no consent) and is shown, to everyone; Money treats it as it treats a disputed act after a change of keys (Money rule 7). The grantor may still adopt it (below), and then it binds. *Binding the grantor by an accomplice's acknowledgement would let a thief holding a grant key sign the victim into a debt; shown, not binding, keeps both sides visible, and an honest grantor who meant it adopts it. Stated cost: a payee who relied on a revoked application's promise, a debt to come rather than a payment made, holds a visible dispute, not a debt, unless the grantor adopts it; a payment already made stays made on its rail, and the payee's own receipt stands.*

*This is Identity rule 34's principle, for a revocation as for a rotation's disowning: nothing another identity relied on in a way the protocol can see is silently erased, and no grantor's omission, careless or chosen, erases what a counterparty holds a signature for. An act nobody else acknowledged and no keeper recorded stays void, so a revocation still cuts off a stolen or misused grant cleanly.* *Stated cost: a thief holding a grant key, with an accomplice who acknowledges its act, gets "disputed" rather than "void": visible to everyone, never erased, and bounded by the grant's own limits. The grantee's own acknowledgement is no such rescue: what decides would be signed by the grantee whose powers the revocation ends.*

**A revocation cites every act under the grant its signer holds.** *Client conformance:* the grantor's client cites, in the revocation's history, every act of the grant key it holds, so that each act's holder, holding only its own act, finds it there. *A grant key's acts are chained on its strand, each naming the one before (rule 75), so an honest grantor's client holding the latest reaches the earlier ones; what it can miss is the tail: an act not yet stored at the grantor's places, or one made at the moment of the revocation. Stated cost: a public revocation lists every act's id, so anyone can count the grant's acts, and a long-lived grant makes a long revocation.*

A revocation of a grant within an area is judged by that area alone (rule 74).

**The grantor may adopt a void or disputed act of a grant key as its own.** An act of the grantor's own key that counts (for a collective, an act of the collective's own key that counts) makes it bind when it does any of these:

- acknowledges it;
- cites it (for a collective, its history holding the act);
- pays on it

**There is no fourth way: the import act is retired** (rule 77).

**Once adopted, no ending racing the adoption voids it.** A revocation, or a departure emptying the area, that does not hold the act still ends the grant for every act after it, never for one the collective already took on.

**Another grantee then manages it under its own grant** (scope 1, naming it), with its own grant key.

*A collective that took the money cannot later say the deal never happened. The adoption is the grantor's own act, never the grantee's: no act signed with a grant key adopts anything.*

#### Rule 77. Import

**Retired**, its number never reused: there is no import act. Type 11 is retired, never reused; adoption is by acknowledging, citing or paying on (rule 76). An act of type 11 is of a type this MIR does not define, and is invalid.

There is no handover. A grant key's acts are the grantor's own, on its chain, so there is no branch to move them from. A void one counts only if the grantor adopts it (rule 76), and another grantee then manages it under its own grant.

*Adoption already does the job, visibly; a second mechanism would have no purpose of its own.*

#### Rule 78. Acknowledgements

An acknowledgement of a grant key's act counts as an adoption only when it is an act of the collective's own key that counts (rule 76), wherever it is stored.

**That means done: sealed to every member and on the collective's chain (rule 57). An Identity witness act (type 15), on neither chain, adopts nothing. The collective adopts by an action of its own key citing the act. No act of a grant key adopts anything (rule 76).**

#### Rule 79. Fate of acts

An act of a grant key that a decision ending its grant does not hold does not bind the collective: it is void, or, where the decision is a revocation, void or disputed as rule 76 says; either way it binds only where the collective (for a person's grant, the grantor) adopted it (rule 76).

Decisions ending a grant are: a revocation, a departure emptying its area, a fork, a closing.

**Counterparties: wait until a later act of the collective cites the act before performing.** From then on the act is adopted (rule 76), and no ending racing that citation voids it.

**The promise, restated**: safe once cited, against everything but the collusion of an ending's signers.

*Stated cost:* a fork or closing whose line is drawn early on purpose, before the citing act, leaves both out of its history, and both are void (the tie rule). Without a clock, a line drawn early on purpose cannot be told from one drawn before the later acts existed, and every cure would let something published after a fork reopen it, against a complete fork's finality (rule 91). It needs every signer of the ending to collude against the counterparty: each conforming client refuses to sign it ("Made before, made after", client conformance). And it stays fully legible: the signed act, the collective's citation of it and the ending leaving them out stand as evidence for any outside court.

#### Rule 80. Evaluation

A client evaluating an act signed with a grant key follows its binding to the grant.

The act binds when all of these hold:

- the grant counts (its area's holders met, or the founding terms carry it), and its grantee signed to accept it;
- the act is signed with the grant's key, cites the grant on the collective's chain, is done (rule 57), and lies within the grant's reach (rule 74) and within the limits its limits cMIP sets (grant fields 3 and 4);
- no decision ending the grant fails to hold it (rules 71, 76, 91), or, where one does, the collective adopted it (rule 76).

**For a person's grant**, the person's own chain stands for the collective's in these conditions: the act cites the grant on the person's chain (the person's identity as chain). **For a holder's member key**, the founding or clone naming the key is the grant, counting once in force (rule 72), the holder's signature naming the key its acceptance, and the areas in force for the act that the agreement gives that holder its reach (rule 73).

An act beyond the grant's reach is not backed by it, whatever its strand. **Nor is an act of the grant key outside its cMIP's limits, as one outside its kinds is not.** A verifier that does not hold the limits cMIP, or the time reference a limit is judged against, answers unknown.

*Every act of a grant key binds or does not.*

### 14. Succession

#### Rule 81. The core gives the language, not the answers

The core gives the language, not the answers.

A party may attach a succession plan to an agreement, as part of its signed terms. A plan names:
- stake successors, each with a share of the party's stakes (summing exactly);
- seat successors, entering automatically or with the members' approval, with no voting weight.

Stake and seat successors may be the same identity or different ones.

**A seat successor carries no voting weight**: plan key 2 lists the seat successors alone; a plan writing a weight beside a successor is invalid, as field 24's head count is. Where a rule counts by share, a seat successor weighs what it holds of the stake, read from field 7, which the plan's stake part gives it; a weight written as a free number is refused.

Changing a plan takes a clone, which needs every member's signature, the party's included (rule 67).

#### Rule 82. Triggers

A plan executes when the party triggers it while alive, or when the authority named by the abandonment clause declares the party absent.

*Death is a complex issue; the core leaves it to the parties, their named authorities and verifying modules.*

#### Rule 83. Execution

Succession is a clone: the new terms carry the stakes and seats the plan gives, and any position attached to a seat is defined at that moment. In a collective, a change of seats changes the parties, which is constitutional.

**A seat passes automatically** (plan key 3, value 0) **only if every party whose voice remains signed that version of the succession plan.** That is: each of them signed a version of the agreement, in the chain up to the parent, whose field 16 carries that party's plan exactly as the clone executes it.

The clone's mark then names the plan, `[3, party]`, with the seat successors entering as its signers. It comes into force on the trigger (rule 82) and their signatures, with no other power, provided it changes nothing but what the plan gives, as rule 64 says:
- the party out and its seat successors in, among the parties and in its place in every holding of the key grammar, every threshold unchanged;
- the party out of every area it held;
- the executed plan dropped;
- and, where the party held the chain key alone, a plan for the successor in its place, naming their own successor, signed by the successor in that clone (rule 59).

**A clone marked with the plan alone that changes anything more is invalid** (a false mark, rule 46): one that places a seat successor elsewhere than in the party's own place, among the parties or in a holding of the key grammar, or that reorders the other parties, changes more than the plan gives.

*Consent given in advance, by everyone still counted (rule 8); a plan that some members cloned without the others is not that version, so it cannot slip a stranger into the constitution through a judicial clause. A plan changes only with every member, so a plan some members clone without the others never comes into force at all.*

Otherwise the seat successor is a nomination, approved under the constitutional change rule (rule 64). That approval is counted among the voices that remain once the absent party's voice is removed (rule 89), as in rule 71. The successor signs the clone too (rule 45).

A stake passes by succession as planned: it is property, not a seat.

**A seat passed by succession carries membership, never the areas its holder held.** The seat carries the party's voice in the constitution and the change rules. The clone that completes the succession, automatic or by nomination, takes the party out of the holders of every area it held. Each such area is refitted as rule 71 says:
- its remaining holders carry on meanwhile;
- an area left with no holder is written with an empty list of holders and stands frozen, its grants ending and reinstated, where wanted, by cloning at the refit, until a constitutional clone gives it a holder, which may be the same clone where the successor is a nomination;
- an area left with fewer holders than its number keeps that number, met by all who remain.

*An area is a job the collective gave a person; a plan is the party's own, and letting it hand out that job would reach into the constitution. The successor is free to be nominated for it.*

In a deal, the clone needs every party whose voice remains (rule 47).

For a collective, the members then rotate and re-split the chain key to include the new key holders, using the grammar's recovery path where a share is gone for good. The succession clone changes the parties, so a rotation declares it (rule 69), and that rotation re-deals the collective's keys to include the successor.

#### Rule 84. Fallback

**Where no plan exists, a succession the party declared under Identity and its successor confirmed (Identity, section 8) makes the successor a nomination for the party's seat, approved as rule 64 says.** No seat passes automatically this way: an automatic entry needs consent given in advance by everyone still counted (rule 83), and an identity-level succession is the party's act alone. The successor signs the clone that seats it (rule 45).

**The stake stays with the party's identity**, which Identity keeps alive (Identity rule 39), paid at its pointer, until a transfer (rule 10) or the abandonment clause (rule 89) moves it.

**A succession not confirmed nominates nobody.** Otherwise, and where no succession was declared, the abandonment clause applies.

*Why: under rule 83's own reasoning a stranger never enters the constitution without the voices that remain; and Identity rule 39 keeps the old identity alive, so the stake needs no successor to be paid.*

### 15. Abandonment

#### Rule 85. Clause

An abandonment clause, signed into the agreement, names who decides absence and which outcomes may follow.

Every collective carries one that covers each party with constitutional power (rule 61). A deal may carry one or not.

**In a collective, a declaration may also name a holder of the collective's stake in itself who is no party**: a departed holder, a stake successor, a buyer. It is made under the clause in force; its authority and its absence-proof cMIP are the clause's, as for a party. Against such a holder its outcomes are the stake-moving ones only: succession, as rule 89 says, or outcome 1 or 2 where the holder's plan names no stake successor; never 3 or 4. **Against a holder who is no party and whose plan names a stake successor, the declaration states no outcome (field 3 empty), and the stake passes by succession** (rule 89); outcome 0 is never stated against a holder who is no party, who has no voice to remove. Rule 61's coverage is unchanged: a holder has no constitutional voice to cover (but see rule 61 where the constitutional change rule counts among a stake's holders). *So a departed holder who vanishes does not block the collective's fees, because its stake moves; without this sentence a declaration could name only a party, and one holder who vanished blocked every change paying the stake less, for good.* *In a deal, a party's death leaves the agreement running as written, with succession plans to step in, and locks nobody out of an identity.*

The authority MUST be an identity:
- the keeper's operator;
- a threshold of the other parties, counted as rule 65 says, so it never asks for more of them than remain. In a collective, they are counted among the voices that remain at the collective's line registering the declaration, or at the recovery rotation where the declaration takes effect there (rule 89): the same place every other departure is judged;
- or a named third party.

**In a deal, the authority is one identity: a party, a keeper's operator, or a collective, never a threshold of the other parties.** Parties who need to decide together form a collective, and name it.

Where a threshold of the other parties needs more than one of them, one signs the declaration and the others add signature acts naming it, as for terms. It counts once the required number have signed ("Abandonment declaration"), placed at the collective's line, or named by the recovery rotation where the declaration takes effect there.

An anchoring cMIP is named as the time reference the authority judges absence against, never as the authority.

Absence means absence from duty: no act by the party on the agreement, for a period measured on the agreement's time reference.

*Client conformance* (as Text rule 6): **before a party signs terms carrying an abandonment clause, its client MUST show in plain words who may declare it absent, with which outcomes, and whether an absence-proof cMIP stands between that authority's word and its stake.**

#### Rule 86. Liveness

A party with nothing else to sign shows presence with a liveness act.

*Presence is shown, never proven by time in the core: whether a party is gone is the judgment of the authority the members chose, or, where their clause names one, of an absence-proof cMIP (rule 87).*

#### Rule 87. Declaration

A declaration is a signed act by the authority the clause names, stating the agreement, the clause, the party and the outcome (type 13). In a collective the party may be a holder of its stake in itself who is no party, under the clause in force, with the stake-moving outcomes only (rule 85).

The core checks:
- that it is signed by that authority (for a threshold of the other parties, by one of them, completed by the others' signature acts until the number is met);
- that the outcome is one the clause allows;
- that field 1 names the version the party declared absent signed last ("Abandonment declaration");
- that, where the party's succession plan names a stake successor, it states neither outcome 1 nor outcome 2 (rule 89).

**Otherwise the declaration is the authority's judgment, a stated cost the party accepted by signing the clause, and a contest shows it.**

Where the clause names an absence-proof cMIP (key 3), the declaration counts only if that cMIP accepts it (task "Absence proof").

A declaration moves nothing by itself. Its outcomes take effect only through what is put in force under it: a record or rotation in a collective, a clone in a deal (rule 89). The checks are judged at the point where that act uses it. A later act of the party, or a later contest, never undoes what was put in force ("Made before, made after"). **A later rotation is such an act**: the declaration, its completing signatures and the signatures its field 1 is checked against keep, for the collective, the effect the line gave them, whatever their signers' later rotations disowned, shown as a dispute on the line.

The clause applied is the one in force, of the agreement it names, for every party (rule 67): a party whose voice was removed before a judicial change is judged by the clause in force, as every departed holder is.

#### Rule 88. Contest

A party who believes they were wrongly declared absent contests it with a contest act.

*Client conformance*: **a declaration naming a party MUST always be shown to that party, broken collective or not, with the way to contest it.**

*A contest shows a declaration, it does not void it: a wrongly declared member regains their voice only by a later version of the agreement naming them. The absentee must be informed: the declaration reaches them, and their contest shows presence. The contest act's format (type 14) is written: it names the declaration it answers and is signed by the party declared absent, which shows presence; the declared member's client lets them sign it ("Contest (type 14)"). A declaration counts only if public or addressed to the member it names, so it reaches them through the very act that uses it.*

#### Rule 89. Outcomes

**A declaration moves a stake only by succession**: the stake, and its pay with it, goes to the stake successors the holder's succession plan names, in the plan's shares (rules 81 to 83). A clone carrying exactly that, put in force under the declaration, needs no signature of the absent holder. Abandonment never lowers what a stake is paid while the stake stays with its holder (rule 48).

*Abandonment protects operational flow; it is not meant to change stakes or pay. What it can trigger in that regard is succession, the stake and its pay going to the successor.*

The outcomes of a declaration may be combined:
- **voice removed**, described below;
- stake redistributed among remaining holders in proportion, **only where the holder's succession plan names no stake successor**;
- stake transferred to parties named or defined by role, **only where the holder's succession plan names no stake successor**;
- obligations redirected or held;
- agreement closed.

A declaration stating outcome 1 or 2 against a holder whose succession plan names a stake successor is refused (rule 87).

**Against a holder of the collective's stake in itself who is no party**: the stake passes to its stake successor, as above, or, where its plan names none, outcome 1 or 2 moves it; never outcome 0, 3 or 4. The clone carrying it, put in force under the declaration, needs no signature of the absent holder. *The succession plan a departed holder signed as a member stays with its stake (rule 67), so the plan exists to execute.*

Voice removed: the party is no longer counted in any change rule or area of the agreement, or of any agreement descending from it (rule 65). This holds:
- in a collective, from the collective's line registering the declaration, or from the recovery rotation where the declared party is the one who would draw that line, the rotation that removes them;
- in a deal, from the declaration on ("Made before, made after").

A clone put in force before then is never undone, whatever version of the clause or of the agreement the declaration names. "Before then" means by a record or rotation of a collective before its line, or, in a deal, by signatures a keeper recorded before recording the declaration.

When the voice is removed in a collective, the remaining members rotate its keys and clone it without the party (rule 69), the recovery path standing in for a share that is gone.

*A declaration is not a clone: it needs no change rule, only the authority the party signed. Removing a voice never takes the stake; that takes the party's succession plan or, where it names no stake successor, outcome 1 or 2, which the party also signed.*

## Part three: rare cases

### 16. Forks of records

#### Rule 90. Forks of records

*In short: where a collective's rules let parties act independently and two records split the agreement, the version before the split stays in force until a later change, recorded after both branches, chooses one.*

Collectives only. Where a change rule of a collective (a clone rule, or an area's power) lets parties act independently, two records can fork the agreement. The parent stays in force until a clone of either branch resolves it (the concurrency rule, terms field 10, is withdrawn). Only a clone written on the collective's record after both lines (a record that each line of the fork is before) resolves it. It comes into force there, and the other branch is not in force.

A clone of a branch is a clone of that branch's latest clone, as records follow the agreement chain one clone at a time (rule 72):
- where a branch moved on, a clone of its first clone resolves nothing;
- where a branch forked again, no clone resolves the fork through it.

Two clones recorded on concurrent lines are such a fork, settled the same way (rule 72).

*A clone names one parent (field 11), so "a clone naming both branches" could not be written; recording a clone of one branch after both lines shows the collective saw the fork and chose. "Its latest clone": read as a clone of the branch's first clone, a resolution would silently drop a clone already recorded on that branch. A clone of one branch recorded after only one of the lines resolves nothing, and two clones of the branches recorded on concurrent lines leave the fork as it was.*

### 17. How a collective ends

#### Rule 91. How a collective ends: four shapes

*In short: a collective ends in one of four ways: a group leaves and the collective carries on (A), it forks into successors (B), it is abandoned but keeps earning (C), or it is dissolved and closed (D).*

Collectives only. *A fork forces a whole lot of decisions to be taken again.*

**A broken collective cannot fork or close before it is rolled back** (rule 93): while it is broken, no fork or closing counts, whatever line it names. *Whoever wants to push an official act fixes the issue first.*

- **A. A group splits off, with the powers to do so.** This is an ordinary membership change (rule 69). The collective re-signs, by a constitutional clone and a rotation, and keeps its identity. The members who leave become departed holders, recorded with their stake in the departed members entry (field 22, rule 68), keeping it under equal treatment, the pointer check and no veto. They may found a collective of their own. Members without the powers can still leave one by one (rule 70) and found something new together.

- **B. The fork proper,** where both groups want changes and disagree.

  Each side first founds its successor: a new collective (its own genesis and keys) whose founding terms have that side's members as parties and name the original in "forked from" (field 23), a back-link only, never a parent.

  **A fork act (type 19) names the successors by their identities, and, signed by the members on its sides with their own identities under the constitutional change rule, closes the original in Agreements.** By default that is every member, a silent member being the abandonment clause's business. Where the founders set a lower rule, the fork may complete without a member, who must then either join a side or leave: a member who signs no side has no seat in any successor, and is a departed holder of each at their percentage. **Where the constitutional change rule counts by share, an ending is counted by share at its line** (operate by share, end by share): the holdings are those field 7 of the agreement in force at the line names, with the stake transfers placed in the ending's history (rule 94, read at the ending's line); each rule the constitution lists is counted as it would be alone (rule 65), the members by their chain signatures on the ending. **Owners who are no members consent as members do, by a chain signature (Identity type 16) with their own chain key naming the ending**, judged as every ending signature is: it counts at its position in their chain, and no later rotation voids it. *The ending names its history by its chain act and kept tips, so nothing in that history can acknowledge a signature on the ending itself: one way to sign an ending, whoever signs.*

  From the fork's line, anything the original's keys sign counts for nothing in Agreements, whatever its key grammar's holders do. A rotation of the original is optional cleanup.

  *Two fork acts signed concurrently on the same agreement, neither naming the other, are concurrent acts on its chain (rule 6): the status quo stands, and neither closes it, until one names the other. **A complete fork or closing is final**: a later ending of the same collective, naming it, counts for nothing. A later fork of a successor is a new fork, of the successor, in sequence.*

  Once the fork is complete:
   - **Ownership.** The fork act transfers the original's stake in each work, as a stake transfer, to the successors, in the shares it names (field 5). By default the shares are in proportion to the stakes of the members on each side in the original itself (field 7), divided in millionths, leftovers by largest remainder as rule 9, ties ordered by the hash of the fork act with each side: `tagged_hash("MOR/law/leftover", [ fork act's id, successor's identity hash ])`, the array of two byte strings in deterministic CBOR, the side whose hash is smallest in bytewise order first. Where the original carries no stakes in itself, each member counts alike: no agreement means equal weight. Each successor then owns its share outright and runs it: pointer, rails, split service. The work's fingerprint never changes.
   - **Departed holders keep their percentage in every successor,** and their stake follows into the successors' future works, a stake in each collective as a whole. Each successor's founding terms keep every departed holder of the original, and every member who signed no side, at the same share of all its income, in its stake in itself (field 7) and its departed members entry (field 22); the members divide the rest. **And no successor's founding terms pay a kept holder less than the original's did, unless the kept holders consent** (rule 95): a successor's plan taking more off the top, or a higher fee, is a change paying them less, as for a clone. *Example: four at 25%, one of them departed; sides taking two thirds and one third: the departed holds 25% of each, the others 37.5% in one or 75% in the other.* *If too many departed members still own shares, the incentive is to found an entirely new collective instead of forking.*
   - **Debts.** The fork cites the original's action head, and hands out every obligation in that history, each to one successor or to several jointly, or it does not take effect ("Fork (type 19)").
     - The successor collective signs for each debt assigned to it, by its own chain signature on the fork (Identity type 16), so members are never made personal debtors of what the collective owed, and no later rotation of the successor unmakes the fork.
     - **A complete fork is never undone over a debt**, nor by a later ending: a debt published after it was in its history, and handed out, or outside it, and void (the tie rule).
     - No successor owes a debt the fork did not hand it.
     - A later fork of a successor hands out, in turn, what an earlier fork handed it, where its own chain signature on that fork lies in its identity chain before the later fork's chain act (the signature is a chain act).
     - An obligation signed by a collective binds it once done, like any act (rule 57): sealed to every member, and on its chain. A debt sealed to the creditor alone binds no one, its signer included, so hiding gains nothing.
     - Publishing the debt is the creditor's choice and protection (client conformance), and no validity rests on where an act is stored.
   - **Grants.** Every grant of the original ends at the fork, the old split service's included. **Every grant key ends at the fork's line**, as when a departure empties an area (rule 71). Each successor grants, with a grant key of its own, what it wants. An act of a grant key in the history the fork cites, or that the original adopted before the fork, binds, as a debt. Any other is missing from the ending's history, and void (the tie rule): its counterparty deals again with a successor.
   - **Money after it.** Payment follows the work's current claim: the work carries nothing and never changes; its claim names the successors as owners, so routing follows to their own pointers and split services. A payee pointer of the original that the ending left outside its history still counts for payments made before the ending's line, as the chain stood for each, and for none made after it ("Fork (type 19)"; Money rule 4).
     - The fork act withdraws the original's open offers: a wallet reading Agreements sees them withdrawn, and pays a successor's offer instead.
     - **A payment that reaches the old split service after the fork** (a wallet not reading Agreements paying a withdrawn offer, a push rail that cannot refuse) **names no claim, or the original's superseded claim, and the original's actions chain never recorded it before the fork: it is no purchase, and is owed back to the payer as a refund** (rule 12; the old service's grant key ended at the fork's line).
     - A sale the original's chain recorded in the fork's history stays a purchase.

     *Client conformance:* the old split service, its grant ended, issues no payment request. *Stated cost: a wallet reading only Money does not see the withdrawal, and could still pay an on-chain address of a dead service; that money is owed back to it.* *The work is the point of payment; it needs nothing of the old collective.*
   - **Defaults.** Whatever the fork act does not settle falls to these defaults.

- **C. Actively abandoned,** where everyone agrees the collective has ended but its works still earn: a plain collective, nothing new. It keeps its rules, changes rails, renews offers and pays every holder until winding it down makes sense. *A plain collective is enough; only how its stakeholders use it changes.*

- **D. Dissolution,** where the collective dies. Its works are sold (stake transfers, rule 10) or released to the public domain (rule 55), then it closes. **Each sale or release of a work is an owners' matter** (rule 95): the owners of the collective's stake in itself who have no say consent, by their holders' rule or each signing. **A closing act (type 20), signed under the constitutional change rule like the fork act, and counted by share at its line where that rule counts by share, as a fork is, ends a collective that holds nothing and owes nothing.** After its line, the collective's keys count for nothing in Agreements. A collective cannot close while it owes anything: one that cannot pay stays open, abandoned, its debts visible, until each is paid or its creditor releases it (rule 92). This is the whole of liquidation: ordinary sales, transfers and releases before the closing, the closing naming no stake. An absentee's money share follows the collective's own rules (its split plan, its absence clause, its terms); what they could not deliver stays visible as an open obligation wherever they sent it, and it does not keep the collective from closing. The closing names nothing for it; field 4 stays for money owed back (rule 93, "Closing (type 20)"). Batching them is a client's or a cMIP's convenience, never a second way in the core.

*"A fork closes the original" holds for shape B only. No group keeps the original identity: a lone holdout keeping the identity, its history and its pointers while everyone else leaves was the cost that closing the original removes. Leaving costs the vote and the right to act, never a stake unless it is sold.*

#### Rule 92. The creditor's release

*In short: only a creditor can forgive a debt, by signing a release.*

The creditor's release is a Money act. **A creditor may end an obligation owed to it without full payment** by a creditor's release (Money type 4), for instance against stakes or a partial payment.

Only the creditor signs it. No rule, majority or court can discharge a debt for it, and a release signed by anyone else ends nothing.

A collective creditor signs it by the area reaching Money alone: forgiving a sum only gives up money, and the release is a Money act, which that area reaches (rule 60).

A release given against future terms (a share of future income, stakes) is also a deal: an Agreements agreement of its own, signed by the area reaching Agreements, which the release names among what it was given against (Money release field 1). The agreement binds on its own, and the release counts whatever it names, its field 1 a record only, never checked. *Plain forgiveness needs the area reaching Money; forgiveness traded for future terms needs both areas, each for its own act.*

Once it counts:
- the obligation is owed by nobody, the debtor or any successor owing it (rule 91);
- it ends the whole debt, whatever was paid toward it: to forgive part, the debtor signs a new obligation for the rest and the creditor releases the old one;
- it counts wherever a verifier holds it, published or not, even before the debt it ends binds.

**A closing checks that every debt is paid or released** (rule 91, D). **For a closing, a release counts only where an act of the collective in the closing's history acknowledges it** ("Closing (type 20)"); one nobody acknowledged is shown, and the closing waits. Everywhere else the release counts wherever a verifier holds it, as above.


*Bankruptcy, walked through: a collective that cannot pay keeps its debts open and visible and cannot close; its assets can go to its creditors by sale or by stake transfers, debt turned into ownership; its members are not personally liable; what was missing was discharge, and MOR has no court: the creditor's release is the creditor's own word.*

### 18. A broken collective

#### Rule 93. A broken collective, and its way back

*In short: a collective is broken when a rotation moves its keys without putting a valid agreement in force, so Identity and Agreements no longer agree on who is in. While it is broken, everything done in its name counts for nothing, and it keeps only what its rules before the break allowed. The way back is a rollback: a later rotation returning to the agreement in force just before the break.*

**What breaks a collective.**

A collective is broken when its keys moved by a rotation whose declared agreement fails rule 69, or whose Agreements declaration is missing or cannot be read. Identity counts the rotation, so the keys have moved, but:

- the clone it declares is not a clone of the agreement in force; or
- it does not change the constitutional tier; or
- it is not complete with the signature acts the declaration names (its mark false, rule 46, included); or
- the rotation declares founding terms; or
- it removes the Agreements declaration; or
- it carries one in neither form.

Agreements then finds no agreement in force, and Identity and Agreements no longer agree on who is in. That rotation is **the broken act**: the first such rotation after the last agreement in force.

*Breaking the collective is what makes such a move worthless. If Agreements ignored the failed declaration and carried the earlier agreement forward, whoever holds the chain key could rotate a member out of the keys at no cost.*

**This is also how a constitution counting membership among a stake's holders is kept** (rule 69): a rotation without the holders' signatures it requires breaks the collective, as one without the members' does. It compels no rotation the key holders refuse. **An owners' matter passed in a broken stretch counts for nothing**, with every other act of the stretch; a rollback registers no holders' vote, so the owners' matter is made again after it.

**The broken stretch and quarantine.**

The broken stretch runs from the broken act to the rollback. Every act of the collective bound to a link of its identity chain in that stretch **stays shown and counts for nothing in Agreements**, during the stretch and after it. This covers the broken act, the rotations after it other than the rollback, records and the clones and departures they write, agreements, releases, debts, grants, and every other act in its name.

Such an act places no signature at all, a member's signature act on a declaration of absence included ("Made before, made after", 2). Its records register nothing (record field 3): only the rollback's registration counts.

The broken collective is quarantined. *A broken collective is broken until fixed: quarantined as much as possible.*

**A grantee's act of the broken stretch counts for nothing**, like the collective's own. The grants issued before the broken act work again after the rollback, which restores every condition as at the act before the break: a grant issued, revoked or ended by an act of the stretch is as it was before the break (rule 73).

A grantee's act is of the stretch where:

- neither the broken act's history nor that of an act of the collective's own key before it holds it; and
- it cites neither the rollback nor any later decision of the collective (rule 58).

**After the rollback, a grantee's act counts only once it cites the rollback or a later decision of the collective.** An act citing nothing newer than the time before the break counts for nothing, and is signed again; this includes one made just before the break that no act of the collective took in.

*Client conformance:* a grantee's client cites the latest decision of the collective it holds in every act it signs. *MOR has no clock of its own, so an unanchored act proves where it stands only by what it cites; an anchor can place an act, and the rule asks for the citation all the same, as the safer path.*

**What others relied on during the stretch (a debt, a sale, a receipt) counts for nothing, for good, and is signed anew after the rollback**, never adopted by citing it (rule 76 does not reach it).

**What the collective keeps.**

**The broken collective keeps exactly what its rules before the break allowed.** The test: would it have been valid under the collective's rules as they stood just before the break? If yes, the collective keeps it; if not, it does not.

A payment is judged by the offer it names, not by when it was paid:

- A sale under an offer made before the break is kept, even received during the stretch. What the collective owes for it (a key delivery) is done once the rollback lets it act.
- A debt paid to the collective under an agreement made before the break is kept, and stands as paid.
- A payment naming nothing, a tip, is kept.
- **A payment under an offer made during the stretch is no purchase and is owed back to its payer** (Money rule 19), unless the sale is signed anew after the rollback, by a receipt of the collective for the same payment. Until settled it stands as a visible open obligation.

A payment shown only by its payer's claim is judged the same way, by the offer it names. The rail's own time (a block, an invoice) is supporting evidence where it exists, never the rule.

*An offer made during the stretch is a version of the collective's agreement descending from the one in force just before the broken act and not from the rollback's clone; a deal the collective signed during the stretch never came into force, its signature counting for nothing. The collective is read as having an internal problem that freezes changes to its external interactions: the collective's own acts of the stretch still count for nothing; what it keeps is only what others did within rules that already held.*

**Money owed back, and how it is repaid.**

**Money owed back from the stretch is repaid like any debt**, with no new act:

- by a payment naming what it repays: the receipt's field 5 or the claim's field 4 naming a receipt or claim of the payment owed back;
- made to its payer;
- proven by either side's record, the payer's receipt or the collective's claim (double entry, Money rule 11).

Repaid in full, it is no longer owed.

**It follows the debt rule exactly** (Money rule 17). The repayment goes:

- to the refund rail the payer's own claim on the payment names (key 7), where it names one, counting up to the payer's vault limit for its unit and, above it, only once it reaches the payer's vault (rule 11);
- otherwise, to the pointer Money rule 17 selects from the payer's own acts on that payment, or the payer's pointer in force where they hold none.

A payment to the payer's vault always counts, and the payer's own receipt counts wherever the money went.

*Why: a repayment counted at any pointer the payer ever published would let a collective repay to an old pointer a thief controls and be cleared while the payer lost the money; Money rules 17 and 29 exist to stop exactly that for every other debt. Stated cost: the collective looks up the payer's current pointer, as any debtor does.*

**A refund owed to a bare key** (Money rule 19) **is repaid once the collective's claim carries the rail's proof that the money reached where the claim signed with that key said**: the same double entry, a claim with a rail proof showing the money arrived.

**Money owed back to nobody stays open and visible, but does not block a closing** (stated here, not in Money). This is a payment that committed no key, whose refund Money rule 19 can pay to nobody. The closing act names every such obligation it leaves open (the closing's field 4), as a settling version names every tip it drops.

*Why: one anonymous tip with no key, received in a broken stretch, kept the collective from ever closing, a protection trapping every member with nobody gaining from it. Stated cost: a collective may close while visibly owing an anonymous payer who could never have been paid.*

**Money owed back to a payer who gave no address**: a payer with no payee pointer, vault, or refund rail on a claim of theirs. Before closing, the collective sends the payer **a notice sealed to their identity, with a deadline on a time reference** (type 24), asking where the money should go.

- If the payer names an address in time, they are paid.
- If not, the collective may close, the debt named in the closing act, visible and unpaid.

A collective may also set the amount aside with a named holder that outlives the closing and pays the payer when they turn up (a keeper or custodian role, for a cMIP to define). The core allows it, shows it beside the debt in the closing act, and requires neither the holder nor any one notice period.

*Why: a stranger who pays a penny from a fresh identity under an offer of the stretch, and publishes nothing, would otherwise freeze the collective's ending for good. After a closing the collective's keys count for nothing, so nothing could be paid later: the notice comes first. The notice demonstrates good faith, its degree set by the notice's conditions.*

*A payer who committed only a one-time key cannot be reached by a notice.*

*The network records and shows the debt and the payer's claim; it cannot force a payment back, as with every obligation in MOR.*

**No fork or closing while broken.**

**A broken collective cannot fork or close before it is fixed.** A fork or closing made during the broken stretch counts for nothing, then and after the rollback (rule 91). And while a collective is broken, no fork or closing counts, whatever line it names, the last good link before the broken act included.

Money owed back from the broken stretch is a debt for a closing's "owes nothing": the collective cannot close until those payers are settled. Two kinds are the exception, and the closing names them as left open:

- money owed back to nobody;
- money owed back to a payer who gave no address and let a notice's deadline pass.

*An emergency has emergency rules: all that happened under it is settled by the rules before the emergency is lifted. A closing on an old line was a quiet way out of the refunds owed for the stretch.*

**The rollback.**

**The way back is a rollback.** A later rotation of the collective declares a rollback: a clone

- whose parent (field 11) is **the agreement in force just before the broken act**, the one in force at that rotation (rule 72); and
- which names the broken act, which it voids and which stays shown ("Collectives in the identity chain").

Nothing is erased. The identity chain carries on: the rollback rotates from the keys the broken stretch holds, as any rotation does (Identity). The agreement chain forks from before the broken act, and the broken clone stays on its own branch, never in force.

*The chain forks from one act before the corrupted one, while showing the corrupted one: that is what the collective faces, a rollback.*

The power it needs. **A rollback needs the constitutional change rule of the agreement in force just before the broken act** (rule 64), counted among the voices that remain at the rollback (rule 65). By default that rule is every party whose voice remains. So a party no longer blocks it:

- whose departure (a resignation, rule 70, or a declaration with outcome 0, rule 89) the collective registered on a line before the broken act; or
- whose resignation, stepping down from an area, or declaration of absence the rollback itself registers.

*Whatever was in force just before the broken act needs the approval that version requires; the broken act is void, and always referenced: the constitutional powers must be met.*

Where every voice that remains for that rule has resigned, nothing meets it (rule 65): no rollback is possible, and the collective stays broken (rule 70, "The last voice").

The rollback's mark (field 4) names the constitutional change rule, `[0]`, with its signers. Where the clone also changes the judicial tier, it names the judicial tier's rule, `[4]`, too, every party whose voice remains (rules 64.1, 67). It names nothing else, whatever its changes: a rollback may change nothing, or may redo correctly the change the broken act failed. *Its mark is checked against these powers, not those of rule 64.2, so a rollback copying its parent unchanged is no false mark (rule 46). Rule 69's "MUST change the constitutional tier" does not apply to it: a rollback is a rotation by its nature, since only a rotation fences off the broken stretch's keys.*

What it registers. **The rollback may register a declaration of absence made during the broken stretch** under the clause of the agreement in force just before the broken act, exactly as a record does outside the stretch, so that a member who dies or vanishes during the stretch does not block the way back. *A faction that is the absence authority could already declare a member absent falsely with no break: the break adds steps, not power; the new risk was attention, which rule 88's client conformance closes.*

**The rollback is a line**, by its kept tips, as every rotation is ("Made before, made after"): its clone is in force for the collective's acts after it, and the resignations it registers take effect there.

In everything else a rollback is a clone like any other:

- complete only with the signature acts its declaration names, counted at the rollback (rules 45, 46, 48);
- every party it adds signing it;
- every constitutional voice covered (rule 61);
- its key grammar valid (rule 59).

What it may never do. **The rollback's clone is never the clone the broken act declared**, which stays on its own branch, never in force: a clone of the same rules, signed afresh, may redo the change.

A rotation in the broken stretch that names another act as the broken act, or declares any other agreement, or a rollback that is not complete, is part of the broken stretch, and a later rollback still names the first broken act.

After a rollback, a later rotation that fails rule 69 breaks the collective again, and the next rollback names that rotation.

**A rollback voids only an act Agreements read as broken, never a valid one.** A rollback naming a rotation that met rule 69, or declared where the collective is not broken, puts nothing in force. Being a rotation declaring an agreement that fails rule 69, it is itself a broken act: the attempt breaks the collective, visibly, and is repaired by a rollback to just before it.

*Repair, not undo: a complete ending is final, and a signature placed counts. A break made on purpose reaches back no further than the agreement in force just before it, which holds every valid decision made until then; a person has no rollback.*

*Client conformance:* **a client MUST NOT propose a rollback unless Agreements read the collective as broken.** *A client that does not comply makes its user's attempt fail as above, visibly, at the attempters' cost.*

*Stated cost: the hold-out. A member whose signature the rollback needs can ask a price for it; it is the veto it has over any constitutional change, worth more on a broken collective.*

*Stated cost: break and hold. Holders of enough of the chain key can break the collective on purpose and refuse the repair, freezing a decision they dislike while staying in; they could already leave it unable to act by refusing to sign, and the break stays shown.*

**Technical breaks, and a first agreement that fails.**

**A rotation that removes the collective's Agreements declaration, or carries one in neither form, is a broken act too**, a technical one, with the same way back.

**A collective whose very first agreement fails has nothing to roll back to: it is simply refounded**, as a new collective, with no rollback. *Stated cost: small; a new collective has no history, works, money or followers yet.*

### Made before, made after

*In short: MOR has no clock, so "before" and "after" are always read on one chain. For anything a collective does, that chain is the collective's own: never a member's personal sequences.*

*MOR has no clock, and every identity, a collective as much as a member, may keep several sequences at once, one per device (Envelopes, "Sequences"). A member's own sequences cannot settle what happened first: members keep as many as they like, forget some, and nobody else can see them all. Envelopes' own principle settles it instead: every "before" in the core is judged inside one named chain, by reference to a specific act (Envelopes rule 16). For anything a collective does, that chain is the collective's own.*

*A collective's sequences form two chains. Its decisions (genesis, rotations, records, fork, closing) are its lines, each citing the action head it saw by the tips it names. Its actions (every other act in its name) are the acts of its devices' sequences, each citing its previous act there. Each action also cites, in `objects`, the decision it acts under, and the heads of the other devices it knows, so that the next act joins the heads where several devices signed at once (rule 58). A sequence is one device's strand of the actions chain; a join ties strands together between lines. The chain's role is to make acts unmissable, not to confirm them.*

**For anything done in a collective's name, "before" and "after" are judged only on the collective's own sequences, never on a member's personal sequences.**

This holds for every event that changes who may act for the collective:

- a member's resignation or stepping down (rules 70, 71);
- a declaration removing a voice (rule 89);
- a member's own rotation (Identity);
- a clone put in force (rules 69, 72); and
- the freeze of an area (rule 71).

Each takes effect for the collective's acts at a line. A line is a record (type 17) or a rotation of the collective. It names the latest act of every sequence the collective keeps other than its own (record field 1; a rotation's kept tips, Identity).

1. **The collective's own acts.** An act of the collective counts as made before a line when the line's history holds it. That is, when it:

   - precedes the line in the line's own sequence;
   - lies in the ancestry of a tip the line names, proved by position and running summary as for a rotation's kept ancestry (Identity); or
   - is reached from there through what those acts cite on the collective's chain, the tips of a record so reached included.

   For a rotation, its kept tips alone count, by Identity's rule.

   Every other act of the collective counts as made after it, whenever it was really made. That includes an act on a branch the line left out, by mistake or on purpose; an offline device's act its drafter had not heard of; and a fork made later from an act before the line.

   **While an act the line's history names is not held, whether an act is before that line is unknown; nothing is void on it.** "Made after" is read only where the line's history is held whole: a verifier with a hole in it voids nothing by the tie rule (point 7) and takes no voice away (rule 65) on that line; it answers unknown, and judges again once it holds the act.

   Lines are ordered among themselves the same way, by the collective's tips alone.

   **For the agreement in force at a record, only the records before it count.** For an action, the records its history holds count, and those before them (rule 58). A record it does not cite, a concurrent one included, decides nothing for it, save where it ends a power the action uses (the tie rule, point 7).

   *An act within its signer's powers counts as the signer knew the collective's head.* *Read literally between two records drawn on two devices, each would count as after the other, each would judge the other's clone under the wrong parent, and a fork of records could never arise.*

2. **Others' acts are placed by the collective's.** An act of another identity done in the collective's name has no place of its own in the collective's sequence. A member's signature act is placed:

   - at the act of the collective it signs;
   - at a record naming it among the signature acts that complete its clone (record field 2);
   - at a rotation naming it among the signature acts that complete the clone it declares (rule 69);
   - at a recovery rotation naming it among the signature acts on a declaration that takes effect there; and
   - at any act of the collective on its chain acknowledging it (Envelopes, `acks`).

   It counts as made before a line when one of those acts does, and as made after it otherwise.

   An Identity witness act of the collective (type 15), on neither chain, places nothing and counts for nothing in Agreements.

   An act signed with a grant key is the collective's own, on the grantee's strand of its actions chain, citing its grant, as every action cites (rule 58). It has no place by its key. It is before a line when the line's history holds it: that is, when an act of the collective before the line joined it, or a later act of the same strand that such an act joined. Where a decision ending the grant's powers does not hold it, the tie rule voids it, or, for a revocation, voids it or shows it disputed as rule 76 says (point 7), save where the collective itself adopts it (rule 76).

3. **A departure takes effect at the collective's line.** A resignation, a stepping down, or a declaration removing a voice takes effect for the collective at the line registering it: a record naming it in field 3. A member's voice remains for an act of the collective made before that line, and through any signature of theirs placed before it. For everything after it, it is gone (rules 70, 71, 65).

   **Two lines neither of which is before the other are concurrent.** Where a departure is registered on both, an act counts as made before it only when it is before both. Records on concurrent lines are a fork of the agreement chain (rule 72).

   A rotation declaring a clone that removes a party, or takes a holder off an area, is that change's own line (rule 69).

   Rarely, a declaration takes effect at a recovery rotation. A declaration takes effect at the collective's line, or, where the declared member is the one who would draw it (for instance the only holder of the collective's signing key), at the rotation made by the recovery path (rule 59), a line by its kept tips. That is the rotation that removes the declared member, declaring the clone that takes that party out of the parties, so that clone is counted without the declared party. Where the declaration's authority is a threshold needing more than its own signer, that rotation names the others' signature acts and places them there. The abandonment authority draws no line for the collective.

   *A rotation names no declaration itself, only the signature acts completing one where its number needs them; a recovery rotation declaring nothing new removes nobody, and leaves the declaration to the first record under the new key.*

4. **A member's own rotation is registered the same way.** The collective registers a member's rotation (Identity) on its line, as it registers a departure (record field 3).

   A signature act of that member made with the key the rotation replaced, and placed before that line, stays valid for the collective, whatever the rotation kept or disowned, where it signs an act the collective's key holders also signed. That is an act of the collective, or a clone that a record or rotation of the collective puts in force, naming that signature among those that complete it. Its status under Identity is unchanged, and every client shows that status.

   Such a signature placed after that line is judged by Identity alone (rule 6).

   *If the member's key was stolen, the thief's signatures placed before the line count too, but only where the collective's key holders also signed: a stolen member key can finish an act the collective signed, never start one; the collective then reorganizes around the theft.*

   *Stated cost:* *within a collective, Identity's theft protection is softened to this extent.*

   *Stated cost:* *a signature on a clone that no act of the collective placed before the line registering its signer's rotation is judged by Identity alone, so one made from a device the rotation forgot no longer counts; the collective's client avoids it by acknowledging members' signatures as they arrive.*

   **This reaches every act a line places.** An act of another identity that a line of the collective (a record, a rotation, a rollback) names, registers, or relies on to count what it registers keeps, for that collective, the effect the line gave it, whatever its signer's later rotation kept or disowned. That is:

   - a resignation or stepping down (type 16) a line registers;
   - a declaration of absence (type 13) a line registers, or a recovery rotation or rollback takes effect on, and the signature acts completing it that the line places;
   - the signature acts of the declared party that the declaration's field 1 is checked against, as the line judged them (rule 87);
   - a signature act a line names among those completing a clone, or places by acknowledging it (point 2), a holder's vote on an owners' matter and a proxy's among them (rules 94, 97);
   - a stake transfer, a proxy and its revocation that a line registers or acknowledges (rules 94, 97);
   - a take-up of an issuance offer that a line registers or acknowledges (rule 96);
   - **a keeper's record that a rule reads to place an act (point 5), or that a version's completeness relies on (rule 47)**: it keeps the effect the rule gave it, whatever the keeper's later rotation disowned, shown as a dispute, as rule 42 says for every agreement, a collective's or a deal's. *A witness chosen in advance cannot un-witness.*

   Its status under Identity is unchanged, and every client shows it as a dispute on the line. The line is the collective's own act (Identity rule 34's "disowned but relied on"; "what others relied on stands"), so the record stays a line, the clone it wrote stays in force, the departure stays taken, and every later record finds its parent. An act a line never placed is judged by Identity alone: disowned by its signer's rotation, it is shown, and decides nothing.

   *Stated cost:* *a thief holding a member's signing key can resign the member; once the collective's line registers the resignation, the member's rotation does not undo the departure for the collective, and the member comes back by a constitutional clone naming them (rule 65). A stolen member key can so take a voice out until the collective puts it back; the alternative would let one member's chain unwind the collective's year.*

5. **The collective's keepers place its own acts a line left out.** An act of the collective that a line left out counts as made before that line when the keepers named by the collective's agreement in force for the line recorded it before recording the line. This is as the keepers' own rule counts what is recorded: any one, a threshold, or all of them (rules 38, 41, 42).

   Keepers place only the collective's own acts: never a member's signature, a grantee's act, or one line against another, which only the collective's tips order.

   *Stated cost:* *a keeper that records a line late opens a window, the trust in keepers rule 42 already states.*

6. **Nothing else places.** An acknowledgement by any other identity (Envelopes), a member's friend or a counterparty, carries no place in the collective's sequence: it keeps the act visible and changes nothing here. A member's personal sequences, and which device signed what, are never read for the collective.

7. **The tie rule.** Where a decision ending powers and an action using them cite neither each other, **the ending wins: the action is void**, or, where the ending is a grant's revocation, void or disputed as rule 76 says. A decision ending powers is a fork, a closing, a departure, a grant's revocation, or a departure emptying an area. So:

   - an act of the collective a fork's or closing's history does not hold counts for nothing (rule 91);
   - a member's voice a line removes is gone for every act the line's history does not hold, an act judged under a version earlier than the one the departure names included (rule 65);
   - an act signed with a grant key that the line emptying its grant's area does not hold is void (rule 71); one that a revocation of its grant does not hold is void, or disputed under rule 76.

   "Does not hold" is read on a history held whole: while an act the history names is not held, the action is unknown for that verifier, never void (point 1).

   *An action missing from the ending's history was made with powers that are ending; its counterparty deals again with a successor. Otherwise anyone holding a power could keep a fork or closing from settling by racing it. A counterparty protects itself by waiting until the actions chain visibly moves past its act before delivering.*

   **The tie rule voids only acts the collective never took on.** An act that an act of the collective's own key that counts cites is adopted (rule 76), and no ending racing that citation voids it. "Cites" is meant as a line cites: the act lies in the citing act's history, so citing a later act of the same strand cites it too.

   *A revocation from a device that had not heard of the citation cannot reach the act; it ends the grant for everything after it. A departure racing the citation takes no voice off the act: the act counts if it counts as judged, or once the departures racing its citation are set aside (whichever of the two judgments counts). Against a fork or closing nothing changes: an act citing it that the ending's history holds puts it in that history, and one the history does not hold is itself void.*

   *Stated limit:* two acts signed at the same moment without seeing each other cannot be ordered by hashes alone; with two chains this remains only where a decision ending powers meets an action using them, and the tie rule settles it. Where the decision ends no power (a record of an operational or judicial clone), each act is judged under the decisions it cites (rule 58).

*Stated cost:*

- *A departure takes effect for the collective's acts when the collective draws its line, not when the member signs it: until then, the departed member's signature still counts on the collective's acts. The line is drawn by whoever produces the collective's signature, who could already withhold it from any act.*
- *An act of the collective on a branch its own line left out counts as made after it, unless a keeper placed it: the cost falls on the collective, whose line it was, and on whoever relied on the act; it is the power its key holders already have at a rotation, which can leave out a branch (Identity rules 33 to 35).*
- *A member who left can still complete an act the collective signed before its line, and it counts: the act was drafted with her in it. A pending act stays completable by whoever held the area when it was made.*
- *A signature the collective never acknowledged before its line, on a clone recorded only after it, no longer counts for its departed signer; a collective's client SHOULD acknowledge members' signatures as they arrive.*

*Client conformance:* the collective's devices share their tips before a line is drawn, and a client warns before drawing one with a device not heard from, as before a rotation (Identity). **For a fork or a closing it refuses**: a member's client signs no fork or closing, its own or another member's, until it has pulled every device's latest acts and holds every act the ending's history should hold.

**In a deal, a declaration draws its own line.** A deal has no sequence of its own, and no collective's line reaches it. An abandonment declaration with outcome 0 (rule 89) removes the party's voice in a deal from then on. Every signature act of that party on the deal counts as made after it, except one that the keepers the deal names recorded before recording the declaration, as their rule counts it (rules 38, 41, 42). So a clone of the deal completed, and so recorded, before it is never undone. The other parties then change the deal without the declared party (rule 47).

**What others relied on stays complete before any declaration's line.** A declaration's line decides only what no relied-on version holds: a version of the deal that a party's or keeper's later act relies on (rule 47) stays complete, the declared party's signature on it included, in a deal that names keepers and in one that names none.

*A deal's authority is one identity: its signature alone makes the declaration, so the declaration is its own line, with nothing to complete after it.* *A signature on a deal's clone names the clone, an agreement, so the deal's keepers can place it; deals that need protection name a keeper.*

## Part four: act formats

All acts are Envelopes MIR acts, private by default, visible to the agreement's parties and keepers unless they choose otherwise.

Every act type below has its exact format.

Every Agreements act type may carry acknowledgements (`acks`, Envelopes rules 18 and 19).

| Type | Name | Signed by |
| --- | --- | --- |
| 0 | Terms | the proposer |
| 1 | Signature | each party (or grantee, or collective member) |
| 2 | Keeper record | a keeper's operator |
| 3 | Work claim | a sole creator; or one creator, who opens it, completed by the other creators' signature acts. Counts only public or addressed to every creator it names |
| 4 | Stake transfer | the seller, who also signs it by a chain signature (Identity type 16), which fixes its place on the seller's own line; completed by the buyer's signature. Names the agreement as `[agreement, agreement]` |
| 5 | Public domain release | a direct owner of the work, completed by the signature acts of every other holder of the stakes it ends, unless the release rule says otherwise (field 24). A collective owner signs by its own rules, across the areas reaching Envelopes, Money and Agreements |
| 6 | Standing offer | always public. Under co-owners' agreement, the owners under its rule: in a deal every party, one signing it and the others by signature acts naming it; in a collective, an act of the collective. A lone seller's offer, by the seller, is itself the agreement with the buyer |
| 7 | *Retired*: the payee's receipt naming the offer, and the claim where there is one, confirms delivery of a publication whose key is public; never reused | — |
| 8 | Split | the split service, with its own key; delivered to every holder it pays |
| 9 | Grant | the grantor. For a collective granting within an area, completed by its holders' signature acts; a reinstatement (field 7) likewise. A payee's grant in a deal: by the payee with its own key, carried by the deal that lists it. Accepted by the grantee's own signature act |
| 10 | Revocation | the grantor, with its own key, naming the grant whose key it removes. For a grant within an area, completed by that area's holders' signature acts |
| 11 | *Retired*: adoption is by acknowledging, citing or paying on (rule 76); never reused | — |
| 12 | Liveness | a party; counts only public or addressed to the agreement's other parties |
| 13 | Abandonment declaration | the authority the clause names, always an identity. For a threshold of the other parties (in a collective only): one of them, completed by the others' signature acts, which the recovery rotation names where the declaration takes effect there |
| 14 | Contest | anyone with standing (rule 54): the party a declaration of absence names, answering it; someone who signed onto a vow, contesting a sale (rule 13); anyone else with standing, contesting another act |
| 15 | *Retired*: a Module states its fee in its own specification (Development field 7); never reused | — |
| 16 | Resignation | the member giving up their voice, or stepping down from one area, naming the drafts they leave behind |
| 17 | Record | the collective, with its signing key: its everyday line |
| 18 | Negotiation message | one side of a negotiation thread (rule 52) |
| 19 | Fork | a member of the collective, with their own identity; completed by the chain signatures (Identity type 16) of every member it lists, meeting the constitutional change rule, with those of holders who are no members where that rule counts among a stake's holders, and by the chain signatures of each successor a debt is assigned to (rule 91) |
| 20 | Closing | a member of the collective, with their own identity; completed by the chain signatures (Identity type 16) of members meeting the constitutional change rule, with those of holders who are no members where that rule counts among a stake's holders (rule 91) |
| 21 | *Retired*: the creditor's release is a Money act, Money type 4 (rule 92); never reused | — |
| 22 | Settlement request | a party who signed a deal's reference version, asking the judge of forks it names to settle the fork (rule 47) |
| 23 | Fork settlement | the judge of forks a deal's reference version names (field 27), or the link of its chain of judgment that took over (field 21), once a settlement request activated it (rule 47) |
| 24 | Notice | a collective, with its own key, to a payer owed money back from a broken stretch who gave no address. Sealed to that payer, or public naming the payment where the payer committed only a one-time key, with a deadline on a time reference (rule 93) |
| 25 | Request to a judge | someone with standing (rule 54), naming a judge the terms name and the question. Counts, and the judge's period runs, only where public or sealed to that judge (rule 35) |
| 26 | Issuance offer | a collective, with its own key: an act in its name (rule 57), offering each holder of a stake its proportion of an issuance; it reaches each holder it is public or sealed to (rule 96) |
| 27 | Proxy | a holder of a stake of a collective, with its own key, naming the proxy whose signature acts on the owners' matters count as its vote (rule 97) |
| 28 | Revocation of a proxy | the holder who signed the proxy, with its own key (rule 97) |

### Terms (type 0)

*In short: terms propose an agreement. The format below lists every field; the paragraphs after it are the checks that make terms invalid, and what each field means.*

```cddl
terms-payload = {
  0 => [+ hash],            ; parties: identity or collective hashes
  1 => tstr,                ; the terms' words, as canonical text; in a collective, the constitution's and the general
                            ; words (each area's own words are in field 20)
  2 => { * uint => hash },  ; cMIPs, at most one per task (task number => cMIP hash); empty if it uses no task
  ? 15 => [+ hash],         ; extensions: cMIPs outside the listed tasks, which add rules and never relax the core's;
                            ; in a collective, adopted by the area reaching Development
  ? 16 => [+ succession-plan] ; succession plans of parties, part of the signed terms
  ? 3 => keepers,           ; named keepers, and the rule for what counts as recorded
  4 => rule / mark,         ; founding terms: the signing rule, which signatures make the agreement exist: every party,
                            ; in a deal and in a collective
                            ; a clone: its mark, which change rules of the parent brought it into force, and by whom
  5 => rule,                ; clone rule: in a collective, for operational matters outside every area (the judicial tier
                            ; needs every party whose voice remains, rule 67); every party in a deal
  ? 18 => rule,             ; constitutional change rule; absent: every party (only with field 12)
  ? 19 => [+ area],         ; operational areas, powers of members (only with field 12)
  ? 20 => { + uint => tstr }, ; each area's own words, as canonical text, by area id (only with field 19)
  ? 6 => [ hash, any ],     ; time reference: the task cMIP and its parameters; a naming for task 10, the same
                            ; specification as field 2's task 10 where both are present
  ? 7 => stakes,            ; stakes defined, in works, publications, or the collective itself, written null
  ? 8 => split-plan,        ; how incoming payments are divided
  ? 28 => fees,             ; fees, each entry borne by every stake, off the top: their own field (rule 28);
                            ; the form is under "Split plan and split"
  ? 9 => abandonment,       ; the abandonment clause
  ; 10, the concurrency rule, is withdrawn: rule 90's default stands; never reused
  ? 11 => hash,             ; parent: the agreement this one clones; objects names it as chain and, as predecessor,
                            ; an act of its chain, the latest the signer's client holds (rule 45)
  ? 12 => key-grammar,      ; present if this agreement founds a collective
  ? 13 => [+ hash],         ; arbitrators or verifiers named, who receive keys to judge content
  ? 14 => hash / [+ hash],  ; where incoming payments go to a split service: in a collective, its grant (one hash);
                            ; in a deal, one grant per payee, each naming this agreement by null. Its one
                            ; grantee is the agreement's designated pointer for payments, the same whoever pays
                            ; (rule 15): the first of the split cMIP's addresses, its backups following in
                            ; field 29
  ? 17 => refund-terms,     ; until when a refund owed on a payment under these terms is claimable: a point on the
                            ; time reference
  ? 21 => [+ chain],        ; the chain of judgment: who takes over from a judge (rule 35)
  ? 22 => [+ hash],         ; the departed members entry: who left the collective (rule 68; only
                            ; with field 12)
  ? 23 => hash,             ; forked from: in the founding terms of a fork's successor, the original collective, a
                            ; back-link (rule 91; only with field 12)
  ? 24 => holders-rule / [ 2, [+ hash] ],
                            ; the release rule: which of a stake's holders sign its release (rule 55); founding
                            ; terms set it, and a clone every owner signs changes it ; a threshold counted by
                            ; share, never by head
  ? 26 => [+ hash],         ; settles: in a deal's clone only, every tip of the deal's fork this version discards, that
                            ; is, the latest version of each other branch, beside its one parent (field 11) on its own
                            ; branch; ascending, none twice; each also cited in objects (rule 47)
  ? 27 => hash              ; the judge of forks: in a deal only, the identity among field 13 that settles the deal's
                            ; forks on a party's request, following its chain of judgment (field 21) where it does
                            ; not act; absent, none does (rule 47)
  ? 29 => backups,          ; the split service's backups, in order, and what counts as a failed attempt: fields of
                            ; the split cMIP the agreement names (field 2's task 8, or the plan's key 1), signed with
                            ; the terms (rule 17); only with field 14
  ? 30 => [+ [ stake: uint, holders-rule, ? waived: true ]],
                            ; the holders' rules: per stake, by its index in field 7, how its holders consent together
                            ; to the owners' matters; ascending by stake, none twice; a stake not listed: every holder
                            ; (rules 94, 95); only with field 12; `true` third: a standing
                            ; waiver of pre-emption on that stake (rule 96)
  ? 31 => [+ hash],         ; transfers counted: in a deal's clone carrying an owners' matter, the stake transfers whose
                            ; buyers it counts as holders, ascending, none twice (rule 94)
  ? 32 => hash / [ 0 ]      ; issues under: in a collective's clone that is an issuance, the issuance offer (type 26) it
                            ; follows (rule 96); `[0]`, no offer, under the stake's standing
                            ; waiver of pre-emption (field 30)
}                           ; key 25 (the relays) is withdrawn and never reused: terms carrying it are invalid

chain    = [ judge, next: [+ [ hash, period: uint ]] ]
                                      ; a judge the terms name, and in order those that take over from it, each with
                                      ; the period, on the time reference, that the one before it has to act once
                                      ; asked
judge    = [ 0, task: uint ]          ; the specification named for judicial task 9, 10, 11 or 14 (field 2, field 6 for
                                      ; 10, or the abandonment clause's key 3 for 14): next are specifications
         / [ 1, identity: hash ]      ; a keeper's operator, an arbitrator or verifier, or the abandonment authority the
                                      ; terms name: next are identities
                                      ; [ 2 ], the split service, is withdrawn and never
                                      ; reused: terms carrying it are invalid; its backups are field 29

backups  = {                          ; terms field 29
  0 => [+ hash / [+ hash]],           ; each backup split service after field 14's, in order: in a collective, its
                                      ; grant; in a deal, its grants, one per payee, each naming this agreement by
                                      ; null, as many as field 14 lists
  ? 1 => any                          ; what counts as a failed attempt: the split cMIP's parameters for it
}

key-grammar = {
  0 => holding,             ; how the signing key is held
  1 => holding,             ; how the chain key is held, usually stricter
  ? 3 => recovery           ; a way to rotate that needs less than every member; required when holding 1 names
                            ; every member
}                           ; key 2 (once: listed act types) is retired and never reused: areas reach acts (field 19)
holding  = [ 0, holder: hash ]                        ; one holder
         / [ 1, threshold: uint, members: [+ hash] ]  ; any k of these members: jointly for the signing key; by
                                                      ; shares for the chain key
         / [ 2, custodian: hash, grant: hash ]        ; a custodian under the collective's grant
recovery = [ 0, custodian: hash, grant: hash ]        ; a custodian holds a share under grant
         / [ 1, authority: hash ]                     ; an escrowed share released by the abandonment authority

succession-plan = {
  0 => hash,                               ; the party whose succession this is
  ? 1 => [+ [ successor: hash, share: uint ]],   ; stake successors, shares in millionths of the party's stakes
  ? 2 => [+ hash],                         ; seat successors, with no voting weight: under a rule by share a successor
                                           ; weighs what it holds of the stake, read from field 7; a successor written with a weight is invalid
  ? 3 => uint                              ; seat entry: 0 automatic where every voice signed this version of the
                                           ; plan, otherwise treated as 1 ; 1 a nomination, approved under the
                                           ; power a change of parties needs (rule 64)
}

abandonment = {
  0 => [ 0, hash ]          ; the authority: a named identity (a keeper's operator, a party, a collective, or a third party)
     / [ 1, uint ],         ; or a threshold of the other parties: in a collective only
  1 => [+ uint],            ; outcomes allowed (rule 89), ascending: 0 voice removed, 1 stake redistributed,
                            ; 2 stake transferred to parties named or defined by role, 3 obligations redirected or
                            ; held, 4 agreement closed
                            ; key 2 (the period of absence) is retired and never reused: terms carrying
                            ; it are invalid
  ? 3 => [ hash, any ]     ; an absence-proof cMIP and its parameters (a period, a time reference), task "Absence proof"
}

keepers   = [ [+ hash], [ 0 ] / [ 1, uint ] / [ 2, [+ hash] ] ]
                                                    ; keeper operators, and the rule for what counts as recorded:
                                                    ; [0] every keeper named; [1, k] at least k of them (any one: [1, 1]),
                                                    ; k from 1 to their number; [2, operators] each of these, none twice,
                                                    ; every one among those named; operators none twice; terms whose
                                                    ; keepers' rule does not fit them are invalid (no rule by share)
rule      = [ 0 ] / [ 1, uint ] / [ 2, [+ hash] ]   ; all parties / threshold / named parties
          / [ 3, stake: uint, share: uint ]         ; those counted, holding together at least this share, in millionths,
                                                    ; of this stake, by its index in field 7
          / [ 4, [2* rule] ]                        ; all of these rules, each counted as it would be alone
          / [ 5, stake: uint, share: uint ]         ; the holders of this stake, members or not, holding together at
                                                    ; least this share of it ; field 18 only
holders-rule = [ 0 ]                                ; every holder of the stake (the default)
             / [ 3, share: uint ]                   ; holders together holding at least this share of the stake, in
                                                    ; millionths
stakes    = [+ [ object: who, [+ [ holder: who, share: uint ]] ] ]
                                                    ; shares in millionths, summing to 1,000,000; leftovers by
                                                    ; largest remainder, the order of holders deciding nothing
                                                    ; each stake is identified by its index here;
                                                    ; an object once; holders distinct, each share above zero;
                                                    ; the object the collective itself: a share of all its
                                                    ; income ; the object a vow's name: held by the
                                                    ; vow's signer (null for the collective's own vow) in
                                                    ; the version first writing it (rule 14)
who       = hash / null                             ; an identity, or null: this collective, only in a collective's terms

area = {
  0 => tstr,                ; its name, as canonical text, shown before signing
  1 => [* hash],            ; holders: parties; empty only in a clone, the area then standing frozen
  2 => uint,                ; how many of the holders decide together: at least 1, and at most the number of holders,
                            ; except in a clone that keeps the number the parent's entry for that area has, which
                            ; may then exceed the holders, counted as rule 65 says
  ? 3 => [+ kind],          ; the acts of the collective it reaches
  ? 4 => [+ field-ref],     ; the operational fields of the terms it reaches
  5 => uint,                ; its id: permanent, unique among the agreement's areas, never reused
  ? 6 => [ cmip: hash, any ] ; a cMIP judging limits on its holders' own keys, and its parameters
                            ; (rule 60)
}                           ; an area reaches at least one act kind or one field; it is identified by its id, never
                            ; by its place in field 19
kind = [ 0, layer: uint ]                 ; every act of this layer ("Layer"): 0 Identity, 1 Envelopes and Text,
                                          ; 2 Money, 3 Agreements, 4 Development
     / [ 1, spec: hash, type: uint ]      ; every act of this type in this specification
field-ref = [ 0, field: uint ]            ; a whole operational field: 7 stakes, 8 split plan, 17 refund terms
          / [ 1, task: uint ]             ; one entry of field 2: the cMIP for this task, where the task is operational

mark  = [+ [ power, signers: [+ hash] ] ] ; each change rule claimed, and the parties whose signatures meet it;
                                          ; ascending by the power's deterministic encoding, no power twice;
                                          ; each power's signers ascending by hash, none twice
power = [ 0 ]                             ; the parent's constitutional change rule (its field 18, else every party)
      / [ 1 ]                             ; the parent's clone rule (its field 5)
      / [ 2, area: uint ]                 ; the power over the parent's area of this id
      / [ 3, party: hash ]                ; the succession plan of this party, signed in advance by every voice
                                          ; (rule 83); its signers are the seat successors entering
      / [ 4 ]                             ; the judicial tier's rule: every party of the parent whose voice remains
                                          ; (rule 67)
      / [ 5, stake: uint ]                ; the holders' rule of the parent's stake of this index (field 30): its
                                          ; signers the holders whose signatures meet it, a proxy's vote naming
                                          ; its holder (rules 94, 97)
```

*A rule's threshold is between 1 and the number of parties; named parties are parties. In a key grammar, members of a holding are parties.*

**Rules by share, and "all of".** A rule by share (`[3, stake, share]`) and "all of" (`[4, rules]`) are for a collective's change rules only: its clone rule (field 5) and its constitutional change rule (field 18). A rule among a stake's holders (`[5, stake, share]`) is for field 18 only, alone or within "all of". The stake each names is one the terms' own field 7 defines; each share is above zero and at most 1,000,000; "all of" lists two rules or more, none of them itself "all of". Terms breaking any of these are invalid, and so is a deal whose field 5 is not `[0]`. *Weights are read from the stakes, never written as free numbers: a clone cannot raise a weight without changing a stake, which rule 8 guards.*

**The holders' rules** (field 30) appear only in terms carrying a key grammar: terms of a deal carrying field 30 are invalid. Each entry names a stake the terms' field 7 defines, once; its share is above zero and at most 1,000,000. An entry may carry a standing waiver of pre-emption, `true` as its third element (rule 96), and field 32 written `[0]` is valid only where the parent's entry for the stake issued carries it. A clone whose entry for a stake differs from its parent's is a draft until every holder of that stake signs it (rule 45). **Field 31** appears only in a deal's clone, and names stake transfers of stakes its parent defines. **Field 32** appears only in a collective's clone, which is then an issuance and is judged as rule 96 says.

**The release rule's threshold is counted by share**: field 24 written `[1, uint]`, a number of heads, is invalid; it is written `[3, share]`, holders together holding at least this share of the stake released.

*An area's holders are distinct parties. A clone that takes a party out of field 0 takes it out of every area's holders too, and founding terms never list an area without holders. A clone that takes a holder off an area need not lower its number: a number left above the holders that remain is met by all of them (rule 65), and the refit sets it again. A clone that changes an area's number sets it within its holders.*

*The signers of each power in a mark are distinct, listed ascending by hash, so that one meaning has one encoding; a mark listing them otherwise is invalid. A threshold is written as the founders chose it; how it counts once voices are removed is rule 65.*

**A clone** is terms with a parent (field 11).

- Its inside names, in `objects`, the parent's chain and the act of that chain it follows: `[[parent, act]]`.
- **A settling version** (field 26) **also names the version it settles, as chain and predecessor**: `[[parent, act], [settled, settled]]`, so that a verifier fetching a deal's versions by their citations finds it. Terms carrying field 26 that do not are invalid.
- Founding terms name no chain.
- A clone's field 4 is a mark; founding terms' field 4 is a rule. Terms whose field 4 has the other form are invalid.

**Founding terms exist only when every party has signed them,** a deal's and a collective's alike. Founding terms whose field 4 is not `[0]` are invalid. *A person is never added to a collective without their approval: a founder who has not signed leaves the collective unfounded, and a member added later joins only by signing the clone that adds them (rule 45).*

**A deal** (terms without field 12) has every party as its clone rule too. Any deal whose field 5 is not `[0]` is invalid, and so are terms of a deal carrying field 18, 19 or 20. *The fields are kept rather than dropped so that every agreement has one format, and a deal says on its face what it needs.*

**In a deal, the absence authority is one identity**: a party, a keeper's operator or a collective, named by the clause's first form, `[0, hash]`. Terms of a deal whose abandonment clause names a threshold of the other parties, `[1, uint]`, are invalid, whatever its number. *A deal has no sequence of its own in which several signatures could be placed against its line. Parties who need to decide absence together form a collective, which orders its own acts on its own sequence, and name it as the authority: "a collective made of collectives".*

**A judge never handles what it judges.** This holds in every agreement, a deal's as a collective's.

A judge here is a specification that is either:

- named in field 2 for a judicial task (9 condition evaluation, 10 time reference, 11 anchoring), or in the abandonment clause's key 3 for task 14 (absence proof); or
- named in field 6 as the time reference, which is a naming for task 10.

A judge is named for no other task, judicial or not, and is not named as an extension (field 15). Terms that do either are invalid, founding terms and clones alike, whatever a clone's mark says.

*It removes a slight conflict of interest. An extension fills no task, but it adds rules to acts; one that is also a judge would handle what it judges, so every way a specification can be named is covered. In a deal every party signs every cMIP, so no area can slip a judge in; the reason is the judge's, not the areas'. It also keeps judges away from areas: no specification serves a judicial task and an area's task at once, so no area's holders ever have a say over a judge, and no change to a judge reaches an area's acts.*

**The chain of judgment** (field 21) says who takes over from a judge the terms name. The judges it may follow are:

- a judicial task the terms name a specification for;
- an identity the terms name as a keeper's operator, an arbitrator or verifier, or the abandonment authority (its first form).

The checks:

- Each judge has one link at most, the links ascending by the judge's deterministic encoding.
- Those that take over from a judge are distinct, and none is the judge itself.
- **A specification that takes over from a judicial task is a judge too, so it is named nowhere else in the terms**: for no task, as no extension, and in no other link.
- Each link names a period above zero: the time the one before it has, once asked, to act, measured on the agreement's time reference. After it, the next may act.
- Terms whose chain follows a judge that can stay silent (an identity) name a time reference (field 6, or field 2's task 10). That time reference is itself a judge, which may have a chain of its own.

Terms breaking any of these are invalid. How a chain passes is rule 35.

*A period is compulsory for every link, a specification's included, though a specification always answers.*

*The chain is signed with the terms, by everyone: in a deal by every party, in a collective as part of the judicial tier (rule 67). So nobody signs again when a judge fails.*

*The split service is not in the chain: its backups are the split cMIP's, in field 29, below. A chain of judgment has stages; a chain of backups has attempts.*

**The split service's backups** (field 29; rule 17).

*How it runs most of the time:* an agreement names one split service (field 14) and no backups; every payment goes to it.

Where the owners choose backups, **field 29 lists them, in order, after field 14's service, with what counts as a failed attempt.** These are fields of the split cMIP the agreement names (field 2's task 8, or the split plan's key 1; where both name one, the same), signed with the terms by everyone who signs them. The checks:

- Field 29 appears only with field 14, and only where the terms name a split cMIP.
- In a collective, each backup is one grant of the collective.
- In a deal, each backup is a group of grants, one per payee, as many as field 14 lists, each the payee's own grant naming "this agreement" by null like field 14's.
- Every grant that field 14 and field 29 list is listed once.

Terms breaking any of these are invalid. The core never reads key 1: what counts as a failed attempt is the split cMIP's. *Attempts, not stages:* no period and no time reference are needed (the chain of judgment's stages and deadlines do not apply to backups).

**The departed members entry** (field 22) appears only in terms carrying a key grammar. It lists identities:

- distinct;
- none of them a party;
- each a holder of a share of the stake in the collective itself (field 7).

Terms breaking any of these are invalid. The entry records only that each is departed, without voice (rule 68); the stakes decide what every holder is paid. *They are stakeholders: the stakes still show them, and the organizational part files the identity as departed.*

**Stakes** (field 7). Each stake:

- names its object once;
- has distinct holders, each share above zero;
- has shares summing to 1,000,000 (rule 9).

Terms breaking any of these are invalid.

**Null names this collective.** A stake whose object is null is a share of all the collective's income. A holder written null is the collective itself, holding that stake. Only terms carrying a key grammar write null. A stake in the collective held by the collective itself is invalid. Founding terms so carry the members' stakes in the collective, and the stakes of the works it owns, from its birth.

*The mechanic is reused: the collective's identity is its genesis act, which declares the founding agreement, so the founding terms could not name it by its hash. In its own terms, founding or clone, the collective is written null, never by its hash, so that one meaning has one encoding; a clone writing the collective's own hash is invalid where a verifier holds its genesis.*

**Forked from** (field 23) appears only in the founding terms of a collective (with field 12, without field 11). It holds the original collective's identity. It is a back-link for readers and decides nothing: the fork act names its successors (rule 91). It is no parent: the founding terms are signed by every founder, as any founding terms are. Terms breaking this are invalid. *A successor is founded before the fork act exists, so it cannot name the act; it names the collective it leaves, rather than its agreement, which may still change before the fork.*

**The release rule** (field 24) says which of a stake's holders must sign its release to the public domain, a rule counted among them (rule 55). It is set in an agreement's founding terms; absent, every holder.

**A clone every owner signs may change it.** A clone whose field 24 differs from its parent's is a draft until every holder of every stake the parent defines has signed it (rule 45). This is besides the power its mark names: **the ordinary clone rule**, field 24 lying in no area (rule 64).

*Freedom of making mistakes and fixing them.* The owners are the holders the parent's field 7 names, a holder written null being the collective, whose consent the mark already carries.

**Field 25 is withdrawn.** Terms carrying field 25 are invalid, and the number is never reused. **Relays are transport**: where a collective's clients publish and look first is client conformance, or a cMIP's (the relay transport cMIP), never the core (rule 57).

*Field 25 named a collective's relays, and made an act done only once it was on one of them. That made an act's validity depend on infrastructure: a vanished or hostile operator could freeze a collective, even its decision to change relays; it gave leverage over the collective; it allowed omission to readers checking one relay; and it failed offline verifiers. That crosses MOR's line that an act is valid by its signatures and content wherever held. The relays were a witness of time; the two chains and the tie rule answer "before" without them.*

**An agreement has one time reference.** Where terms carry both field 6 and a cMIP for task 10 in field 2, the two name the same specification, field 6 adding its parameters. Terms where they differ are invalid, founding terms and clones alike. *One check: terms that use either place alone stay valid.*

**Areas** (field 19) appear only in terms that carry a key grammar (field 12) (scope). Area words (field 20) appear only for an area that exists.

**No two areas reach the same act or the same field.** Two kinds overlap when some act could belong to both, as computed from the six MIR hashes and the terms' own fields 2 and 15 ("Layer"). So:

- `[0, layer]` overlaps `[1, spec, type]` when `spec` is a MIR of that layer, a cMIP the terms name for a task of that layer, or, for Development, an extension they name;
- an area reaching a whole layer overlaps a field reference to a task of that layer, since that area already decides that task's cMIP (rule 63).

The one exception is a specification the terms name for tasks of two layers or more, never a judicial task. Its acts belong to each of those layers, and are reached by the area reaching each, which then decide them together (rule 60).

A field reference names an operational field or task ("Tiers").

Genesis and rotations are never in an area's reach: the key grammar governs them. Nor are records (type 17), judged by the clone they name. An area whose kind names a genesis or a rotation (Identity's chain acts) or a record is invalid, never read as reaching nothing.

Terms breaking any of these are invalid.

**An area is followed by its id.** The ids of an agreement's areas (area key 5) are distinct. An area of a clone is the parent's area with the same id, wherever it sits in field 19. Grants (field 5), area words (field 20), steppings down (resignation field 1), marks (`[2, area]`) and a refit (rule 71) name an area by its id. A clone may drop an area, and adds one only under an id that no earlier version of the agreement, back to its founding terms, gave to an area. Terms breaking either are invalid. *An area keeps its id through any redraw, so removing or reordering areas never moves a grant, words or a refit onto another area.*

*Which layer an act belongs to is computed from its own `spec` field, the six MIR hashes and the collective's own terms, never from a label the cMIP's author chose. A collective that gives Money to some members gives them the Money MIR's own acts, its receipts and claims on every rail among them, and the acts of every Money cMIP it adopted, by its own list. A new rail Module needs no clone at all: the collective's payee pointer, an act of the area reaching Money, or its vault names it, and a receipt counts only on a rail Module so named (Money rule 4).*

*An act of a specification the terms name nowhere falls in no area. In a collective with areas it counts for nothing, unless a `[1, spec, type]` kind of some area names it, since otherwise an unlisted specification would be a way around every area.*

*A specification named for tasks of two layers belongs to both. Where the two layers are reached by different areas, each area's holders must sign its acts, as for a clone across two areas, and naming it for the second layer needs both areas' power (rule 63), so neither area's holders can bind the other's acts alone.*

*Stated cost:* *a collective cannot give its releases to one member and its other publications to another by layer alone, since both are Envelopes publications; a `[1, spec, type]` kind still names a single type.*

### Signature (type 1)

*In short: a signature act is one identity's consent to an act that needs it.*

```cddl
signature-payload = {
  0 => hash,                ; the act signed: terms, a clone, an act of a collective an area reaches, a grant within an area,
                            ; or an abandonment declaration whose authority is a threshold of the other parties;
                            ; an issuance offer, taking up its proportion (type 26); a collective's
                            ; stake transfer or release of a work, consenting as an owner
  ? 1 => signing-key        ; on a collective's founding or clone, the signer's own key, in
                            ; Identity's form [ scheme, key ], made by its app as it signed (rule 73)
}
```

A signature naming a key on any act but a collective's founding or clone names no member key: an act bound to it counts for nothing.

The inside names, in `objects`, the act signed as both chain and predecessor: `[[signed, signed]]`. *A signature follows the act it signs.*

Signatures naming one act are parallel consents to it, never a fork among themselves: rule 6's forks are between acts that change an agreement.

A signature counts only while it is valid under Identity's rules. The exception is a signature a collective placed before its line registering its signer's rotation (rule 6), and every act a line of a collective places (rule 6, "Made before, made after", 4). A signature on a deal's version that a later act relies on keeps the version complete (rule 47).

### Standing offer (type 6)

*In short: a standing offer sells something at one price to whoever pays, until it is withdrawn or its time runs out. A lone seller makes it alone; co-owners make it under their agreement.*


```cddl
offer-payload = {
  ? 0 => hash,          ; under: the co-owners' agreement (a version) it is made under; present only where co-owners stand
                        ; behind it ; absent: a lone seller's offer, itself the agreement with the buyer
  1 => [* sold],        ; what it sells, ascending by deterministic encoding, none twice; empty only in a later version,
                        ; which withdraws the offer
  2 => amount,          ; the price, one amount
  ? 3 => hash / 0,      ; who is paid: a payee, or 0, the stakes, each holder's own pointer (rule 16); required
                        ; with field 0 ; absent with no field 0: the signer
  ? 4 => tstr,          ; the offer's words, canonical text; required where it names a vow
  ? 5 => any,           ; until: a point on the time reference after which it takes no payment
  ? 6 => [hash, any],   ; a lone seller's own time reference
  ? 7 => refund-terms   ; a lone seller's own refund terms
}
sold = [ 0, publication: hash ]       ; a publication (Envelopes type 0), by its act id
     / [ 2, [ cmip: hash, any ] ]     ; a proven offer: a delivery whose proof any verifier checks from data alone, under a
                                      ; cMIP and its parameters (widened from access to bytes; a stream's key alone)
     / [ 3, vow: hash ]               ; a vow (Envelopes type 5), by its name, its genesis id
```

`[ 1, work ]` (any publication carrying a work) is not defined.

Its `objects`: a later version names `[first, previous]` first. Where field 0 is present, they name the agreement as `[agreement, agreement]`; a collective's offer then cites its chain.

**It counts** where:

- it is valid and public;
- under a deal, it is signed by every party;
- its field 3 is a payee the agreement's field 14 lists (in a collective, the collective), or, where the agreement names no split service, the stakes;
- a lone seller's payee is the signer, or one who accepted it by a signature act (Money rule 17);
- field 5 needs a time reference.

**A purchase following it** names its agreement (a version of its lineage), or it is no purchase. A payment following a lone seller's offer names no claim and is accepted under the offer's terms; one the time reference places past field 5 is owed back.

A split service's grant key may receipt under an offer that pays its grantor under the grantor's own claim.

**A relay's delivery record counts on a purchase naming an offer** where its object is the media of a publication the offer sells (any of them).

**An offer naming a vow** (`[ 3, vow ]`) sells it with its words, under whatever agreement claims it, or as a lone seller's offer. It names the vow by its genesis id, never a later version. An act this verifier holds that is not a vow, or is a later version, keeps the offer from counting. A vow made void by its signer's rotation does not: the name others wrote down stands. *What the vow promises, how it is delivered, quantities and seats, and what it became are the cMIPs'.*

**A service sold under a cMIP's terms.** *Paying for a service is an agreement.* A lone seller's offer selling access under a cMIP (`[ 2, [ cmip, terms ] ]`) carries that cMIP's terms as its own. The access form is the form of every proven offer, any delivery whose proof a verifier checks from data alone (rule 11); the verifier checks the proof, never the cMIP's say-so. Their deadlines are points on the offer's own time reference (field 6). What a default owes (a refund of the price, to the payer the payment committed to) is judged under them, by a client that holds that cMIP. Its refund terms (field 7) end a refund unpaid past their point.

A pooled anchoring service sells so, a proven offer: its tiers, prices or price scheme, and deadlines are its cMIP's terms, and its tickets and batch publications its acts under the offer. Money carries the refund's payment and shows it repaid (Money rules 19 and 17). *Money says how money moves and that it has moved; Agreements say what must happen, in return or in consequence.*

**Refund terms (terms field 17, an offer's field 7).** `refund-terms = { 0 => [ 0, point: any ] }`: the refund is claimable until this point on the time reference. Terms carrying it name a time reference (rule 33). Past the point, the refund is ended (rule 11).

### Work claim (type 3)

*In short: a work claim ties a work, by its hash, to its creators.*

```cddl
work-claim-payload = {
  0 => hash,            ; the work hash
  1 => [+ hash],        ; the creators, ascending, none twice, the signer (who opens it) among them
  ? 2 => [hash, any]    ; a work-claim cMIP and what it produces (a pre-publication commitment)
}
```

It is bound when every other creator listed has a signature act naming it. It counts only public or addressed to every creator. It carries no roles. Where no agreement writes shares, the default holder is the creator who opened it.

### Stake transfer (type 4)

*In short: a holder hands all or part of a stake to a new holder.*

```cddl
transfer-payload = {
  0 => hash,            ; the version of the agreement defining the stake
  1 => uint,            ; the stake, by its index there
  2 => hash,            ; to: the new holder
  3 => uint,            ; how much, in millionths of the stake: above zero, at most what the seller still holds at its place on its own line
  ? 4 => [+ hash]       ; what the seller took for it: a payment made before it; read only to find what an over-sale owes back
}
```

Its `objects` name the agreement as `[agreement, agreement]`, never a fork.

It takes effect once the seller's chain signature names it and the buyer's signature act completes it (rule 10), where it fits what the seller still held at that place. An over-sale confers nothing. **The buyer's signature act is read as a closing reads another identity's receipt** ("Closing (type 20)"): for a collective's owners' matters it counts where the collective's line placed the transfer (rule 94), and is kept there whatever the buyer's later rotation disowns, shown as a dispute; elsewhere it is judged as Identity judges a signer's own act (Identity rule 34).

**A collective's transfer of a stake it holds in a work** takes effect only once, besides the above, the owners of the collective's stake in itself who have no say consented, by that stake's holders' rule or each such holder signing it by a signature act naming the transfer, placed by the collective's line (rule 95). **For a collective's owners' matters, a transfer counts only where the collective's line placed it** (rule 94): an act of the collective acknowledging it, or a record naming it in field 3.

Across versions, a stake is followed by its object. A payment for it fulfils it (the receipt's field 5, the claim's field 4) or is named in its field 4.

### Issuance offer (type 26)

*In short: a collective offers each owner of a stake its own proportion of new shares, on the same terms, for a period; what is not taken up goes to the new owners it names.*

```cddl
issuance-offer-payload = {
  0 => hash,                ; the version of the collective's agreement defining the stake, in force where the offer is made
  1 => uint,                ; the stake, by its index there
  2 => uint,                ; the size of the issuance: the new shares' part of the stake after it, in millionths, above zero
                            ; and below 1,000,000; every existing share is scaled by what is left
  3 => [+ [ holder: hash, share: uint ]],
                            ; the new holders, who take what the existing holders do not take up, and their parts of it,
                            ; in millionths summing to 1,000,000; ascending by holder, none twice
  4 => any,                 ; the period: a point on the agreement's time reference, until which each holder it reached may
                            ; take up its proportion
  ? 5 => amount,            ; the price of the whole issuance, each holder taking up paying its proportion of it; absent: none
  ? 6 => tstr,              ; the offer's words, canonical text: its terms, the same for every holder and new holder
  ? 7 => [ hash, any ]      ; a cMIP and its parameters, offering more as a term of the issuance: staged issuance,
                            ; vesting, other terms, never less than this offer
}
```

Signed by the collective, with its own key: an act in its name, done once sealed to every member and on its chain (rule 57), counting as rule 60 says. Its inside names the agreement as chain and predecessor, `[[agreement, agreement]]`, followed by its chain citations, as an action cites (rule 58).

**It reaches a holder** where it is public, or sealed to that holder among its recipients (Envelopes, `to`), as a declaration reaches the member it names; a holder that is itself an agreement, only where it is public (rule 57). Its period runs for a holder only where it reached that holder. Which holders it must reach, and their proportions, is the roster where the offer is placed on the collective's chain (rule 94).

**A holder takes up its proportion** by a signature act naming the offer (type 1), before the point field 4 names, as the time reference judges it, placed by the collective's line or not (rule 96), with its payment where field 5 names a price: a Money receipt or claim fulfilling the take-up (the receipt's field 5, the claim's field 4). A take-up takes the holder's whole proportion; a cMIP of field 7 may offer more.

**It counts** only under terms naming a time reference (field 6, or field 2's task 10), in a collective. Whether its point has passed is the time reference's answer, which a verifier states as it states every time reference's answer: the core reads no time reference (rule 33).

The issuance that follows it is a clone naming it in terms field 32, judged as rule 96 says.

### Proxy (type 27) and revocation of a proxy (type 28)

*In short: a holder hands its vote on the owners' matters of one stake to a proxy, for one version or until revoked; a revocation takes it back.*

```cddl
proxy-payload = {
  0 => hash,                ; the version of the collective's agreement defining the stake
  1 => uint,                ; the stake, by its index there
  2 => hash,                ; the proxy: the identity whose signature act on a version counts as the holder's vote
  ? 3 => hash               ; scoped: the version, one owners' matter, it votes on; absent: standing, every owners'
                            ; matter on the stake until revoked or replaced
}
proxy-revocation-payload = {
  0 => hash                 ; the proxy (type 27) it revokes
}
```

A proxy is signed by a holder of the stake, with its own key. Its inside names the agreement as chain and predecessor, `[[agreement, agreement]]`, as a resignation does, and it is never a fork. A revocation is signed by the holder who signed the proxy, and names the proxy as chain and predecessor, `[[proxy, proxy]]`. Each counts only public, or sealed to the collective among its recipients, and only where the collective's line placed it: an act of the collective acknowledging it, or a record naming it in field 3 (rule 97).

A proxy whose signer holds nothing of the stake at its placement, or that names its signer as the proxy, counts for nothing. A proxy act signed by anyone but the holder, a proxy among them, counts for nothing: no sub-proxies (rule 97).

A later proxy of the same holder on the same stake, placed by the collective's line, replaces the earlier one from its placement on.

### Split plan and split (type 8)

*In short: the split plan (terms field 8) says how money reaching the owners is divided; a split (type 8) is the split service's act dividing one payment, payout by payout, which anyone can check against the plan. Fees are their own terms field, 28.*

```cddl
split-plan = {                ; terms field 8
  0 => [+ share-rule],      ; stakes, then roles, then named receivers, then metric shares; every share but a
                            ; stake's is named in key 4, off the top
  1 => hash,                ; split cMIP, which declares the remainder rule and computes the amounts
  ; 2 is not used: fees are terms field 28, their own field
  ? 3 => unfilled,          ; what happens to a role share nobody fills; absent: [ 0 ], to the stakes
  ? 4 => [+ uint],          ; indexes of shares taken off the top, before the rest is divided; the rest goes to
                            ; the stakes alone, by their parts
  ? 5 => uint,              ; who bears payout rail fees: 0 each receiver (default), 1 the split service
  ? 6 => amount / uint,     ; maximum fee per payout the service may deduct: absolute, or in millionths of the payout;
                            ; absent: no cap, flagged before signing (rule 25)
  ? 7 => any,               ; the split cMIP's parameters (a waterfall's thresholds)
  ? 8 => [+ metric]         ; the metrics the plan divides by (rule 29)
}
share-rule = [ 0, stake: uint, part: uint ]         ; a stake, by its index in field 7; paid to its current holders
           / [ 1, role: tstr, part: uint ]          ; a role, by its name, filled at payment time
           / [ 2, receiver: hash / null, part: uint ] ; a named receiver that holds no stake (a service, a position);
                                                    ; null: this collective, its own money, paid to its vault, in a
                                                    ; collective's terms only, off the top (rule 22)
           / [ 3, metric: uint, part: uint ]        ; a metric share, by its index in field 8
metric     = [ module: hash, measurer: hash, ? params: any ]   ; the measurer named in advance (rule 29)
unfilled   = [ 0 ] / [ 1 ] / [ 2, party: hash ]     ; to the stakes pro rata / held open / to a named party
fees       = [+ fee]                                ; terms field 28
fee        = [ module: hash, part: uint, ? scope ]  ; borne by every stake, off the top (rule 28)
scope      = [ 0, publication: hash ]               ; payments for this publication; absent: every payment
           / [ 1, rail: hash ]                      ; payments by this rail Module

split-payload = {
  0 => hash,                ; the receipt or payer's claim that brought the money to the owners' payee pointer
  1 => [+ payout],          ; each payout, summing exactly to the amount received
  2 => hash,                ; the split cMIP used
  3 => hash,                ; the owners' agreement whose stakes it pays
  ? 4 => [+ [ stake: uint, [+ [ holder: hash, count: uint ]] ]],  ; the running count of leftover units (rule 9)
  ? 5 => uint,              ; its number: one numbering across every split the service makes under a deal, any branch, from 1
  ? 6 => [+ hash],          ; the Modules and cMIPs it ran under (rule 28): the service's statement, never what decides
  ? 7 => [+ hash],          ; the metric records it divides by (rule 29)
  ? 8 => [+ [ stake: uint, transfer: hash ]]   ; per stake, the latest transfer it follows: the service's statement
}
payout = {
  0 => hash,                ; the receiver
  1 => uint,                ; the amount, in the smallest part of the receipt's unit
  ? 2 => uint,              ; the stake it pays, by its index in the agreement's field 7
  ? 3 => tstr,              ; the role it fills
  ? 4 => hash,              ; the evidence for that role: an act signed by someone other than the split service and the payee,
                            ; or, for a rail Module's role share, the receipt or claim naming that rail Module, where the
                            ; payee's own pointer or vault names it (rule 22); only with key 3
  ? 5 => hash,              ; a fee: the module whose fee it pays, its receiver being key 0 ; never with key 2 or 3
  ? 6 => uint,              ; the rail fee deducted, within the plan's maximum
  ? 7 => uint               ; the metric share it pays, by its index in plan field 0 (rule 29); never with keys 2, 3 or 5
}
```

**Fees (terms field 28).** Fees are their own terms field, not a key of the plan; their form is `fees` above. An area may reach field 28 apart from field 8 (`field-ref` `[0, 28]`).

**Every payout matches its stake exactly** (rule 27). For each stake a split pays, each holder receives its share of what the split pays that stake, exactly as rule 9 sends each leftover unit under the default rule, and nobody else receives on it.

A holder that is another agreement receives at the one pointer that agreement names for money arriving from above (rule 15). The plan never divides that holder's share among the next agreement's owners.

Every fee is taken before the stakes divide, so it falls alike on every stake. Any deviation, either way, breaks the plan, and is shown so. *No holder is paid a whole smallest unit or more below its exact share, and none above it by as many units as the stake has holders or more, which leaves room for the leftovers of rounding going to one holder (rule 21).*

Each payout is then paid as a simple Money payment, with its own receipt naming the split. A fee taken off the top, and an app share for the client that brought the sale (a role share), are both expressed in the plan.

**What a verifier checks against the plan:**

- a role payout fills a role the plan names; several fillers of one role are paid equally, within one unit;
- a named receiver's payout is one the plan names; the collective's own part (a receiver written null) names the collective as its receiver, and is paid to an entry of the collective's vault, never its flow pointer (rule 22);
- a fee payout is a fee entry's, and every fee an entry makes owed on the payment is paid;
- a metric payout names a metric share; its receiver is one the plan already pays, never the metric's measurer; the measurer is not the split service; and the split names a record of the metric's Module signed by the measurer (the record's format is the Module's);
- a payout on a stake whose holder is another agreement names, as its receiver, the one pointer that agreement designates for payments (rule 15): field 14's one grantee, or a lone owner; or, for that payment, a backup of its field 29 once every attempt before it failed, as its split cMIP defines failure (rule 17).

A holder designating none cannot be paid, and any payout on its holding to anyone else is a mismatch: **never an address the owners did not sign.**

How much each share gets is the split cMIP's: a verifier without it shows the amounts as unknown, never as wrong.

**A split is delivered to every holder it pays**: each receiver of a payout naming a stake is among its recipients (Envelopes, `to`), unless the split is public, a public split being delivered to everyone. Every fee it takes is a payout naming its module and its receiver. *The core cannot know who owns a service, so it never hides a fee: every holder sees what was taken before their share, and by whom.*

### Grant (type 9) and revocation (type 10)

*In short: a grant lets a grantee act for the grantor, within limits, with a key of the grantee's own that is scoped to the grant. A revocation removes that key.*

```cddl
grant-payload = {
  0 => hash,                ; grantee
  1 => uint,                ; scope: 0 sign new deals, 1 manage named existing ones, 2 act for an identity (post, publish, spend)
  ? 2 => [+ hash] / null,   ; the agreements concerned, for scope 1; null: this agreement, the deal whose terms list the grant
                            ; in field 14 ; null only with scope 1, never with field 5, 7 or 8
  ? 3 => any,               ; limits: per-deal cap, total, spending, as the grant's cMIP defines
  ? 4 => hash,              ; the cMIP defining the limits
  ? 5 => uint,              ; for a collective: the area whose power issues it, by its id in the agreement in force at the grant's binding
  ? 6 => [+ kind],          ; the acts it reaches; required with field 5, and within that area's reach
  ? 7 => hash,              ; a reinstatement: the ended grant it reinstates, fields 0 to 6 repeating that grant's (rule 71); only with field 5
  ? 8 => null,              ; the grantor is this collective, the one whose founding terms name the grant ; never with field 5 or 7
  9 => signing-key          ; the grant key, in Identity's form [ scheme, key ]: made and held by the grantee, scoped to this grant
}

revocation-payload = {
  0 => hash                 ; the grant whose key it removes
}
```

**The grant key** (field 9). The grant names the public part of a signing key the grantee made and keeps (the grantee makes it, so nobody else holds a key that acts as the grantee's within the collective). **The grant counts only once its grantee signed a signature act naming it** (type 1), accepting it, a grant by founding terms included. **For a collective, the grantee's acceptance is placed as a member's signature is** ("Made before, made after", 2): once an act of the collective acknowledges it, it keeps for the collective the effect that placement gave it, whatever the grantee's later rotation disowns, shown as a dispute; an acceptance no act of the collective placed is judged by Identity alone.

An act signed with the grant key names the grant as its `binding` (Identity: a scoped key, which this MIR judges), and its signer is the collective. It is the collective's own act, on the grantee's strand of the collective's actions chain, which cites the grant in `objects` as the decision it acts under (rule 58). It is judged at the latest link of the collective's identity chain among its grant and the decisions it cites.

A publication a grantee makes with its grant key is the grantor's own, signed by it, so payment reaches the grantor whatever client pays; it needs no `for` field.

An act of the grant key is backed (rule 80) only within the grant's reach (field 6, or, for a grant naming no area, acts no area reaches), only within the limits its limits cMIP sets (fields 3 and 4: an expiry among them, judged against a time reference), and only while the grant has not ended for it. It is never backed for a decision: a grant key signs no record, grant or revocation, and no act of it places a member's signature or adopts another act for the collective.

A grant survives the collective's rotations. It ends only by a revocation, a departure emptying its area, a fork or a closing.

**A grant from an area** (field 5) is signed by the collective and completed by signature acts of the area's holders (rule 74). Each kind in field 6 must lie within the area's reach: every act it could reach, computed as for overlapping kinds from the agreement in force ("Areas"), is an act the area reaches. Otherwise the grant is invalid.

**A reinstatement** (field 7) is the exception, and **is a clone of the ended grant.** Its fields 0 to 6 must repeat those of the grant it names, which must be a grant within the area of the same id; one whose fields differ is invalid. It backs acts only within the refitted area's reach (rule 71). Its key (field 9) is its own, the grantee accepts it again, and it takes nothing on.

**A grant of this collective by its founding terms** (field 8) is written before the collective exists, so founding terms can name it (field 14, field 29). Its grantor is the collective whose founding terms name it. **It is carried by the founding terms, which every founder signs**: signed by a founder, the core not judging which, it counts through them. The collective's key takes over from then on, for every later grant and revocation. The grantee accepts it by its own signature act. It names no area, and reinstates nothing.

**A payee's grant in a deal.** Where a deal names a split service, each payee grants the service a grant key of its own, written in the deal's terms. The grant names "this agreement" by null (field 2, scope 1), as founding terms name "this collective". So it is written before the deal, and the deal names it: **field 14 lists one grant per payee.**

The grant is a person's grant (rule 73), signed by the payee with its own key, and public. It counts only through the deal: once a deal its grantor signed lists it in field 14 and exists, every party having signed it: signing the deal signs them. Before that, it backs nothing. The split service signs to accept each grant, as every grantee does.

Every grant field 14 lists names the same grantee, the one split service, and is signed by a distinct party. A deal breaking this carries none of them.

**A backup split service holds grants the same way.** Field 29 lists them, one per payee, grouped by service. Each is the payee's own grant naming "this agreement" by null, signed by the payee and accepted by that service, counting once the deal exists, as field 14's do. Every group is signed by the payees that sign field 14's grants, one each. Each group names a service of its own, never field 14's. A deal breaking this carries none of its grants, field 14's included; a verifier not holding every grant it lists cannot tell, and answers unknown. *Which service receives a payment is the split cMIP's, attempt by attempt (rule 17); a backup's grant keys count from the deal's existence, as in a collective, and both services' pointers pass the pointer check (rule 16).*

**A payee's grant counts once a deal its grantor signed lists it and exists, every party having signed.** "This agreement" is the deal with the versions it clones, and a grant listed by two deals its grantor signed serves each. *The mechanic is the collective's founding grant, and the service is no party to the deal, so leaving it stays a revocation, never a renegotiation.*

**Each payee can revoke its own grant** (type 10, an everyday act, public, rule 73); the other payees' grants stand.

**What a split service's grant key may sign.** One rule holds for every split service, whoever granted it, a person or a collective. A split service's grant is a grant field 14 names, or one field 29 names for a backup, in any version of the agreement: a payee's grant in a deal, or a collective's grant.

**Its key signs only receipts for money coming in under the grantor's own claims and offers.** That is, a Money receipt (type 2) whose payee (field 3) is the grantor, and which is a purchase naming the grantor's own claim (field 9: in a deal, the deal or a version of it; in a collective, its agreement or a version of it), or a payment following it (field 5).

It never signs a receipt whose payer (field 2) is the service, nor a payout the grantor is owed (a receipt carrying a batch, field 8, or naming a split in field 5 or in `objects`), and no other act, save one exception.

The exception is a layer's incoming share with the rail's proof behind it. Where the rail pays an account the grantor holds (a bank transfer to the owner), its grant to the next layer's split service lets that service sign for the share on the grantor's behalf: a receipt whose payee is the grantor, naming another identity's split that pays the service as the receiver of a share held by an agreement of the grantor's naming it (rule 15). **The rail's proof is required on that receipt** (the rail's answer, valid). Never a payout of the service's own split, nor such a receipt without the rail's proof: there the limit stands whole. *The owner can grant what it wishes to whom it chooses. The limit stops a service from forging the payee's own receipt for its own split's payouts; with the rail's proof behind the signature, the forgery is impossible, so the limit stays only where no rail proof stands behind it. One rule for all.*

**A grantor may scope its grant more narrowly still**: the kinds it names (field 6), and for a collective the area's reach, still apply before this limit.

"The service" is every service the grantor's agreement names for the split service, its backups included, since money from any of them to the grantor is a payout; a grant to the same identity that neither field 14 nor field 29 names is no split service's grant, and reaches what its kinds name, the grantor's choice, in plain view.

*Example: a label owns a quarter of a film; the film's service owes the label 40. The label's own split service, holding the label's grant key, cannot sign "the label received 40": that is a payout the label is owed, under another agreement's claim. The label signs its own receipt once the 40 arrives.* *Whoever uses a grant key still signs from under its own identity, by the grant key.*

So a payout without the payee's own receipt stays the service's open obligation (rule 30): the service cannot sign "the payee received its payout" in the payee's name. The grant names the grantee's identity, which signed to accept it, so every act of the key is traceable to who used it.

*A payout is a receipt carrying a batch or naming a split; "payments on its offer" are receipts following the deal itself, or purchases of its work, which name its claim anyway; a receipt received by anyone but the grantor is never backed.*

**Revocation (type 10).** A revocation is a decision of the grantor, signed with its own key and citing the collective's chain as an action does. It names the grant whose key it removes, never a place on the grantee's strand. For a grant within an area, it is completed by that area's holders' signature acts, judged by that area alone (rule 74).

It is a decision ending powers: **an act signed with the grant key that the revocation's history holds was done within the grant's powers, and binds; any other, racing it or after it, is void** (the tie rule), unless another identity acknowledged it, or a keeper recorded it before recording the revocation: then it is valid and shown as disputed, and confers nothing unless the grantor adopts it (rule 76).

### Request to a judge (type 25)

*In short: a party asks the judge its terms name to decide a question about an act.*

```cddl
judge-request-payload = {
  0 => hash,            ; the version of the agreement whose terms name the judge
  1 => judge,           ; the judge asked, in the terms' `judge` form
  2 => hash             ; the question: the act it is about
}
```

Its `objects` name the version as `[agreement, agreement]`. It counts only public or sealed to the judge. It reaches each link of the chain of judgment it is public or sealed to, in order (rule 35). Standing, for a request to a judge, is that of a party who signed the version.

### Liveness (type 12)

*In short: a party's sign of life on an agreement.*

`liveness-payload = { 0 => hash }`: the agreement, named as `[agreement, agreement]`. It counts only public or addressed to the agreement's other parties. In a collective, it counts only where the collective placed it.

### Release (type 5)

*In short: a release ends the owners' claim on a work and publishes its content keys, so the work becomes public domain (rule 55).*

```cddl
release-payload = {
  0 => hash,                ; the work released, by its hash
  1 => [+ [ agreement: hash, index: uint ]],  ; the stakes in it whose claim ends: the work's owners, past and present
  ? 2 => [+ hash],          ; the work claims binding it to its creators: its creators' history
  ? 3 => [+ [ publication: hash, key: bstr .size 32 ]],  ; the content key of each publication carrying it, published
  ? 4 => [ point: any, keeper: hash ]   ; a timed release: the point on the time reference, and who delivers the keys then
}                           ; field 3 is present unless field 4 is
```

It is signed by one direct owner of the work, with its own key. Every other holder whose signature the release rule asks for adds a signature act naming it (type 1). Its inside names, in `objects`, each agreement whose stake it ends, as chain and predecessor.

**A release is public** (its content key on its outside), so that the content keys it carries are published.

A release is complete when:

- it is valid under Identity, and public;
- each stake it names is a stake of that agreement whose object is the work;
- the release rule of each such agreement in force (field 24; absent, every holder) is met among that stake's holders, a threshold counted by share;
- where a collective releases a stake it holds, the owners' consent of rule 95 is given.

**Every direct owner of the work signs, departed holders of a stake in the work included, unless the release rule says otherwise.**

A holder that is a collective signs by its own rules. Its signature (the release itself, or its signature act on it) counts only with its consent. A release, or a collective's signature on one, is reached by the areas reaching every layer a release touches, Envelopes, Money and Agreements, each such area that exists meeting its threshold (rule 60). *Two levels of ownership: the collective owns the work; its members and departed holders own shares of its income, not the work. Releasing is a business decision of the collective; it affects every holder alike. Unlike a price, it ends the claim for good: a price can be raised again, a release cannot be undone.* **And since it pays the holders of the collective's stake in itself less, by effect, it is an owners' matter** (rule 95): the release counts only once the owners of the collective's stake in itself who have no say consent, by that stake's holders' rule or each signing, placed by the collective's line.

**A timed release** names a point on the time reference of each agreement whose stake it ends, which must name one, and the identity that delivers the content keys then (field 4). The claim ends at that point, as the time reference judges it, and is undetermined where it answers undetermined. *Stated cost: the keys' delivery rests on that identity; if it vanishes, the work is released in Agreements but its key may never come out; the owners may always publish the key earlier. That identity is named for the keys alone, not as a keeper in rule 37's sense, which never receives content keys as such (rule 43).*

*Every stake in a work lives in its one claiming agreement and its transfers on that agreement's chain, so a release signed by every holder there is complete; a claim the release does not name, by another identity, stays openly contested beside it (rule 7), and the release cannot end it.*

### Negotiation message (type 18)

*In short: a negotiation message is one message in the talk between two sides that leads to a deal.*

```cddl
negotiation-payload = {
  0 => tstr,                ; the message, as canonical text (Text MIR)
  ? 1 => hash               ; format: a text format cMIP, as a text act may name one (Text MIR)
}
```

Signed by one side of the negotiation, with its own key.

A thread is identified by its first message:

- the first message names no chain in `objects`;
- every later message names, in `objects`, the thread and the message it follows, `[[thread, previous]]`, the previous message being the thread's first message or another negotiation message of the same thread.

The two sides of a thread are the signer of its first message and the first other identity to sign a message of it. **A message signed by anyone else is no part of that thread's record.**

Each message's `acks` name the latest message of the other side that its signer has received, and nothing else. A side that has received nothing from the other side yet carries no `acks`.

Its text is shown as a text act's is, plain text always available (Text rules 3 to 6).

*A text act carrying acknowledgements is invalid (Envelopes rule 18), so negotiation lives on Agreements' own layer: a reaction or a text reply can never be a negotiation message.*

*Stated cost:* a client that does not implement Agreements shows a negotiation message as unknown, never as text; a negotiation needs an Agreements client, which it needs anyway to sign the deal it leads to.

### Contest (type 14)

*In short: a contest answers an act: a party declared absent answers the declaration, or someone who signed onto a vow contests its sales. It shows the dispute, and voids nothing.*

```cddl
contest-payload = {
  0 => hash,                ; the act it answers: a declaration of absence (type 13); or, for a vow, the sale it contests
  ? 1 => tstr / [hash, any] ; for a vow only: the promise not kept, in words (canonical text) or in a cMIP's terms
}
```

A contest of a declaration is signed by the party the declaration names, with its own key. Its inside names the declaration as both chain and predecessor, `[[declaration, declaration]]`, as a signature does.

**It shows that party's presence and the dispute, and voids nothing** (rule 88). It is shown beside the declaration, to everyone who reads it. A voice the declaration removed comes back only by a later version naming the party. A contest signed by anyone else shows nothing.

*Client conformance:* the declared member's client lets them sign it, and publishes it.

A contest of a vow: field 0 names **the sale it contests**: the buyer's payer's claim of the payment that is a purchase under an offer naming the vow, or, for an offer at no price, the signature act by which the buyer signed onto it (rule 13). Its inside names the vow by its name (its genesis id) as chain and predecessor, `[[vow, vow]]`. Field 1, optional, says which of the vow's promises was not kept (a buyer may know only that something went wrong). Signed by the buyer of that sale, it marks that sale contested, and voids nothing. Signed by anyone else, it shows nothing.

**A contest of any other act**, other than a declaration of absence or a vow's sale, by anyone with standing (rule 54), is in the same format: field 0 names the act it contests, field 1 is absent (a contest of such an act carrying it is invalid), and its inside names that act as both chain and predecessor, `[[act, act]]`. It is shown beside the act it contests, and changes nothing about any act's validity, status or selection. Signed by anyone without standing, it shows nothing.

### Settlement request (type 22) and fork settlement (type 23)

*In short: when a deal forks, a party who signed it asks the deal's judge of forks to settle it, and the judge's settlement keeps one branch and drops the rest.*

```cddl
request-payload = {
  0 => hash                 ; the reference: the version of the deal with two complete versions of it
}
settlement-payload = {
  0 => hash,                ; the settlement request (type 22) that activated the judge
  1 => hash,                ; the version of the fork it keeps: its line, and what grows on it, in force from then on
  2 => [+ hash]             ; every tip it drops, ascending, none twice, none the version kept
}
```

A settlement request is signed by a party who signed the reference, with its own key. Its inside names the reference as chain and predecessor, `[[reference, reference]]`.

A fork settlement is signed, with its own key, by the judge of forks the reference names (field 27, one of field 13), or by the link of its chain of judgment that took over (field 21). Its inside names the request as chain and predecessor, `[[request, request]]`.

How they settle a deal's fork is rule 47.

*A settlement that names no request, or a request by someone who did not sign the reference, settles nothing: the judge never acts on its own initiative.*

*Field 1 is any version of the fork, usually the tip of the branch the judge keeps as it saw it; a version made on that branch after it is on the line kept.*

### Resignation (type 16)

*In short: a resignation is how a member leaves a collective, giving up their voice in its agreement. A stepping down gives up only one area.*

```cddl
resignation-payload = {
  0 => hash,                ; the agreement of the collective in which the member gives up their voice
  ? 1 => uint,              ; stepping down: the area given up, by its id in that agreement ; absent: the whole voice
  ? 2 => [+ hash]           ; the drafts its signer had signed and leaves behind, ascending, none twice
}
```

Signed by a party to that agreement, with its own key. Like a signature, its inside names the agreement as both chain and predecessor, `[[agreement, agreement]]`, and it is never a fork.

Only a party to an agreement founding a collective may resign or step down. A deal is left only as its terms provide.

What it gives up:

- Without field 1, it gives up the signer's voice in that agreement and in every agreement descending from it by clones (rule 70).
- With field 1, it is a stepping down. It gives up only the signer's place among that area's holders, in that agreement and its descendants, until a constitutional clone refits the area (rule 71). The signer must hold that area there.

It takes effect for the collective's acts at the collective's line registering it (record field 3). It never takes effect at its place in its signer's own sequences, which are not read ("Made before, made after").

**A resignation, or a stepping down, names the drafts it leaves behind.** A version it names never brings its signer back, nor, for a stepping down, gives the signer the area back, whichever line puts it in force. What she signed while present still speaks where rule 65 counts it.

*Client conformance:* her client names the drafts when she resigns or steps down. *Stated cost:* a draft it fails to name brings her back.


**A resignation or a stepping down is spent once its signer comes back.** Its signer comes back by any later version they signed that names them again, as a member, or as holder of the area, whether or not the resignation was ever registered.

- "Named again" is by a constitutional clone (never an ordinary clone that only copies the list), signed by a signature act of theirs that is valid.
- "Later" is descending from the agreement the resignation names: the only order MOR can see between the two acts.

Once spent, it is still on the record and visible, but no line registers it, by a record or by a rollback. So it can never take a voice or an area its signer kept or came back to.

*A member who means to leave signs a new resignation after the last version they signed.*

The version a line registering a departure puts in force, whose signature the departing member placed before that line, never brings them back: it is the version of their leaving, not of a return.

### Record (type 17)

*In short: a record is the collective's everyday line. It writes a new clone of the collective's agreement on the record, or registers members' departures and rotations, or both.*

```cddl
record-payload = {
  ? 0 => hash,              ; the complete clone of the collective's agreement it writes on the record
  ? 2 => [+ hash],          ; the signature acts that complete that clone ; present exactly when field 0 is
  1 => [* kept-tip],        ; the latest act of every other sequence the collective keeps
  ? 3 => [+ hash]           ; what it registers: resignations and steppings down by parties, abandonment
                            ; declarations removing a party's voice, and parties' own rotations, each by its act's id;
                            ; also stake transfers, proxies and their revocations, and take-ups of an
                            ; issuance offer
}                           ; at least one of fields 0 and 3

kept-tip = [ act: hash, position: uint, summary: hash ]   ; as in Identity: the act's id, its position, and the running summary including it
```

Signed by the collective, with its signing key, as an everyday act in its own sequence. Its inside names, in `objects`, the clone as both chain and predecessor, `[[clone, clone]]`, as a signature does. A record naming no clone names the agreement in force for it the same way, `[[agreement, agreement]]`.

**A record is the collective's everyday line** ("Made before, made after"): field 1 names the collective's other sequences.

A record is a decision. Its own sequence and the tips field 1 names are the action heads it saw, the previous decision among their history. It carries no other citation.

**A record must be done.** A record that is neither sealed to every member of the agreement in force for it, nor public, is no line: it puts nothing in force, registers nothing, and places no signature (rule 57). *One rule for every act in the collective's name, no exception by type. Example: a key holder draws a record registering a member's resignation, sealed to nobody: it is no line, and the member's voice remains until a record that is done registers it.*

**A record puts its clone in force with the signature acts it names, and only those.**

- Each act field 2 names must be a valid signature act naming the clone (rule 6).
- The record places each at its own position.
- The clone counts as complete at the record only if those signatures meet the powers its mark names, counted as rule 65 says at the record.

A signature act naming the clone that field 2 does not name counts nothing toward it at this record, whenever it was made. *Naming the signatures keeps a record from being completed late, by a signature arriving after it, and then knocking a later record out of force; and it keeps a member who had already left from signing a clone and being counted.*

A record puts its clone in force only if both hold:

- that clone is complete, judged with the signature acts it names; and
- its parent is the agreement in force for the record act itself (rule 72), or, where records on concurrent lines leave a fork for the record (rule 72), the latest clone of one of its branches, the record coming after both lines (rule 90).

Records follow the agreement chain one clone at a time, never skipping it.

A record of a clone that changes the constitutional tier puts nothing in force: only a rotation declares such a clone (rule 69).

**A record's two parts count apart.** A record whose clone it does not put in force still registers what its field 3 names, and is still a line.

No area reaches a record, as none reaches a rotation. It is judged by the clone it names alone, whose own mark already carries the consent it needed. So the holders of an area reaching Agreements cannot hold up another area's change. *The key grammar's holders produce the collective's signature on it, as on every act of the collective; they could already withhold that signature from any area's acts, so the record gives them no new power.*

**Field 3 registers departures and rotations.** Each act it names must be one of these:

- a resignation or stepping down (type 16) by a party of the agreement in force for the record;
- an abandonment declaration removing such a party's voice (rule 89);
- a rotation (Identity) of such a party's own identity;
- a stake transfer (type 4) of a stake the agreement in force for the record defines, which the record places for the owners' matters (rule 94);
- a proxy (type 27) or a revocation of one (type 28) by a holder of such a stake, which the record places (rule 97);
- a take-up of an issuance offer (type 26) of such a stake, which the record places (rule 96).

A record naming any other act there counts for nothing.

**Each act field 2 or field 3 names is judged as placed at this line** ("Made before, made after", 4), and so is each signature act a declaration it registers needs, and each signature act that declaration's field 1 is checked against. Where these sections ask for a valid act, the act counts as "Made before, made after", 4, counts a member's signature: valid under Identity, or placed by this line before any line of the collective registering the rotation that voided it. Kept so, it is shown as a dispute on the line, and the line stands.

Each takes effect for the collective's acts at this line ("Made before, made after"), whether or not the record puts its clone in force. A resignation spent by its signer's return registers nothing (type 16).

The record is not before itself. A clone the same record writes is judged with the departures it registers already in effect. So a signature it places counts for its departed signer only where an act of the collective placed it before.

*Client conformance:* the collective's client registers a departure, and a member's rotation it learns of, in its next act, as it records a complete clone at once (rule 72). The rotation that follows a departure (rule 69) still fences off the departed member's key share. *A departure needs nobody's permission; the line is the collective's, drawn with the signing key, and a collective that delays it only delays itself.*

*The rare case, in detail:* when a declaration removes a voice. A declaration removes a voice when:

- it counts (type 13), judged as placed at this line;
- its outcomes include 0;
- its party is a party of the agreement in force for the record; and
- the agreement it names is that agreement or one it descends from by clones.

Where its authority is a threshold of the other parties, that number is:

- counted at this record, among the other parties whose voice remains there, with the resignations and steppings down this same record registers in effect, other declarations it registers not (rule 85); and
- met by its signer with the signers of the signature acts naming it that this record or an earlier act of the collective acknowledges.

### Abandonment declaration (type 13)

*In short: an abandonment declaration declares a party absent, under the abandonment clause of an agreement the party signed, with the outcomes that clause allows.*

```cddl
declaration-payload = {
  0 => hash,                ; the agreement in which the party is declared absent
  1 => hash,                ; the clause's version: the version of that agreement whose abandonment clause (field 9) it applies,
                            ; the last one, back from field 0 through its parents, that the party signed (rule 87); as the judicial
                            ; tier changes only with every member whose voice remains (rule 67), it carries the clause in force for every such party
  2 => hash,                ; the party declared absent: a party to the agreement of field 0, or, in a collective, a
                            ; holder of the collective's stake in itself who is no party
  3 => [* uint]             ; the outcomes (rule 89), ascending, none twice, each one the clause allows; at least one
                            ; against a party; against a holder who is no party, 1 or 2 where its plan names no stake
                            ; successor, and none where it names one, succession alone moving the stake
}
```

Signed by the authority the clause names (rule 85), with its own key. Like a resignation, its inside names the agreement as both chain and predecessor, `[[agreement, agreement]]`, and it is never a fork.

Where it takes effect:

- In a collective, at the collective's line registering it (record field 3). Or, where the declared party is the one who would draw that line, at the recovery rotation: the rotation that removes the declared party.
- In a deal, at its own place, as the deal's keepers record it ("Made before, made after").

**A declaration counts only if it is public, or addressed to the member it names**, or to the holder it names. Public means its content key is on its outside. Addressed means sealed to that member among its recipients (Envelopes, `to`), with others if its signer wishes. One that is neither counts for nothing, so no line or rollback can register it.

*Why it needs no agreement on relays: a declaration has effect only where an act registers it, and that act names it; whoever checks that act, the member named included, must obtain the declaration to accept it, so it reaches the member through the very act that uses it, or that act does not count for them.*

*Client conformance, SHOULD: the declaration is published where the collective's clients look, and delivered to the member's inbox, so that they see it early; this is not the safeguard.*

A declaration counts only when it passes rule 87's checks:

- it is valid under Identity, or, registered by a line of a collective, valid as that line placed it (below);
- the party is a party to the agreement field 0 names, or, in a collective, a holder of its stake in itself who is no party;
- field 1 is the last version of that agreement, back from it, that the party signed by a valid signature act, and carries an abandonment clause; for a holder who is no party, field 1 is the version field 0 names, whose clause is the clause in force (rule 87);
- every outcome it states is one the clause in force allows, the abandonment clause of the agreement field 0 names; against a holder who is no party, the stake-moving ones only: none, its stake passing by succession, where its succession plan names a stake successor, or else outcome 1 or 2; never 0, 3 or 4 (rule 89); and
- it is signed by that clause's authority (the clause in force, for every party, one whose voice was removed before a judicial change included). That is the identity the clause names, or, where the authority is a threshold of the other parties, one of them, its number counted where the declaration takes effect (rule 85).

A declaration that fails any check, judged at the point where the record names it, is no declaration there: a record naming it in field 3 counts for nothing (record, field 3). A later act of the party never makes a record fail that passed where it was drawn.

**A later rotation is such a later act.** The declaration, the signature acts completing it, and the party's signature acts its field 1 is checked against are judged as the line that registered them placed them ("Made before, made after", 4): an authority that rotates and disowns its declaration, or a declared party that comes back, rotates and disowns its signature on the version field 1 names, is shown as a dispute on the line, and the line stands, with what it put in force.

*The core does not check the period of absence: it is judged on the time reference, outside the core, and a party wrongly declared absent contests it (rule 88). Field 1 names a version rather than repeating the clause, so a verifier checks it against the version the party signed, and a declaration can never pick a friendlier clause. That version carries the one clause in force for every member whose voice remains (rule 67); the field and its check stay as they are.*

*Where a party's voice was removed before a judicial change, the version it signed last may carry an older clause; the clause in force applies to it, as to every departed holder.*

*The rare cases, in detail:* an authority that is a threshold of the other parties.

**Where the authority is a threshold of the other parties, one of them signs the declaration and the others add signature acts naming it** (type 1, field 0), as for terms. It counts once the required number have signed: its signer and the signers of those signature acts, each among the other parties whose voice remains where it takes effect, meet the number counted there.

In a collective, a signature act on a declaration counts as the collective's own sequence places it ("Made before, made after", 2). Toward a line, it counts only where the collective placed it at that line or before:

- by an act of the collective acknowledging it before the line; or
- by the record registering the declaration acknowledging it (Envelopes, `acks`), as a record names the signature acts that complete its clone.

A signature the collective places only after that line completes nothing there; a later record registering the declaration counts it. *Fixing the signatures at the line keeps a declaration from being completed late, by a signature arriving after its line, and changing what an earlier line did.* *Client conformance: the collective's client acknowledges the others' signatures in the record registering the declaration, or before.*

Where the declaration takes effect at a recovery rotation, the collective cannot act without the declared member before it. So **the recovery rotation places the others' signatures: its Agreements declaration names them**, as it names its clone's, and they count at that rotation ("Collectives in the identity chain"). A signature act on the declaration that the rotation does not name, and that no act of the collective placed before it, completes nothing there. *Client conformance:* the members who make the recovery rotation name in it the signature acts on the declaration that they hold.

In a deal the authority is one identity, never a threshold of the other parties, so a deal's declaration is complete with its own signer.

### Collectives in the identity chain

*In short: a collective's own identity chain says which agreement it lives under. Its genesis declares the founding agreement; a rotation declares a clone that changes the constitutional tier; every other clone is written by a record.*

Each declaration sits in the declarations slot (Identity). Kind 0 is **the agreement the collective lives under.**

- A collective's genesis declares its founding agreement: `[AGREEMENTS, 0, agreement]`, the value being its id.
- A rotation declares, with the same kind, each complete clone that changes the constitutional tier, membership included (rule 69). Its value names the clone and the signature acts that complete it: `[AGREEMENTS, 0, [clone, [+ hash]]]`. Only such clones need the collective's keys to rotate.
- The recovery rotation (the one removing a declared member, rule 85), where a declaration taking effect there needs other signatures than its signer's, adds a third element naming those signature acts: `[AGREEMENTS, 0, [clone, [+ hash], [+ hash]]]`.
- A rollback (rule 93) declares `[AGREEMENTS, 0, [clone, [+ hash], broken, [* hash]]]`. These are: the clone; the signature acts that complete it; the broken act (the rotation's id, as an act of the collective's identity chain); and the acts it registers, each by its act's id, possibly none. Those acts are resignations and steppings down (type 16), declarations of absence (type 13), and the signature acts on those declarations that their authority needs (type 1). Its third element, a hash and not a list, tells it from the recovery rotation's form.

Every other complete clone is written by a record act in the collective's own sequence (rule 72).

**A rotation puts its clone in force with the signature acts its declaration names, and only those**, as a record does:

- each must be a valid signature act naming the clone (rule 6);
- the rotation places each at its own position; and
- the clone is complete at the rotation only if it is complete (rule 45) judged with those signature acts alone, the powers its mark names counted as rule 65 says at the rotation.

A signature act naming the clone that the declaration does not name counts nothing toward it there, whenever it was made. *So no signature arriving after the chain key's ceremony completes the clone late, and the records made since are never judged anew. Identity is unchanged: a declaration's value is the higher MIR's to define (Identity rule 31). The genesis declares founding terms, which exist only when every founder has signed them, and names no signatures.*

For an act the collective's key signs, the agreement in force is the agreement named by the latest declaration of kind 0 at the chain act its key is bound to. Where a valid record made since that declaration counts for it, it is instead the clone named by the latest such record (rule 72):

- for a record, a record before it;
- for an action, a record its history holds, or one before such a record (rule 58).

Its areas are the ones that act is judged by.

**A rotation that declares no new agreement changes keys, not rules.** Its declarations slot carries the Agreements declaration on unchanged, and the agreement in force carries forward, recorded clones included. The records made since the last declaration, under that rotation's binding and the earlier ones, keep counting. Only a rotation declaring a new value of kind 0 supersedes the records before it. *Otherwise a collective that adopted a payment cMIP by a record, then rotated to fence off a departed member's share before the refit was signed, would silently undo the adoption made by the area reaching Money for every act under the new key.*

*The rare cases, in detail:* a recovery rotation and a rollback.

**A recovery rotation places the signature acts on a declaration that its third element names, and only those**, as it places its clone's.

- Each act the third element names must be a valid signature act (rule 6) naming an abandonment declaration (type 13) against a party that the declared clone takes out of the parties and that the collective's signing key cannot be produced without under the agreement in force at the rotation: the declaration that takes effect at this rotation.
- A rotation naming any other act there puts nothing in force, as one naming an act that is not a signature on its clone.
- The rotation places each at its own position. The declaration's number is counted there, among the other parties whose voice remains at the rotation, met by its signer with the signers of the signature acts placed at or before it (rule 85).

*The declared member is the one who would draw the collective's line, so no act of the collective can place the others' signatures before the rotation; the rotation itself does, at the chain key's ceremony, so no signature arriving later completes the declaration there.*

**A rollback puts its clone in force as a rotation does, and registers departures as a record does** (rule 93). It counts only in the broken stretch, naming as broken the broken act itself. Further:

- its clone's parent is the agreement in force just before the broken act;
- its mark names the powers rule 93 gives it; and
- the clone must be complete at the rollback with the signature acts its second element names, and only those. The powers are counted as rule 65 says at the rollback, with the departures registered on lines before the broken act in effect and none registered in the broken stretch.

Each act its fourth element names must be one of these:

- a valid resignation or stepping down (type 16) by a party of the agreement in force just before the broken act, naming that agreement or one it descends from by clones, a stepping down naming an area its signer holds there;
- a declaration of absence (type 13) with outcome 0 against such a party, under the clause of that agreement or one it descends from. Its authority is counted at the rollback, among the other parties whose voice remains there, the resignations and steppings down it registers in effect and other declarations not, as a record counts it. It is met by its signer with the signers of the signature acts naming it that the fourth element also names, which the rollback places at its own position, as a recovery rotation places those its third element names;
- or such a signature act.

A rollback naming any other act there puts nothing in force. A resignation spent by its signer's return (type 16) is valid there and registers nothing. **Each act a rotation's declaration names, a recovery rotation's and a rollback's included, is judged as placed at that rotation** ("Made before, made after", 4): a later rotation of its signer is shown as a dispute on the line, and the line stands. *No act of the broken stretch places a signature (rule 93), so the rollback itself places the declaration's other signatures, as the recovery rotation does.*

Each takes effect at the rollback, its line, for the collective's acts after it. The clone the rollback declares is judged with them already in effect, as a record judges the clone it writes (record field 3). So a member whose departure the rollback registers is not counted for it. The exception: a signature of theirs on the clone that an act of the collective placed before the rollback counts there, for that clone alone, its signer still counted among the voices that remain, as at every other line. *The resignations and declarations of the broken stretch cannot be registered by any record of the stretch, so the rollback registers them; otherwise one member who resigned, or vanished, and stayed silent would block the way back for good under "every party".*

### Notice (type 24)

*In short: a notice asks a payer whom a collective owes money back where the money should go, with a deadline.*

```cddl
notice-payload = {
  0 => hash,                ; the payment owed back: one receipt or claim of it (rule 93)
  1 => [ hash, any ]        ; the deadline: a time reference, and the point on it
}
```

*Most of the time a collective never sends one.*

A collective owing money back from a broken stretch to a payer who gave no address (no payee pointer, vault, or refund rail on a claim of theirs) sends that payer, before it closes, **a notice sealed to their identity** (or public). The notice asks where the money should go, with a deadline on a time reference.

Signed by the collective, with its own key. Its inside names the payment as chain and predecessor, `[[payment, payment]]`.

If the payer names an address in time, they are paid. If not, the collective may close, the debt named in the closing act, visible and unpaid (rule 93; the closing's field 4).

Its conditions (the deadline, how it was sent) are visible, so its good faith can be judged: *the notice demonstrates good faith, its degree set by the notice's conditions.*

Whether the deadline has passed, with no address given within it, is the time reference's answer, which a verifier states as it states every time reference's answer: the core reads no time reference. The core requires no one notice period; it shows the one a collective chose.

*The rare case, in detail:* a notice to a one-time key. This applies to a payer who committed only a one-time key. A bare signing key cannot be sealed to, so the notice is public, naming the payment, and its holder finds it by looking. If the key's holder names where to pay in time, it is paid. If not, the closing names the debt, visible and unpaid, and does not wait.

*Stated cost:* the payer's privacy: the key is linked to this collective.

*A payer who committed only a one-time key cannot be reached by a notice.*

### Fork (type 19)

*In short: a fork divides a collective among its members into new collectives, one per side. Every member it lists signs it on their own identity chain. It must hand every debt binding the collective in the history it cites to a side, or to several sides jointly, and the members must meet the constitutional change rule. Once complete, it is final.*

```cddl
fork-payload = {
  0 => hash,                ; the original's agreement in force at the fork
  1 => hash,                ; the original collective
  2 => hash,                ; the original's identity-chain act (genesis or a rotation) its latest acts are bound to
  3 => [* kept-tip],        ; the latest act of every sequence the original keeps under that chain act: the fork's line,
                            ; the action head it cites
  4 => [2* side],           ; the sides
  ? 5 => [+ fork-share],    ; the original's stakes, where not divided by default
  ? 6 => [+ [ obligation: hash, sides: [+ uint] ]]  ; every obligation in the history it cites: the side, or the sides jointly, owing it
}
side       = [ successor: hash, members: [+ hash] ]   ; the collective the side founded first, by its identity, and its members
fork-share = [ agreement: hash, index: uint, [+ share: uint] ]   ; a stake the original holds, and each side's share, in millionths
```

Field 4 names each successor once, never the original. Field 5's shares sum to 1,000,000, one per side, one entry per stake. Field 6 names each obligation once, its sides ascending, each a side the act lists. A fork act breaking any of these is no fork.

**How it is signed.** A fork is signed by one member of the original collective, with their own identity, never the collective's key. Its inside names, in `objects`, the original agreement as chain and predecessor, `[[agreement, agreement]]`, followed by `[agreement, ending]` for each earlier fork or closing of the same collective it names, the act it follows on that chain, each once. Its chain citations, if any, come after.

**Every member it lists, its own signer included, signs it by a chain signature** (Identity type 16): an act on that member's identity chain, signed with their chain key, naming the fork. A signature act (type 1), or the fork act itself, is no member's signature on it. **Each successor a debt is assigned to signs it as a member does, by a chain signature with its own chain key** (Identity type 16), naming the fork. A successor's signature act (type 1) is no successor's signature on it. *An ending is final, and finality cannot rest on another chain's future, which a signature act, voidable by its signer's later rotation, would be. Stated cost: one chain-key ceremony per successor per fork, in a process already slow and heavy, as rotations are.*

**A fork is complete when:**

- it is valid under Identity;
- its chain act counts in the original's identity chain, and every kept tip it names holds there (as a rotation's kept ancestry, Identity);
- the agreement it names is the one in force at its line, the original's records before the line counted (rule 72);
- every member it lists is a member whose voice remains at the line (rule 65's voices, the departures registered before it counted), is on one side, and signed it by a chain signature that counts in their identity chain and is not void (below);
- **those members meet the constitutional change rule, counted as rule 65 says**;
- where that rule counts among a stake's holders, each holder who is no member that the count relies on signed it by a chain signature that counts in their identity chain and is not void, as members do;
- each side's successor is held, and its founding terms, as its genesis declares them, have exactly that side's members as parties, and keep, in its stake in itself (field 7) and its departed members entry (field 22), every departed holder of the original and every member whose voice remains on no side, each at exactly their share of all the original's income (how the members divide the rest is theirs);
- **each successor's founding terms pay no kept holder less than the original's agreement in force at the line pays it, as rule 48 reads "paid less", or the kept holders consented as rule 95 says**: the original's stake in itself and the successor's compared, its part of the plan, what each plan takes off the top, the fees, and the split cMIP and its parameters;
- each successor a debt is assigned to signed it by a chain signature that counts in the successor's identity chain and is not void, the successor's own key grammar producing it;
- it hands out everything in the history it cites, as the next paragraph says; otherwise the fork does not take effect;
- the fork is done: sealed to every member of the agreement in force at its line, or public, as it cites the collective's head by its line; otherwise it does not take effect either.

*Information runs free: an ending is no exception.* *Client conformance: the members' clients also make a fork public, since creditors and buyers depend on it.*

**What it must hand out.** Field 6 names:

- every obligation of the original in that history, published or not, paid or not, save one that does not bind the collective. An obligation is handed out only if it binds the collective, that is, done (sealed to every member or public, and on its chain, rules 57 and 58) and within its signer's powers, or adopted (rules 76 and 78); one signed with a grant key is included;
- every obligation an earlier fork handed to the original, where the original's own chain signature on that fork lies in its identity chain before the fork's chain act.

**Otherwise the fork does not take effect.** A verifier that does not hold every act that history names cannot tell what it must hand out: for it, whether the fork is complete is **unknown**, never "not complete", so a client tells the two apart. It judges the fork once it holds them.

**What lies outside that history is void** (the tie rule, "Made before, made after", point 7): an obligation of the original the fork's history does not hold binds no one, and no successor owes it. The one exception is a payee pointer left outside it, which still counts for payments made before the fork's line, as the chain stood for each payment, and for none made after it (Money rule 4). *Why: otherwise members could erase, by a fork on an old line, sales they had visibly taken on (rule 76); failing the fork instead would let one payment block an ending, which closings avoid as well (rule 93).*

**Whether a fork took effect is judged only by what its own history holds.** The departures registered before its line count for its check of the obligations it must hand out; a departure registered after it never changes it. *A fork that left out a debt binding in its history stays incomplete, whatever is registered after it; the members sign a new one that hands the debt out.*

*A fork cites the action head; everything in that history is handed out, or the fork does not take effect. A successor owes only what the fork handed it, by its own signature. Publishing has no place in the line, and needs none, since what the line cites is handed out whether published or not; a debt published late was either in the history, and handed out, or outside it, and void. "Everything" is every obligation, published or not, and paid or not, so that whether a fork took effect never changes with what happens after it.* *While the history is not all held, the fork is unknown for that verifier.*

With no stakes written, a member who signs no side keeps each member's equal share, leftovers to the first (rule 9).

**The line.** An act of the original counts as made before the fork when it is bound to an earlier link of the original's identity chain than the fork's chain act, or, bound to that chain act, lies in the history the fork cites: a kept tip it names, their ancestry, and whatever acts there cite on the original's chain, a grantee's act so joined included. Every other act of the original counts as made after it.

*Forking is a constitutional decision. Signing with their own identities keeps the key grammar out of it: a key holder refusing to rotate could otherwise block the fork and keep the identity. If the members are in a position to fork, how they fork is up to them: each side founds its collective first, and the act names it, so no successor needs to claim descent.*

Field 23 of each successor names the original collective, not its agreement, since the agreement may still change before the fork.

**Finality.** **A complete ending is final**: a later fork or closing of the same collective, one that names it (directly, or through the endings it names), counts for nothing.

An ending names another in its own act's `objects`, or through a signer's own chain: where one member's chain signature on it lies, in that member's identity chain, after the same member's chain signature on the other. *Each identity chain is one line that never branches, so any two ending signatures of one member are ordered, whatever devices the member uses: a later ending sharing a signer with a complete one names it.*

Of the complete endings, the one that counts is the one every other names or is named by, and that names no other such one. Where there is none, the collective is not ended (two endings each naming the other through two signers' chains included).

**Ties among endings.** Only two complete endings sharing no signer and neither naming the other are concurrent, made without knowing each other, and **neither counts**, as any race between equals, until a third ending naming both settles it. That third ending is ordered against both, and counts, as a clone of both branches resolves concurrent records; a later one naming it counts for nothing. *A tie no ending could settle would trap the collective for good, against the right of exit.*

**A signature on an old proposal, finished late, counts for nothing.** A member's chain signature on an ending counts for nothing where it lies, in their identity chain, after their own chain signature on another ending of the same collective that names it, in `objects` or through a signer's chain. Once a member signed the ending that replaced a proposal, they can no longer finish it.

**A drafter names the endings they signed.** A fork or closing is no ending where its signer signed, earlier in their own identity chain than their chain signature on it, another fork or closing of the same collective that it does not name in its `objects` (directly, or through the endings it names there). It is never complete, and its signatures count for nothing, for naming and for the rule on old proposals alike. *Checkable on the drafter's own chain, unlike "every ending a client holds": without it, a drafter leaving out a proposal they had signed, then the proposal finished late, leaves two endings each naming the other through two signers' chains, which no reading of the chains can tell from a true race.*

"Names" is judged here in two steps, so that no signature both judges and is judged: first by `objects`; then, among the signatures left, through signers' chains too; what an ending names through signers' chains is read from the signatures left after both steps.

*Stated cost: where the drafter of the ending replacing an old proposal never signed that proposal and leaves it unnamed, the proposal can still be finished late, by members who signed no ending naming it, with others' earlier signatures: it then counts as the earlier ending, and the one replacing it for nothing, or the two name each other through two signers' chains, and neither counts until a third ending names both. This needs the drafter not to have held the proposal (a true race) or to bypass their client, which names every ending it holds; under a threshold it needs no more than members who signed neither. The late signatures lie on their signers' chains, visible.*

*Client conformance: a member's client signs no ending that another ending it holds names, nor any ending of a collective once it holds a complete one.* *Stated cost: every member signing an ending goes through the chain-key ceremony, so forks and closings are slow and heavy, as rotations are.*

*A complete fork is never undone: a debt published after it was either in the history it cites, and so handed out, or outside it, and void; and a later ending, naming it, counts for nothing.* *Client conformance: a new ending names every ending of the collective its client holds, complete or not.*

### Closing (type 20)

*In short: a closing ends a collective for good. Its members sign it as they would a fork. It can complete only once the collective holds nothing and owes nothing, with some money owed back left open as the rules allow. An absentee's share is left where the collective's own rules sent it.*

```cddl
closing-payload = {
  0 => hash,                ; the collective's agreement in force at the closing
  1 => hash,                ; the collective
  2 => hash,                ; its identity-chain act its latest acts are bound to
  3 => [* kept-tip],        ; the latest act of every sequence it keeps under that chain act: the closing's line
  ? 4 => [+ left-open]      ; money owed back it leaves open (rule 93):
                            ; ascending by payment, none twice
}
left-open = [ payment: hash, notice: hash / null, ? holder: hash ]
                            ; the payment owed back, by one receipt or claim of it; the notice sent to its payer (type 24), or
                            ; null where it is owed to nobody; a holder that outlives the closing, where the collective chose one
```

**How it is signed.** A closing is signed by one member of the collective, with their own identity. Its inside names the agreement as chain and predecessor, `[[agreement, agreement]]`, followed by `[agreement, ending]` for each earlier fork or closing of the collective it names, as a fork does. Its signer and the other members sign it by chain signatures on their identity chains, as for a fork.

**A closing is complete when:**

- it is valid under Identity;
- its line holds, and the agreement it names is the one in force there, as for a fork;
- **it is done**: sealed to every member of that agreement, or public (client conformance: made public);
- its signer and the members whose chain signatures on it count are members whose voice remains at the line, its signer among them, and, with the holders below, **meet the constitutional change rule**, counted as rule 65 says;
- where that rule counts among a stake's holders, a holder who is no member counts toward it only by a chain signature on the closing that counts in their identity chain and is not void, as members do;
- **the collective holds nothing**: no stake of a latest version of an agreement a verifier holds names it as a holder, by its identity or as null in its own terms, other than in its own income, unless the work's release (rule 55) counted with the collective's consent placed before the line;
- **the collective owes nothing**: every obligation that binds it and lies before its line, that is, in the history it cites (one outside it is void, the tie rule), and every obligation it owes as a fork's successor (rule 91), is fulfilled in full by receipts a verifier holds, or ended by its creditor's release (Money type 4, rule 92). An obligation it owes as a successor is placed by the original's line, not its own. **A receipt or a release of another identity counts toward this only where an act of the collective in the closing's history acknowledges it (Envelopes, `acks`), or the receipt is the collective's own**: it is then placed by the collective's line, and kept there whatever its signer's later rotation disowns, shown as a dispute. **A receipt or release nobody acknowledged is shown, and the closing waits** until an act of the collective acknowledges it. **An absentee's money share, left where the collective's own rules sent it, does not block the closing** (below);
- **no money is owed back from a broken stretch**, whatever the closing's line (rule 93), save what field 4 leaves open as the rules allow (below);
- **the collective is not broken.**

**A verifier that does not hold every act the closing's history names cannot tell what the collective owes: for it, whether the closing is complete is unknown**, as for a fork, never "not complete". It judges the closing once it holds them.

**Money owed back that field 4 may leave open** is of two kinds:

- money owed back to nobody;
- money owed back to a payer who gave no address, named with a notice of the collective sealed to that payer, or public, whose deadline has passed with no address given.

Every such payment it leaves open, it names. *A holder that outlives the closing and pays the payer when they turn up may be named beside such a payment (a keeper or custodian role, for a cMIP to define). The core allows it and shows it, and requires neither the holder nor any one notice period.* *Stated cost: a collective may close while visibly owing an anonymous payer who could never have been paid; and while visibly owing a payer who gave no address and let the notice's deadline pass.*

**An absentee's money share.** A collective is liquidated by ordinary sales and transfers before its closing (rule 15): its stakes sold or moved by stake transfers, or released, so that it holds nothing. The money share of someone it has nowhere to pay, an absentee, does not block the closing. Where that share goes, and how the absentee is reached or waited for, follows the collective's own rules: its split plan, its absence clause, its terms, written before anyone left. The core checks two things only:

- the closing counts only when the collective holds nothing (above);
- nothing owed vanishes silently: what the collective's rules could not deliver stays visible as an open obligation wherever those rules sent it (its split service, for instance, rule 24), until a valid claim appears: never lost, never taken.

The closing needs no entry for it: field 4 stays for money owed back. The closing names no stake and moves none. *Batching the sales, transfers and closing together is a client's or a cMIP's convenience, never a second way in the core.*

A refund past the terms the buyer accepted by paying is ended, and is not a debt that binds the collective: it never blocks a closing.

**After its line**, the collective's keys count for nothing in Agreements, as after a fork, and its open offers are withdrawn, as by a fork. *Work is either sold or made public domain: clean.* *A collective that cannot pay cannot close: it stays open, abandoned, its debts visible. "Holds nothing" is judged on the latest versions of the agreements a verifier holds; a stake sold by a transfer (type 4) is not seen, so a collective that sold one is shown as still holding it.*

**Finality and ties.** A complete closing counts only as a fork does:

- it is final, and a later ending naming it (in `objects` or through a signer's chain) counts for nothing;
- of two concurrent ones sharing no signer, neither counts until a third names both;
- a signature on it placed after its signer's signature on an ending naming it counts for nothing;
- one whose signer signed earlier an ending it does not name is no closing.

**Judged on its own history.** Whether a fork or closing is complete is judged on its own line and history, as if it were the ending that counts: its own debts, its own departures, its own signatures; never with another ending already in force. A complete ending that names a final one (directly, or through the endings it names), and so counts for nothing, is shown as such ("complete, counts for nothing: an ending it names is final"), and an incomplete one as incomplete. *The answer does not depend on which ending a verifier judged first. A closing's own conditions (receipts that fulfil its debts, "holds nothing") may still change as a verifier comes to hold more acts.* *A receipt or release a closing relies on is one the collective's own line placed, so a creditor's later rotation cannot reopen a closed collective's debt; the collective client's habit of acknowledging what it relies on does it, in the closing's own history.*

### Keeper record (type 2)

*In short: a keeper record is a keeper's signed statement that it recorded one act. Its records, one after the other, are the keeper's sequence: a signed order anyone can check from any copy.*

```cddl
keeper-record-payload = {
  0 => [ act: hash, signer: hash ]   ; the act recorded, by its id, and the identity that signed it
}
```

Signed by the keeper's operator, with its own signing key, as an everyday act of the operator: **one keeper record per act recorded.** A keeper record is public. It names no chain in `objects`. Its `prev` names the operator's previous keeper record, so a keeper's records form one sequence of its operator's (rule 37): **the keeper's sequence**, whose order Envelopes' position and running summary make checkable from any copy (Envelopes rule 13; rule 39).

**What a verifier checks of a keeper record:**

- it is valid under Identity as an everyday act of its signer, and public, and its payload is in this format;
- it counts, for any rule that reads it, only as rule 42 says: the verifier holds the act it names; that act is valid as an act; that act's signer is the identity the record names beside it; and the record's signer is the operator of a keeper named (terms field 3) by the version, in force for the act, of an agreement the act belongs to ("Belongs to"), or, for an act of a collective placed against the collective's own line, one the collective's agreement names ("Made before, made after", 5);
- it counts only while its signer's chain keeps it valid (Identity, "Evidence counts only while its signer's chain keeps it"), save where a rule of this MIR read it to place an act or a relied-on version's completeness rests on it (rule 42), and where a collective's line placed it ("Made before, made after", 4): it then keeps the effect the rule gave it, whatever the keeper's later rotation disowned, shown as a dispute;
- one record is before another only when it lies in the other's ancestry in the keeper's sequence; records of two operators, or of two sequences, are not ordered against each other by their keepers; a keeper keeps one sequence for its records (rule 37).

**An anchor of a keeper record** (Envelopes, task "Anchoring") shows that record, and every record before it in the keeper's sequence, to have existed by the anchor's point (rule 41). A keeper record places nothing on a time reference by itself (rule 33).

*A record of an act nobody can produce changes no verdict; a record naming the wrong signer counts for nothing.*

### Creditor's release (retired type 21)

*Agreements type 21 is retired and never reused. The creditor's release is Money type 4, "Creditor's release": forgiving a sum only gives up money, a simple money decision, so a collective creditor signs it by the area reaching Money alone. A release given against future terms is also a deal, an Agreements agreement of its own (rule 92).*

## Part five: tasks, reasoning and freeze scenarios

### Tasks

- **Split.** A cMIP accepts an amount received and a split plan. It produces payouts that sum exactly, with its declared remainder rule and the evidence for each role share. **It also accepts the agreement and the attempts made for a payment, and produces the receiver for this payment** (rule 17). The receiver is one of the services the signed terms name: field 14's first, or a backup of field 29 only once every attempt before it failed as the cMIP defines failure. It is never an address the owners did not sign. Its fields (field 29's key 1) say what counts as a failed attempt.
- **Condition evaluation.** A cMIP accepts a condition and the acts it refers to, and produces true, false, pending or unknown.
- **Time reference.** A cMIP accepts a point on its reference (for example a block height) and an act, and produces whether the act is before, after, or undetermined.
- **Absence proof.** This is a judicial task: it joins condition evaluation, time reference and anchoring; a judge never handles what it judges, and the chain of judgment applies. A cMIP accepts an abandonment declaration, the clause's parameters and the acts the history of the record or clone using it holds (acts of the party the collective acknowledged or a keeper recorded before the line). It answers whether the party's absence is shown: accepted, refused, or unknown. It may use anchors, liveness acts and acknowledgements as it defines. A declaration it refuses or cannot judge does not count. It is task 14, and a chain of judgment names it as any judge. A client MUST NOT sign terms naming an absence-proof cMIP it does not implement, as for an unknown extension. *Proving absence from time needs a clock the core does not have (every time check tried was found to have a way around it), so the core keeps the members' chosen judgment and leaves proof to modules that compete on it.*
- **Anchoring** is an Envelopes task. Agreements use it for deadlines, time references and claim priority, an agreement naming the anchoring cMIP and the time reference it relies on. *Identity's validity does not depend on it.*
- **Grant limits.** A cMIP defines how a grant's limits are expressed and checked. Its limits may include an expiry, judged against a time reference; an act of the grant key outside them is not backed (rule 80), and a verifier without the cMIP or the time reference answers unknown.
- **Work claims.** A cMIP MAY define pre-publication commitments carried by a claim.

### Reasoning

- **Terms plus signatures.** *One signer per act makes every consent a separate, visible act. An agreement is complete exactly when its rule is met, a party refuses simply by not signing, and nobody owes anything they did not sign for.*
- **Identities, not keys.** *Keys change; promises should not. Naming identities means a rotation never breaks a deal.*
- **Stakes and seats can go to different people.** *A stake is property: passing it changes only who is paid. A seat is trust: the other members chose to share control with that person. A plan can give the stake to an heir and the seat to someone trusted for the job, paid through a position.*
- **Collectives are identities.** *A collective that receives money must sign receipts and rotate like anyone else. Giving it its own keys, held as its members choose, keeps one Identity MIR for everyone. The price is that a threshold signature looks like one signature; where individual consent matters, an area can require visible member signatures. And every grammar leaves a way through that survives the loss of any one key holder. So a dead member never freezes the key, not even a sole holder, whose successor holds an escrowed share: the rule is about the exit, not the number. A collective's member signatures are judged under the rules its own chain declared when the key signed. So leaving is final: a former member's signature never again counts on what the collective signs after its line registering the departure, though it can still complete what the collective signed before it.*
- **Keepers stamp sealed envelopes, and only real ones.** *Proving an act existed before a rotation needs its id and signer, not its content. Keepers that never read keep private deals private; judging content is left to arbitrators and verifiers the parties choose and deliver keys to. A record counts only alongside the act it names, so a bought record of nothing changes nothing.*
- **Protected clauses, one version for everyone.** *A stake cannot be reduced without its holder's signature. But the clause that decides absence, the keeper who judges it and the split service could move the stake in two steps; and an arbitrator, a condition cMIP or an anchoring cMIP chosen after the fact could decide who wins. So in a collective they change only with every member's signature. Keeping, for each party, the version that party signed would leave two members with no judge they both signed. One version, signed by all, gives every dispute one judge; a holdout makes a standoff, which talking or a fork of the collective settles.*
- **A chain of judgment.** *Unanimity could trap a collective whose judge disappears. Naming in advance who takes over, signed by everyone, replaces a failing judge with no new signature.*
- **A departed holder is paid, and consulted on the owners' matters by its holders' rule.** *A stake is a share of income, not a say, save on what touches every owner alike. Whoever keeps a stake after leaving accepts the collective's decisions, and is protected by what company law protects: the stake never shrinks without them, and no term treats it worse than a member's. Keeping their old judges would leave a split service keeping two sets of terms for one payment.*
- **A fork closes the original, under the constitutional rule, each side founding its successor first.** *When a collective splits, no side keeps its identity, so a lone holdout cannot keep the name, history and pointers while everyone else leaves; every side starts anew, and what was made together still pays everyone who made it. A fork is a constitutional decision: where the founders allowed less than everyone, a member who signs no side keeps their money, as a departed holder of each successor, and loses only the seat. The members sign with their own identities, so no key holder can block the fork by refusing to rotate; the fork act names the successors, so none has to claim descent.*
- **Every debt goes somewhere.** *A fork cites the collective's history and hands out every obligation in it, each successor signing for its own, or it does not take effect; what lies outside that history is void, the ending winning. A collective's acts are done only once sealed to every member and on its chain, so hiding a debt gains nothing. A collective that owes cannot close, and a debt ends only by payment or its creditor's release, a Money act.*
- **Two chains.** *Hidden debt, late publishing, the missing device, the stray payment and the payment a fork superseded had one cause: MOR has no shared clock, acts are ordered only where one cites another, and a collective's history may branch, so an act could live on a branch nobody builds on. A collective's decisions and its actions are kept on two chains that cite each other. An act within its signer's powers counts once done, citing the head as its signer knew it, and becomes the new head; a decision cites the head it saw. The chain makes acts unmissable; it does not confirm them. Where two acts signed at the same moment never saw each other, only a decision ending powers against an action using them matters, and the ending wins.*
- **One thing is to act, one thing is to read.** *Powers can be segmented among members; information runs free among them. What a collective plans can stay with some members for a while; once done, it is a fact touching everyone in it, sealed to every member and on its chain. Negotiation is talk, and may stay secret; the deal it leads to is done like any act.*
- **An act is valid where it is held.** *Naming relays would make an act's validity depend on infrastructure: an operator vanishing or turning hostile could freeze a collective, even its decision to change relays; readers checking one relay could be shown less; an offline verifier could decide nothing. The two chains and the tie rule already answer "before"; a relay is transport.*
- **A grant is a key.** *A grantee who signed with a personal key pointing to a grant would act outside the collective's chains, where no ending could place its acts. A grant key makes the grantee's acts a strand of the collective's own actions chain, so a revocation, an emptied area, a fork or a closing places them like every other act, and the tie rule settles the rest.*
- **The burden is where the action takes place.** *A buyer can only guess the current version; the seller knows it. On a request rail the seller's request fixes the claim; on a push rail each holder's own record settles it, the only clock MOR has, belonging to the one answering for it. Old versions stay readable forever; they must not be actionable.*
- **Buying is not pure money.** *A work's claim and offer are Agreements, so a purchase names the claim it pays under, and a payment that names none is no purchase: the money goes back to whoever paid. A stray payment exists only because a rail moves money without reading Agreements.*
- **Payment follows the claim.** *The work is the point of payment; the fork changes only who owns it, so the money follows to the new owners' own pointers, and nothing keeps the old service alive.*
- **A judge and the constitution together need both rules.** *Otherwise two members of three could change a judge by adding a one-word change to the constitution.*
- **The split service vouches for the owners' pointer.** *A pointer names rails, never an identity, so the check is that the service's own signed pointer carries every address: a bypass needs the service to sign, publicly, for an address not its own.*
- **Splits are seen by everyone they pay.** *The core cannot know who owns a service; it can make sure every holder sees every fee taken before their share, and who received it.*
- **A release ends a claim, never a history.** *A work given to everyone still says who made it and who owned it. Every direct owner signs, since a majority releasing would give away the minority's value. Where the owner is a collective, the work is its asset, and releasing it is a business decision taken by its own rules, which every holder of its income feels alike.*
- **Payouts follow stakes, exactly.** *Equal treatment is not a separate test: a split that pays every stake exactly, every fee alike, treats every holder alike; any deviation, either way, is visible.*
- **Deals are everyone's; majorities belong to collectives.** *A deal is as simple as possible while remaining rich: nobody's terms change without their signature, so a party never has to read a rule to know whether they can be outvoted. The price is that one silent party blocks every change, as in everyday deal making. Where many parties or a long relationship need majorities, areas and tiers, they found a collective.*
- **The constitution never freezes.** *The constitution can repair every other tier, so it alone must survive the loss of anyone whose signature it needs. Every such member is covered by an abandonment clause they signed, which can remove their voice, never their stake.*
- **Leaving needs no permission; removal never takes the stake.** *Anyone can walk out of a collective alone, keeping what they own. A constitution may allow removing a member from the board; it can never force them to give up their shares: removed from the board is not forced to sell.*
- **Areas are computed from the collective's own list, and govern it.** *What the core sees of a cMIP is the result of a task, and the task belongs to a layer. Reading the area from which task the collective adopted the cMIP for, never from the cMIP's own label, lets every verifier agree; and letting that area's holders adopt it lets a new rail join the area reaching Money without a constitutional change. Extensions belong to no task, so they sit in Development; one that reaches into another layer needs that layer's holders too, and binds nothing they did not approve.*
- **A number never freezes a rule.** *When fewer voices remain than a rule asks for, all of them together meet it: the same as "every party" already does, and the founders' number wherever enough remain.*
- **Before and after are the collective's.** *With no clock, "before" means before a named act in one named chain. A member may write from any number of devices, forget some, and nobody else can see them all; every patch that asked which of a member's lines an act named opened another gap. The collective's own sequence is one every member signed up to and every verifier can follow, so what was done in the collective's name is before or after an event by where it sits there. A departure takes effect at the line the collective draws; a member's signature sits at the act it signs, or where the collective acknowledged it; a grantee's act, signed with its grant key, sits on the collective's own chain.* **What others relied on stands.** *A completed clone stays complete, a paid publication stays paid, a buyer the collective acknowledged or paid is protected, a year of receipts a member signed for the collective survives a declaration of absence. A line drawn without a branch costs the collective that drew it, never a member's forgotten phone.* **Only the collective's acts place.** *An acknowledgement by a friend carries no place in the collective's sequence; a record names exactly the signatures it puts in force, and so does the rotation declaring a constitutional clone, so nothing completes either late and nobody who already left is counted. A reinstatement is the collective's own act, made after the new holders took the area.* **A declaration takes effect at the same line.** *A voice removed by someone else ends at the collective's line: what a record or rotation already put in force stays, whichever version of the clause the declaration names, and what the absent member signed for the collective before it keeps its signature.*
- **Absence in a deal is judged by one identity.** *A deal has no sequence of its own, so nothing could place several signatures against its line. Parties who must decide together are already a group with a say of its own: they form a collective, which has a sequence, and name it.*
- **A signature speaks for the moment it was made.** *A member who signed and then left was still acting when they signed; their signature stands, and the rule asks for exactly what it asked before they left, neither less nor more.*
- **Nobody is left holding an area they no longer have, and no hire outlives an emptied area.** *A clone that removes a member takes them off their areas, and an area may wait, empty and frozen, for the collective to refit it, keeping the founders' number for the refit. An empty area is an emergency: its grants end at the line that emptied it, which places every act of their keys like any act of the collective. The new holders clone, one by one and visibly, the hires they want back, by the area's own rule, and nothing is left undetermined.*
- **A judge is never chosen by one area.** *The time reference, the condition cMIP and the anchoring cMIP can decide who wins a dispute; they stay judicial, as every member signed them, whoever runs Agreements' other tasks. And a judge never handles what it judges: a specification chosen as a judge serves no other task and is no extension, however it is named and in a deal as in a collective. This removes a conflict of interest and keeps every area out of the choice of judges.*
- **Nothing passes around the areas.** *A specification the collective never listed would let whoever holds its key act around every area, so in a collective with areas it counts for nothing until it is adopted. One that serves two layers is named for the second only with the power over both layers' tasks, and each of its acts counts only when the holders of each area reaching it have signed it, as their area's rule asks (rules 60, 63).*
- **Every change is on the record at once.** *With no clock, the collective's own sequence is what tells which version governed an act, and who still had a say. A rotation is the chain key's ceremony, kept for changes to who decides; every other change, and every departure, is written with the signing key, at once.*
- **Nobody is added, and nobody is held.** *A collective exists only when every founder signed it, and nobody joins it or takes an area without signing. A holder can step down at once; the area waits, frozen, for the collective to refit it, rather than running on without anyone answerable for it.*
- **A split service holds authority, not discretion.** *It is named by grant, bound to the plan, and every incoming receipt or payer's claim must be matched by a split. It can still steal, but never invisibly, and never by naming its own sybil as the referrer: role-share evidence is signed by someone on the other side.*
- **Shares follow stakes, holders follow transfers.** *A plan written in holder names would go stale at the first sale. Written in stakes, it pays whoever the agreement's transfer chain names, and two services compute the same answer.*
- **Leftovers by largest remainder, ties taking turns.** *Every client must compute the same result, so the rule is deterministic. It must also move no money when holders are reordered, since a reorder changes no share and needs no holder's signature: giving leftovers to the first listed holder would let a clone that only reordered holders send every small payment to one of them. Largest remainder keeps every payout within one smallest unit of its exact share. Ties cannot be broken by a hash of an act the divider signs, since the signer chooses its salt and can grind the hash. Taking turns, counted along the service's own receipts, lets history decide, and keeps equal holders within one unit of each other over any run of payments.*
- **Keepers are chosen up front.** *Testimony is only trusted if nobody could pick a friendly witness after the fact.*
- **Many ways to split, one vocabulary.** *A paying client may pay each owner directly; a service may receive and forward at once; or it may pool and let owners withdraw, with every unwithdrawn share an open obligation rather than a number in a private database. Each is a competing split cMIP on the same vocabulary.*
- **Splits belong to owners.** *Money delivers money to a pointer; the owners' agreement decides who gets what. Each payout is a simple payment with its own receipt, so a missing one is as visible as a broken route.*
- **Exact sums, declared remainders, bounded fees.** *Rounding is where money quietly disappears; an unbounded fee line is where it disappears loudly. The plan states the maximum, and the payer's claim fixes what arrived.*
- **Role shares pay for help that is only known at payment time.** *A reposter, a relay or a module earns when its part in a sale is evidenced by someone other than the party paying it, so rewards attach to settlements, never to attention, and never to the service's say-so. That someone is the payer the payment commits to: help known in advance takes a named share.*
- **A stake pays through whoever sells.** *The core cannot tie a sale to a work's stakes without judging which claim is the claim, and a rule that tried would be evaded by re-encoding the file. What it can do is make a sale outside the claiming agreement visible, and give builders every piece needed to refuse or rate such sales. The core is as little intrusive as possible, and carries the bits and bolts for sound solutions on top.*
- **Clone, never modify.** *Nothing in MOR is updated. A clone that meets its rule is the only way terms change, and the parent's history stays intact.*
- **Nobody loses their say by default.** *Protecting the abandonment clause guards one route to removing a party's voice; rewriting the clone rule is another, which would be unguarded. So the rules that decide who decides form their own tier, changed by their own rule, which is everyone unless the founders, all of them, agreed otherwise. What judges the members changes only with all of them. Everything else is operational, and can be handed to the members who run it, as areas: they decide there without asking the rest, and can hire, but never give more than they hold.*
- **The tier follows from the bytes.** *Which power a clone needs is read from which fields it changes, never from what it says about itself, so two verifiers cannot disagree, and a clone cannot slip a constitutional change past as an operational one.*
- **Every version says who brought it into force.** *A field every agreement carries should never be free text that looks binding and is not. A clone's field 4 is a claim anyone can check: this power, these people. A false claim sinks the clone.*
- **A grantor cannot unsay a deal it was paid on.** *Voiding a deal the collective acknowledged or collected on would leave the counterparty with a contest and nothing else. So a revocation never cuts off what the collective itself adopted, wherever its act sits. Deals the collective never touched are the counterparty's risk, which is why a counterparty waits until a later act of the collective cites its deal before performing.*
- **Default made visible.** *The core cannot force anyone to pay or deliver. It can make sure nobody can hide that they did not.*
- **What the collective's line placed stays placed.** *Another identity's evidence counts only while its signer's chain keeps it, save where a collective's own line placed it: a resignation, a declaration, a vote, a receipt a closing relies on. Otherwise one member's theft protection would unwind a collective's year, and a creditor's rotation would reopen a closed collective. Identity's evidence rule and this one are a pair. Endings rest only on chain keys and the collective's own chain, never on another chain's future.*
- **Unknown is not after.** *A verifier with a hole in a line's history cannot tell before from after, so it says unknown and voids nothing; two honest verifiers then never disagree on void.*
- **Owners consent together on what hits them alike.** *A company's owners who run nothing could only veto, each alone. A holders' rule they accepted lets owners holding enough of a stake agree, by share, on what hits every one of them alike; nothing reaches what one of them is paid unlike the others. Issuance keeps every owner's proportion within its reach; a sale of the works, and a fork's successors' plans, are owners' matters too. Weights are read from the stakes, never written as free numbers. Every addition is optional, its default the rule without it, so the members' collective the core keeps on its own is unchanged.*
- **Owners see what their money does.** *A prohibition on spending cannot be checked; sight can: a collective's Money acts reach every holder of its stake in itself, member or not. The right of exit means little to an owner who cannot see why to leave.*

### Parameters

- **The nesting bound**: an agreement may sit at most 8 layers deep among agreements that are parties of agreements (rule 15).

Left to cMIPs: the encoding of conditions (the condition-evaluation task) and of grant limits (the grant-limits task; grant fields 3 and 4).


### Freeze scenarios

The scenario numbers after each colon are those of the freeze test suite.

#### Making an agreement

- Agreements carry cMIP hashes, one per task: all.
- A party bound only by its own signature; founding terms naming a non-signer's debt stay a draft, the debt shown unsigned: 3.
- Terms with a bidirectional override shown visibly before signing: 1.
- Negotiation record with acknowledgements, proven complete by negotiation messages, Agreements acts; a text act in the thread is no part of it and carries no acknowledgement; disclosure by publishing keys: 5.

#### Works and stakes

- Stakes, stake rule, transfer naming both chains; the split service pays the buyer after a sale: 1, 3.
- Binding a work; conflicting claims; a co-owner's lone sale visible as outside the claiming agreement: 2.
- A deal made on a vow: the stake in it held by the vow's signer, the platform's fee off the top, the rest hers, the platform raising its fee only with her signature; terms writing the stake held by anyone else invalid; an offer under a deal writing no stake in the vow it sells counting all the same: 1.

#### Selling

- Standing offers; key delivery or confirmation as the binding moment; an anonymous refund claimable only by a signature with the key the payment committed to, never by presenting the rail proof: 2, 5.
- One push payment whose holders' receipts name different claims: the receipt naming another claim than the payment's commitment is a wrong receipt, counting for nothing; until the rail answers, the payment is pending: 3.
- On a push rail, two holders of a song clone their deal: receipts each holder records before its own signature on the new version make a purchase; after it, every holder refunds; until every holder signs its receipt, the payment is pending; on a request rail, the claim the seller's request committed to is bought: 3.

#### Splits

- A deal's version raising a fee, signed by every party, stays a draft until a holder who is no party, paid less by it, signs; a collective's clone lowering a stake's part of the plan, met by its area, stays a draft until each departed holder of that stake signs; a member outside the area is not asked: 1, 3.
- A plan putting a role share, a named receiver or a metric share in the rest is invalid; the same off the top is valid, and what is left goes to the stakes alone, by their parts and within each by ownership: 1, 2, 3.
- A declaration of absence passes a holder's stake to its stake successor, by a clone the holder does not sign; one stating outcome 1 or 2 against a holder naming a stake successor is refused; with none named, outcome 1 shares the stake out; no declaration lets a version pay less a holder whose stake stays: 1, 3.
- Conservation and remainder rule; module fees and an omitted fee visible; maximum fee per payout, and a plan stating none flagged before signing; split by an attention metric evidenced by a record the identity running the service signs, never by a Module: 1, 2, 7.
- The collective's own reserve: a plan naming the collective off the top, written null, pays that part to the collective's vault, never split again; with no vault declared, the plan is invalid; raising the part is an owners' matter, a departed holder asked: 3.
- A role share filled by a reposter, with the payer's signed referral; a referral named by the service alone earning nothing; an unfilled role share following the chosen option: 2. A relay's delivery record counting for its role share only when the payer's claim acknowledges it, and only for the object paid for; a claim the relay signs itself confirming nothing: 2.
- A rail Module's role share evidenced by the split service's own receipt naming a rail Module the owners' pointer names; a receipt naming a rail Module the owners' pointer and vault never named evidences nothing: 2.
- A relay's delivery record and a service's use record: each counts only acknowledged by the payer the payment's commitment names, never the payer a receipt names; a verifier not told the relay transport cMIP gives the same answer: 3.
- A split service that receives a payment and fails to publish a split, or under-reports against the payer's claim, shown as an open obligation; the owners switch services: 2, 7.
- The owners' payee pointer carrying an address the split service's own pointer does not, or a vault entry its own vault does not, is shown as bypassing it by an Agreements client before paying; one whose addresses and vault entries are all in the service's own, or in those of the service its chain names, is ordinary; a Money-only wallet pays it as it stands (rule 16); with no split service, a wallet reading Agreements pays each holder by the stakes, through a collective holder to its own holders: 3.
- A deal's payees grant its split service in its terms: each payee's grant names this agreement by null, the deal lists them in field 14, one per payee; before every party signed, none counts; then the service's receipts in each payee's name for purchases under the deal's claim are backed; one whose payer is the service, one naming a split or carrying a batch, one for another agreement, one another payee received, and any act but a receipt are not; a payee revokes its own grant, its later receipts void, the other payee's standing; a deal listing grants to two services carries none: 3.
- A deal's backup split services are the split cMIP's fields (field 29): for each backup, one grant per payee, grouped by service, as many as field 14 lists, every grant once; a single hash there, a short group, or a group in a collective is invalid; a chain of judgment following the split service (`[ 2 ]`) is invalid; the backup's grant keys count once the deal exists, under the same limit, receipts whose payer is either service refused; a payee's pointer leading to the backup passes the pointer check; a group signed twice by one payee, or naming field 14's own service, leaves the deal carrying none of its grants: 3.
- The split task: a layer pays the designated pointer; a payout to a backup counts only for a payment whose earlier attempts failed as the split cMIP defines failure; the next payment starts again from the first; a payout to an address the owners did not sign is a mismatch whatever is stated; each participant's cut is checked at its flow address or vault: 3.
- One rule for every split service: a collective's split service, named by field 14, receipts with its grant key a fan's purchase under the collective's own claim and a payment following its terms; never a receipt whose payer is the service, a payout the collective is owed by another work's service, nor money under another agreement's claim, even citing an older head than the version naming the service; a second grant to the same identity, named by no field 14, reaches what its kinds name: 3.

#### Time, conditions and judges

- Named time reference: 1, 7.
- Recurring obligation; lapse shown as past its terms: 7.
- A chain of judgment: the condition cMIP answers "unknown", the next in the chain decides, with no new signature; a fallback named elsewhere in the terms is invalid; a silent arbitrator is followed after its period on the time reference, a chain following an identity without a time reference is invalid (rule 35): 3.
- Terms naming one specification for a judicial task and for another task are invalid: 3.
- A deal naming one specification for anchoring and payments is invalid; a judge named as an extension, or in field 6 and for a payment task, is invalid: 1, 3.
- Terms whose field 6 and field 2's task 10 name different specifications are invalid; naming the same one is valid: 1, 3.

#### Keepers

- Keepers named by the agreement; keepers check homes; recorded acts survive rotation as disputes; a record of an act nobody holds confers nothing: 1.
- Several keepers with a threshold; a keeper lost, its records surviving in copies: 1.
- A receipt belonging to a deal through the claim it pays under, recorded by the deal's keeper before the payee's rotation: disputed; the same recorded by a keeper no agreement it belongs to names: void; a post, belonging to no agreement, placed by no keeper: a component under "Agreements".
- A deadline met, shown by an anchor of the act at or before the point; a deadline missed, shown only by a named keeper's sequence anchored past the point holding no record of the act; a keeper's word alone, or an anchor of the act after the point, never showing it missed: undetermined: a component under "Agreements".
- A keeper's later rotation disowning a record a rule read to place an act: the placement stands, the record shown as a dispute; several keepers, each record judged against its own keeper's record of the rotation, the agreement's rule counting them; a keeper's second sequence ordering nothing against its first: a component under "Agreements".

#### Changing an agreement

- Clone drafts; a deal's clone signed by two of three stays a draft; the concurrency rule withdrawn; status quo default; the judicial tier changed only by every member, one version for everyone: 1, 3.
- A clone changing the arbitrator, signed by two of three, stays a draft; signed by all three, it judges everyone: 3.
- A deal's version completed and relied on (a purchase naming it, a split under it, a keeper's record of it, a later version cloning it), then a party rotates and disowns its signature on it: the version stays complete for everyone, the signature shown disputed; a version nothing relied on yet is judged as Identity judges its signatures; a deal's terms carrying a holders' rule (field 30) are invalid: 1.
- A holders' rule: Dana holds 10% of a collective's stake in itself under a 60% holders' rule she signed; owners holding 70% take 5% off the top for a studio, the mark naming `[5, stake]` with them; she votes no and the clone comes into force, every holder paid alike less; a clone lowering her share alone stays a draft until she signs; a holder who never accepted the rule is still asked; a clone adding a holders' rule signed by every holder but one stays a draft: 3.
- A holder votes on an owners' matter, the record names the signature, the holder rotates and disowns it: the change stays in force, the vote shown disputed: 3.
- Who holds what at the vote: a seller votes, sells, and the buyer votes on the same version: one of them counts, by the collective's line placing the transfer; a transfer acknowledged by the collective but not held by the verifier leaves the vote unknown; in a deal, a version naming the transfer in field 31 asks the buyer, one not naming it the seller: 1, 3.
- Counting by share: a constitutional change rule by share, departed holders holding half the stake in itself: the members' sixty per cent is of what the voices that remain hold; voices that remain holding less than it asks for meet it together; "all of" a majority of members and owners holding 60%, each counted as alone; a rule by share used for keepers, or as a deal's clone rule, invalid: 3.
- Areas count heads: a holder of most of a stake who holds an area with two others counts one head toward its threshold, as each of them does; an area's threshold written by share is invalid: 3.
- An issuance under a sixty per cent holders' rule: every existing share scaled alike; each holder offered its proportion by an issuance offer public or sealed to it; one declines by letting its period lapse, one takes up and pays, the rest goes to the new holders the offer names; an offer that did not reach a holder starts no period for it and the issuance waits; an issuance every holder signs needs no offer; one giving a holder that took up less than its proportion, or scaling one share unlike the others, is a draft; a share added any other way needs every holder: 3.
- A standing waiver of pre-emption: set in a stake's entry of field 30 by a clone every holder signs; an issuance under a sixty per cent holders' rule then names no offer (field 32 `[0]`) and is complete once owners holding 60% and its new holders sign; a clone setting or removing the waiver signed by every holder but one stays a draft; a buyer who bought under it is bound by it; a proxy's vote on it counts nothing; field 32 `[0]` without the waiver in the parent: the issuance is invalid: 3.
- The owners' matters: a collective selling its work to its members' new collective for a token price, with a departed holder and no holders' rule: the transfer waits for her signature, placed by the collective's line; under a holders' rule she accepted, holders meeting it carry it; the same for a release: 3.
- A fork whose successor's founding plan takes an overhead the original's did not, a departed holder kept at her share of field 7: the fork is incomplete until the kept holders consent; with the same plan, complete: 3.
- A kept holder consents to a fork by a signature act an act of the original in the fork's history acknowledges, then rotates and disowns it: the fork stays complete, the consent shown disputed; a consent nobody acknowledged is shown, and the fork waits: 3.
- Proxies: a holder names a proxy, standing over its stake, placed by the collective's line; the proxy's signature on an owners' matter counts as the holder's vote; the holder's own signature on the same version beats it; a proxy scoped to one version votes on no other; a revocation placed before the version's line, or a newer proxy, ends it; a proxy act placed after the version's line counts nothing there; a proxy's vote on changing the holders' rule, on waiving pre-emption, on taking up an issuance, or on a transfer of the stake counts nothing; a proxy act signed by the proxy counts nothing; a proxy in a deal counts nothing: 3.

#### Public domain

- Public domain: a release every direct owner signs, ending the claim, naming the history and publishing the content key; one with an owner missing incomplete; a release rule allowing fewer, changed by a clone every owner signs; a claim it does not name shown beside it; a collective owner releasing by its own rules across its areas; a timed release ending the claim at its point, none without a time reference: 2, 3.
- The release rule counted by share: a holder splitting its share among identities it controls gains nothing; a release rule written as a number of heads is invalid; a collective's release of its work, with a departed holder, waits for the owners' consent: 2, 3.

#### A collective

- Collective born from its founding agreement, with a grammar that leaves a way to rotate; a grammar with a single holder and no successor rejected; a dead member's share released by the recovery path: 3.
- A collective founded on a threshold of its founders does not exist; a member added without signing is not added: 3.
- Founding terms carrying the members' stakes in the collective and its own grant, written null; a deal writing null, or a clone writing the collective's own hash, invalid: 3.
- Acknowledgement recorded by the collective: 3.
- A collective keeps two chains: its actions cite the decision they act under and the heads they join; an act citing nothing on the chain, or an act off it, counts for nothing; a joined act is before a line drawn after the join, an unjoined one after it; an act signed before a record it never cites is judged under the decision it cites (rule 58): 3.
- A record not sealed to every member, nor public, is no line: it puts nothing in force and registers nothing: 3.
- A collective's debt sealed to its creditor alone binds no one; sealed to every member, it binds, wherever it is held; a grantee's act sealed to its counterparty alone is not backed; terms carrying field 25 are invalid (rule 57): 3.
- A collective's negotiation message is talk: it binds nothing, and need not be sealed to every member: 3.
- A collective with departed holders pays a member from its reserve: the payout is done only sealed to every holder of its stake in itself, or public; sealed to members alone, it binds no one: 3.
- A collective's stake in itself held in part by a deal and in part by another collective: a payout is done only public, a holder being a deal; sealed to the deal's pointer, or to the parties of one of its versions, it binds no one; with no deal among the holders, sealed to the other collective and every other holder; an issuance offer reaches those holders the same way: 3.

#### Tiers and clones

- A majority clone that rewrites the clone rule without the minority is incomplete under the default constitutional change rule; one whose mark claims the clone rule for it is invalid: 3.
- An area's power: its holder changes the area's fields alone, and grants within it; a grant beyond its issuer's area is invalid; an act beyond a grant's reach is not backed: 3.
- A clone whose mark names a rule it did not meet, or another rule than the one its changes require, is invalid; a clone changing two areas, each area's power met and named, comes into force at once; an operational clone needs no rotation: 3.
- A clone changing an area and a judicial clause needs both powers: 3.
- A version changing the constitution and a judge needs the constitutional change rule and every member for the judge, its mark naming both: 3.
- A mark whose signers are not ascending by hash is invalid: 3.
- The collective's receipts, Money acts, fall in the area reaching Money, and count only on a rail Module its pointer or vault names, implementing the payment cMIP its terms name; an act of a cMIP its terms name nowhere falls in no area and, the collective having areas, counts for nothing, whoever signs it, until the area adopts it; an act of an adopted extension falls in the area reaching Development: 3.
- The holder of the area reaching Money adopts a second payment cMIP alone; the clone is recorded at once, without a rotation, and governs the collective's acts from the record on; a new rail needs no clone, only a payee pointer the holder signs: 3.
- A rotation declaring nothing new: the payment cMIP the holder of the area reaching Money adopted by a record stays in force, receipts on its rail Modules counting with that holder's signature under the new key: 3.
- The holder of the area reaching Development adopts an extension alone; one acting on Money needs the holder of the area reaching Money too; an under-declared extension binds nothing beyond the areas that approved it: 3.
- An extension declaring Money in its specification (Development, field 10) is dropped only with the holder of the area reaching Money too: 3.
- A specification named for tasks of two layers held by different areas: its acts need both areas' holders, and naming it for the second layer needs both areas' power: 3.
- The holder of the area reaching Agreements adopts a grant-limits cMIP alone, and cannot replace the time reference or anchoring cMIP: 3.
- A holder leaves an area by nomination, by proposal or by redraw; a holder steps down at once and the area is frozen until the change: 3.
- A holder of an area held by two steps down: the other carries on, and the area's grants with it; the only holder steps down: the area and its grants stop until the refit: 3.
- A constitutional clone removes one area and reorders the others: a grant keeps falling in the area of its id; a clone giving a new area a retired id is invalid: 3.
- Owners choosing operators: a collective's constitutional change rule counted among its stake's holders by share; owners vote the operators out; the operators do not rotate; the clone stands complete and unexecuted, the collective running on; where the constitution gave the owners the recovery custodian, the rotation is made; a rotation by the operators without the owners' signatures breaks the collective; the founding client warned where no key share or custodian was under the owners' control: 3.
- A member removed keeps a quarter of the collective's income as a departed holder: no voice, the current judges applying; a clone lowering that stake without the holder's signature stays a draft; every payout matching its stake exactly, a deviation either way shown; every split delivered to every holder it pays, naming each fee and who received it (rule 68): 3.

#### Members joining and leaving

- A collective with a threshold key grammar signs a receipt; a member leaves and the others rotate; the member who left, with another, signs an act an area reaches, and it does not count: 3.
- A member resigns alone and keeps their stake; a member removed under a constitution that allows it keeps their stake: 3.
- Under a constitutional rule of two of three, a removal completes without the removed member, the seat part of their plan going with it and its stake part staying; dropping the stake part too needs every member: 3.
- A constitutional change rule of three of three, one voice removed: the two remaining meet it: 3.
- Three of three: a member signs a clone and resigns; the clone still needs both others: 3.
- A resignation takes effect at the record registering it, not before; a record registering a departure and writing a clone judges the clone with the departure in effect: 3.
- A record writing an incomplete clone and registering a resignation: the clone is not in force, the resignation takes effect at the record: 3.
- An old resignation never registered, then a constitutional clone naming its signer that they signed: registered later, it takes nothing: 3.

#### Grants

- A grant hands its grantee a grant key, which the grantee accepts by signing; the grantee's acts, signed with it, are the collective's own, citing the grant; one not citing it, or beyond its reach, is not backed; a revocation, completed by the area's holder, removes the key: an act its history holds binds, one racing it or after it is void, or disputed where another identity acknowledged it or a named keeper recorded it, conferring nothing, one the collective adopts binds: 3.
- A person grants an agent a key and revokes it by everyday acts, public, with no rotation; an act the revocation's history holds binds, one racing it is void, or disputed under rule 76; a grant a thief made with the stolen signing key dies at the owner's rotation; a private grant backs nothing (extended): 3.
- The label revokes Marco; deal 4, void, is adopted by the label's own acknowledgement, and Sofia manages it under her own grant and key; no handover: 3.
- A grantee's act that one device of the collective cites, and a revocation from another device that never heard of it: the act binds, adopted; the grantee's later acts are void, or disputed under rule 76: 3.
- A departure empties an area: a grantee's act the line's history holds binds; acts racing it, or after it, are void; one the collective acknowledged binds; a reinstatement after the refit, a clone of the ended grant with its own key, backs new acts and takes nothing on: 3.
- The collective acknowledges a void act of a revoked grant's key after the revocation: it binds, adopted; an act a counterparty acknowledged after the revocation is shown disputed, confers nothing, and binds once the collective adopts it: 3.
- A frozen area's grants end; after the refit, only the grants the new holder reinstates, by cloning, back acts: 3.
- In an area held by two deciding together, one holder alone does not reinstate a grant; both do: 3.
- A signature act on an ended grant reinstates nothing; a reinstatement act, completed by the refitted area's holders after the refit, reinstates it; one made before the refit counts for nothing: 3.
- A grant whose limits cMIP sets an expiry on a time reference: an act of its key after the point is not backed, the grant quiet and still revocable; a verifier without the cMIP or the time reference answers unknown; an act beyond its cMIP's limits is not backed, as one beyond its kinds: 3.
- A revocation of a grant within the refitted area counts only with that area's holder's signature act, although no area of the collective reaches revocations: 3.

#### Succession

- A seat passes automatically when every voice signed that plan; a plan cloned by two of three makes the successor a nomination: 3.
- The automatic succession clone puts the successor in the departed member's place in the parties and in every holding of the key grammar, every threshold unchanged, and drops the executed plan; one that changes a threshold, leaves the successor out of a holding or keeps the plan is invalid; the rotation declaring it re-deals the keys, and the successor's voice then counts: 3.
- A succession plan whose key 2 writes a seat successor with a voting weight beside it is invalid; listed alone, the successor enters as rule 83 says and, under a rule by share, weighs what the plan's stake part gives it: 3.
- Where the dead member held the chain key alone, the succession clone also carries a plan for the successor, naming their own successor, signed by them in it, and comes into force; one without it is invalid (rule 59): 3.
- A seat passed by succession leaves the dead member's area to be refitted: 3.
- A succession clone takes the dead member off their areas; an area left empty is frozen; founding terms with an empty area are invalid: 3.
- A succession clone leaves an area's number above its one remaining holder, who meets it alone: 3.
- A party with no plan and a confirmed identity-level successor: a nomination, approved under the constitutional change rule, never an automatic seat, the stake staying with the party's identity; an unconfirmed successor nominates nobody: 3.

#### Abandonment

- Abandonment declaration signed by the named authority, an identity, under the clause the party signed; contest by a contest act: 1.
- Liveness act: 1.
- A deal whose abandonment clause names a threshold of the other parties is invalid; the parties name one identity, a keeper's operator or a collective they form: 1.
- A founding agreement leaving a member with constitutional power uncovered by the abandonment clause is rejected; a member with constitutional power dies, is declared absent, loses their voice, and the others change the constitution: 3.
- A clone signed by a member later declared absent, already on the record, stays in force; their signature on another clone, which no record names and nothing acknowledges, no longer counts; a year of receipts they signed before the collective's line keeps their signature: 3.
- A threshold abandonment authority of two of the other parties, one of them resigned and registered before the declaration's line: the one who remains declares alone: 3.
- A threshold abandonment authority of two of the other parties, both counted: one signs the declaration and the other a signature act naming it, which the collective acknowledges before its line or in the record registering it; the declaration counts there; a signature it acknowledges only after that record completes nothing there, and a later record registers the declaration: 3.
- A threshold abandonment authority of two of the other parties against the only holder of the signing key: the recovery rotation removing them names the second signature act, and the declaration counts there; one that names none, the second signature placed nowhere before it, leaves the declared member counted; one naming another act puts nothing in force: 3.
- A declaration naming another clause version than the one the absent member signed last, an outcome the clause does not allow, or signed by someone other than the authority registers nothing (rule 87): 3.
- A departed holder vanishes: declared absent under the clause in force, though no party, its stake passes to its plan's successor, who signs the sale; with no plan, outcome 1 shares it out and the sale proceeds; a declaration against it stating outcome 0, 3 or 4 counts for nothing: 3.
- A collective whose constitutional change rule counts among its stake's holders, its abandonment clause allowing outcome 0 alone, or naming as authority the holder it would have to declare: invalid; allowing outcome 1 or 2 with an authority able to declare every holder who is no party: valid, a vanished holder's stake moved and the constitution changed without it: 3.
- A declaration of absence registered by a line, then its authority rotates and disowns it, or the declared member rotates and disowns the signature field 1 names: the record stays a line, the declaration shown disputed: 3.

#### Forks of records

- Two registrations of one departure drawn concurrently: an act after one of them only counts as made after the departure; two sibling clones recorded concurrently leave their parent in force: 3.
- Two sibling clones recorded on concurrent lines: the concurrency rule being withdrawn, the parent stays in force until a clone recorded after both resolves it: 3.
- A clone of one branch of a fork, recorded after both lines, resolves it; one recorded after only one line does not: 3.

#### How a collective ends

- A group splits off with the powers to do so: an ordinary membership change, its members departed holders (rule 91, shape A): 3.
- A collective forks: each side founds its successor first; the fork act names them; every member signs it with their own identity, by a chain signature with their chain key, on a side, under the constitutional rule; the original's later acts count for nothing; its ownership passes to the successors by the members' stakes; the departed holder keeps their share in each; the fork hands out every debt in the history it cites, an unpublished one included, or does not take effect, each successor signing for its own, and publishing that debt later changes nothing; a successor owing such a debt cannot close until it is paid or released; grants end, the split service's included, every grant key at the fork's line, a grantee's deal the fork's history cites binding and any other void; a debt signed off the fork's history is void; open offers are withdrawn; a payment to the old service naming no claim, or the original's claim never recorded by the original's chain before the fork, is no purchase, owed back to the payer; one naming the fork is pending until a successor's chain records it; a member on no side under every-party, a stranger on a side, a successor dropping a departed holder, or two concurrent forks sharing no signer, close nothing (rule 91, shape B): 3.
- Under a constitutional rule of two of three, two members fork without the third, who signs no side and is a departed holder of each successor at their percentage: 3.
- A fork hands out its whole history: a debt on another device that the line's history joined must be handed out, one on a device it leaves out is void and owed by nobody; while an act the history names is not held, the fork is not complete; a successor's own later fork hands out what it inherited: 3.
- The tie rule: a public debt on a device a closing's history does not cite is void, and does not keep the collective from closing; cited, it does: 3.
- A sale recorded on the actions chain: the collective's receipt before the fork, in its history, stays a purchase; one off the fork's history, or after it, is no purchase, refunded (rule 12): 3.
- A fork drawn on a line before a debt the collective had cited: complete, the debt void, the debt and its citation standing as evidence; a member's client that has not pulled every device's latest acts refuses to sign it: 3.
- A fork on an old line leaving out a payee pointer, a payment to it and its taking-on inside: the payment still counts; a fork leaving out a binding debt, then a departure registered after it: the fork stays incomplete: 3.
- A complete fork, then a second complete fork naming it: the first stays final, the second counts for nothing; a second fork naming nothing in its `objects`, signed by the same members: it names the first through their chains, and counts for nothing; a signature act on an ending counts for nothing; two complete closings sharing no signer: neither counts; a third, signed by one signer of each, settles it: 3.
- An old proposal finished late: a member's signature on it, placed after their signature on the ending naming it, counts for nothing; under a threshold, a member who signed neither finishing it is the stated cost; a second fork whose drafter signed the first and leaves it out of its `objects` is no fork, and the first, finished late, ends the collective: 3.
- A closing sealed to one member takes no effect; public, it does: 3.
- A successor that signed a complete fork by a chain signature with its own chain key later rotates: the fork stays final, no rotation voiding a chain act; a successor's signature act (type 1) on a fork is no signature on it, and the fork stays incomplete: 3.
- A closing relying on a creditor's release that an act of the collective in its history acknowledged: the creditor rotates and disowns the release; the closing stays complete, the release shown disputed; a release nobody acknowledged is shown, and the closing waits: 3.
- A closing whose history names an act the verifier does not hold is unknown for it, never complete and never "not complete"; held, it is judged; the same for a fork: 3.
- A collective releases its work by its own rules, then, holding nothing and owing nothing, closes; its later acts count for nothing; a closing while it holds the work, or owes anything, does not take effect, and completes once one debt is paid and the other ended by its creditor's release after a partial payment; a release signed by anyone but the creditor ends nothing (rule 92): 3.
- A collective forgives a debt owed to it by the area reaching Money alone: the release counts once the area's holder signs, a member holding no area reaching Money signing changes nothing (rule 92): 3.
- A bankrupt collective settles with its creditor: a partial payment, a share of its work by a clone of its agreement, and the creditor's release for the rest; it then owes nothing, and, still holding its share of the work, stays open (rule 92): 3.

#### A broken collective

- A broken collective: a rotation whose clone's mark is false breaks it, every act of the broken stretch counting for nothing; a resignation in the stretch names the agreement in force before the broken act; a rollback naming the broken act, registering the resignations, under that agreement's constitutional change rule, brings it back, the broken act still shown; a rollback under another power, of another parent, or naming another act, puts nothing in force; no rollback once every voice has resigned; the last voice warned before leaving, and the last constitutional voice; the broken clone refused as the rollback's clone; grantees' acts of the stretch counting for nothing and the grants working again after the rollback; payments received in the stretch owed back unless signed anew; a declaration of absence made in the stretch registered by the rollback with its signatures, an acknowledgement in the stretch placing nothing; a rotation removing or garbling the declaration broken with the same rollback; no fork or closing while broken, whatever its line, and money owed back a debt before closing; a grantee's act after the rollback counting once it cites it; what the collective keeps judged by the offer a payment names; the contest act; a returned member's old resignation spent, and an old stepping down once the member holds the area again; money owed back repaid by a payment naming it, by either side's record, then no longer blocking a closing; a declaration neither public nor addressed to the member counting for nothing: 3.
- Money owed back from a broken stretch: repaid to the refund rail the payer's own claim names, never elsewhere; to the pointer the payer's own acts hold, never a newer one; to a bare key where its claim says; owed to nobody, it does not block a closing that names it; owed to a payer with no address, a notice sealed to them, its deadline passed, and a closing naming it: 3.

#### Made before, made after

- A record naming the latest act of every other sequence of the collective; an act of the collective in a sequence it does not name counts as made after it: 3.
- A signature from a device a member's resignation never mentions: placed by the act of the collective it signs, or the record naming it, it counts when that act is before the collective's line, whatever the device: 3.
- A signature placed after the collective's line that only a friend acknowledged counts toward nothing: an acknowledgement by another identity places nothing: 3.
- A record counts only with the signature acts it names; a signature arriving after it completes nothing there, and a later record stays in force: 3.
- A receipt on a branch the collective's line left out counts as made after it, unless the collective's keeper recorded it before the line; a keeper never places a member's signature: 3.
- After the line, the key holders fork from an act before it to backdate a receipt the departed holder of the area reaching Money signs: it counts as made after the line: 3.
- A departed holder of the area reaching Money completes, after the line, a receipt the collective signed before it, and it counts; an early signature counts where the collective acknowledged it before its line, and not otherwise: 3.
- The holder of the area reaching Money's own rotation forgets one of their devices: receipts signed from it before the collective's line registering the rotation keep the signature; a signature with the old key on a receipt made after the line does not: 3.
- A clone signed from a device a member's own rotation forgot, recorded before the line registering that rotation: the signature stays valid for the collective, and the clone stays in force: 3.
- A rotation declaring a constitutional clone with too few of its signature acts named puts nothing in force; a signature arriving after it completes nothing there: 3.
- A member resigns, the collective registers it and lives a year under that line; the member rotates and disowns the resignation: the line stands, every record after it stands, the resignation is shown as disputed, and the member comes back only by a constitutional clone naming them: 3.
- A verifier with a hole in a line's history voids nothing on it and takes no voice away: it answers unknown, and judges again once it holds the act; an action citing a decision it does not hold is unknown: 3.

#### Forked deals

- Two complete versions of one deal: the version before the split is the reference while the deal has two branches; an act following either branch counts, judged under that branch's latest version, a purchase included (the buyer is protected); a branch that grows settles nothing; only a complete version naming both branches (field 26) settles the fork, finally, the other branch never coming back; a tangled fork stays on its reference, a buyer under a version every party signed protected; a settling version signed by the parties of both branches, naming the discarded tip and citing it; only complete versions of the deal read as settling, a draft or a stranger's act changing nothing; the arbitrator settling on a party's request only; one numbering across a deal's splits, a gap the holder's alarm; a seller's client raises the alarm on a payment naming a version that does not descend from the one it holds, or one it does not hold, and a notice on an older version; a buyer's client checks the offer is current; a version beyond the named tip, citing the settlement or not, changing nothing; a tangled deal settled by a version naming every tip it discards, signed by the parties of every branch; only the judge of forks the reference names settling, none where it names none; a deal's split with no number, or a repeated one, breaking the plan: 1.
- A settled deal, then a version on the dropped branch the settlement did not hold, a version grown there by a party left alone, a second settlement by the judge holding the first, and a plain version on either branch: nothing reopens, the line in force stays; a settlement holding a newer version of a branch whose older version it names settles nothing; a settling version naming a version not held is a plain version until held; the judge of forks silent past its period, the next link settles: 1.