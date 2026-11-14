# Questions: every place a reading had to be chosen

For Nobody, allegedly. Written by the builder of `claude/agreements-c-2`, from `spec/agreements-draft-19.md` and `spec/report-block-2.md` alone.

Each entry gives the sentence, the readings seen, the one chosen and why, and whether it changes an answer a verifier gives. "The text" is `agreements-draft-19.md`; "the block" is `report-block-2.md`.

**There are 33 questions that change an answer a verifier gives (A1 to A33). They come first.** After them come 13 that do not change an answer, or change only how one is worded (B1 to B13). Things the text itself already lists as open (its sections 6 and 8) are here only where the build had to pick a side to go on.

---

## Part A. Questions that change an answer (33)

### A1. `resolves` without `continues`: rejected, or an act that resolves nothing?

- **The sentences.** Section 3.1, row `continues`: "exactly when `resolves` is present". Section 3: "An act missing a field marked required is not an Agreements act: it is rejected". Against section 3.2: "an act that names no `continues` resolves nothing", and G12.3: "An act that names `resolves` and no `continues` resolves nothing."
- **Readings.** (a) Such an act lacks a required field and is rejected: it never reaches the grammar rules. (b) It is an act like any other, which resolves nothing, and may itself be a tip.
- **Chosen: (b).** The text says twice what such an act does, which it could not do if the act never reached the grammar. `continues` without `resolves` is rejected, since nothing says what that would be.
- **Changes an answer: yes.** Under (a) the act is rejected; under (b) it counts, can be a tip, and can make a fork.

### A2. G14.3's "invalid": the Terms act, or only its answer about the baked work?

- **The sentences.** Section 2 lists among the cases of invalid: "Terms for a baked work that name neither the baker among their parties nor a succession (G14.3)", and says "An invalid act counts for nothing." G14.3: "the answer is **invalid**: they cannot count as an agreement for that work", and: "A verifier MUST give that answer with every answer about that work that rests on those Terms (a receiver, a filling, a leg report)."
- **Readings.** (a) The Terms act is invalid as an act: never in force, no receiver, nothing. (b) Only the answer to "do these Terms count as an agreement for that work" is invalid; the Terms are still read by every other grammar rule, and each answer that rests on them carries the answer about the work.
- **Chosen: (b).** G14.3 asks that the answer be given *with* a receiver, a filling and a leg report, which presupposes those answers are still given; and it says "cannot count as an agreement for that work", not "cannot count".
- **Changes an answer: yes.** Under (a) such Terms are never in force and nothing under them is answered.

### A3. Which place judges whether the first Terms act is in force?

- **The sentence.** G7.2: "Where the Terms in force name a place, an act within the agreement counts [...] only once the place shows it". G4.6: "per the agreement's place".
- **The gap.** Before the first Terms act is in force, no Terms are in force, so no place is "named by the Terms in force".
- **Readings.** (a) The first Terms act and its assents are judged per the place that act itself names. (b) Until something is in force nothing needs the place: as shown.
- **Chosen: (a).** It is the only place the agreement has, and G4.8(d) applies the same idea to an update (judged per the place the original names).
- **Changes an answer: yes.** An assent the place does not show does not count under (a).

### A4. Does a fork in one signer's line depend on the place?

- **The sentence.** G6.2: "A verifier holding both MUST answer **fork** for that signer". G7.2 makes an act count "for any answer about what is latest [...] only once the place shows it".
- **Readings.** (a) A fork in a line is seen from the acts held, whatever the place shows. (b) An act the place does not show is not a tip.
- **Chosen: (a).** "Holding both" is the test G6.2 gives, and (b) would let a signer hide a fork by keeping one tip out of the place. Every answer about a line is given **as shown**.
- **Changes an answer: yes.**

### A5. Is "built on a tip" carried through acts that rest on acts that rest on a tip?

- **The sentence.** G12.2: "nothing built on either tip counts: no act that follows a tip in the line, and no act that rests on a tip or on an act that follows it (it names it, in `terms`, `under`, `answers` or `relies_on`, or was made within or under Terms of that branch)."
- **Readings.** (a) As written: an act is built on a tip when it follows it in the line or itself names the tip or an act that follows it. (b) Also when it names an act that is only built on a tip by naming.
- **Chosen: (a),** the words as they stand.
- **Changes an answer: yes,** for an act two namings away from a tip.

### A6. A Terms act that replaces a version already replaced

