# Questions: every place a reading had to be chosen

Each entry gives the sentence, quoted from `spec/agreements-draft-17.md` unless marked as the report block's; the readings seen; the one built, and why; and whether it changes an answer a verifier gives. Nobody was asked. Part A holds the questions that change an answer, the heaviest first. Part B holds the ones that do not.

"The verifier" below is this one. Stand-ins are in `STANDINS.md` and are not repeated here unless a reading of the text hangs on them.

---

## Part A. Questions that change an answer

### Q1. A fork is made of assents, and assents to a tip "do not count"

> "Where two Terms acts each replace the same Terms (G4.8), and each has the assents its own `in_force` requires (G4.1), they are a fork (G12)" (G4.7)
>
> "While a fork stands, neither tip counts as the next act of its line, and nothing built on either tip counts: … no act that rests on a tip … (it names it, in `terms`, `under`, `answers` or `relies_on`, or was made within or under Terms of that branch)." (G12.2)

- **Readings.** (a) Literally: once the fork stands, the assents to each tip (which name it in `terms`) do not count; but then neither tip "has the assents its own `in_force` requires", so neither is an update, so there is no fork, so the assents count again. The two sentences undo each other. (b) An act made within a tip still counts for one thing: whether that tip was signed. Nothing it would do under the tip's Terms counts, because those Terms are not in force.
- **Chosen:** (b). It is the only reading under which a fork of Terms can stand at all. An act that rests on a tip in any other way (`under`, `answers`, `relies_on`, a payment's version) does not count for anything.
- **Changes an answer:** yes. Under (a) no fork of Terms is ever answered.

### Q2. A delegation ended by the delegator alone, and the new version's own `in_force`

> "a version of those Terms that removes or narrows the power replaces them (G4.8) when it carries the delegator's part alone, without the agent's." (G5.5)
>
> "An **update** of an act is an act … that all the parties that needed to sign it have signed: … for Terms, those whose signatures G4.8 and G4.1 ask for." (G12.1)

- **Readings.** (a) The ending version must also be in force by its own `in_force` (G4.1): if it still names the agent as a signer and carries no `in_force`, the agent's assent is needed after all, and the default of G5.5 is empty. (b) "replaces them … when it carries the delegator's part alone" settles both: G4.8's signatures and the new version's coming into force.
- **Chosen:** (b), for the default and for a signed condition that leaves ending to the agent (then the agent's part alone). Otherwise the delegator's default would depend on how the ending version happens to list its parties.
- **Changes an answer:** yes: whether a delegation has ended, and so whether an act made under it binds (G5.4).

### Q3. How a verifier knows that Terms are a delegation, and what "removes or narrows" is

> "Terms in which one party, the **delegator**, gives another, the **agent**, a power to make acts for it are a delegation." (G5.5)
>
> "A verifier reads them from those Terms, in the form their cMIP gives (G2, task K1); where it cannot, the answer is **unchecked** (G9)." (G5.5)

- **Readings.** No field says "this is a delegation", who the delegator is, or what the conditions on ending are. (a) Infer it from `powers` and the parties. (b) Take all three from the body's reading (K1).
- **Chosen:** (b): the K1 plug-in says whether the Terms are a delegation, who gives and who gets the power, and how it ends (by default, by the agent alone, or in a form the stub cannot read). "Removes or narrows" is read as: the new version's `powers` are a strict subset of the old ones. A power narrowed by changing its rule is not seen as narrowing.
- **Consequence, and it is wide:** for any Terms that carry `powers`, a replacement that lacks signatures is **unchecked**, not **not shown**, whenever the verifier cannot read the body (no K1 cMIP, or the words not held): it cannot tell whether G5.5 would have let one part alone replace them.
- **Changes an answer:** yes.

### Q4. "Invalid" for an act that breaks a MUST, against "not an Agreements act"

> "**invalid**: an act cannot count as what it claims to be, and the act itself shows it: no act the verifier does not hold could make it count" (section 2)
>
> "An act missing a field marked required is not an Agreements act." (section 3)

- **Readings.** The text names one case of **invalid** (G14.3). It does not say what a verifier answers for Terms whose `authors` name a non-party (G3.1), that name a role with no `in_force` (G4.2), whose role lacks its kind, `what` or cMIP (G10.1), whose body or cMIP is not a fingerprint (G2.1), or for an act whose `previous` names another signer's act (G6.1). (a) Reject them as not Agreements acts. (b) **invalid**. (c) Read them anyway.
- **Chosen:** a missing required field, or bytes that cannot be parsed, are rejected and never reach the grammar ("not an Agreements act"). Everything in the list above is answered **invalid**, with the grammar rule named: the act itself shows it, and no other act could mend it. An invalid act counts for nothing.
- **Changes an answer:** yes (invalid, where a reader might expect not shown, or a rejection).

### Q5. A counter signature and the last assent, with no order between them

> "A counter signature is available only until every party has signed, which these grammar rules read as: until those Terms are in force (G4.1)." and "Whether the Terms were in force before the counter signature is an answer about what is latest: per place, or as shown (G4.6)." (G4.10)
>
> "Between two acts of which neither so names the other, no order is shown from the acts alone." (G13.3)

- **Readings.** With no place, and neither act citing the other, the acts held do not say which came first. (a) The counter signature wins. (b) The assents win. (c) **not shown**.
- **Chosen:** (c). With a place, positions are replayed and settle it. Without one, order comes from citing only (G13.3): if the counter signature cites the assents that complete the Terms, it changes nothing; if the completing assent cites the counter signature, the assent is cancelled; otherwise whether the Terms are in force is **not shown**, with that reason. (With two or more counter signatures and no place, the verifier does not try every order: it answers only if the result is the same whether all or none of them took effect.)
- **Changes an answer:** yes.

### Q6. "Follows that assent in the signer's own line"

> "A counter signature is a Statement of kind `counter_signature`, by the signer of an assent, whose `terms` names the Terms that assent names, and which follows that assent in the signer's own line (G6.1)." (G4.10)

- **Readings.** (a) Directly: its `previous` is the assent. (b) Anywhere later in the line.
- **Chosen:** (b). A signer may have made other acts in between.
- **Changes an answer:** yes, for a counter signature not made directly after the assent.

### Q7. "Every party bound by them", when nobody is bound, and who a party is

> "an assent to it in the name of every party bound by them, or what a power for replacement in them requires" (G4.8)

- **Readings.** (i) Who is counted: only parties the Terms name (signers, agreements, and signers who fill a role), or anyone who assented. (ii) What if the Terms came into force under an `in_force` rule that needed nobody's assent: then no party is bound, and "every party bound" is nobody.
- **Chosen:** (i) the parties named, role fillers included; a stranger's assent makes no one a party. An agreement named as a party is counted where it signed (G1.2). Where the last version was itself made under a power, those bound through that power (G5.2) are counted too. (ii) taken as written: if nobody is bound, any Terms act that replaces them and is in force by its own `in_force` replaces them. That is odd, and is the text's.
- **Changes an answer:** yes.

### Q8. A stranger's assent to a tip that is set aside "stands"

> "An act signed by someone who signed none of the acts of a branch whose tip is set aside, and that rests on an act of that branch …, stands: it has the effect these grammar rules gave it, or would have given it had that tip stood alone." (G12.5)
>
> "Every other tip is **set aside**: it never counted as the next act of the line (for Terms, it never counted as the next version)" (G12.3)

- **Readings.** An assent by someone who did not sign the tip Terms rests on the tip. (a) It "stands" with the effect it would have had "had that tip stood alone": its signer is bound by the tip's Terms. (b) It stands as an act, shown as disputed, but binds no one: the Terms it names were never a version, and an assent binds "only while the Terms it names are in force" (G4.3).
- **Chosen:** (b) for assents and other Statements within the branch. For a payment that names a version of the branch (the text's own example), (a): its receiver is read from that version, as if the tip had stood alone, and the payment and its receiver are shown **disputed**. "The acts of a branch" is read as the acts of the line (the Terms acts), so whoever signed one of those gets no protection for what they built on it.
- **Changes an answer:** yes.

### Q9. A payment that names a tip while the fork stands

> "Whatever a payer … gives or owes to an agreement, it gives or owes to that agreement's **receiver for the act**: the `receiver` named by the version of that agreement which the act that gives or owes names (G13.1)." (G1.4)
>
> "nothing built on either tip counts" (G12.2); "A verifier MUST answer from them every answer that rests on the Terms in force: among others, a receiver for an act" (G4.7)

- **Readings.** (a) The receiver the tip names. (b) **fork**, and no receiver. (c) The receiver named by the Terms both branches replace, with the fork shown. The text lists "a payment made under a tip" as open (section 8.4).
- **Chosen:** (c), and the payment itself is marked as built on a tip.
- **Changes an answer:** yes.

### Q10. A resolving update in one signer's own line

> "A fork is resolved by an update of the shared act: an act of the same line whose `previous`, or for Terms whose `replaces`, names the shared act; which names every tip in `resolves`; and which names one of them in `continues`." (G12.3)

- **Readings and choices.**
  1. Two acts that both name no `previous` have no shared act. *Chosen:* a resolving update that also names none resolves them.
  2. The resolving update names the same `previous` as the tips, so by G6.2's words it is a third tip. *Chosen:* it is not ("A resolving update is not a tip of the fork it resolves"), but an act that names `resolves` and does not in fact resolve the fork it sits in is an act like any other, and is a tip. Otherwise a fork could be hidden by adding a `resolves` field.
  3. "names every tip": *chosen:* it may name more than the tips; it must name all of them.
  4. Two resolving updates that continue the same tip (open in section 8.4): *chosen:* the fork is resolved; only those "that continue different tips are themselves a fork" (G12.7).
  5. After the fork is resolved, the signer's next act may name either the resolving update or the last act of the continued branch as `previous`. *Chosen:* both are read; neither makes a fork.
  6. A resolving update signed under a power (G12.4) for a fork in someone's line stands in the line of whoever signed it, so its `previous` cannot name the shared act. *Chosen:* it resolves by naming the tips; its `for` names the signer whose line forked.
- **Changes an answer:** yes, each of them, for forks in a signer's line.

### Q11. An agreement that signs with no power for it

> "Where x's Terms give no such power, x signs by every one of its signers; by which acts is not said (P17)." (G1.2)

- **Readings.** (a) It cannot be shown to have signed at all. (b) P17's "least reading": "an act by each of them, its `for` naming the agreement and its `under` its Terms in force".
- **Chosen:** (b): one assent per signer of x, each with `for` x and `under` x's Terms in force. A party x names as an agreement is not one of its signers (one level). Where the `for` of an act names an agreement, `under` must name Terms of that agreement ("for an agreement, Terms of its own", section 3.2); a signature under other Terms is read as no signature.
- **Changes an answer:** yes: whether Terms whose `in_force` asks for an agreement's signing are in force.

### Q12. Where Terms "name a cMIP that answers" the Control task

> "Terms hold the Control task (K8) where they name a cMIP that answers it (G2); or where they name as `receiver` one of their own parties (G1.3) and give a part to any other party" (G11.2)

- **Readings.** No field of section 3.1 names a cMIP for K8, and none says what "a part" is ("The grammar gives them no form", section 2, Shares).
- **Chosen:** a K8 cMIP is looked for among the fingerprints in `relies_on`. "Gives a part" is read from the body, through K1: a share for any party other than the receiver. Where the body cannot be read, or `relies_on` names something the verifier does not hold and cannot identify, whether the Terms hold the task is **unchecked**. The doer is whoever the Terms name as `receiver` among their own parties.
- **Changes an answer:** yes: **no report called for**, **unanswered** and every leg under those Terms hang on it.

### Q13. A settlement the verifier does not hold

> "Two leg reports that name the same settlement match where they state the same amount for it (K8's row)." (G11.4(c))
>
> "Where the verifier lacks an act a report names, or the Terms that hold a leg's task, that part is **unchecked** (G9)." (G11.4(d))

- **Readings.** Two reports name the same settlement fingerprint and state the same amount; the settlement itself is not held. (a) **matched**: matching needs only the fingerprint and the amounts. (b) **unchecked**: the verifier lacks an act the reports name.
- **Chosen:** (b), for a leg report's tie and for a payment's confirmation. What each side states is still shown.
- **Changes an answer:** yes.

### Q14. Who "reads as a side of that leg"

> "**Unanswered** is said only by a verifier that reads as a side of that leg, or along a path the people of that leg signed for (G11.6(a))." (G11.4(d))

- **Readings.** A verifier is software; whose eyes it reads with is not in the acts.
- **Chosen:** the caller says who is reading (S16). The reader is a side of the legs under Terms that hold the Control task if it is their doer, or a signer the body gives a part to. For anyone else, and when no reader is given, a call with no report is **not shown**. No path of G11.6(a) can be read (the text gives no way to name a data cMIP), so the answer to "did the signers agree to a path beyond the leg" is always **unchecked**, never **shown**. Reports the verifier holds are read whoever is reading: the verifier does not ask how they reached it.
- **Changes an answer:** yes (**unanswered** against **not shown**).

### Q15. The baker: how Terms "name a succession", and what passes the rights

> "Where the Terms name neither the baker among their parties nor a succession, the answer is **invalid** … Where they name the baker or a succession and the acts held do not establish (a) or (b), the answer is **not shown**. Where the verifier lacks the envelope or an agreement of the succession, it is **unchecked** (G9)." (G14.3)

- **Readings and choices.**
  1. *Naming a succession:* "Each agreement of the succession names the one before it (G13.2)", so: `relies_on` names Terms of another agreement that name the same envelope.
  2. *Invalid only when the act shows it:* if `relies_on` names something not held, it might be the succession: **unchecked**. If a party is named by role, the baker might fill it: **not shown**. Only Terms with neither, and nothing unheld, are **invalid**. Without the envelope the baker is unknown: **unchecked**.
  3. *"Bound by them" (G14.2(a)):* the Terms must be, or have been, a version, with an assent in the baker's name.
  4. *What passes the rights* is "not said" (P18). A stub kind of Statement, `k11.pass-rights`, made in the earlier agreement by whoever holds the rights, with the next holder as its content. Without that K11 cMIP: **unchecked**.
- **Changes an answer:** yes.

### Q16. A receiver that is a spot with several fillings, or not a party

> "Where the receiver so named is one of that agreement's own parties, the answer is that party." and "If a version read names no receiver, or the spot named is not filled, … no one receives, and the answer is **not shown**." (G1.4)

- **Readings.** A spot "MAY be filled many times" (G10.2); the text speaks of one party who receives.
- **Chosen:** exactly one filling: that signer. Several: **not shown** (which of them receives is not said). A `receiver` that names a signer or role the Terms do not list among their parties: **not shown**. A payment that names, as the version it read, Terms that are not shown as a version of that agreement: **not shown**.
- **Changes an answer:** yes.

### Q17. One unchecked update makes the rest of the chain unchecked

> "Where the verifier lacks the cMIP, whether the update replaces is **unchecked** (G9), and so is every answer that rests on it." (G4.11(h)); "from there on, which Terms are in force is unchecked for them" (section 7)

- **Readings.** Two Terms acts replace the same version; one is fully signed and checked, the other cannot be checked. (a) The checked one is the next version. (b) Which Terms are in force is unchecked: the other might be an update too, which would make a fork.
- **Chosen:** (b). The last version established is still named.
- **Changes an answer:** yes.

### Q18. An act made under Terms that were replaced: before or after?

> "Where whoever is bound through `for` is bound only through Terms that have been replaced, a verifier MUST answer from G7: per place if the Terms name one, otherwise as shown." (G5.4)

- **Readings.** The sentence says where the answer comes from, not what it is. (a) Never bound once the Terms are replaced. (b) Bound if the act was made before the replacement.
- **Chosen:** (b). With a place: the act's position against the position of the replacing Terms act (not of the last assent that completed the replacement: a simplification). Without: order from citing only; if neither names the other, **not shown**. The qualifier is that of the agreement whose Terms gave the power.
- **Changes an answer:** yes.

### Q19. The contest: against what, reaching whom, and when it is a buyer's

> "It is made against an offer or a vow (section 2): its `agreement` names the root of that offer or vow, and its `terms` names the version it is made against. Whom it reaches follows from what it is made against: the signers of that offer or vow (one level, G1.2), and no one else." (G11.8)

- **Readings and choices.**
  1. Must the Terms name a role to be "an offer"? "Whether such a deal, signed by parties who were known, is an offer or a vow is not said". *Chosen:* a contest against any version is read alike; whether the Terms name a role is shown beside it and changes nothing.
  2. "The signers": *chosen:* the parties those Terms name as signers. Signers who fill a role are not counted (P17 leaves it open; G1.2 counts "the parties its own Terms name as signers").
  3. `terms` must name Terms shown as a version (or a tip set aside, then disputed); otherwise **not shown**.
  4. A buyer's contest: *chosen:* shown where a payment by the same signer is held whose settlement is the one the contest names and whose version is the one the contest names. No payment held: **unchecked**.
- **Changes an answer:** yes.

### Q20. The place used to judge a replacement, and a place that answers twice

> "Where the Terms in force name a place, an act within the agreement counts … only once the place shows it, and at the position the place gives it." (G7.2)

- **Readings.** A new version may name another place, or none. Which place decides whether it has replaced the old one? P16 asks the same of clocks.
- **Chosen:** the place (and the clock) named by the version in force, that is the one being replaced. The Terms act itself must be shown by the place, as its assents must. Two answers held for one place identifier are read as no answer: **unchecked**.
- **Changes an answer:** yes.

### Q21. `previous` names an act the verifier does not hold

> "it lacks … an act named by fingerprint … it MUST answer **unchecked** for every answer that depends on it." (G9.1)

- **Readings.** (a) Every answer that uses the act is unchecked: a fork might hide in the part of the line not held. (b) The act counts; its signer's line is marked unchecked before it. Every answer about a line is already "as shown".
- **Chosen:** (b). Under (a) a verifier holding part of a long line could answer nothing.
- **Changes an answer:** yes (shown, as shown, against unchecked).

### Q22. Powers through powers, receivers through agreements: how deep

> "Whether a verifier must follow any depth, or a limit applies, is open." (P2); "G1.4 sets no limit on how many agreements a receiver may point through" (P8)

- **Chosen:** eight levels for each; beyond, **unchecked** (the verifier could not apply the rule). A chain of more than 256 versions is unchecked from there.
- **Changes an answer:** only for sets deeper than that.

### Q23. Whether Terms were "in force" when asked of Terms that were replaced

> "a verifier MUST NOT answer that Terms or an agreement have ended." (G4.9)

- **Readings.** Asked "are these Terms in force?" of a version that was replaced, the text gives no word but the answers of section 2.
- **Chosen:** **not shown**, with the reason "Terms that replace them are in force (G4.5)". Never "ended", and the agreement itself is always shown as going on in its last version.
- **Changes an answer:** it fixes which word is used.

### Q24. Readings that did not settle

Forks in signers' lines, forks of Terms and powers to resolve lean on one another (a power to resolve lives in Terms whose being in force may itself hang on a fork). The verifier reads the set in rounds until two rounds agree, at most six. If they never agree it says so, and every answer that rests on a fork in that set is to be taken as **unchecked**. No test set and no fuzzing input found so far does this; the text does not say what to do if one does.

- **Changes an answer:** only in such a set.

---

## Part B. Questions that do not change an answer, or only its form

### Q25. Something named "in another way"

> "Anything a Terms act refers to in another way is not part of those Terms. A verifier MUST NOT read it as part of them." (G2.2)

Against G2.1 ("MUST be named by fingerprint"). *Chosen:* a `body`, or a cMIP, that is not a fingerprint makes the Terms **invalid** (Q4), since without them nothing can be read. A `relies_on` entry that is not a fingerprint, and any field the text does not give, are not read at all and are counted in the output as "not part of the act as read". The Terms stay valid.

### Q26. A role's cMIP with parameters

> "the cMIP answering task K12 that says who fills it, by fingerprint." (G10.1); "whether a role's cMIP takes parameters" is open (section 8.4)

*Chosen:* it may carry parameters, as `condition` does. Only the stub reads them.

### Q27. The kinds of a power

> "`powers` … list of (kind, rule)" and "**Power**: an entry in Terms saying that a Statement of a given kind (or a replacing Terms act, or a resolving update) binds the parties when a given rule says yes."

The text gives no word for the last two kinds. *Chosen:* `replaces` and `resolves`. A power for replacement also lets its holders sign a resolving update ("it is a power for replacement …, which its rule may limit to resolving updates", G12.4).

### Q28. What a rule is given

> "a rule's parameters, the acts held, the answers of the place and the clock the agreement names" (K4's row)

*Chosen:* the plug-in gets the rule, the Terms that name it, the act asked about, the time (with its qualifier) where one is given, and may ask the grammar whether an assent in a given name is present. An `in_force` rule that reads no assent at all (the stub `k4.yes`) puts Terms in force with none; that is the rule's, not the grammar's.

### Q29. Terms with no party named as a signer, and no `in_force`

> "If `in_force` is absent, they are in force when there is an assent to them in the name of every party named as a signer." (G4.1)

*Chosen:* as written: with no such party they are in force at once. (Terms that name only agreements. Terms with a role must carry `in_force`.)

### Q30. The version a filling was earned under

> "The version a filling was earned under is the version its proof names (G13.1)." (G10.6)

*Chosen:* the K12 plug-in says which version; the stub gives the `terms` of the claiming Statement. Later versions that change its rights are looked for only among the versions shown after it in the chain; where its own version is not in the chain shown, none is listed.

### Q31. More than two tips

> "A **fork** is two updates of the same act of one line" (G12.1)

*Chosen:* three or more updates of the same act are one fork with that many tips; a resolving update must name them all.

### Q32. Which lines of one report belong to which payment

> "what a leg report (G11.3) names as received where what it passes on comes out of a pool" is open (P6)

*Chosen:* when a result is read leg by leg from a payment, every "passed on" line of a doer's report that names the payment's settlement as received is shown as a leg of that result. No share of it is computed.

### Q33. Before paying: which agreements are "of a sequence"

> "Asked which agreements of a sequence carry a call for reports, a verifier answers from their Terms alone (G11.2), one agreement at a time" (G11.4(e))

*Chosen:* starting from the agreement asked about, the verifier follows `receiver` from agreement to agreement, and stops at the first that names one of its own parties, or none. Parties named as agreements (the works a work stands on) are not walked: which of them a payment reaches is their own Terms' and cMIPs' to say.

### Q34. The report block's "two sides who both signed"

> (report block) "the **leg**: two sides who both signed"; (text, G11.2) "Whoever receives is the receiving side of its leg whether or not it signed the Terms that carry the call."

The two disagree, and the text says so (section 8.1). *Chosen:* the text's reading: a receiving side's report is read whether or not that side signed anything.

### Q35. A report from someone who is not a side

*Chosen:* a doer's report is tied only if its signer is the doer the Terms name; a confirmation only if its signer is the receiving side as G1.4 reads it. A report by anyone else is still listed, as its signer's word, and ties nothing. A receiving side's line that no doer's report answers is shown **one side only**.

### Q36. Time written inside an act

> "A time written inside an act is reported as its signer's claim." (G8.2)

No field of section 3 holds a time, so the verifier has none to report. Times come only from a clock or an anchoring, and are shown with their qualifier.

### Q37. Words

"Not an Agreements act" is shown as "rejected". The chain statuses the tool prints ("a version", "not shown as a replacement", "set aside", "resolves nothing") are plain words for people; the answers proper are only the words of section 2 and the block's five.
