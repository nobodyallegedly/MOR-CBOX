# MIR: Money

*Draft 11, freeze-ready, for Nobody, allegedly, to approve.*

## How to read this MIR

Normal text is the protocol. Italic text is commentary: reasons, examples and plain summaries. A summary that starts *In short* repeats what the rule says; where the two seem to differ, the rule's own text decides.

The rules are numbered in reading order. A rule of another MIR is cited by that MIR's name and the rule's number ("Agreements rule 12").

Some marks recur:

- *Client conformance:* a duty of clients (wallets, a payer's or an owner's client). The protocol cannot check it from acts alone, so it is stated for builders.
- *Stated cost:* a weakness the rules accept knowingly, written down so nobody mistakes it for an oversight.

The MIR has four parts. Part one is a payment as it runs most of the time: where it goes, how it is proven, who is seen, debts, buying and refunds, fees. Part two is the rare cases: routes across rails through conversion services, and theft of the signing key. Part three is the act formats. Part four is the tasks, the reasoning, the parameters and the freeze scenarios.

## Purpose

Money defines how a simple payment moves through MOR: never the rails themselves, and nothing about who owns what. A payer pays a payee pointer; Money's job ends when the money reaches it. How the money is then divided among owners is defined by their agreement, in Agreements.

Money defines what a settlement means to the protocol, whatever rail it happened on. Whatever the rail, Lightning, stablecoin or fiat, the receiver signs the same settlement receipt, and only the rail's proof differs, checked by the rule of the rail Module that defines it. When payer and payee use different rails, a conversion service, an identity that receives on one rail and pays on another, carries the money in between, and the payment reads as one verifiable chain of receipts. If a route breaks, the receipts show exactly which hop and which agreement failed. And because every payment can be recorded from both ends, a receiver cannot make money invisible by staying silent.

It also gives every client a shared language for:

- who owes whom, how much, in what unit (obligations);
- where payment to an identity (a person, an organisation or a collective) goes (payee pointers), with a flow that is easy to change and a vault that is not.

A payment to a work or a person, such as a tip, needs only this MIR and the payee pointer.

## Dependencies

Identity, Envelopes and Text; and, of Development, the specification format alone: a Money client decodes a rail Module's specification as Development's Part three says ("How the content is decoded", "Not in the format: no specification", rule 10), to know whether it is one and to read its fields 5, 11 and 13, and reads nothing more of Development. Money never depends on Agreements: a Money-only client, such as a wallet that sends tips, is a complete client. Where a Money state depends on Agreements, a Money-only client shows it as unknown. *A purchase names an Agreements claim (rule 18): a Money-only client cannot make one, and shows the claim a receipt names as unknown; buying a work under an Agreements claim needs a client that reads Agreements.* **An obligation's states are shown by clients that read Agreements; a Money-only client shows them as unknown** (rule 15).

## Words used in this MIR

### Rails and units

- **Rail.** A payment system outside MOR (Lightning, on-chain Bitcoin, ecash, stablecoins, cards, bank transfers), used through a rail Module.
- **Payment cMIP.** The cMIP that fills the payment task for an agreement (task 6): it defines how rail Modules plug in, how a receipt and a claim carry a rail's proof, and how they are checked. An agreement names at most one (Agreements; Development rule 8).
- **Rail Module.** A Module under a payment cMIP for one rail: the rail's address and vault-source formats, its proof format, and its verification rule. **Its specification states the rail's kind, a request rail or a push rail (Development, field 11), and what its proof binds, of payee, purpose and previous hop (Development, field 13)**; a verifier reads both there, never from the Module's code. *A rail Module's specification without them is no specification (Development): a receipt or claim naming it is unknown to every client.* Several rail Modules work under one payment cMIP, one per rail (Development rule 9). *A Module is a specification: it signs nothing and holds no money; the identities acting under it do.* *A proof binding the purpose binds the payer the payment commits to, both carried in one commitment (Development, field 13), so on such a rail a buyer is "the payer the payment commits to" (Agreements rule 22).*
- **Unit.** What an amount is counted in, in its smallest indivisible part. A unit is a small specification of its own, which rail Modules reference, so the same unit has one name on every rail that carries it.

### Where payment goes

- **Payee pointer.** Where payment to an identity goes: one or more rails in order of preference, each naming the rail Module that carries it. Together with the vault, it names the rail Modules the payee accepts.
- **Flow pointer.** An identity's payee pointer for ordinary incoming payments, changed with the signing key.
- **Vault.** An identity's declared set of safer receiving entries, per unit and per rail, changed only with the chain key. The vault is the grammar for setting up safer pointers, if the owner wishes to.

### Proving a payment

- **Hop.** One movement of money on one rail, from one party to the next.
- **Settlement receipt.** The common proof of one hop, signed by its receiver, the same for every rail: who paid whom, how much, in what unit, under which agreement.
- **Payment claim.** A payer's record of a payment, carrying the rail's proof. Evidence on equal footing with a receipt.
- **Verification answer.** The result of checking a receipt or claim against its rail: valid, invalid, pending or unknown, with the hash of the rail Module whose rule computed it, and the trusted party it relied on, if any.
- **Batch.** A set of payouts a split service pays with one rail payment, as its grant allows (Agreements).

### Debts and purchases

- **Obligation.** A signed statement by a debtor that it owes a creditor an amount, in a unit.
- **Discharge.** A complete route, or several, that bring the owed amount to the creditor's payee pointer.
- **Creditor's release.** A signed statement by a creditor that it lets an obligation owed to it go, wholly, without full payment (type 4; Agreements rule 92).
- **Purchase.** A payment for a work under an Agreements claim, naming the claim it pays under (receipt and claim field 9, rule 18).

### Routes

- **Route.** One simple payment carried in several hops, across rails or units, through conversion services. Each receipt names the previous one; where the next payee signs no receipt, the paying hop's claim may also name it.
- **Conversion service.** An identity that receives on one rail or unit and pays on another, at the rate of its own offer: it signs a receipt as receiver, with a forward, and is the payer of the next hop. A party, not a specification.

## Part one: a payment as it runs most of the time

*How it runs most of the time:* a payer reads the payee's pointer and vault, and pays on a rail the payee accepts. The payee signs a settlement receipt; the payer may sign a payment claim, carrying the same rail proof. Anyone holding either can check it by running the rail Module's rule. A debt is an obligation the debtor signs, discharged when payments naming it reach the creditor's pointer.

### 1. Where a payment goes

#### Rule 1. The pointer that counts

The payee pointer that counts is the latest of an unbroken, unforked chain. Two pointers naming the same predecessor are contested: clients use the last pointer before the fork and warn the owner.

#### Rule 2. The flow and the vault change with different keys

A flow pointer is changed with the signing key. The vault is changed only in genesis or a rotation.

#### Rule 3. Vault limits, per unit, fail closed

*In short: where a vault is declared, each unit has its own limit; a payment above it goes to the vault, and a unit the vault does not cover cannot be paid to the flow at all. With no vault declared, everything goes to the flow.*

The vault and limits that apply to a payment are those the payee's chain declares. So a vault changed by a rotation, flow off included, applies at once to every payment not yet shown as made; a payment it affects is protected as rule 29 says.

An identity that declares no vault has no vault rule: every payment goes to its flow, and the owner has chosen no protection.

Where a vault is declared:

- a single payment in a unit counts as paid to the flow only if the vault has an entry for that unit and the payment is no larger than that unit's limit;
- above the limit it MUST be paid to the vault, under any entry of that unit;
- a limit of zero means every payment in that unit goes to the vault.