- **The sentence.** G4.7: "Where two Terms acts each replace the same Terms (G4.8), and each has the assents its own `in_force` requires (G4.1), they are a fork". G4.11(a) hands the update condition "The version in force (G4.6) and the proposed update: a Terms act whose `replaces` names that version."
- **Readings.** (a) Any two updates of the same Terms are a fork, however late the second comes: if T was replaced by U long ago and a second, fully signed act names T, T is the last shared version again and U stops. (b) Only an act whose `replaces` names the version in force is a proposed update; a late act naming an older version replaces nothing.
- **Chosen: (a).** G4.7 and G12.1 define a fork from the acts alone and say nothing of when; and with no place nothing shows which came first. The cost is real: those the update condition accepts can take an agreement back to any earlier version by signing a rival to it (the text's own "A fork is then a veto", section 7).
- **Changes an answer: yes.**

### A7. "Everyone who signed the original": are parties bound without an assent asked only if the original still names them?

- **The sentence.** G4.8(a): "and each party bound by the original with no assent of their own, because the update condition of the version before it answered valid for it (G5.1(c))."
- **Readings.** (a) Only those the original itself names among its parties. (b) Everyone G5.1(c) binds, named or not.
- **Chosen: (a):** the word is "party", and a party is "one named in Terms". Otherwise a party an update took out (section 5.2, "A party wants to leave") would still be asked for every later default update.
- **Changes an answer: yes** (whether a default update is valid or not shown).

### A8. "A signer who fills a role" among everyone who signed, when the spot's cMIP is lacking

- **The sentence.** G4.8(a): "each party in whose name an assent to the original counts (a party named as a signer; a signer who fills a role; ...)".
- **Readings.** Where the K5 cMIP is not held, who fills the role is not known. (a) Every default update of such Terms is then unchecked. (b) Unchecked only where an assent to the original by someone who is not a named signer is held (who might be a filler); otherwise the fillers, if any, did not sign and are not asked.
- **Chosen: (b):** the least that G9.1 asks ("every answer that depends on it").
- **Changes an answer: yes.**

### A9. A counter signature with no place: which assents "complete the Terms" under an `in_force` cMIP

- **The sentence.** G4.10: "a counter signature that names the assents which complete the Terms came after them, and changes nothing; a completing assent that names the counter signature came after it, and the assent it cancels is not counted. Where neither names the other, whether those Terms are in force is **not shown**".
- **Readings.** With the default it is every signer's assent. With a cMIP (two of three) "the assents which complete" is not defined. Chosen: the `in_force` task is asked again with only the acts the counter signature names, directly or through others: if it answers yes, the counter signature came after completion. If any assent to those Terms names the counter signature, it came before. Otherwise the order is not shown.
- **One more case, not in the text.** Where the Terms would not be in force even leaving the counter signature out (the other party never assented), nothing has completed them: the counter signature is read as made before they were in force, as shown, and cancels.
- **Which counter signature is read against which assent.** "which follows that assent in the signer's own line: directly, or after other acts of that signer". A signer may assent, cancel, assent and cancel again. Chosen: an assent is read against the nearest counter signature that follows it in the line; a later counter signature follows an assent already answered, and is read only against the assents made after the one before it. The other reading (every later counter signature is read against every earlier assent) gives the same answers in every case I could build, at a cost that grows with every round.
- **Changes an answer: yes** (counted or not; in force or not shown).

### A10. G5.4: "the position at which the replacing Terms came into force"

- **The sentence.** G5.4: "by the act's position against the position at which the replacing Terms came into force; otherwise as shown, by citing alone (G13.3), and **not shown** where neither the act nor the replacement names the other." The text lists the position as open (section 8.4).
- **Chosen.** Per place: the later of the replacing Terms act's position and, for each party it names as a signer, the position of the first assent in that party's name. By citing: the act names the replacement, so it was made after; the replacement, or an assent to it, names the act, so it was made before; otherwise not shown.
- **Other readings.** The position of the replacing Terms act alone; or the first position at which the update condition and `in_force` were both met, computed from prefixes.
- **Changes an answer: yes,** in the window between a replacing act and its last assent.

### A11. After a fork is resolved, where does the next act attach?

- **The sentence.** G12.3: "that signer's next act MAY name as `previous` either the resolving update or the last act of the continued branch: both are read, and neither makes a fork." For Terms, which act a next version names after a resolving update is listed as open (section 8.4).
- **Chosen.** An act that names a resolving update is read as following the last act of the continued branch. So one act naming the resolving update and another naming the branch's last act are two updates of the same act: a fork. The same reading is applied to Terms (`replaces` naming the resolving update).
- **Other reading.** The two namings are two different positions and never fork with one another.
- **Changes an answer: yes.**

### A12. An act shaped as a resolving update where there is no fork

- **The sentence.** G12.3: "An act that names `resolves` and does not in fact resolve the fork it sits in [...] is an act like any other, and may itself be a tip: a fork is not hidden by adding the field."
- **Readings.** Where only one update exists and a second act names it in `resolves` and `continues`: (a) there is no fork to resolve, so the second act is an act like any other, a second update, and the two are a fork; (b) it is read as agreeing with the one update and nothing happens.
- **Chosen: (a).**
- **Changes an answer: yes.**

### A13. A power to resolve a fork in one signer's line: which kind?

- **The sentence.** G12.4: "For a fork of Terms, [...] it is an entry of `powers` of kind `resolves`, and of no other kind. [...] For a fork in one signer's line, it is a power to make that act for the signer (G5.1(b))".
- **Readings.** (a) Kind `resolves` for a line too. (b) The kind of the Statement that carries `resolves` (so a power of kind `assent` would let its holder resolve by an assent).
- **Chosen: (a).** A power to resolve is "part of the judicial" (G5.6(c)), named there as "an entry of `powers` of kind `resolves`".
- **Changes an answer: yes.**

### A14. An agreement that signs by all its signers: which acts are one signing?

- **The sentence.** G1.2: "Where x's Terms give no such power, x signs by every one of its signers: one act by each of them, its `for` naming x and its `under` naming x's Terms in force."
- **The gap.** For an assent, "the same act" is clear: each signer's assent to the same Terms. For any other kind nothing says which acts of the several signers belong together.
- **Chosen.** Built for assents. For another kind made for an agreement whose Terms give no power for it, the answer is **unchecked**, with that reason.
- **Changes an answer: yes.**

### A15. An agreement's assent and a counter signature

- **The sentence.** G4.10: "Whether such a party can itself make a counter signature, and by which act, is not said (P17)."
- **Chosen.** A counter signature cancels only an assent its own signer made in their own name. An assent made for an agreement, or for a person under a power, is not cancelled by any counter signature.
- **Changes an answer: yes.**

### A16. A Statement whose `agreement` is not the agreement of the Terms it names

- **The sentence.** Section 3.2: `agreement`: "fingerprint of the root"; `terms`: "fingerprint of the Terms act it is made within". Nothing says what follows if they disagree.
- **Readings.** Rejected; invalid; or not shown as a Statement within those Terms.
- **Chosen: not shown,** and it counts for nothing there. It is not in section 2's list of invalid cases, and it has all its fields. The same for a Terms act whose `agreement` is not the root of the Terms it would replace: not shown as a version.
- **Changes an answer: yes.**

### A17. A field that must hold a fingerprint and does not

- **The sentences.** G2.2: "A `body`, or a cMIP in a socket, that is not named by fingerprint makes the Terms **invalid**"; "An entry of `relies_on` that is not a fingerprint [...] are not read". Nothing is said of `replaces`, `agreement`, `terms`, `previous`, `under`, `envelope`, `resolves`, `continues`, `answers`, `settlement`, or a Statement's `kind` that is neither one of the three words nor a fingerprint.
- **Chosen.** For those fields the act cannot be read as carrying the field: **rejected**. `body` and cMIPs: invalid (G2.1). `relies_on`: left unread, in a Statement as in Terms.
- **Changes an answer: yes** (rejected, where another builder might say invalid).

### A18. A role whose `what` or kind is there but wrong

- **The sentence.** G10.1: a role MUST name "its `what`: the X, Y, Z or W, by fingerprint (G2)"; section 2: "a role that lacks its kind, its `what` or its cMIP (G10.1)".
- **Chosen.** A `what` that is not a fingerprint, and a kind that is none of the five, are read as lacking: invalid, G10.1. A role's cMIP that is there and not a fingerprint: invalid, G2.1. A `what` on a role of kind future is not read.
- **Changes an answer: yes.**

### A19. Terms with no author at all

- **The sentences.** Section 3.1: `authors` required. G3.1: "`authors` MUST name one or more of the parties".
- **Readings.** No `authors` is a missing required field (rejected), or a breach of G3.1 (invalid).
- **Chosen: rejected.** An author who is not a party named as a signer or a role (a stranger, or a party named as an agreement) is invalid, G3.1.
- **Changes an answer: yes** (rejected or invalid).

### A20. `previous` that names something held which is no act of an agreement

- **The sentence.** G6.1: "An act whose `previous` names an act of another signer, or of another agreement, is **invalid**".
- **Chosen.** Where `previous` names a report, a payment or another record that is no Terms act and no Statement, it is read as naming an act not held: the act counts and the line is unchecked before it.
- **Other reading.** Invalid.
- **Changes an answer: yes.**

### A21. The doer where `control` names a cMIP and the Terms name no party of their own as receiver

- **The sentence.** G11.2: "For K6, the doer is whoever carried out the passing on [...]: the party those Terms name as `receiver` among their own parties."
- **Chosen.** Where the receiver is another agreement, or none, no doer is shown for those Terms: a report with a doer's lines is listed as its signer's word and ties nothing.
- **Other reading.** The doer is the receiver for the act, followed through agreements (G1.4).
- **Changes an answer: yes.**

### A22. Who is "the receiving side" for a confirmation when no doer's report is held

- **The sentence.** G11.4(b): "A report ties only where its signer is a side of the leg: [...] a confirmation where its signer is the receiving side as G1.4 reads it. A report by anyone else is listed, as its signer's word (G8), and ties nothing." G11.2: "the receiving side of each leg is whoever something was passed on to".
- **The gap.** Who something was passed on to is known only from reports.
- **Chosen.** A doer's line "passed on to X" makes X (or X's receiver for the act, where X is an agreement) the receiving side of that line: only that signer's confirmation ties to it. A receipt stated by a party the Terms name as a signer, which no doer's report held speaks of, is shown as **one side only**. A receipt stated by anyone else is listed and ties nothing.
- **Changes an answer: yes.**

