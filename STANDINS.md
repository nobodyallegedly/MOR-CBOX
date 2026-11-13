# Stand-ins

The Agreements text leans on layers that are not in the box (its section 8.1). Each thing below is the simplest stand-in that let the work go on. None of it is MOR. Each sits behind one small interface, named in the last column, so that it can be replaced without touching the grammar.

Every stand-in record has the same outer shape, decoded in one place (`src/decode.c`):

```
mor-act 1
type <terms|statement|payment|settlement|report|envelope|place|clock|piece>
signer <identifier>
<field> <value...>        any number of lines, in any order
sig <64 hex digits>
```

Lines are printable ASCII, single spaces, ended by a newline. Blank lines and lines starting with `#` between records are skipped. An identifier is 1 to 48 characters of letters, digits, `_`, `-`, `.`. `mor-agreements seal` turns a record that ends in a bare `sig` line into a sealed one and prints its fingerprint.

| # | Stand-in | The sentence of section 8.1 it stands in for | What was chosen, and why | Interface |
|---|---|---|---|---|
| S1 | **Act format** | "The core's act format: whether it already gives fingerprints, a per-signer line of acts (which would make G6 redundant), acts signed by several people at once, and field names." | The text record above. Field names are the ones of section 3 of the text, one field per line; a repeated field (`party`, `relies_on`, `answers`, `resolves`, `power`, `call`) is one line per entry. One act has one signer: nothing is signed by several people at once. The core gives no per-signer line here, so G6 is built in full. Text was chosen over a binary form because every test case can then be read by a person. | `mor_set_load()`; all parsing in `src/decode.c` |
| S2 | **Fingerprint** | Same sentence ("whether it already gives fingerprints"); and section 2, "the identifier the core gives to fixed bytes". | SHA-256 over the record's bytes from `mor-act 1` up to, not including, the `sig` line; written as 64 lowercase hex digits. A well-known hash was needed only so that acts can name one another; any collision-resistant hash would do. | `mor_fp_of()` in `src/set.c` (the only caller of libcrypto) |
| S3 | **Signature and signer** | "What the Sign layer does when a key is replaced or lost, since that decides whether G6 sees a fork." | **Not a signature.** A signer is an identifier, and the `sig` line is a tag anyone can compute: SHA-256 over a fixed label, the signer's identifier and the act's fingerprint. It holds no key, so the repository holds none, and it proves nothing about who made an act. It exists so that "the signature does not verify" is a path the decoder has and the tests exercise. A signer never changes: a replaced or lost key is not modelled, so G6 sees a fork exactly when one identifier has two acts with the same `previous`. | `mor_standin_sign()`, `mor_standin_verify()` in `src/set.c` |
| S4 | **A cMIP, named by fingerprint** | "How cMIPs are identified in practice, so that 'named by fingerprint' in G2 can be met." | A cMIP is an entry in a table the caller hands the verifier (`mor_plugins`): a fingerprint, the task it answers, and a function. A cMIP not in the table, or marked absent, is one the verifier "lacks" (G9). The stub cMIPs shipped here have the fingerprint SHA-256 of `mor-stub-cmip-1:` and their name; `mor-agreements stubs` lists them. A cMIP "with its parameters" is the fingerprint followed by the rest of the line. | `mor_cmip`, `mor_plugins`, `mor_cmip_find()`; stubs in `src/stubs.c` |
| S5 | **Place** | "What the Order layer's place can answer: in particular whether it can sign 'nothing later than this', and how a place is identified without a server address." | A record of type `place`, whose signer is the place's identifier, listing fingerprints in order (`entry <fp>`). An act is "there" if listed; its position is its index; "anything later" is whatever follows. One record is the place's whole answer; none held, or two for one identifier, is "cannot get an answer from a named place" (G9.1). A place is identified by its signer identifier and nothing else: no address. This is task K6 as far as the grammar needs it. | `mor_view_for()`, `mor_pos()` in `src/grammar.c` |
| S6 | **Report, and call** | "The report, a block of the shared language … What the block has not yet said: how a call is written, where it sits in the act that holds the task, and how a report names it by fingerprint … how a report is written as an act, and whether it is a Statement of this text". | A call is a Terms line `call <task> <doer|receiving|both>`; anything after "from whom" is never read (G11.6). A report is its own record type, not a Statement, and stands in no signer's line: `call <fp>` (the act that holds the call), `side <doer|receiving>`, `relied <fp>` (acts relied on), and K8's lines: `received <amount> <settlement>`, `passed <amount> <to> <settlement>`, `own <amount>`, `kept <amount>`, `owed <amount> <to>`. A report names the call it answers by the fingerprint of the Terms that carry it; one call per task per Terms act. A report with no `call` is rejected (the block's R2). A settlement named in a line counts as an act the report relied on. | record type `report`; read in `mor_k8_read()`, `src/legs.c` |
| S7 | **The seam: a confirmation of a payment** | "*A confirmation with no call written* … that a report may name the call it answers by naming the act that holds it (here the payment, by its settlement)". | The confirmation of a buyer's payment is a report whose `call` is the settlement's fingerprint, as G11.7 says ("that is how it names the call it answers"). | `mor_payment_read()` |
| S8 | **Payment and settlement** | "What a settlement is, as the layer that holds value gives it, so that the `settlement` field (G8.5) names the right thing; and what a payment is there: how it names the version of the work agreement it read". And: "What a payment is as an act, in the layer that holds value: where the payer's call sits in it, and whether a payer can state that no confirmation is called for". | A `payment` record, signed by the payer: `agreement <root>`, optionally `version <fp>` (the version it read), `settlement <fp>`, `amount`. A `settlement` record is opaque: only its fingerprint and its signer ("whoever gave it", G8.5) are read. Every payment carries the default call; a payer cannot say that no confirmation is called for. | record types `payment`, `settlement` |
| S9 | **Amount** | "How that layer writes an amount, so that the amounts in a leg report (task K8) can be read by everyone alike (P20)." | A whole number of at most 18 digits, no leading zero, then a unit of 1 to 8 capital letters or digits (`100 EUR`). Two amounts are the same when number and unit are the same; nothing is converted or rounded. | `get_amount()` in `src/decode.c`, `amount_eq()` in `src/legs.c` |
| S10 | **Envelope and baker** | "What the core calls an envelope, baking and a baker (G14): whether an envelope has a fingerprint of its own, how it states who baked it, and whether a work may have several bakers." | An `envelope` record with one line, `baker <identifier>`. Its fingerprint is the record's. One baker per work. | record type `envelope`; `mor_work_read()` |
| S11 | **Clock** | "What a clock is, which layer gives it, and how it is identified without a server address (G7.4)." | A `clock` record whose signer is the clock's identifier, with `now <whole number>`: the clock's one answer to "what time it is". Times are bare numbers; only plug-ins read them. | `mor_time_for()` |
| S12 | **`for` holding an agreement** | "Whether the core has an act signed by several people at once, for an agreement that signs by every one of its signers (G1.2; P17); and whether the core's `for`, if it has one, can hold an agreement's root." | `for signer <id>` or `for agreement <root>`. An agreement with no power signs by one assent per signer (QUESTIONS.md, Q15). | `mor_ref` |
| S13 | **The verifier's words** | "Whether the verifier's answers (shown, not shown, fork, valid and not valid, invalid, unchecked) are already named in another layer, so that the words match". | The words of section 2 of the text, as one C enumeration, plus the block's five words. Nothing else is ever answered. | `mor_ans` in `include/mor.h` |
| S14 | **Contest** | "Whether the core already has a contest." | Built as the text writes it: a Statement of kind `contest`. | `mor_contest_read()` |
| S15 | **A body's words** (not listed in 8.1) | Section 3.1: `body` is the "fingerprint of the words". | A `piece` record: fixed bytes, as `data <text>` lines. Its signer is not read. The stub cMIP `k1.lines` reads `share <party> <n>` and `delegation <delegator> <agent> <default|agent|opaque>` lines as "parts for machines". | record type `piece`; `mor_k1_read()` |
| S16 | **Who is reading** (not listed in 8.1) | G11.4(d): "Unanswered is said only by a verifier that reads as a side of that leg". | An option naming the signer on whose behalf the verifier reads (`--reader`), and two more naming an anchoring the reader accepts where an agreement names no place or clock (`--anchoring-place`, `--anchoring-clock`; G13.4). | `mor_options` |

## Sentences of section 8.1 with no stand-in, and why none was needed

- "Seeing beyond one leg: an agreed path": the text does not say what a data cMIP is or how Terms name one. Nothing was invented. The verifier answers **unchecked** for whether the signers agreed to such a path, and never **shown**.
- "The research's own open question: how much a proven liar must stand to lose": nothing in the grammar reads it.
- "What a receiving side's 'automatic' report is": a confirmation is always a report record signed by the receiving side.
- "What access to what was bought is": the grammar does not read it (G11.7).
- "What a client is": the tool's own output keeps G12.8 (a tip is never shown as resolved), and that is tested; nothing else was built.

## The stub cMIPs

Stubs exist so that the grammar can be run with a plug-in's answer, and with none. Marking a stub absent (`--without <name>`, or `--no-plugins`) gives the answer **unchecked** wherever the grammar needed it.

| Task | Stub | What it answers |
|---|---|---|
| K1 Body | `k1.lines` | reads the piece's lines: shares, and whether the Terms are a delegation and how it ends |
| K4 Rule | `k4.yes`, `k4.no` | yes, or no, to everything (valid, or not valid, as an update condition) |
| K4 Rule | `k4.threshold N <party>...` | yes when N of the parties listed have an assent in their name; as a power over a Statement, yes when its signer is listed |
| K4 Rule | `k4.delegate <signer>...` | yes when the act asked about is signed by one of them |
| K4 Rule | `k4.before T` | yes while the time (per the clock named, or the anchoring) is before T |
| K4 Rule | `k4.same-parties` | an update condition from the bytes alone: valid only if the update names the same parties |
| K8 Control | `k8.hand` | a name only: Terms that name it in `relies_on` hold the Control task |
| K10 Judge | `k10.judge` | hands back what it was given; shows that the interface carries `authors` (G3.2) |
| K11 Kind | `k11.claim`, `k11.confirm`, `k11.pass-rights` | three kinds of Statement, used by the stubs below and by G14 |
| K12 Spot | `k12.claim [one\|many] [max=N] [until=T] [confirm=<signer>]` | a filling is a `k11.claim` Statement whose content is the role's name, confirmed where asked; says whether the spot still takes new identities, and why |
| K13 Standing | `k13.count` | counts a signer's forks, contests and assents from the acts held |

K2 (Identity), K3 (Assurance), K5 (Witness), K7 (Tally) and K9 (Delivery) have no stub: no grammar rule, as written, asks any of them a question. K6 (Place) is S5.
