# The report: a block for all of MOR (proposal 2)

*The MOR lead, 13 November 2026. Proposal 1 (`report-block-1.md`) published again, after the exchanges "A person who receives for others", "Who signs the deal, and who can call", "The payer's seat", "The contest" and "Reports are data" in `docs/decisions.md` (branch `claude/lead-records-2`). Not a decision. Not yet written into any spec text.*

**The wish** (Nobody, allegedly): "can I with a single click reward all the people that contributed to the service or good via MOR". And: "Building blocks over solutions."

## In plain words

A report is data, and in the words of Nobody, allegedly: "Data needs to move along paths that people agree, with signature." The one path the core gives is the **leg**: two sides who both signed, the one who carries out an obligation and the one it is carried out for. So a report moves **leg by leg only**, in every layer.

Any task may **call for a report**, or not. Where it does, the call says **from whom**: the side that carried out the obligation, the side that received, or both. It does not say where: the **where is implied by the leg**, since a report goes to the other side of its own leg, where that side receives. Only those who signed the act that holds the task can make the call.

A report carries three things, whatever the task: **the call it answers, the acts it relied on, and who signs it.** Everything else (amounts, a holder's own part, a fee kept, what is still owed) is the task's to define.

Each side signs its own half; an act both name (for a payment, the settlement) ties the halves. Matching halves are a knot; halves that do not match are shown side by side; a half with no answer is shown as one side only. A call nobody answered is shown as unanswered; a task with no call, as no report called for.

A receiver who holds for others then carries out the next obligation, within their own agreement, and the same exchange happens there: every leg is the same exchange. Seeing beyond one leg (a partner who wants the end of the waterfall, a request from further up) is another path, and moves only where the people whose data it carries agreed to it with their signature, built as a data cMIP. The core gives no call that names an outside reader.

## Precisely (second wording)

- **R1. Call.** An act that names a task MAY attach a call to it. A call states the task and from whom a report is expected (the side carrying out the obligation, the receiving side, or both, as the task defines those sides). A call binds only the signers of the act that holds it.
- **R2. Report.** A report is a signed act carrying: the call it answers, by fingerprint; the acts it relied on, by fingerprint; its signer. A task MAY define further content. A report that names no call is no report.
- **R3. One leg.** A report is addressed to the other side of its own leg, and to no one else. A call cannot name any other reader.
- **R4. Halves.** Where a call expects reports from both sides, a verifier shows, for each act both reports name: matched, mismatched (both shown, neither chosen), or one side only.
- **R5. Silence.** A call with no report is shown as unanswered; a task with no call, as no report called for. Neither is read as anything else.

**Lie exposed.** "It went through", said by one side alone; "you never asked", said by a side whose signers did.

**Protects.** Both sides of every leg, who each hold the other's signed word. And everyone further along or further up, whose data moves nowhere they did not sign for.

## What changed from proposal 1

- The call no longer says where; the where is implied by the leg (R1, R3).
- Leg by leg only, in every layer; no call names an outside reader (R3). Paths beyond one leg are data cMIPs people signed for.
- The other content is unchanged.

## Open

1. Is a call per task, or per act holding several tasks?
2. How "where that side receives" is found without a server address (the Deliver layer's, and the security rules).
3. Whether a report can answer several calls at once (one batch, many payments; pools, P6).
4. Whether "unanswered" ever ends.
5. Names: "call", "report", "leg", "matched", "one side only", "unanswered" are the MOR lead's, as technical wording; "leg" is his word.