### A23. Halves where the call expects one side only

- **The sentence.** The block's R4: "Where a call expects reports from both sides, a verifier shows, for each act both reports name: matched, mismatched [...], or one side only."
- **Chosen.** Matched, mismatched and one side only are shown only where the call says `both`. Where it expects one side, each report held is shown as stated, and no tie is answered.
- **Other reading.** Compare whenever two halves happen to be held.
- **Changes an answer: yes.**

### A24. Who reads "as a side of that leg" (unanswered, or not shown)

- **The sentence.** G11.4(d): "**Unanswered** is said only by a verifier that reads as a side of that leg, or along a path the people of that leg signed for (G11.6(a))."
- **Chosen.** The reader is a side where it is the doer, or a party those Terms name as a signer. It reads along a signed path where the Terms' data cMIP (K8) answers yes for that reader; K8's question is about a report, and here there is none, so the stub is asked with no report. Otherwise: not shown.
- **Changes an answer: yes** (unanswered or not shown).

### A25. A buyer's confirmation against the block's R2

- **The sentences.** The block's R2: "A report that names no call is no report." The text, G11.7: "These grammar rules read every payment as carrying that call [...] It names the payment, by the settlement both sides name [...] and that is how it names the call it answers". The text names this seam itself (section 8.1).
- **Chosen: the text's reading.** A report whose `call` names a payment, or the settlement a payment names, is the confirmation of that payment. A report that names nothing at all is rejected, as R2 has it.
- **Changes an answer: yes.** Read strictly by R2, every payment is one side only.