**Where the vault has several entries for one unit, the smallest of their limits is that unit's limit**: no larger single payment counts as paid to the flow, and a limit of zero on any of them turns the flow off for that unit.

**The same limit bounds a refund paid to a payer's refund rail** (rule 17).

A payment in a unit for which the declared vault has no entry MUST NOT be paid to the flow: it is undeliverable until the owner adds an entry by rotation, and stays an open obligation (rule 5).

The vault is public, so a payer's client MUST check it and pay where the rules say.

*Fail closed is the safe side: a thief with the signing key cannot open a unit the vault does not cover. A missed payment is acceptable as long as the debt can be honoured later: it stays open and is paid once the unit is added (rule 5).*

#### Rule 4. The payee decides which rails it accepts

*In short: the agreement names the payment cMIP; the payee's pointer and vault name the rails; a receipt on any other rail counts for nothing.*

An agreement names its payment cMIP (Agreements, terms field 2, task 6). The payee's flow pointer and vault name the rail Modules it accepts, one in each rail or entry.

A receipt or claim counts only if all of these hold:

- the rail Module its field 0 names is the one named by the rail of the payee's pointer, or the entry of the payee's vault, that the payment was paid to;
- that pointer is the payee's own and counts for that payment by rules 1 and 17;
- that pointer and vault are the ones rule 29 selects for that payment, as the chain stood for it (rules 3 and 29);
  - **except that a collective's pointer that a fork or closing of the collective left outside its history still counts for payments made before the ending's line, as the chain stood for each, and for none made after it** (Agreements rule 91);
- where the payment falls under an agreement, that rail Module implements the payment cMIP the agreement names (Development, field 5).

A receipt or claim naming any other rail Module counts for nothing, whatever its rail's proof shows.

A refund paid to the refund rail the payer's own claim names (key 7) meets this rule's first two conditions as a payment to a rail of the payer's own naming, within the bound rule 17 sets; the others apply to it as to any payment.

A payer whose rail the payee does not accept cannot pay on it, except through a conversion service that pays the payee on a rail it accepts.

*A new rail Module therefore needs a new payee pointer (the signing key) or a new vault (the chain key), never a clone of an agreement; for agreements already signed, the payee's client signs one act on each, citing the new pointer (rule 17). In a collective, the pointer is a Money act, so the holders of the area reaching Money choose the rails (Agreements rule 60). Clients may build in cross-rail services charging a fee, giving their users free choice of currency and rail while delivering to the payee what it can receive.*

#### Rule 5. Undeliverable payments

When a receiver has no payee pointer, or no rail the payer shares, or the vault rules leave nowhere to pay, the obligation stays open: a visible debt owed to that hash until it can be paid. Conversion services may bridge rails. How long an obligation stays open, and whether it is redirected, is for the agreement to define (Agreements).

#### Rule 6. The owner is informed

*Client conformance: whether a notice arrived can never be checked, so these are not validity rules; the inbox already carries them.*

- **No clock.** Before genesis, or a rotation, that leaves the identity with no declared clock, the owner's client SHOULD say plainly that a theft's loss will be the owner's whatever it anchors (rule 29).
- **Before signing.** The owner's client SHOULD warn before the owner publishes a standing offer, or signs terms, priced in a unit the owner's declared vault does not cover: nobody could pay it.
- **Debts.** The owner's client SHOULD show an obligation owed to the owner in a unit the vault does not cover as "owed in a unit your vault cannot receive", with the way to add it (a rotation).
- **Spontaneous payers.** A payer's wallet that refuses a payment under rule 3, or finds nowhere to pay under rule 5, SHOULD send the payee an ordinary message to its inbox route (Identity; Envelopes, "How acts reach people"), saying what it tried to pay, in which unit, for what, and why it could not. The message is a text act like any other: it creates no obligation and no act type.

### 2. Proving a payment: receipts and claims

#### Rule 7. One format on every rail, signed by the parties

Receipts and claims on every rail use the formats of this MIR (Part three). The rail's proof is the only part that differs between rails, in the format the payment cMIP and the rail Module define.

A receipt is signed by the receiver of the hop:

