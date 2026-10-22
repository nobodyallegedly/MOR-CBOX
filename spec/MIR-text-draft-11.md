# MIR: Text

*Draft 11, freeze-ready, for Nobody, allegedly, to approve.*

## How to read this MIR

Normal text is the protocol. Italic text is commentary: reasons, examples and plain summaries. A plain summary in italics (*How it runs most of the time*) repeats what the rules say; where the two seem to differ, the rule's own text decides.

The rules are numbered in reading order, save rule 14, which stands beside the text act it describes. A rule of another MIR is cited by that MIR's name and the rule's number ("Envelopes rule 30").

This mark recurs:

- *Client conformance:* a rule for the signer's own client, which no verifier can check.

The MIR has four parts. Part one is text as it runs most of the time: one form for every text, the text act, and what a client shows. Part two is the exact rules of canonical text. Part three is the act format. Part four is the tasks (the bound on formats), the reasoning, the parameters and the freeze scenarios.

## Purpose

Text is the only native media of MOR, because the protocol itself speaks in text: terms, agreements, grants, names, messages. This MIR defines two things:

1. **Canonical text:** the one form every piece of text takes inside any act, so that the same words always give the same bytes, and therefore the same hash.
2. **The text act:** a plain message every client can read. Whatever cMIPs two clients use, their users can always exchange text.

## Dependencies

Identity, for signers. The act format is the envelope defined in the Identity MIR and completed by the Envelopes MIR. Every client implements this MIR.

## Words used in this MIR

- **Canonical text.** A string that meets the rules below. Every text field of every act, in every MIR and cMIP, is canonical text.
- **Text field.** Every CBOR text string anywhere in an act: on its outside, in its inside, in its payload, as a map key, inside an array or a tag, whatever specification defines the field, and whether or not the verifier implements it.
- **Text act.** An act whose payload is a canonical text, readable by any client.
- **Format.** An optional cMIP that gives text extra meaning when displayed, such as emphasis or links. The text stays readable as plain text without it.
- **Pinned Unicode version.** The version of the Unicode Standard whose tables every verifier uses for the normalization check: Unicode 17.0.
- **Bidirectional controls.** The characters U+202A to U+202E and U+2066 to U+2069, which change the visual order of text without changing its bytes.

## Part one: text as it runs most of the time

*How it runs most of the time: a person types; their client should normalize what they typed into canonical text, so the same words always give the same bytes; the text is signed inside an act like any other. Every client can show it as plain text, whatever format it names. Before anything is signed, a client must be able to show its plain text, and before terms, a grant or a clone is signed, it must show the bidirectional controls as what they are.*

### 1. One form for every text

#### Rule 1. Every text field is canonical

Every text field of every act MUST be canonical text. An act with a text field that is not canonical is invalid. For a private act, only those who hold its key can check this, and they MUST before relying on it.

This holds whatever else in the act the verifier cannot read: an act of a specification the verifier does not implement, carrying a text that is not canonical, is invalid, not unknown (Envelopes rule 30, "Invalid beats unknown").

*Canonical text is defined exactly in Part two (rules 7 to 12).*

#### Rule 2. The client normalizes for the user

A client composing text SHOULD normalize it for the user: convert line breaks to LF, apply NFC, remove trailing spaces and a final line break, remove U+FEFF, replace each TAB with spaces, and remove other control characters and noncharacters, telling the user when it removed something visible, so users never see a rejection for invisible reasons.

### 2. The text act

A text act is an Envelopes MIR act of `type` 0 under the hash of this MIR, written `TEXT`. The fields it carries are rule 14's. Like every act, its inside is locked: a public text carries its key, a private one is delivered to its recipients, possibly in a sealed container. A private text looks like any other act to a relay. *Its payload is in Part three.*

#### Rule 14. What a text act carries