### A26. A path beyond the leg for a buyer's leg

- **The sentence.** G11.6(b): "A verifier MUST NOT read the people of a leg as having agreed to be read from beyond their leg unless Terms they signed say so". Section 8.2, item 14: "A buyer signs a payment and no Terms. [...] How a buyer agrees to a path for it is not said".
- **Chosen.** For a report of a buyer's leg (a confirmation of a payment) the answer to K8's question is **no** for every reader who is not a side, whatever the receiving side's Terms name.
- **Other reading.** Ask only the receiving side's data cMIP.
- **Changes an answer: yes.**

### A27. Which Terms' data cMIPs are asked about one report

- **The sentence.** G11.6(a): "Where a report carries the data of people bound by different Terms (the two sides of a leg), the data cMIP of each is asked, and the report is read beyond its leg only where every one asked answers yes".
- **Chosen.** The Terms that carry the call the report answers; and, for each agreement a line of the report names as passed on to or owed to, that agreement's Terms in force. A receiving side named as a signer is asked through the Terms that carry the call (it is their party). If Terms that should be asked are not held: unchecked.
- **Changes an answer: yes.**

### A28. A baker who fills a role

- **The sentence.** G14.3: "Where they name the baker or a succession and the acts held do not establish (a) or (b), the answer is **not shown**; so it is where they name a party by role, which the baker might fill."
- **Readings.** (a) Terms that reach the baker only through a role are always not shown. (b) Where the spot's cMIP says the baker fills the role and an assent counts in the baker's name, G14.2(a) is met (the baker is "a party in it from then on", G10.3).
- **Chosen: (a),** the sentence as written. (b) is not built.
- **Changes an answer: yes** (not shown, where (b) would say shown).