- with its own key, or with a grant key it granted whose grant reaches receipts (a person's or a collective's);
- for a collective, with its own key or a grant key of its, its split service's among them;
- for a payee of a deal naming a split service, with its own key or the grant key it granted the service, or a backup the split cMIP's fields name (Agreements field 29);
- a split service's grant key, a collective's as a payee's, signs only receipts for money coming in under the grantor's own claims and offers, never one whose payer is the service nor a payout the grantor is owed, save a layer's incoming share on the owner's behalf with the rail's proof behind it (Agreements rule 16);
- each such receipt is judged by Agreements (Identity's scoped key, Agreements rule 80); a Money-only client shows such a receipt's standing as unknown.

**A collective's Money acts and its holders' member keys.** Every Money act a collective signs, its payee pointer (type 0), an obligation it owes (type 1), a receipt it gives (type 2), a claim it makes (type 3) and a release it grants as creditor (type 4), is signed with the collective's own key, or with the member key of a holder of an area whose reach includes that act type: the key the holder's signature of the collective's founding or of a clone names, counting as Agreements rule 73 says.

Each such act is judged by Agreements (a member key's act as scoped, Identity rule 8), under the area that reaches its type and that area's holders and threshold, as every act of an area is (Agreements rule 60): it counts where the holders of that area meet its threshold as Agreements rule 65 counts it, the other holders signing it as Agreements rule 60 says, its limits judged as for every act of the area (a pointer moves no amount). An act of the collective that no area reaches counts on the collective's own signature, and one signed with a member key there counts for nothing (Agreements rules 60 and 73). A split service's grant key signs for a collective under its grant, judged under the area that issued it (Agreements rule 74). A Money-only client shows the standing of every such act unknown, and pays no collective through a pointer it cannot judge.

*Why: money matters belong to the area the constitution gives money to, and nobody holds a special position the constitution does not give. Stated cost: a Money-only wallet shows a collective's Money acts unknown, and cannot pay a collective whose pointer its Money area signed; it needs a client that reads Agreements.*

A claim is signed by the payer, or, for a payer that committed to a bare key, by any identity carrying that key's signature in key 8. **The payer may sign it with a grant key it granted whose grant reaches claims** (a person's or a collective's), judged as scoped, by Agreements, a Money-only client showing its standing unknown.

*Why: a grant key's act is its grantor's own (Agreements rule 73), so an application holding a person's grant to itself signs that person's claims, and a payee's application its receipts, within the grant's reach and limits, and every reader sees on whose authority. A receipt or claim under a grant that does not reach its type, that the grant's revocation does not hold, or that lies past its limits, counts for nothing (Agreements rule 80), save as the next paragraph says.*

A receipt or claim under a revoked grant that Agreements shows disputed rather than void (Agreements rule 76) is treated as a disputed act after a change of keys (rule 29): it is shown, and for money it holds no pointer, signs no receipt and selects no version, unless its grantor adopts it (Agreements rule 76). *A payment already made stays made on its rail, and a receipt of it the payee signed with its own key stands.*

A specification never signs one.

A claim's key 8 MUST verify under the key it names, over the bytes shown, or the claim is invalid.

#### Rule 8. Checked by the rail Module's rule

A receipt or claim is verified by running the published rule of the rail Module named in its field 0 against the rail's proof. The answer is valid, invalid, pending or unknown, with that Module's hash. Anyone can run the same rule independently.

#### Rule 9. A trusted party, named

Where a rail cannot give a proof anyone can check, as with card and bank rails, the rail Module's rule MAY rest on a named trusted party's signed confirmation. The verification answer MUST name that party, so everyone sees what the answer rests on.

#### Rule 10. Valid only when the answer is valid

A client MUST NOT treat a receipt or claim as valid unless its verification answer is valid.

#### Rule 11. Double entry

*In short: the payer's claim is evidence as good as the receiver's receipt; where they disagree, the greater amount counts until the receiver signs a matching receipt, and the payment's own commitment decides who and what it was for; where the rail binds no payee or purpose, the payer's claim decides what it fulfils.*

A payment claim is admitted as evidence on equal footing with a receipt. A payer MAY publish one at any time.

Where a claim and a receipt are of the same payment (rule 12) and disagree in amount, payee, what the payment fulfils, or, on a route, the previous hop's receipt it passes on (rule 24), the disagreement is shown as an open question on the receiver, and the greater amount counts as received until the receiver signs a receipt matching the payment.

**Where they disagree in payee, in what the payment fulfils, or in the previous hop's receipt, the payment's own commitment decides** (as for a purchase, rule 18):

- the act naming what the rail's proof commits to counts, the other counts for nothing;
- where the rail binds no payee or purpose, or no previous hop, the payer's claim decides what the payment fulfils and which forward it passes on, and the receiver's contrary receipt stays shown as a dispute on the receiver.

*It is the payer's money, and the payer chose its purpose in paying; a receiver cannot rename it afterwards and hold a payment hostage by never resolving it. Stated cost: nothing records what the payer chose in paying on such a rail, so a payer can later relabel a payment, an old tip as a debt payment; the receiver's receipt saying otherwise stays shown. It is rare, since a rail commonly binds payee and purpose.* **What a rail's proof binds, its rail Module's specification says, in field 13: the set of payee (0), purpose (1) and previous hop (2) the proof binds, required on every Module whose field 5 names a payment cMIP (a cMIP whose field 2 lists this MIR's task 6); a verifier reads it there, never from the Module's code** (Development's specification format). **Where field 13 holds part of the set, the two bullets above apply element by element: for each of payee, purpose and previous hop, the rail's proof decides where field 13 holds it, and the payer's claim where it does not, the receiver's contrary receipt shown as a dispute; where the payee is unbound, the payment address the payment was paid to (rule 4) decides whose payment it is.**

Where a payer holds a valid rail proof and the payee has signed no receipt, the claim alone shows the money arrived.

#### Rule 12. One payment, one count

What "the same payment" is, each rail Module says (for instance, on Lightning the payment hash; on-chain the output the transaction created). Money's rules that tell payments apart (this rule, rules 11 and 29) read it through the rail Module, never through the proof's bytes: two proofs of one payment (a payment mined again after a chain reorganisation) are one payment, counted once. *A rail Module SHOULD also make its valid proof unique for a payment in a given place (the on-chain rail Module accepts one index where Bitcoin's tree repeats a node); the identity of the payment does not rest on it.*

Records of one payment for one purpose are never shares of it: a hop's receipt and its payer's claims (rule 11, a second claim written once the rail answers included, rule 29), and one push payment's receipts by its several holders (below). Receipts or claims of one payment that name different purposes, payees or forwards MAY share it only if they name the same batch, and their amounts together do not exceed what the payment shows. Otherwise a verifier holding both counts neither until the receiver signs a receipt that resolves them. **On a route, forwards too:** receipts or claims sharing one payment under a named batch may each name a different previous hop's receipt, one payment carrying several forwards in parts, for claims as for receipts; the money never counts twice (rule 24).

**One push payment to several holders** (Agreements rule 12) is one payment: each holder's receipt carries its one rail proof, for the amount that holder received, without a batch. Where they name different claims, the payment's commitment decides (rule 18).

### 3. Receivers and senders

#### Rule 13. A receiver is public; its flows are its choice

A receiver's identity is always public. The visibility of its incoming flows is its choice, from counterparties only (the default) to fully public; public views SHOULD aggregate.

#### Rule 14. A sender is never required

The core never requires a sender's identity. A receipt's payer field MAY be absent; delivery then goes to a key the payer supplies (Envelopes: a bare device or application key). A payer's claim, when published, is the payer's own choice of disclosure.

### 4. Debts

#### Rule 15. Discharge, and an obligation's states

An obligation is discharged when valid routes ending at the creditor's payee pointer, each naming the obligation, sum to the owed amount.

A refund owed to a bare key (rule 19) ends where the claim carrying that key's signature says, and rule 4's pointer and vault do not apply to it.

An obligation's states are:

- unsigned (Agreements: the debtor has not signed the agreement);
- open;
- discharged (by payment);
- **released** (by its creditor, rule 16);
- past its terms;
- closed by clone;
- redirected.

***Past its terms*: Agreements decide when an obligation's terms have passed and what follows** (Agreements rule 33); Money only shows the state. Past its terms, closed by clone and redirected depend on Agreements; a client that does not implement Agreements MUST show them as unknown.

**An obligation's states are shown by clients that read Agreements; a Money-only client shows every state of an obligation as unknown, an obligation naming no agreement (an IOU) included.** *Open, discharged and released are Money's states, but judging them reads which payments count toward a debt and which pointer it is paid to (rules 17 and 29), which MOR 1's clients do where they read Agreements. A small Money view answering the IOU case without Agreements (an obligation naming no agreement, paid to the creditor's pointer or vault, acknowledged by the creditor) is noted for MOR 2.*

#### Rule 16. The creditor's release

An obligation is released when a creditor's release (type 4) names it, valid under Identity and signed by the creditor the obligation names; a release signed by anyone else ends nothing.

- A release ends the whole obligation, whatever was paid toward it. To forgive part, the debtor signs a new obligation for the rest and the creditor releases the old one.
- It counts wherever a verifier holds it, published or not, and ends an obligation whether or not that obligation binds yet.
- What it names in field 1 is a record only, never checked.

*Where the creditor is a collective, its release counts by its own rules, and is reached by the area reaching Money alone (Agreements rule 92); a Money-only client cannot check that, and shows such a release's standing as unknown.*

#### Rule 17. Which pointer a debt is paid to

*In short: a payment to the flow counts toward a debt only if it went to the pointer the creditor's own acts on that debt saw, or a later one; a payment to the vault counts. An obligation with no act of the payee's on its agreement counts only if paid to the vault, unless the payee acknowledges it. Money owed back follows the same rule; a refund to the payer's refund rail counts only up to the payer's vault limit, above it only at the vault.*

**The rule.** A payment to an identity's flow pointer counts only toward what has, as its version that counts, that flow pointer's version or a later one:

- for an obligation, the version selected below;
- for a payment under an agreement or offer, the version rule 29 selects;
- for a tip, the version of the payee pointer it follows.

Toward anything whose version that counts is earlier, it counts only if paid to the vault.

**The version that counts for an obligation** is the latest of the payee's pointer chain held by an act of the payee's own. The payee's acts are its acts on the agreement:

- its signature acts;
- its offer, where the payee signed the offer (an offer another identity signed is not the payee's act; the payee's act is then its signature accepting it);
- any later act of the payee's on the agreement.

The latest pointer any of them holds counts.

An act holds what is reachable through its citations (`prev`, `objects`, `acks`, `refs`, directly or through what they cite), never a hash merely written in its payload. For selecting the payee's pointer, the walk passes only through the payee's own acts, never through an act another identity signed, so what a drafter's terms or a debtor's IOU cite never reaches it. A forked pointer chain counts only up to the fork (rule 1).

**A payee's client signing an act that can pay its signer (its signature act on terms, its offer) MUST cite in `refs` the latest of the payee's pointers it can find where the payee's routes act says its Money acts are found, whichever device published it** (client conformance). *During a theft window this can cite a thief's pointer, as the stolen key can sign any of the payee's acts: the owner's cost until the lock change.* *An identity keeps one sequence per device, and its own acts cite only their own device's line; citing the latest pointer directly lets a deal signed on one device find a wallet published from another. Where the pointers are published is Money's business, not Identity's: homes stay the layer for keys.*

The version an obligation names is informative only: where it differs, the obligation is read as naming the version so selected.

An obligation with no such act, such as one naming no agreement act (field 4 absent), counts only if paid to the vault, unless the payee acknowledges it with an act of their own that holds a pointer; an Agreements client checks that.

*There is no clock to say when an act was made; what it cites says what it saw. The pointer is judged by the act of the one it pays, never by the act of the one who pays or drafts: a debt re-signed to name a pointer the payee's own act never saw, such as a thief's newer one, names a version that does not count for it. A payment to an older flow of the payee's still counts; without a clock nothing can tell whether it came after the payee left that wallet, so a payee who abandons a wallet keeps it able to receive or moves what arrives there, a stated cost.*

**Money owed back follows this rule exactly** (Agreements rule 93). A refund owed to an identity on a payment is paid:

- to the refund rail the payer's own claim on that payment names (key 7), where it names one, **counting as repaid only up to the payer's vault limit for its unit, as rule 3 reads it; above it, only once it reaches the payer's vault**;
- otherwise, to the pointer this rule selects from the payer's own acts on that payment (its claims for it, its acts acknowledging a receipt or claim of it), or to the payer's pointer in force where they hold none.

A payment to the payer's vault always counts, and the payer's own receipt counts wherever the money went. A payer that declares no vault has no vault rule (rule 3). **A refund owed to a bare key has no vault, so no limit**: it is paid where the claim signed with that key says (rules 15 and 19).

*Why the limit: a thief holding the payer's signing key during a theft window could sign a claim naming their own refund rail; the vault exists partly to protect large debts.*

*A repayment to any pointer the payer ever published would let a debtor repay a thief's pointer and be cleared; this rule and rule 29 stop that for every debt.*

### 5. Buying, refunds and referrals

#### Rule 18. A purchase names the claim it pays under

*In short: a payment for a work under an Agreements claim names that claim, except one following a lone seller's standing offer, which names none; whether it is a purchase, and what is owed when it is not, Agreements say; any refund is paid as rule 19 says.*

A receipt or claim of a payment for a work under an Agreements claim (a standing offer, or a publication carrying a work an agreement claims) names, in field 9, the claim it pays under: the claiming agreement, and the line at which the payer's client read it current (Agreements rule 12). The payment commitment carries it on the rail (payment cMIP).

**Whether the payment is a purchase, and what is owed when it is not, Agreements say** (rule 12, which also reads which claim a purchase may name); **a refund they owe is paid as rule 19 says.**

**Which claim the payment buys under is settled where the payment takes place** (Agreements rule 12):

- **on a request rail** (the payee's side commits to each payment before it is made, as the payment cMIP's commitment does), the claim a purchase names is the one the seller's request committed to;
- **on a push rail** (the payer pays an address with no request), **each holder of the work's stake in the claim named settles on its own chain whether its receipt is a sale** (Agreements rule 12, which says what each holder owes otherwise); a refund owed is paid by rule 19.

**Where the seller is a collective, Agreements say when a payment becomes a sale** (once the collective's actions chain records it) **and what is owed when it does not** (Agreements rule 12); a refund owed is paid by rule 19.

**The payment decides.** Where the receipts or claims of one payment (one rail proof) name different claims, the claim is the one the payment's commitment names. **A receipt naming another claim is a wrong receipt, shown as such, counting for nothing**: its commitment, recomputed from it, is not the one the rail proof carries, and the rail Module's rule refuses it (payment cMIP). The rest are judged as above. Until a verifier has the rail's answer, such a payment is **pending** (rule 8). *No payment is left undetermined.* **Which rails are request rails and which push rails, the rail Module's specification says, in field 11 (0 a request rail, 1 a push rail), required on every Module whose field 5 names a payment cMIP (a cMIP whose field 2 lists this MIR's task 6); a verifier reads it there, never from the Module's code** (Development's specification format). **A request rail binds payee and purpose: Development's format requires field 13 to hold both wherever field 11 is 0, or the specification is no specification.** *So on a request rail the proof always shows which claim the seller's request committed to.*

A payment to an identity's own pointer (a tip) or toward an obligation is no purchase and names nothing. *Stated cost: a Money-only wallet can pay a claimed work's publication, but cannot tell whether it bought it; what is owed when it did not, Agreements say (rule 12), and a Money-only wallet can still receive the refund (rule 19).*

**A payment following a lone seller's standing offer** (one naming no co-owners' agreement, Agreements rule 11) **names no claim**: the offer is itself the agreement with the buyer, and Agreements say what the payment accepts (rule 11).

**A payment for a stake transfer** (Agreements rule 10) fulfils the transfer (field 5), or is named in its field 4; what it buys, and what the seller owes when the transfer is an over-sale, Agreements rule 10 says; a refund owed is paid by rule 19.

#### Rule 19. Refunds to the committed key

*In short: an anonymous payer's refund goes to whoever signs with the key the payment committed to, never to whoever shows the rail's proof; a payment that committed to no key leaves its refund unclaimable.*

A refund owed on a payment whose payer is not named is owed to whoever signs with the bare key the payment committed to as its payer (receipt field 2). It is paid where a claim carrying that key's valid signature (key 8) says (key 7). Such a refund is outside rule 17's selection of a pointer, since its creditor is a bare key with no pointer or vault.

Presenting the rail proof proves nothing about who paid: on some rails the payee and every node on the route learn it (Lightning's preimage).

**A refund to a bare key is shown repaid** by the refunder's claim naming the payment refunded (field 4), whose rail proof shows the money reached where the claim signed with that key said: the same double entry as rule 11, a claim with a rail proof showing the money arrived.

A payment that committed to no key leaves its refund unclaimable: the obligation stays open and visible, and nobody can take it (what that means for a broken collective's closing, Agreements rule 93 says).

Until claimed and discharged, the obligation stays open and visible. Whether an unclaimed refund ever lapses is for the offer's terms (Agreements); the core sets no lapse.

*The payer's wallet supplies the key when it asks for the receiving address, and recomputes the commitment from what the payee's side signed before paying, so the payee's side cannot swap the key (payment cMIP).*

#### Rule 20. Referrals, and evidence of a rail Module's use

A referral counts as evidence for a role share (Agreements) only when it is signed by the payer, in a claim; a referral the receiver or its split service names on its own earns nothing.

The evidence that a rail Module carried a payment is the receipt or claim naming it in field 0, signed by a party; a Module signs nothing (Development rule 15).

### 6. Fees and attention

#### Rule 21. Payouts net of fees

A payout (Agreements) counts as discharged when its receipt shows the amount minus the rail fee the split plan says the receiver bears, and no more than the plan's maximum fee per payout, where the plan states one (a plan stating none sets no cap: Agreements rule 25); what the split service owes for a shortfall beyond it, Agreements say (the split plan, rule 25).

Amounts below a rail's minimum cannot be moved until they reach it (what the split service holds meanwhile, Agreements rule 24 says).

#### Rule 22. No fees for specifications

A plain payment in Money carries no fees for specifications beyond what the rail itself charges. The creators of cMIPs and Modules earn where there is an agreement (Agreements).

#### Rule 23. Attention

MOR only ever sees a payment and, in Agreements, its split. Tracking plays or views belongs to the cMIPs that choose it.

## Part two: rare cases

### 7. Routes across rails

*Most payments are one hop. Where payer and payee use different rails or units, conversion services carry the money in several hops, each with its own receipt, and these rules read the hops as one route.*

#### Rule 24. A valid route

*In short: a route is a chain of hops, each shown by its receiver's receipt or by the paying hop's own claim, each naming the hop before it and carrying on exactly what that hop forwarded.*

A route is valid when every receipt and claim in it is valid, each hop names the previous hop's receipt, and each hop's received amount equals the previous hop's forward. **A hop names the previous hop's receipt when a valid receipt or claim of that hop names it: the next payee's receipt (field 6), or the paying hop's own claim (key 10).** **Where a receipt and a claim of one hop name different previous receipts, rule 11 decides:** the payment's commitment, where it binds the previous hop (its rail Module's field 13 holds 2); otherwise the payer's claim, the receiver's contrary receipt shown as a dispute on the receiver. *This is double entry along a route, as rule 11 gives a single hop: where the next payee signs no receipt, the paying hop's claim with a valid rail proof shows the money arrived, and so that the forward was passed on.*

**Rule 12 bounds it:** claims of one payment that name different previous receipts count only as rule 12 lets receipts or claims share a payment: one payment, one count. A named batch lets one payment pass on several forwards in parts, for claims as for receipts; without one, neither counts until the receiver resolves them.

#### Rule 25. Only conversion hops forward

Only conversion hops forward. A conversion hop forwards in the next unit, at the rate its agreement defines.

#### Rule 26. Disclosure along the route

Each hop's receipt, or the claim that shows it (rule 24), MUST be disclosed at least to the next payee and to the final creditor, so the creditor can verify the whole route. Everything else may stay private. *A service relying on its claim to show a hop discloses it to the final creditor; undisclosed, the forward stays open for the creditor, so it is the service's interest anyway.*

#### Rule 27. A hop that forwarded nothing

When a receipt declares a forward but no valid receipt or claim of the next hop names it, as rule 24 reads them, the route stops at that hop. The forward stays an open obligation of that hop, under the agreement named in the forward.

#### Rule 28. Failure named

In every case, the receipts and claims name the exact hop and agreement that failed. What follows (refund, penalty, contest) is Agreements' business.

### 8. Theft of the signing key: anchor or bear the loss

#### Rule 29. Good faith

*In short: a payer who followed the payee's published pointer and vault is protected. When the owner, after a theft, changes the locks by a rotation and anchors that change on the clock it declared, a payment the change affects still counts if the payee's own receipt shows it, or if the payer's claim was anchored no later than the lock change's point (when its home quorum is anchored; before the first, where several lock changes affect it). Until the change is anchored, the owner bears the loss.*

**Which pointer a payment followed.** A receipt or payment claim names the payee-pointer act it followed, directly or through the obligation or agreement. For a payment under an agreement or offer with no obligation between, the pointer followed is selected as rule 17 selects it; rule 17 then applies to it as to an obligation's.

**Theft: anchor or bear the loss.**

A **lock change** is a rotation after which a payment that followed the payee's chain as published before it no longer follows the chain as published after it: a voided pointer, a vault limit lowered or removed, an entry or source replaced.

**Where the lock change is not anchored** (its home quorum not anchored on the payee's declared clock, below), a payment that followed the pointer and vault published before it counts as made: the owner bears the theft window.

**Where it is anchored**, a payment it affects (one that followed the chain as published immediately before it and no longer follows it after) counts as made only if:

- (a) **the payee's own receipt shows it**: a receipt on a line of the payee's that the rotation kept, or signed with a key the payee's chain binds after it, or signed by the payee's split service with a grant key whose grant still stands; or
- (b) **the payer's claim is anchored before or at the lock change's point**, each act judged by its earliest anchor.

**Which claims, and which anchors**: the payer's claims of the same payment as the rail Module says.

- **Where the clock's Module reads the rail's own proof as the payment's anchor** (a clock Module whose clock is the chain the rail pays on, and that rail's Module; *for instance the Bitcoin clock Module and the on-chain rail*), the payment's proof is that payment's anchor, its point the block that holds it, provided it passes the clock Module's check. The comparison is "the payment's block before or at the lock change's block", read through the payer's valid claims, and a pending claim's anchor counts for nothing.
- **On any other clock**, the earliest anchor among the payer's claims of the same payment counts, once one of them is valid: a claim written and anchored at payment, while the rail still answered pending, protects the payment once it is proven.

*Stated cost: a thief and a payer together can anchor a pending claim in the window for a signed transaction they hold back, broadcast that very transaction after the lock change, and be protected; the owner bears, having accepted on-chain payments under a clock that cannot see them, and closes it by declaring Bitcoin as its clock. Another transaction for the same commitment is another payment, and is not protected. Not chosen: only a valid claim's anchor counting, which let the owner void any honest on-chain payment by a lock change within the hour of its confirmations.*

**The version a protected payment was paid under.** Where a payment counts as made under this rule, the version it was paid under is selected from the payee's acts as they stood before the lock change, those the rotation voided included, so a debtor who paid a deal the stolen key re-pointed does not pay twice. Otherwise, for money, an act the rotation did not keep holds no pointer, signs no receipt and selects no version, whatever acknowledgement keeps it visible as disputed (Identity rule 34).

**Several lock changes.** Where several anchored lock changes affect a payment, it counts only if the claim is anchored before the first of them. A payment a lock change affects stays affected, even if a later rotation would let it follow the chain again.

**The lock change's point is when its home quorum is met and anchored** (Identity's receipt checks and Identity rule 28): the point at which the home receipts the home rule requires for the rotation to count are all anchored, each receipt passing Identity's checks; with a single home, that home's receipt.

- A lock change counts from when the homes the rule requires publicly hold it, not from when it was signed or one home took it in, so one signed and kept back, or held by one home only, has no point yet.
- For a self-hosted identity, which counts on its rotation alone (Identity rule 4), the rotation's own anchor is its point, a stated cost of that trust model.
- For a rotation that moves to new homes (a homeless rotation), the point is read from the new homes' quorum under the new home rule.

*As with a registered letter, what counts is not when it was sent but when its receipt was confirmed.*

**Anchors are compared on the clock the payee's chain declared before the lock change**: on the main reference where the lock change is anchored there; otherwise on the backup, and then a claim anchored on the main reference also counts as made, whatever its point. A lock change counts as anchored only on a reference that clock names; after it, a claim anchored on neither is not protected.

A payee that declared no clock has chosen no protection, as with no vault, and its lock changes count as not anchored, so the owner bears.

A payer's client MUST read the payee's clock before paying, as it reads the vault (rule 3). Anyone may anchor any act.

*Client conformance:*

- After a lock change, the owner's client MUST obtain the home receipts that make up its quorum and anchor them on the main reference, and on the backup only where it cannot use the main one.
- **It SHOULD anchor them through two independent anchoring services, or by its own transaction, at an urgent tier**, the earliest anchor counting (a SHOULD: a service may hold a paid hash back, unseen, up to its tier's deadline, and one working with a thief would let payments to the thief mined meanwhile count against the owner; the earliest of two anchors closes one service's delay; cost: two small prices, or one on-chain fee, per lock change, a rare event).
- A payer's client MUST write the payer's claim at the moment it pays and SHOULD anchor it promptly, and, on a rail whose proof is pending at payment, write a second claim once the rail answers valid; how is the anchoring cMIP's business.
- **A payer's client that cannot anchor on any reference the payee's clock names MUST say so before paying: the payment will not be protected against a lock change.** *A payee may declare a clock Module no payer's client implements; the payer would then anchor on nothing the rule compares, and a lock change could put every such payment in doubt (the cost rule 29 states, with the payer's defence gone). The verdicts stay as written; the payer is told before it pays.*

*There is no clock, so a payment made during the theft window and one made afterwards on an outdated copy of the payee's chain look the same; only an anchor obtained at payment proves which side of the lock change a payment fell on. The owner bears what the stolen key did until the lock change, and reports it by anchoring; after it, a payer whose client did not anchor bears the loss, by the choice of client: a client that moves other people's money, and takes a cut for it, is responsible for doing it properly. Stated costs: the owner, by a lock change after receiving payments, can put in doubt every payment it affects whose receipt the rotation did not keep (disowning its own receipt included) and whose claim is not anchored by the point; the lock change is visible on the owner's chain forever, counts only from a home's anchored receipt, and a client that anchors promptly defeats it. An old payment never anchored and never receipted can be put in doubt by a later lock change.*

The loss from a theft window falls on the owner until the lock change is anchored, never on a payer whose payment the payee's receipt or an earlier anchor shows. A payment that did not follow them (for example, paid to the flow above the limit) is not protected.

## Part three: act formats

All acts are Envelopes MIR acts. Payee pointers are public; obligations, receipts, payment claims and creditor's releases are private by default, visible to their parties.

```cddl
amount = [ unit: hash, value: uint ]      ; unit: the unit's specification; value in its smallest part
rail   = [ module: hash, address: bstr ]  ; a rail Module and the rail-specific address data, which only that Module reads
```

### Payee pointer (type 0)

```cddl
payee-pointer-payload = {
  0 => hash,               ; payee: the identity this pointer is for, which is also its only signer (for a
                           ;   collective, signed with its own key or the member keys of its area reaching Money, rule 7)
  1 => uint,               ; version: 1, then the previous version plus one
  ? 2 => hash,             ; the previous payee pointer for this payee (absent for version 1)
  3 => [+ rail]            ; rails in order of preference
}
```

For an identity, this is the **flow pointer.** Pointers form a chain: each names the one it replaces and carries its version plus one, so nobody can jump ahead, and a fork is visible and contested. A fork behind an act the owner's later rotation kept is settled by that rotation.

**Only identities have payee pointers.** A payee pointer is valid only if signed by the identity it is for, with that identity's own key; collectives sign their own, **with their own key or with the member keys of the area reaching Money, judged by Agreements as rule 7 says.** A publication has no pointer of its own: it carries a price, and payment goes to the pointer of the identity named in its `for` field, or, if none, of its signer (Envelopes). *A Money-only wallet needs nothing else: it reads `for`, follows that identity's pointer, and pays. Whether a grant backs the `for` is Agreements' business.* Where there are splits, that identity's pointer leads to its split service (Agreements).

```cddl
price = amount   ; carried in a publication's media entry (Envelopes, key 7)
```

The publication's price is the plain-payment price, for Money-only clients. Where the owners also publish a standing offer, the offer governs deals made under Agreements.

### The vault

Declared in the Identity MIR's declarations slot, in genesis or a rotation, so only the chain key can change it:

```cddl
vault       = [ MONEY, 0, [+ vault-entry] ]
vault-entry = [ unit: hash, rail-module: hash, source: bstr, limit: uint ]
; source: what the rail Module needs to obtain a fresh receiving address: a descriptor or extended key to derive one from, the key of a node that issues one (Lightning), or a fixed address for rails with neither
; limit: the largest single payment in this unit that may go to the flow; 0 means flow off for this unit
```

Several entries may share a unit, on different rails, so a dead rail does not make the vault unreachable until the next rotation. For each payment, a fresh receiving address is obtained under the source as its rail Module defines: derived from it where the rail allows derivation, or issued by the node the source names, so a vault never reuses an address by protocol design. *The core does not know what a source is; the rail Module does. On Lightning, the source names the vault's own node, whose invoices a payer verifies against it; nothing is derived offline.* A null value for kind 0 removes the vault (Identity).

### The clock

Declared in the Identity MIR's declarations slot, in genesis or a rotation, so only the chain key can change it, as the vault is:

```cddl
clock = [ MONEY, 1, [ main: [ hash, any ], ? backup: [ hash, any ] ] ]   ; the main anchoring reference and, optionally, a backup, each a clock Module under the anchoring cMIP and its parameters, naming one clock (Envelopes, task "Anchoring"), in that order (rule 29)
```

*Name your services, name your clock. An anchor is compared with another party's anchor, so somebody must name the reference, and it must be the party a thief cannot be: the owner, with the chain key. A null value for kind 1 removes the clock.*

### Obligation (type 1)

```cddl
obligation-payload = {
  0 => hash,               ; debtor: identity or collective; the signer
  1 => hash,               ; creditor: an identity
  2 => amount,
  3 => hash,               ; the creditor's payee-pointer act the debtor names; informative only: the version that counts is the one rule 17 selects
  ? 4 => hash              ; the agreement or offer it arises from (Agreements)
}
```

An obligation is signed by the debtor. A creditor's statement of what it is owed is a claim, never an obligation.

### Settlement receipt (type 2)

```cddl
receipt-payload = {
  0 => hash,               ; rail: the rail Module that carried this hop
  1 => bstr,               ; the rail's proof, as the payment cMIP and that rail Module define it
  ? 2 => payer,            ; payer: identity or collective, or an anonymous payer's bare key (rule 19); absent if the payer stays anonymous and committed no key
  3 => hash,               ; payee: who received on this hop
  4 => amount,             ; how much was received, in this hop's unit
  5 => hash,               ; fulfils: the act this hop follows (an obligation, agreement, offer or payee pointer, for instance)
  ? 6 => hash,             ; the previous hop's receipt (absent on the first hop)
  ? 7 => forward,          ; conversion hops only: what is passed on to the next hop
  ? 8 => hash,             ; batch: the batch this receipt belongs to, when one rail proof covers several payouts (Agreements)
  ? 9 => purchase          ; a purchase: the claim it pays under (rule 18)
}

purchase = [ agreement: hash, line: hash ]   ; the work's claiming agreement (Agreements), and the line at which the payer's client read it current

forward = [ next-payee: hash, amount, agreement: hash ]   ; amount in the next hop's unit
payer   = hash / signing-key                              ; signing-key as Identity defines it: [ scheme, key ]
```

**What a payment fulfils.** Field 5 names the act the payment follows. Money judges it only where it names an act to which a rule of Money or Agreements gives a payment following it a meaning: an obligation to discharge, an agreement or offer to follow, a payee pointer, a stake transfer (Agreements rule 10), a receipt or claim of a payment it repays or refunds (rule 19; Agreements rule 93), a take-up of an issuance (Agreements rule 96), a split (Agreements rule 23), for instance. A payment whose field 5 names any other act fulfils nothing in Money's sense: its field 5 discharges nothing and changes no verdict of this MIR; the receipt is otherwise judged as any receipt is. *An act Money reads for another purpose, such as a vault's declaration, is not thereby an act a payment fulfils.* *A payment cMIP may give such a payment a meaning of its own, judged from the act alone; Money does not read it. The examples in the field's comment are not a closed list.* The same holds for a claim's field 4.

**An anonymous payer's key.** A payer who stays unnamed but wants a refund to remain claimable puts a bare signing key of its own, in Identity's `signing-key` form and used for this payment only, where the payment names its payer: in the payment commitment the payment cMIP defines, and so in the receipt's field 2. *It names no identity: the key is fresh, and links to nothing else the payer signs. It is not the delivery key of rule 14, which is an encryption key and cannot sign; a payer who wants both supplies both.*

A receipt is signed by the **payee of the hop**: the party that received the money. The previous hop's receipt is also named in `objects`, so a route is a chain in the Envelopes MIR's sense. *No receipt carries bytes nobody checks, and a referral is evidence only when the payer signs it (rule 20).*

A receipt carries no split. The final payee is the payee pointer; if that pointer belongs to a split service, the split and each payout are defined by the owners' agreement, in Agreements, and each payout is itself a simple Money payment with its own receipt.

### Payment claim (type 3)

```cddl
claim-payload = {
  0 => hash,               ; rail: the rail Module used
  1 => bstr,               ; the rail's proof
  2 => hash,               ; the payee paid
  3 => amount,
  4 => hash,               ; fulfils: the act this payment follows, as receipt field 5 ("What a payment fulfils")
  ? 5 => hash,             ; the receipt this claim disagrees with, if any
  ? 6 => referral,         ; referral: who led the payer to this payment, signed here by the payer (Agreements, role shares)
  ? 7 => rail,             ; where a refund owed on this payment is to be paid (rules 17 and 19)
  ? 8 => anonymous,        ; an anonymous payer's claim: the key the payment committed to as payer, and its signature (rule 19)
  ? 9 => purchase,         ; a purchase: the claim it pays under, as in the receipt (rule 18)
  ? 10 => hash             ; the previous hop's receipt, where this payment passes on a forward, as receipt field 6
                           ;   (rules 24 and 27)
}

referral  = [ identity: hash, evidence: hash ]   ; the referrer, and the act (a repost, a page) the payer followed
anonymous = [ key: signing-key, sig: bstr ]       ; sig: by key, over tagged_hash("MOR/finance/anonymous-claim", [ field 0, field 1, field 2, field 3, field 4, field 5 or null, field 6 or null, field 7 or null, field 9 or null, field 10 or null, inside key 3 or null, inside key 7 or null, inside key 8 or null ])
```

A claim carrying key 8 is the claim of the payer that committed to that key. Its signer, the act's signer, may be any identity, a one-time identity where the payer wishes to stay unnamed; the payer is the key, and the signature in key 8 binds it to this claim's rail, proof, payee, amount, purpose, disputed receipt, referral, refund rail, purchase and previous hop, so nobody can lift it onto another claim. The signed bytes are the array shown, encoded in deterministic CBOR, with null where key 5, 6, 7, 9 or 10 is absent; the signature also covers the act's own citations, its `objects`, `acks` and `refs` (inside keys 3, 7 and 8, null where absent), so nobody can re-wrap the payer's claim with other citations or acknowledgements. For an anonymous payer's claim, its history for rules 17 and 29 is only what those covered citations hold; its `prev`, which belongs to whoever signed the act, is not the payer's and does not count. The signature scheme is the key's, as Identity defines it.

A payment claim is the payer's side of the record. It carries the rail's proof, so anyone can verify that the money reached the payee's rail address. It is private by default like every act; publishing it, to a counterparty or more widely, is the payer's choice and reveals the payer only to whom the payer chooses.

### Creditor's release (type 4)

```cddl
release-payload = {
  0 => hash,               ; the obligation it ends
  ? 1 => [+ hash]          ; what the creditor took instead, for the record: receipts, an Agreements agreement it was traded for, stake transfers; never checked
}
```

Signed by the creditor the obligation names, alone; nobody else's signature counts, and none is needed. Field 1 names each act once. *Forgiving a sum only gives up money, a simple money decision, so the release is a Money act (Agreements' type 21 is retired). A release given against future terms (a share of income, stakes) is also a deal, an Agreements agreement signed on its own, which field 1 names (Agreements rule 92).*

## Part four: tasks, reasoning and freeze scenarios

### Tasks

- **Payment.** One payment cMIP per agreement fills this task. It accepts an act it follows (an obligation, offer or payee pointer, for instance; "What a payment fulfils"), an amount, and the payee's flow pointer or vault entry the payment goes to. It defines how rail Modules plug in under it, one per rail; how a payment on a rail carries what the receipt and claim will say (who is paid, how much, for what, by whom); how a receipt and a claim carry a rail's proof; and how a verifier checks them, by running the named rail Module's rule (valid, invalid, pending, unknown, and the trusted party relied on, if any). Each rail Module defines its rail's address and vault-source formats, how a fresh receiving address is obtained under a vault entry's source, its proof format, and its verification rule; **its specification states the rail's kind (Development, field 11) and what its proof binds (Development, field 13), and the payment cMIP reads both from the specification, never from code.** The receiver signs the receipt; the payer signs the claim; the cMIP and its Modules sign nothing.
- **Conversion.** A cMIP defining how a conversion is offered and evidenced: a conversion service, an identity, receives on one rail or unit and pays on the next at the rate of its own offer, and signs a receipt with a forward in the next unit; its onward payment is a hop like any other, under the payment cMIP.

Splits are an Agreements task. A custodial flow service, which holds an identity's hot pointer and enforces a rate off-protocol, is an identity, a party the owner points the flow at; it may run any Module it likes, and it is not one.

### Reasoning

- **Money ends at the payee pointer.** *A payment goes to the pointer set by the owners of what is paid for. How it is then divided is the owners' agreement, which is Agreements. That keeps Money simple enough for any wallet, and keeps every rule about sharing in one place.*
- **One receipt, every rail.** *Rails come and go; the core must outlive them. If every rail speaks the same receipt, a route across Lightning, a stablecoin and a bank transfer reads as one chain. Only the rail's proof differs.*
- **Rails are Modules under one payment cMIP.** *An agreement names one cMIP per task; if each rail were a cMIP, every agreement would be confined to one rail. So the payment cMIP is the common language of an agreement's payments, and the rails plug in beneath it, as many as the parties use. A text cannot sign: receipts are signed by the people who received, claims by the people who paid. Services that convert or hold money are parties with their own offers, not specifications.*
- **The owner is told what the vault refuses.** *Fail closed protects the owner from a thief, at the cost of payments that cannot arrive. A failed payment leaves no act behind, so nothing can make its absence a rule; the owner's own client and the payer's wallet tell the owner instead, through the inbox that already exists.*
- **Honest about fiat.** *Card and bank rails cannot give proofs anyone can check. Naming the trusted party in the verification answer keeps that visible instead of pretending otherwise.*
- **Double entry.** *The receiver signs the receipt, and the receiver is the party that benefits from silence. A payer's claim is the other half of the ledger: private by default, published by choice, and admitted as equal evidence. A mismatch between the two is itself the alarm. The measured promise is this: hiding income requires the payer's silence or collusion. Nothing more can be promised by a protocol that cannot see payments made outside it, and payer-side splitting cMIPs, where each owner is paid directly, are the models that need no service's honesty.*
- **Chained hops name where things break.** *When a route breaks, the last valid receipt and the missing next one point at exactly one hop and one agreement. Money says where; Agreements say what follows.*
- **The vault is a grammar for safer pointers.** *The flow is easy to change and easy to steal; the vault is where a thief with the phone cannot point payments. Three ways a thief takes money through the flow: the backlog, closed after the lock change because the version that counts for a debt is the latest the payee's own acts on a kept line hold, and only the debtor signs a debt, though during the window the stolen key can sign the payee's acts and re-point its deals, the owner's cost; the large payment, closed by a per-unit limit; and the stream between theft and rotation, which no clockless rule can cap. So the core gives the two ends of the range, per-payment limits and flow off, and leaves the middle to services that hold the hot pointer and enforce a rate for owners who need one. Detection is shortened by the payer's claim and the inbox: the owner's client sees money land where it did not point.*
- **Per unit, fail closed.** *A limit in one unit cannot be compared with a payment in another, and a thief with the signing key chooses the unit. So every unit has its own limit, and a unit with none goes to the vault. Where several entries cover one unit, the smallest limit applies: a limit per rail would let the payer's, or a thief's, choice of rail pick the laxer one.*
- **The receiver decides how it is paid.** *A payer cannot pay on a rail the payee does not accept. Rail Modules change far more often than agreements; if terms named them, every upgrade would need a clone, every party signing in a deal. So the agreement names the payment cMIP, and the payee's pointer or vault, which its owner changes alone, names the rails. A receipt on a rail the payee never named counts for nothing, so a collective's areas and two clients agree on every receipt.*
- **A refund goes to a key, not to a proof.** *On Lightning the payee and every node on the route learn the preimage, so "whoever presents the proof" could be any of them. A key the payer committed to before paying is held by the payer alone.*
- **A rail's costs, stated.** *A rail Module states what its rail costs its users, and clients show it. A Lightning rail Module, for instance, names the payee's node in its pointer or vault: a public node exposes its channels and the coins behind them, may expose its network address, links identities that share it, and a Lightning vault stops a thief with the signing key, not one who takes the node.*
- **One name per unit.** *A satoshi is a satoshi on every rail that carries it, Lightning or on-chain. Naming units by their own small specification stops every hop between two rail Modules from looking like a conversion. A test network's coin is a unit of its own: a regtest satoshi is not a satoshi.*
- **Good faith protects payers.** *A payer who followed the published pointer and vault did everything right. Theft is the owner's risk, and protection against it is a market.*
- **Private by default.** *Receipts and claims are acts like any other, locked, and disclosed only as far as the route, or the payer's choice, requires.*
- **Payments, not attention.** *Rewards attach to settlements, never to attention: MOR's iron law.*

### Parameters

Left to the payment cMIP and its rail Modules (the payment task):

- How a payment on a rail carries the id of the obligation, agreement or payee-pointer act it follows, and the rest of what the receipt and claim will say. *A payment commitment, one hash over the payee, the payer (an identity, an anonymous payer's bare key, or nobody), the amount, what it fulfils and where it was paid, which each rail Module carries on its rail (for Lightning, as the invoice's description hash, signed by the payee's node), is one such way.*
- Rail proof formats, and vault source formats: defined by each rail Module.

The unit specification: a specification of kind 2 (Development, Part three, "A unit"). Money reads only its spec hash: two amounts are in one unit only where they name the same hash.

### Freeze scenarios

*Each bullet names the scenarios of the freeze test suite v30 that cover it.*

#### Where a payment goes

- Payment for a publication to the identity in `for`; a Money-only wallet and a full client pay the same identity; the wallet checks the vault; for a work under an Agreements claim, only the full client's payment, naming the claim, is a purchase: 2, 3.
- Flow and vault; a thief changing the flow pointer cannot collect obligations that name an earlier version, nor re-issue them; a unit switch goes to the vault; a dead vault rail is covered by another; flow off: 1, 5.
- Several vault entries for one unit: the smallest limit applies: 1, 5.
- A receipt on a rail the payee's pointer names counts; one on a rail it never named counts for nothing; in a collective, the holders of the area reaching Money choose the rails: 3.
- A payment refused under rule 3 or 5: the payer's wallet sends the payee an ordinary inbox message; the debt stays open: 5.

#### Proving a payment

- Verification answers, including a trusted party named for a fiat rail: 1, 2.
- A payee pointer signed by another identity than the one it is for, an obligation signed by its creditor, a receipt signed by another than the hop's receiver, and a Money act with a field its format does not define or of a type Money does not define: invalid in every client: 2.
- Double entry: a receiver under-reports, the payer's claim exposes the gap, the greater amount counts; a payout arrives short of the plan's maximum fee: 2.
- A rail whose field 13 binds the payee and not the purpose: a receipt and a claim disagreeing in payee, the proof decides; disagreeing in purpose, the payer's claim decides, the receipt shown as a dispute; on a rail binding no payee, the payment address the payment was paid to decides whose payment it is: 2, 3.
- One proof claimed against two obligations counts for neither; one batched payout proof covers several payouts: 2, 7.

#### Receivers and senders

- Receiver visibility dial; sender never required; a payer's claim published by choice: 2, 5.

#### Debts

- Obligation signed by the debtor, and discharge, unit-agnostic, possibly by several routes: 1.
- A creditor's release, signed by the creditor alone, ends an obligation wholly after a partial payment; one signed by anyone else ends nothing; a collective creditor's counts only with the area reaching Money: 3.

#### Buying, refunds and referrals

- A payment for a claimed work naming no claim is no purchase, and is owed back to the payer; one naming the current claim is a purchase, a collective seller's once its actions chain records it; one naming a collective's claim a fork superseded, never recorded by the original's chain before the fork, is owed back; on a request rail, the claim the seller's request committed to is bought whatever came after; on a push rail, a payment two holders received is a purchase where both receipts came before their signatures on the new version, and refunded by both where one came after: 3.
- Two clients built with different readings of a rail Module read its kind (field 11) and what its proof binds (field 13) from its specification, and give the same answer on a push-rail purchase and on a disagreement in purpose; a rail Module's specification implementing a payment cMIP without them is no specification, and a receipt naming it is unknown: 2, 3.
- A request rail Module (field 11 0) whose field 13 lacks the payee or the purpose: no specification, and a receipt naming it unknown: 2, 3.
- Undeliverable payment kept as an open obligation; refund to an anonymous payer claimed by a signature with the key the payment committed to, which a routing node holding the rail's proof cannot make: 2, 5.
- A referral signed by the payer earns a role share; one named by the service alone earns nothing: 2.

#### Routes across rails

- A cross-rail route through a conversion service, read as one chain of receipts; one hop fails to forward, and the receipts name that hop and its agreement: 2.
- A conversion service pays on and the next payee signs no receipt: the service's claim naming the previous hop's receipt passes the forward on; two claims of one payment, naming two forwards and no batch, count for neither, and under a named batch count in parts; a receipt and a claim of one hop naming different previous receipts, the payer's claim deciding where the rail binds no previous hop, the receipt shown as a dispute; the claim disclosed to the final creditor: 2.

#### Theft of the signing key

- A payment to a pointer a lock change voids counts as made until the lock change's home quorum is anchored on the payee's declared clock, and after it only with the payee's kept receipt or a claim anchored by the point; a payment that ignored the vault is not protected: 1.
- A payee declares a clock Module the payer's client does not implement: the client says so before paying, the payment unprotected against a lock change (client conformance): 1.

#### Covered by Agreements

- Conservation and remainder rule; module fees and omitted fees; splits by attention metrics.