Besides what every act carries (Envelopes, Part three, "The act"), a text act carries, on its outside, `signer`, `binding`, its content key if it is public, and `to` where it is addressed or answers someone; inside, `spec` (`TEXT`), `type` 0, the everyday fields (`prev`, position and running summary, Envelopes rule 13), the payload (Part three), and optionally `refs` (the acts or web resources it responds to or mentions) and `hint`. A text act carrying `acks` is invalid. A text act carrying `objects` is invalid, save two cases. First, where its signer is a collective, it may carry exactly one object, `[collective, decision]`, naming that collective itself as chain and, as predecessor, its own latest decision (a link of its identity chain or one of its records), the pointer by which Agreements rule 58 places a collective's action on its actions chain. *Client conformance:* a client names the latest such decision it holds; a verifier accepts, as predecessor, any of the collective's own records or links of its identity chain. Second, where the act is signed with a grant key (a scoped key, Identity rule 8), it may carry exactly one object, `[grantor, grant]`, naming its signer (the grantor) as chain and, as predecessor, the grant its `binding` names, the grant it acts under (Agreements rules 58, 73 and 75); whether the grant backs it is Agreements' to judge. Any other `objects` on a text act, whoever signs it and with whatever key, is invalid. **Where the act is signed with a grant key, a holder's member key included, the second case decides**, read for a member key as below, whoever the signer is; the first case is for an act signed with the collective's own key. **For a holder's member key** (Agreements rule 73), whose `binding` names the holder's signature act rather than a grant, the one object is `[collective, version]`: the collective, its signer, as chain, and, as predecessor, the founding or clone that signature names, the version that is its grant. A verifier that does not yet hold the signer's identity chain, the decision named, or the grant its binding names (for a member key, the signature act and the version it names), answers unknown.