### A29. What passes the baker's rights

- **The sentence.** G14.2(b): "each passes the baker's rights to a party of the next, as its own Terms and the cMIPs in their sockets say". G14.3: "These grammar rules define no act that passes the baker's rights (P18)."
- **Chosen.** Read from the earlier agreement's words by its body cMIP (K1): the stand-in's machine line `passes <signer>` (STANDINS.md, S10). Where those words or that cMIP are not held: unchecked. Where they are held and say nothing of it: not shown.
- **Changes an answer: yes** (it decides shown, not shown or unchecked for every succession).

### A30. A payment that names a version deeper in a branch set aside

- **The sentence.** G12.5: "A payment that names a version of that branch is read as if the tip had stood alone".
- **Chosen.** Built for a payment that names the tip itself: its receiver is read from the tip, shown as disputed. A payment that names a Terms act that follows a tip set aside is answered **not shown** as a version: reading it "as if the tip had stood alone" would need the whole branch read again as if no fork had been.
- **Changes an answer: yes.** A limit of this build.

### A31. Rights of a filling earned under a tip set aside, and other fillings by a disputed proof

- **The sentence.** G12.5: "An act signed by someone who signed none of the acts of a branch whose tip is set aside, and that rests on an act of that branch [...] stands: it has the effect these grammar rules gave it, or would have given it had that tip stood alone."
- **Chosen.** A proof (the witness's Statement) made within a tip set aside, by a witness who signed no Terms act of that branch, still earns a filling, shown as **disputed**. Its later-version answers (G10.6) are not given, since the version it was earned under never counted as a version.
- **Changes an answer: yes.**

### A32. A reader's anchoring from which no answer is held

- **The sentence.** G13.4: "Only where the agreement names none does each reader accept the anchoring it chooses." G9.1: "cannot get an answer from a named place".
- **Chosen.** Where the agreement names no place and the reader accepts one from which the verifier holds no answer (or two that contradict), answers about what is latest are **unchecked**, not "as shown".
- **Other reading.** Fall back to as shown.
- **Changes an answer: yes.**

### A33. Delegation: a version that narrows a power, and where its conditions are read

- **The sentences.** G5.5: "The delegator's part alone is then all that the default of G4.8 and G4.1 ask of that version"; "Where a verifier cannot read the body of Terms that carry `powers` [...] whether a version that lacks signatures replaces them is **unchecked**"; section 8.3, item 4: "Which one a verifier reads first is not said."
- **Chosen.** Where the Terms name an update condition, it answers, and the body's conditions are not read. Where they name none: if everyone who signed the original signed the update, it is valid by default and the body is not needed; otherwise the body is read; if it cannot be read, unchecked; if it says the Terms are a delegation and the new version's `powers` are the old ones less one or more, the delegator's part alone (or the agent's, where the stated condition leaves ending to the agent) makes the version valid *and* in force; any other stated condition this verifier cannot evaluate: unchecked. "Terms that carry `powers`" is read as: at least one entry in `powers`, of any kind.
- **Changes an answer: yes.**

---

## Part B. Questions that do not change an answer, or only its wording (13)

### B1. The qualifier where versions name different places

G4.8(d) judges a replacement per the place the original names; G4.6 qualifies what is in force per the agreement's place. Chosen: the answer "these Terms are in force" carries the place the last version names (or as shown, if it names none), and each replacement's own answer carries the place it was judged by. The answer itself is the same either way.

### B2. Answers about a line

G6.1: "every answer about a line is already as shown or per place (G7)". Given A4, every answer about a line is given **as shown**.

### B3. A report is not a Statement and stands in no line

