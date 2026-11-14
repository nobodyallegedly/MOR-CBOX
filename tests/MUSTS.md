# Every sentence with MUST or MUST NOT, and the test case for it

For Nobody, allegedly. There are no test vectors in the box: every case below was written from the text alone, and is the builder's reading of it (see `QUESTIONS.md`). A case that passes shows that the code does what the builder read, not that the reading is right.

`spec/agreements-draft-19.md` holds 65 sentences with MUST or MUST NOT in its grammar rules (some hold two). Each has at least one case in `tests/test_mor.c`. Two of them bind someone other than a verifier, and are tested only as far as a verifier's own behaviour goes: G3.2 binds a cMIP (the test shows that `authors` is handed to a rule in a socket), and G12.8 binds a client (the test shows that the verifier never hands out a tip as resolved).

This file is made by `tests/gen_musts.py`.

## Part 1. Sentence by sentence

| # | Grammar rule | The sentence | Test case(s) |
|---|---|---|---|
| 1 | G1.2 | Where an act's `for` names an agreement, its `under` MUST name Terms of that agreement: an act made for an agreement under any other Terms is not that agreement's signature. **One level.** Its signers are the parties its own Terms name as signers. | `g1_2_under_other_terms` |
| 2 | G1.2 | A party it names as an agreement is not one of them, and neither are that agreement's people: these grammar rules MUST NOT be read as making an agreement's signing need the signature of anyone inside an agreement it names, unless its own Terms say so. | `g1_2_one_level` |
| 3 | G1.4 | Where that act names no version, a verifier MUST take the receiver from that agreement's Terms in force, per that agreement's own place, qualified **per place** with that place named, or, where it names none, per the anchoring the reader accepts, or **as shown** (G4.6, G7, G13.4). | `g1_4_no_version_qualified` |
| 4 | G1.4 | Which Terms of that agreement are in force is answered by G4.5 to G4.8: while a fork of its Terms stands, the receiver is the one named by the Terms both branches replace, and the verifier MUST show the fork with that answer and MUST NOT choose a branch (G4.7). | `g1_4_fork_receiver` |
| 5 | G2.1 | The `body`, every piece in `relies_on`, and every cMIP named anywhere in a Terms act MUST be named by fingerprint. | `g2_1_fingerprints` |
| 6 | G2.2 | A verifier MUST NOT read it as part of them. | `g2_2_not_part` |
| 7 | G3.1 | `authors` MUST name one or more of the parties named as signers or roles. | `g3_1_authors` |
| 8 | G3.2 | A cMIP that reads a judge's decision, in whichever socket it sits (section 5.3), MUST accept `authors` as an input. | `g3_2_authors_input` |
| 9 | G4.2 | Terms that name a party by role MUST carry `in_force`. | `g4_2_role_needs_in_force` |
| 10 | G4.6 | A verifier MUST take it per the agreement's place, qualified **per place** (G7.2); or, where no place is named, per the anchoring the reader accepts, with it named, or else **as shown** (G7.3, G13.4). | `g4_6_qualified` |
| 11 | G4.6 | It MUST NOT give it as latest without saying what the answer stands on. | `g4_6_qualified` |
| 12 | G4.7 | A verifier MUST answer **fork** for which Terms replace them, show both tips, and MUST NOT choose between them, by position or in any other way. | `g4_7_fork_of_terms` |
| 13 | G4.7 | A verifier MUST answer from them every answer that rests on the Terms in force: among others, a receiver for an act (G1.4), who fills a spot (G10.2), and what a rule the Terms name produces (G5; tasks K2 to K4). | `g4_7_answers_from_shared` |
| 14 | G4.7 | It MUST show the fork with each such answer, per place or as shown. | `g4_7_answers_from_shared` |
| 15 | G4.9 | No act ends Terms, or an agreement, without replacing them, and a verifier MUST NOT answer that Terms or an agreement have ended. | `g4_9_never_ended` |
| 16 | G4.11(c) | A verifier MUST answer **not valid** for it, and MUST name the condition beside that answer, with the ground the cMIP gives: an act, reported by who signed it (G8), or a time, per the clock the agreement names (G7.4), or, where it names none, per the anchoring named or as shown (G13.4). | `g4_11_c_not_valid`, `g4_11_c_time_ground` |
| 17 | G4.11(i) | A verifier MUST NOT answer, of itself, that an agreement is locked, or that it can never be updated. | `g4_11_i_no_lock` |
| 18 | G5.3 | It MUST NOT later be counted as made for anyone else. | `g5_3_own_name_only` |
| 19 | G5.4 | A verifier MUST answer from G7: per place if the Terms that gave the power name one, by the act's position against the position at which the replacing Terms came into force; otherwise as shown, by citing alone (G13.3), and **not shown** where neither the act nor the replacement names the other. (Whether Terms have been replaced is answered by G4.5 to G4.8; | `g5_4_replaced_power_place`, `g5_4_replaced_power_citing` |
| 20 | G5.5 | A verifier MUST NOT answer that such a delegation can never be ended: whether the agent will end it is **not shown**, and the verifier shows the condition as the Terms the delegator signed state it. | `g5_5_delegation` |
| 21 | G5.6(d) | A verifier MUST NOT read an act made under a power as naming or replacing a judge; as giving, widening, narrowing or removing a power to resolve; or as changing whom a contest reaches. | `g5_6_powers_cannot_touch_judicial` |
| 22 | G6.1 | Every act within an agreement MUST name in `previous` its signer's previous act within the same agreement, or none if it is the signer's first. | `g6_1_previous` |
| 23 | G6.2 | A verifier holding both MUST answer **fork** for that signer, and MUST NOT choose between the tips. | `g6_2_line_fork` |
| 24 | G7.2 | A verifier MUST take those answers from the place, qualified **per place**. | `g7_2_place` |
| 25 | G7.3 | Where no place is named, a verifier MUST qualify every such answer **as shown**, or **per anchoring** where the reader accepts one, with it named (G13.4). | `g7_3_as_shown` |
| 26 | G7.3 | It MUST NOT give such an answer as latest or whole without saying what it stands on. | `g7_3_as_shown` |
| 27 | G7.4 | The place the Terms in force name is the one every reader uses for that agreement, whether a party to it or not: a verifier MUST NOT give an answer of G7.2 for that agreement per another anchoring. | `g7_4_one_place_for_all` |
| 28 | G8.1 | For any answer about the world (a deed done, a thing's quality or quantity, an event, a person's presence or absence), about time, or about value under Control, a verifier MUST report who stated it: the one who performed, the one who received, a named witness, the controller, or the place. | `g8_who_said_it` |
| 29 | G8.3 | A verifier MUST NOT report value as moved on the strength of an Agreements act alone. | `g8_who_said_it`, `g11_7_buyers_leg` |
| 30 | G8.4 | A verifier MUST report them with whoever confirmed them. | `g10_2_fillings`, `g11_3_readable_without_cmip` |
| 31 | G9.1 | Where a verifier cannot apply a rule it needs, because it lacks a cMIP named by fingerprint, lacks an act named by fingerprint, lacks the acts of an agreement named as a party, or cannot get an answer from a named place, it MUST answer **unchecked** for every answer that depends on it. | `g9_1_unchecked`, `g7_2_place` |
| 32 | G9.2 | It MUST NOT answer **not shown** or **invalid** because it stopped. | `g9_2_where_a_verifier_stops`, `g5_powers_through_powers` |
| 33 | G10.1 | A role in `parties` MUST name: | `g10_1_role_fields` |
| 34 | G10.2 | A verifier MUST answer each filling with its proof, each act reported by who signed it (G8.4), qualified per place or as shown (G7). | `g10_2_fillings` |
| 35 | G10.5 | A verifier MUST give that answer with its cause, as the cMIP gives it, per place, per clock, per the anchoring named or as shown (G7, G13.4). | `g10_5_new_identities` |
| 36 | G10.6 | A verifier MUST NOT read a later version as changing a filling's rights under an update condition which that filling's own version does not name. | `g10_6_not_under_another_condition` |
| 37 | G10.6 | A verifier MUST name, with each filling, the version it was earned under, and each later version that changes its rights, with what that rests on: the holder's own assent, or the condition named and its answer. | `g10_6_rights` |
| 38 | G10.7 | No condition triggers: these grammar rules have no answer **triggered**, and none that a filling's rights **no longer apply**, and a verifier MUST NOT give either. | `g10_6_rights` |
| 39 | G10.7 | What the update says of those rights, whether the holder is still a party in the spot (G10.3), and whatever else follows, are as that update and the version the filling was earned under state; a verifier reads them from there (G2) and MUST NOT answer them of itself. | `g10_6_rights` |
| 40 | G11.3 | What K6's row asks of a leg report MUST be readable without any cMIP those Terms name: a verifier that lacks them still reads the leg reports, and reports what they state as stated by their signers. | `g11_3_readable_without_cmip` |
| 41 | G11.4 | **Reading across legs.** A verifier asked about a result that depends on a sequence MUST read it leg by leg, from the reports it holds (G11.6), and give each leg's part with who signed it (G8.4). | `g11_4_sequence` |
| 42 | G11.4(c) | Where they do not, the verifier MUST show both, with the amount each states, and MUST NOT choose between them. | `g11_4_ties` |
| 43 | G11.4(d) | A verifier that does not, because that leg's reports travel no further than the leg (G11.6), MUST NOT show the call as unanswered: for it, that leg's reports are **not shown**. | `g11_4_d_silence` |
| 44 | G11.5 | Where the doer is itself one to whom those Terms give a part, its report states that part as its own, and a verifier MUST NOT read it as kept (K6's row). | `g11_3_readable_without_cmip` |
| 45 | G11.6 | A call MUST NOT be read as naming a place, or a reader. | `g11_6_call_reads` |
| 46 | G11.6 | A call MUST NOT be read as expecting a report on a task held by other Terms than the ones that carry it. | `g11_6_call_reads` |
| 47 | G11.6(a) | These grammar rules give no such path themselves: no call, no field and no answer of this text carries a report beyond its leg, and a verifier MUST NOT read any as doing so. | `g11_6_data_path` |
| 48 | G11.6(b) | **Who has agreed.** A verifier MUST NOT read the people of a leg as having agreed to be read from beyond their leg unless Terms they signed say so (G5.1): by naming such a data cMIP in `data` (K8), or by a rule in them under which others decide it. | `g11_6_data_path` |
| 49 | G11.6(b) | Where those Terms say that a majority decides, the majority decides (G5.1(b)): a verifier reads a member who was outvoted as that rule says, and MUST NOT answer, beyond it, whether that member agreed. | `g11_6_b_majority` |
| 50 | G11.7 | As for any two halves, a verifier shows a payment so confirmed as **matched**, one whose confirmation states another amount as **mismatched**, and one the receiving side has not confirmed as **one side only**; it MUST NOT show a payment as received on the buyer's word alone (G8.3). | `g11_7_buyers_leg` |
| 51 | G11.8 | A verifier MUST NOT read a contest as contesting anyone but the signers of what it is made against: not a service those signers picked, not a deal of theirs, not any other agreement of the sequence. | `g11_8_contest` |
| 52 | G11.8 | It MUST NOT read a contest as giving its signer sight of any leg (G11.6, G11.7). | `g11_8_contest` |
| 53 | G12.2 | A verifier MUST show both tips, and what it holds that is built on them, with the fork; | `g12_2_built_on_tip` |
| 54 | G12.2 | MUST answer **fork** where G4.7 and G6.2 say; and MUST NOT choose between the tips, by position or in any other way. | `g12_2_built_on_tip`, `g4_7_fork_of_terms`, `g6_2_line_fork` |
| 55 | G12.3 | A resolving update MUST name every tip, and MAY name more acts than the tips. | `g12_3_must_name_every_tip` |
| 56 | G12.4 | A verifier MUST report such a resolving update with the power it was signed under and whoever it rests on (G5.1, G8). | `g12_4_power_to_resolve` |
| 57 | G12.5 | A verifier MUST show that act, and every answer resting on it, as **disputed**. "The acts of a branch" are the acts of the line: for Terms, the Terms acts; whoever signed one of them is not protected here for what they built on it. | `g12_5_disputed` |
| 58 | G12.8 | **A client's duty (client conformance).** A client that holds both tips of a fork MUST NOT show either tip, or anything built on either, as a resolved act: not as the next version, not as in force, not as that signer's latest act. | `g12_8_client` |
| 59 | G13.2 | A verifier MUST NOT read an act as having relied on a version or act it does not name. | `g13_1_read_against_named` |
| 60 | G13.4 | A verifier MUST NOT give, per a reader's own anchoring, an answer that the place or the clock the agreement names gives. | `g7_4_one_place_for_all` |
| 61 | G13.4 | A verifier MUST give each such answer **per anchoring**, with the anchoring named (**per place** or **per clock** where it is the one the Terms name), or **as shown**. | `g7_4_one_place_for_all`, `g7_3_as_shown` |
| 62 | G14.3 | A verifier asked whether Terms count as an agreement for a baked work MUST read the envelope, the baker it states, and, under (b), every agreement of the succession. | `g14_baker`, `g14_succession` |
| 63 | G14.3 | A verifier MUST give that answer with every answer about that work that rests on those Terms (a receiver, a filling, a leg report). | `g14_baker` |
| 64 | G14.5(a) | **Shown beside a bake.** Where a verifier holds two or more envelopes that carry the same work, as the core says of their bytes (section 8.1), it MUST show, beside each of them and beside every answer of G14.3 about Terms for one of them, that the same work is baked by others too, with each other envelope it holds and the baker each states (G8). | `g14_5_same_work` |
| 65 | G14.5(b) | **Never picks.** A verifier MUST NOT choose between bakes that carry the same work: not the first, by position, by time or in any other way, and it MUST NOT answer which baker made the work. | `g14_5_same_work` |

## Part 2. Case by case

Every case of the test program, with the sentence it cites (`build/test_mor --list` prints this table). Cases beyond the 65 sentences test other sentences of the text: defaults, answers, and the words of section 2.

| Grammar rule | Test case (tests/test_mor.c) | The sentence it tests |
|---|---|---|
| G1.2 | `g1_2_under_other_terms` | Where an act's `for` names an agreement, its `under` MUST name Terms of that agreement: an act made for an agreement under any other Terms is not that agreement's signature. |
| G1.2 | `g1_2_one_level` | these grammar rules MUST NOT be read as making an agreement's signing need the signature of anyone inside an agreement it names, unless its own Terms say so. |
| G1.2 | `g1_2_named_not_bound` | Naming an agreement as a party asks nothing of it: it need not sign to be named, [...] and where it has not signed it is not bound by any act of these Terms, and neither are its parties. |
| G1.2 | `g1_2_power_signature` | An act is signed by an agreement x when its `for` names x, by its root; its `under` names Terms of x that are in force (G4.6) and give a power for acts of that kind; and that power's rule, applied to the acts held, says yes. |
| G1.4 | `g1_4_no_version_qualified` | Where that act names no version, a verifier MUST take the receiver from that agreement's Terms in force, per that agreement's own place, qualified per place with that place named, or, where it names none, per the anchoring the reader accepts, or as shown (G4.6, G7, G13.4). |
| G1.4 | `g1_4_fork_receiver` | while a fork of its Terms stands, the receiver is the one named by the Terms both branches replace, and the verifier MUST show the fork with that answer and MUST NOT choose a branch (G4.7). |
| G1.4 | `g1_4_not_shown_cases` | If a version read names no receiver, or names as receiver a signer or a role that is not among its own parties, [...] no one receives, and the answer is not shown. [...] If the verifier lacks that agreement's acts, it is unchecked (G9). |
| G1.4 | `g1_4_through_agreements` | Where it is another agreement (G1.3), [...] a verifier reads that agreement's receiver in the same way, and so on, until one of an agreement's own parties is reached. |
| G1.4 | `g1_4_role_receiver` | or the spot named is not filled, or is filled more than once (which of them receives is not said), [...] no one receives, and the answer is not shown. |
| G2.1 | `g2_1_fingerprints` | The `body`, every piece in `relies_on`, and every cMIP named anywhere in a Terms act MUST be named by fingerprint. |
| G2.2 | `g2_2_not_part` | Anything a Terms act refers to in another way is not part of those Terms. A verifier MUST NOT read it as part of them. |
| G3.1 | `g3_1_authors` | `authors` MUST name one or more of the parties named as signers or roles. |
| G3.2 | `g3_2_authors_input` | A cMIP that reads a judge's decision, in whichever socket it sits (section 5.3), MUST accept `authors` as an input. |
| G4.1 | `g4_1_default` | If `in_force` is absent, they are in force when there is an assent to them in the name of every party named as a signer |
| G4.2 | `g4_2_role_needs_in_force` | Terms that name a party by role MUST carry `in_force`. |
| G4.6 | `g4_6_qualified` | A verifier MUST take it per the agreement's place, qualified per place (G7.2); or, where no place is named, per the anchoring the reader accepts, with it named, or else as shown (G7.3, G13.4). It MUST NOT give it as latest without saying what the answer stands on. |
| G4.7 | `g4_7_fork_of_terms` | A verifier MUST answer fork for which Terms replace them, show both tips, and MUST NOT choose between them, by position or in any other way. |
| G4.7 | `g4_7_answers_from_shared` | A verifier MUST answer from them every answer that rests on the Terms in force: among others, a receiver for an act (G1.4), who fills a spot (G10.2), and what a rule the Terms name produces (G5; tasks K2 to K4). It MUST show the fork with each such answer, per place or as shown. |
| G4.8(c) | `g4_8_default_update` | Under the default, a Terms act that lacks one of those signatures replaces nothing: the Terms it names stay in force, and it is not a tip of a fork. A verifier answers not shown for it as a replacement |
| G4.8(e) | `g4_8_e_unchecked_update` | Where two Terms acts name the same original, and for one of them the verifier cannot tell whether it is an update [...], which Terms are in force is unchecked from there on [...]. The verifier names the last version it has established. |
| G4.8 | `g4_8_new_version_in_force` | Whether the replacing Terms are themselves in force is G4.1's, as draft 18 has it (G4.5; section 8.3). |
| G4.9 | `g4_9_never_ended` | No act ends Terms, or an agreement, without replacing them, and a verifier MUST NOT answer that Terms or an agreement have ended. |
| G4.10 | `g4_10_counter_place` | Whether the Terms were in force before the counter signature is an answer about what is latest: per place, or as shown (G4.6). With a place, the positions it gives settle it. |
| G4.10 | `g4_10_counter_citing` | With none, only citing gives an order (G13.3): a counter signature that names the assents which complete the Terms came after them, and changes nothing; a completing assent that names the counter signature came after it, and the assent it cancels is not counted. Where neither names the other, whether those Terms are in force is not shown, and the verifier says why. |
| G4.11(c) | `g4_11_c_not_valid` | A verifier MUST answer not valid for it, and MUST name the condition beside that answer, with the ground the cMIP gives: an act, reported by who signed it (G8), or a time, per the clock the agreement names (G7.4) |
| G4.11(c) | `g4_11_c_time_ground` | [the ground the cMIP gives:] or a time, per the clock the agreement names (G7.4), or, where it names none, per the anchoring named or as shown (G13.4). |
| G4.11(i) | `g4_11_i_no_lock` | A verifier MUST NOT answer, of itself, that an agreement is locked, or that it can never be updated. |
| G4.11(h) | `g4_11_h_lacks_cmip` | Where the verifier lacks the cMIP, whether the update replaces is unchecked (G9), and so is every answer that rests on it (G4.8(e)). |
| G5.1(b) | `g5_1_b_power` | the act's `for` names them [...]; its `under` names Terms by which they are bound and which are in force; those Terms give a power for acts of this kind; and that power's rule, applied to the acts held, says yes |
| G5.1(c) | `g5_1_c_bound_by_update` | the act is a Terms act that replaces Terms by which they are bound, and the update condition of those Terms answers valid for it (G5.2). |
| G9.2 | `g5_powers_through_powers` | A verifier that stops following (powers through powers, G5; receivers from agreement to agreement, G1.4; the versions of a long chain) answers unchecked for what lies beyond where it stopped, and says how far it followed. |
| G5.2 | `g5_2_no_power_for_replacement` | No power makes a Terms act replace, and no power binds a party to one: `powers` has no kind `replaces`. An entry of `powers` of that kind gives no power; a verifier says that it left it unread (G2.2). |
| G5.3 | `g5_3_own_name_only` | An act with no `for` and no `under` is made in its signer's own name only. It MUST NOT later be counted as made for anyone else. |
| G5.4 | `g5_4_replaced_power_place` | A verifier MUST answer from G7: per place if the Terms that gave the power name one, by the act's position against the position at which the replacing Terms came into force; |
| G5.4 | `g5_4_replaced_power_citing` | otherwise as shown, by citing alone (G13.3), and not shown where neither the act nor the replacement names the other. |
| G5.5 | `g5_5_delegation` | A verifier MUST NOT answer that such a delegation can never be ended: whether the agent will end it is not shown, and the verifier shows the condition as the Terms the delegator signed state it. |
| G5.6(d) | `g5_6_powers_cannot_touch_judicial` | A verifier MUST NOT read an act made under a power as naming or replacing a judge; as giving, widening, narrowing or removing a power to resolve; or as changing whom a contest reaches. |
| G6.1 | `g6_1_previous` | Every act within an agreement MUST name in `previous` its signer's previous act within the same agreement, or none if it is the signer's first. An act whose `previous` names an act of another signer, or of another agreement, is invalid (section 2). |
| G6.2 | `g6_2_line_fork` | A verifier holding both MUST answer fork for that signer, and MUST NOT choose between the tips. |
| G7.2 | `g7_2_place` | A verifier MUST take those answers from the place, qualified per place. |
| G7.3 | `g7_3_as_shown` | Where no place is named, a verifier MUST qualify every such answer as shown, or per anchoring where the reader accepts one, with it named (G13.4). It MUST NOT give such an answer as latest or whole without saying what it stands on. |
| G7.4 / G13.4 | `g7_4_one_place_for_all` | a verifier MUST NOT give an answer of G7.2 for that agreement per another anchoring. || A verifier MUST NOT give, per a reader's own anchoring, an answer that the place or the clock the agreement names gives. || A verifier MUST give each such answer per anchoring, with the anchoring named (per place or per clock where it is the one the Terms name), or as shown. |
| G8.1 / G8.3 | `g8_who_said_it` | a verifier MUST report who stated it: the one who performed, the one who received, a named witness, the controller, or the place. || A verifier MUST NOT report value as moved on the strength of an Agreements act alone. |
| G9.1 | `g9_1_unchecked` | Where a verifier cannot apply a rule it needs, because it lacks a cMIP named by fingerprint, lacks an act named by fingerprint, lacks the acts of an agreement named as a party, or cannot get an answer from a named place, it MUST answer unchecked for every answer that depends on it. |
| G9.2 | `g9_2_where_a_verifier_stops` | A verifier that stops following [...] answers unchecked for what lies beyond where it stopped, and says how far it followed. It MUST NOT answer not shown or invalid because it stopped. |
| G9.2 | `g9_2_counter_work` | A verifier that stops following [...] answers unchecked for what lies beyond where it stopped, and says how far it followed. [a case found by fuzzing: counter signatures (G4.10) that lean on one another] |
| G10.1 | `g10_1_role_fields` | A role in `parties` MUST name: its kind [...]; its `what` [...], by fingerprint (G2), except for kind future (G10.4); its spot socket (task K5) |
| G10.2 / G8.4 | `g10_2_fillings` | A verifier MUST answer each filling with its proof, each act reported by who signed it (G8.4), qualified per place or as shown (G7). || A verifier MUST report them with whoever confirmed them. |
| G10.5 | `g10_5_new_identities` | A verifier MUST give that answer with its cause, as the cMIP gives it, per place, per clock, per the anchoring named or as shown (G7, G13.4). |
| G10.6 / G10.7 | `g10_6_rights` | A verifier MUST name, with each filling, the version it was earned under, and each later version that changes its rights, with what that rests on: the holder's own assent, or the condition named and its answer. || these grammar rules have no answer triggered, and none that a filling's rights no longer apply, and a verifier MUST NOT give either. || a verifier reads them from there (G2) and MUST NOT answer them of itself. |
| G10.6 | `g10_6_not_under_another_condition` | A verifier MUST NOT read a later version as changing a filling's rights under an update condition which that filling's own version does not name. |
| G11.2 | `g11_2_control_task` | Terms hold the Control task (K6) where their field `control` names a cMIP (G2); or, with that socket empty, where they name as `receiver` one of their own parties (G1.3) and give a part to any other party |
| G11.3 / G11.5 | `g11_3_readable_without_cmip` | What K6's row asks of a leg report MUST be readable without any cMIP those Terms name: a verifier that lacks them still reads the leg reports, and reports what they state as stated by their signers. || Where the doer is itself one to whom those Terms give a part, its report states that part as its own, and a verifier MUST NOT read it as kept (K6's row). |
| G11.4(c) | `g11_4_ties` | Where they do not, the verifier MUST show both, with the amount each states, and MUST NOT choose between them. |
| G11.4(d) | `g11_4_d_silence` | A verifier that does not, because that leg's reports travel no further than the leg (G11.6), MUST NOT show the call as unanswered: for it, that leg's reports are not shown. |
| G11.4 | `g11_4_sequence` | A verifier asked about a result that depends on a sequence MUST read it leg by leg, from the reports it holds (G11.6), and give each leg's part with who signed it (G8.4). |
| G11.4(e) | `g11_4_e_before_paying` | Asked which agreements of a sequence carry a call for reports, a verifier answers from their Terms alone (G11.2), one agreement at a time, per place or as shown (G4.6), and unchecked where it lacks them. |
| G11.6 | `g11_6_call_reads` | A call MUST NOT be read as naming a place, or a reader. A call MUST NOT be read as expecting a report on a task held by other Terms than the ones that carry it. |
| G11.6(a) / G11.6(b) | `g11_6_data_path` | no call, no field and no answer of this text carries a report beyond its leg, and a verifier MUST NOT read any as doing so. || A verifier MUST NOT read the people of a leg as having agreed to be read from beyond their leg unless Terms they signed say so (G5.1): by naming such a data cMIP in `data` (K8), or by a rule in them under which others decide it. |
| G11.6(b) | `g11_6_b_majority` | Where those Terms say that a majority decides, the majority decides (G5.1(b)): a verifier reads a member who was outvoted as that rule says, and MUST NOT answer, beyond it, whether that member agreed. |
| G11.7 | `g11_7_buyers_leg` | a verifier shows a payment so confirmed as matched, one whose confirmation states another amount as mismatched, and one the receiving side has not confirmed as one side only; it MUST NOT show a payment as received on the buyer's word alone (G8.3). |
| G11.8 | `g11_8_contest` | A verifier MUST NOT read a contest as contesting anyone but the signers of what it is made against: not a service those signers picked, not a deal of theirs, not any other agreement of the sequence. It MUST NOT read a contest as giving its signer sight of any leg (G11.6, G11.7). |
| G12.2 | `g12_2_built_on_tip` | A verifier MUST show both tips, and what it holds that is built on them, with the fork; MUST answer fork where G4.7 and G6.2 say; and MUST NOT choose between the tips, by position or in any other way. |
| G12.3 | `g12_3_line_resolved` | An act that names `resolves` and no `continues` resolves nothing. [...] After a fork in a signer's line is resolved, that signer's next act MAY name as `previous` either the resolving update or the last act of the continued branch |
| G12.3 | `g12_3_must_name_every_tip` | A resolving update MUST name every tip, and MAY name more acts than the tips. |
| G12.3 | `g12_3_terms_resolved` | It resolves the fork only when it is signed as an update of the shared act must be: [...] for Terms, so that the update condition of the shared Terms answers valid for it, which says who must sign it; by default, everyone who signed the shared version |
| G12.4 | `g12_4_power_to_resolve` | A verifier MUST report such a resolving update with the power it was signed under and whoever it rests on (G5.1, G8). |
| G12.5 | `g12_5_disputed` | A verifier MUST show that act, and every answer resting on it, as disputed. |
| G12.7 | `g12_7_rival_resolvers` | Two resolving updates of the same fork, each signed as G12.3 asks, that continue the same tip resolve it, and are no fork. Two that continue different tips are themselves a fork |
| G12.7 | `g12_7_rival_resolvers_terms` | Neither resolves the first fork while the other stands: every tip stays stopped (G12.2), and the shared act stays the last act of the line that counts. That fork is resolved the same way: by a resolving update that names the two rival ones in `resolves` and one of them in `continues` |
| G1.4 | `g1_4_loop` | or an agreement is met a second time on the way, no one receives, and the answer is not shown. |
| G12.8 | `g12_8_client` | A client that holds both tips of a fork MUST NOT show either tip, or anything built on either, as a resolved act: not as the next version, not as in force, not as that signer's latest act. [a duty of clients; no verifier can check it: tested only as far as the verifier's own output goes] |
| G13.3 | `g13_order` | An act that names another, directly or through acts that name it, came after it [...]. Between two acts of which neither so names the other, no order is shown from the acts alone. |
| G13.2 | `g13_1_read_against_named` | A verifier MUST NOT read an act as having relied on a version or act it does not name. |
| G14.3 | `g14_baker` | A verifier asked whether Terms count as an agreement for a baked work MUST read the envelope, the baker it states, and, under (b), every agreement of the succession. || A verifier MUST give that answer with every answer about that work that rests on those Terms (a receiver, a filling, a leg report). |
| G14.2(b) | `g14_succession` | one of their parties, bound by them, holds the baker's rights through a succession: agreements for the same envelope, of which the first meets (a); each passes the baker's rights to a party of the next [...]; and the last is these Terms. |
| G14.5(a) / G14.5(b) | `g14_5_same_work` | it MUST show, beside each of them and beside every answer of G14.3 about Terms for one of them, that the same work is baked by others too, with each other envelope it holds and the baker each states (G8). || A verifier MUST NOT choose between bakes that carry the same work: not the first, by position, by time or in any other way, and it MUST NOT answer which baker made the work. |
| section 2 / 3 | `s3_rejected` | An act that lacks a required field (section 3), or bytes that cannot be read as an act, never reach these grammar rules. A verifier shows them as rejected, which is no answer of this text. |