**A verifier that implements Text but not Agreements** cannot tell a collective, a grant key or a decision from other identities, keys and acts. It answers unknown for a text act carrying exactly one object of either allowed shape (`[signer, act]`, the chain being the act's own signer), and invalid for any other `objects`. *Stated cost: a collective whose decisions are sealed to its members leaves its texts unknown to anyone else who holds no decision they name.*

Chained texts (a co-written document, a collective's log, a thread in one agreed order) are a cMIP's: each version stays a plain text act, and the cMIP carries the chain in a companion act of its own that names it. *A text act belongs to no core chain, save two pointers: the one by which a collective's text sits on that collective's own actions chain, and the one by which a grantee's text names the grant it acts under on its grantor's chain. Each names only the signer's own decision or grant, so it attaches nothing to anyone else's chain, and an individual still chains its own texts by a cMIP's companion act. Allowed any other `objects`, anyone could attach text acts to another's chain (a deal, a collective's record) by naming it, for a careless client to show as if they belonged there, and a text act naming an agreement's head would read, by Envelopes rule 15 taken literally, as a fork of that agreement. With a companion act every word stays readable by every client, at one extra act per step.*

#### Rule 3. Plain text, always

A client MUST be able to display any valid text act as plain text, whatever its format.

#### Rule 4. An unknown format still shows its text

A client that does not implement the format a text act names MUST still display its text as plain text. Format is the only case where an unknown cMIP does not make an act unknown: the text itself is always understood (Envelopes rule 32).

### 3. What a client shows

#### Rule 5. Warnings, not rules

A client SHOULD warn about lookalike characters (letters from different scripts that look the same) and about invisible characters that can make text display differently from its bytes: the zero-width space (U+200B), the bidirectional marks (U+061C, U+200E, U+200F) and the format characters: every character of the pinned Unicode version whose general category is Cf (format) or whose property is Bidi_Control or Default_Ignorable_Code_Point.

These are client warnings, not validity rules: text in every script, mixed as people need, stays valid. The zero-width joiner (U+200D) and non-joiner (U+200C) are needed by emoji sequences and several scripts, and are never warned about for their presence alone.

#### Rule 6. What you sign is what you saw

*Client conformance.* Before any act is signed, a client MUST be able to show all of its text as plain text, character for character, and SHOULD show it that way by default for agreements, terms and grants. Before signing terms, a grant or a clone (Agreements), a client MUST show every bidirectional control visibly, as an escape, in that plain-text view, so that no clause can display in an order different from its bytes. What you sign is what you saw. *This is a rule for the signer's own client; no verifier can check it, which is why it is conformance and why Agreements' act types are named explicitly.*

## Part two: the exact rules of canonical text

### 4. The six rules

A canonical text is a sequence of Unicode characters, encoded as a CBOR text string, that meets all of these rules:

#### Rule 7. UTF-8

It is valid UTF-8. The character U+FEFF (byte order mark, zero-width no-break space) never appears, anywhere in the text.

#### Rule 8. One line break

Lines are separated by LF (U+000A) only. CR (U+000D), and the line and paragraph separators U+2028 and U+2029, never appear.

#### Rule 9. No control characters

No control characters other than LF: nothing from U+0000 to U+001F except U+000A, nothing from U+007F to U+009F. This excludes TAB.

#### Rule 10. No trailing spaces or line break

No line ends with a space character, the text does not end with one, and the text does not end with LF. The space characters are: U+0020, U+00A0, U+1680, U+2000 to U+200A, U+202F, U+205F, U+3000. The list is fixed, so it cannot change with Unicode.

#### Rule 11. No noncharacters

None of the 66 code points Unicode reserves for internal use: U+FDD0 to U+FDEF, and the last two code points of every plane (U+FFFE, U+FFFF, U+1FFFE, U+1FFFF, and so on up to U+10FFFE, U+10FFFF).

#### Rule 12. NFC

The text is in Normalization Form C, checked with the tables of the pinned Unicode version. A character not yet assigned in that version is left as it is.

**The empty string and bidirectional controls.** The empty string is canonical text. Bidirectional controls are valid canonical text: isolates have legitimate uses in mixed-script text, and the protection against their misuse is in display (rule 6), not in validity.

### 5. How a verifier checks them

#### Rule 13. Exactly, with the pinned tables

A verifier MUST check rules 7 to 11 exactly, and rule 12 with the tables of the pinned Unicode version, never with whatever version its system happens to have. Where a text breaks more than one of rules 7 to 12, a verifier that names the rule broken names the lowest-numbered of them; the verdict, invalid, is the same whichever is named.

## Part three: act format

### Text act (type 0)

```cddl
text-payload = {
  0 => tstr,        ; the text, canonical
  ? 1 => hash       ; format: the cMIP that gives the text extra meaning
}
```

The payload is decoded strictly: a closed map, field 0 a text string, field 1, if present, a 32-byte hash, and no other field. A text act whose payload is not in this format, or whose type is not 0, is invalid, under the MICS's common convention for every specification (Envelopes rule 30). *Every client implements this MIR, so every client shows such an act invalid: a text act with no field 0, a byte string in field 0, a field 2, or type 7 under `TEXT` is not a text act anywhere.*

## Part four: tasks, reasoning, parameters and freeze scenarios

### Tasks

- **Text format.** A cMIP accepts a canonical text and produces a rendering for display. It must accept every canonical text. It may change how text looks, but it may hide only its own markup: the characters its specification declares as markup, in the positions it declares them, and nothing else; and whatever its declaration says, it never hides a letter, a digit or a combining mark (general categories L, N and M), a currency sign (Sc), a mathematical sign (Sm) next to a digit, the percent, per-mille and per-ten-thousand signs (U+0025, U+066A, U+FE6A, U+FF05, U+2030, U+0609, U+2031), a character between two digits, a plus or minus sign (U+002B, U+002D, U+2212, and the dash-like minus signs U+2010, U+2011, U+2012, U+2013, U+2796, U+FE63, U+FF0D) directly before a digit or a currency sign or directly after a digit, any run of hidden characters between two digits (a line break that ends a block excepted, since a new block is shown, not hidden), emphasis or code markup between two digits, a full stop or comma directly before a digit, a bracket directly around an amount, a space or apostrophe between two letters, or a question or exclamation mark; a format's declaration says which of its hidden characters end a block; where the floor forbids hiding a markup character, the format shows it as text rather than refusing the document; it must never add text that is not in the bytes. It shows the characters it does not hide in the order of the bytes, and it never makes one of them invisible or unreadable by styling: no colour, size or position hides one. Plain display is always the fallback, and before signing, the plain text is always available (rule 6). *A format that could hide letters could hide a clause from readers of an offer; bounding what it may hide keeps readers, not only signers, protected. A format that could hide everything but letters and digits could still hide a minus sign, a decimal point or a vowel mark, and change what an amount or a word says without hiding a letter; so the bound is the format's own declared markup, which a client implementing the format checks its rendering against; conformance, as rule 6. But the format's author writes that declaration, and the bound must hold against the publisher who picks the format; so under every declaration lies a floor the protocol sets, the characters that carry what an amount or a word says, which no declaration can make hideable. Banning all punctuation and symbols would forbid markup itself; the floor names only what carries meaning. A format that could move text, a clause to a footnote at the end, a "not" to another column, could make a reader see an order the bytes do not say, as a bidirectional override can; a format that could shrink or colour text to nothing could hide it without "hiding" it. So a format keeps the order and keeps what it shows readable.*

### Reasoning

- **One form, one hash.** *Agreements, names and text works are compared and hashed as bytes. If the same words could be written in two byte forms, two people could sign "the same" terms with different hashes, or claim "the same" work twice. Canonical text removes that.*
- **One final form.** *Editors often add a final line break without the writer noticing. Banning it removes the most common way the same words end up with two hashes.*
- **Noncharacters and U+FEFF are out.** *Unicode reserves noncharacters for internal use, and U+FEFF is an invisible leftover of file encodings. Neither belongs in text people exchange.*
- **What you sign is what you saw.** *Formats may hide markup, as a Markdown renderer hides the asterisks around bold words; they may not move words or make them vanish by styling. That is fine for reading, but not for signing: before anything is signed, the plain text is always one tap away, and it is the default for terms. Bidirectional controls are the one case where even plain text can lie about its order, so for the acts that bind people they are shown as what they are.*
- **The rules are fixed, not borrowed.** *Unicode grows every year. Rules that follow whatever version a machine has would let two verifiers disagree on whether an act is valid. So the space list is written out, control characters are defined by range, and normalization uses one pinned version. New characters, such as new emoji, remain usable. One rare edge remains: a combining mark added to Unicode after the pinned version is treated as unassigned, so two orders of such marks could both pass. Every verifier still agrees, which matters more. Unicode's stability policy guarantees that text in NFC under one version stays in NFC under every later one, so pinning never shuts out future text.*
- **No TAB.** *A tab displays at different widths in different places, and can hide alignment tricks in terms. Text that needs layout uses a format.*
- **Warnings, not rules, for lookalikes.** *Banning mixed scripts would exclude real people writing real languages. Deciding what looks alike is judgement, so it belongs to clients.*
- **Every client can open text.** *Every client implements the Envelopes MIR, so every client can unlock a public text and any private text addressed to it. Locking everything does not weaken text's universality.*
- **A text act is closed.** *Its fields are listed as a rule, as every payload map is closed, so no reader takes a text act for part of another's chain (its only pointers being the signer's own two, rule 14), and no light act carries the weight of an acknowledgement. What is given up, a chain of plain text acts of one's own, is kept by a cMIP's companion act, every word still readable by every client.*
- **Formats never lock text away.** *Text is the one media every client understands, so no cMIP may make a message unreadable. A format adds meaning; it never replaces the words, and it never hides them.*

### Parameters

- The pinned Unicode version: Unicode 17.0.
- Maximum text length, if any: left to relays' storage policy.
- Language tags: not included; a format or a cMIP can carry them.

### Freeze scenarios

Components exercised (freeze test suite v30; scenario numbers are the suite's):

- Text universal across sub-networks: scenario 5 (a messaging client with no Money or Agreements still exchanges text with both parties).
- Canonical text in every act: all scenarios, since terms, names and messages are text.
- A text act with a format the receiving client lacks, displayed as plain text: scenario 5.
- Terms containing a bidirectional override, shown with the control visible before signing: scenario 1.
- A formatted text act whose rendering hides only markup, adds nothing, and shows the rest in the order of the bytes: scenario 5.
- A text act with no field 0, with a field 0 that is not a text string, with a field 2, and one of a type Text does not define: invalid in every client: a component under "Envelopes and Text".
- A text act carrying `objects` outside rule 14's two cases, invalid at every verifier, and never a fork of another MIR's chain; one carrying `acks`, invalid: a component under "Envelopes and Text".
- A collective's text act carrying, as its one object, its own decision on its actions chain: valid, and counted under Agreements rule 58; with another collective's chain, a predecessor that is no decision of its own, two objects, any other object, or an individual signer naming its own chain save as the grant case allows: invalid, and counting for nothing in Agreements: a component under "Envelopes and Text".
- A text act signed with a grant key, carrying as its one object `[grantor, grant]`, the grant its binding names: valid under Text, and judged by Agreements' backing (rules 73 to 80), backed, not backed or unknown; naming another act of the grantor, another chain, or two objects: invalid: a component under "Envelopes and Text".
- A text act signed with a holder's member key, carrying as its one object `[collective, version]`, the founding or clone its binding's signature names: valid under Text, and judged by Agreements; naming the signature act itself, or another version: invalid at a verifier implementing Agreements: a component under "Envelopes and Text".
- A cMIP act a client does not implement, carrying a text that is not canonical: invalid, not unknown: a component under "Envelopes and Text".