Section 3.2: "It is not a kind of Statement." Whether it stands in its signer's line is listed as open (section 8.4, from draft 11). Chosen: it does not; a report carries no `previous`. No fork can therefore be seen among reports.

### B4. The call's `from whom`, and anything more on a call

G11.6: "A call MUST NOT be read as naming a place, or a reader." In the stand-in a call is a task and from whom. Extra tokens on a call line are left unread and said so. Whether a call is per task or per act (the block's open point 1) does not arise: only K6's call is read.

### B5. A call in Terms that do not hold the task

G11.2: "Terms that hold the task MAY carry a call for it". Chosen: a call for Control in Terms that do not hold the Control task is left unread, and said so.

### B6. The words for a counter signature's outcome

The text gives no answer word for "it cancelled" or "it changed nothing". Chosen: about a counter signature, **shown** means it cancels its signer's assent; **not shown** means it is not shown as cancelling (it changed nothing, it follows no assent, or the order is not shown), with the reason.

### B7. The words for an assent

Chosen: about an assent, **shown** means it is counted in someone's name and binds while the Terms it names are in force, which they are; **not shown** otherwise, with the reason (Terms not in force, replaced, a tip). An assent within a branch set aside is **shown, disputed**, and the text says it binds no one.

### B8. The words for a resolving update signed under a power

G4.11(g): "the condition is not asked of it". So neither valid nor not valid fits. Chosen: **shown**, naming G12.4, the power's rule and the signer (G12.4's MUST).

### B9. Task outputs passed on as "yes" and "no"

K8 "answers yes or no"; K5 says "whether it still takes new identities". These are outputs of tasks, not answers of the text. They are printed as yes and no, with the cause or ground the cMIP gives; unchecked where the cMIP is lacking.

### B10. "Who the judge is"

G5.6(d) is written as what a cMIP author must do: "a verifier applies the cMIP and cannot see inside it" (section 8.2, item 8). Nothing was built that reads a judge. The test shows only that an act made under a power changes no power to resolve and no reach of a contest.

### B11. G3.2 and G12.8 are not a verifier's to check

G3.2 binds a cMIP ("MUST accept `authors` as an input"); G12.8 binds a client. For G3.2 the build hands `authors` to every power's rule and a stub uses it. For G12.8 the test checks only that the verifier's own output never shows a tip as in force or as the next version.

### B12. How far "what a rule the Terms name produces" is answered from the shared version

G4.7: "A verifier MUST answer from them every answer that rests on the Terms in force". Chosen: while a fork stands, powers, the update condition, spots, the receiver, the Control task and the call are all read from the last shared version, each shown with the fork. This is the text; it is listed because the build had to enumerate "every answer".

### B13. Counter signatures that lean on one another

G4.10 makes whether a counter signature cancels depend on whether the Terms were in force before it, which depends on the other assents and on *their* counter signatures. With no place, and signers who assent and cancel many times without naming anything, the reading of one leans on all the others. Fuzzing found a set of acts on which this did not end in useful time. Two things were done. The reading was made lighter (question A9: the nearest counter signature; each one's reading is kept and not done again). And a limit was set: G9.2 names three things a verifier may stop following ("powers through powers [...]; receivers from agreement to agreement [...]; the versions of a long chain") and this is not one of them; it is read as a fourth. Past a set number of readings the answer is **unchecked**, and the verifier says that it stopped. It changes an answer only for sets of acts made to exhaust it.

---

## Where the text and the block do not read alike

1. **R2 against G11.7** (A25 above): a confirmation with no call written. The text names the seam; the build follows the text.
2. **"two sides who both signed"** (the block's plain words) against G11.1, where the sides are whoever carries out and whoever receives. The build follows G11.1: a receiving side need not have signed anything.
3. **R4** speaks of calls that expect both sides; the text's G11.7 shows a payment as matched, mismatched or one side only although the default call expects only the receiving side's confirmation. The build treats the payment itself as the doer's half (the payer signed it), as G11.7 does.
4. **The block's open point 3** (one report answering several calls): a report in the stand-in names one call.

## What could not be decided from the text at all, and was left unbuilt

- What a verifier answers while a payment is "pending" (section 9.7): no such state exists in the stand-in.
- Whether money shown as still owed is later shown as paid (P20): "still owed" lines are shown as stated and tied to nothing.
- Pools (P6): one report line names one settlement.
- A judge, a tally, identity, assurance, delivery, standing (section 5.3): modules of cMIPs; none is a socket, and none was built.
