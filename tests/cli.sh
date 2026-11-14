#!/bin/sh
# cli.sh: the command-line tool, end to end, on the example in examples/.
# usage: sh tests/cli.sh path/to/morv
set -eu
M=$1
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
fail() { echo "FAIL cli: $1"; exit 1; }

"$M" seal examples/song.draft > "$T/song.acts"
cmp -s "$T/song.acts" examples/song.acts || fail "sealing the example is not reproducible"
"$M" verify "$T/song.acts" > "$T/out.txt"
grep -q '^\[G4.7\] replaced by .*: fork' "$T/out.txt" || fail "the fork of Terms is not answered"
grep -q '^\[G11.7\] payment .*: matched' "$T/out.txt" || fail "the confirmed payment is not matched"
grep -q '^\[G11.4\] tie .*: mismatched' "$T/out.txt" || fail "the two amounts are not shown as mismatched"
if grep -q ': unanswered' "$T/out.txt"; then fail "a reader beyond the leg is shown a call as unanswered"; fi

# the same file read on behalf of a side of the leg, with a question
ROOT=$("$M" fp "$T/song.acts" >/dev/null; "$M" verify --full "$T/song.acts" | sed -n 's/^\[G4.7\] in force \([0-9a-f]*\) .*/\1/p' | head -1)
[ -n "$ROOT" ] || fail "no root found"
"$M" verify --reader @ben --receiver "$ROOT" --in-force "$ROOT" --before-paying "$ROOT" "$T/song.acts" > "$T/out2.txt"
grep -q '^\[G1.4\] receiver .*@ada' "$T/out2.txt" || fail "the receiver is not answered"
grep -q '^\[G11.4(e)\] call ' "$T/out2.txt" || fail "before paying is not answered"

# bytes that are no act are rejected, and never crash the tool
printf 'not an act\nmor-act 1 terms\nsigner @a\n' > "$T/junk"
"$M" verify "$T/junk" > "$T/out3.txt"
[ "$(grep -c ': rejected' "$T/out3.txt")" = 2 ] || fail "junk is not rejected"
head -c 4096 /dev/urandom > "$T/rand"
"$M" verify "$T/rand" > /dev/null || fail "random bytes"
"$M" verify /nonexistent 2>/dev/null && fail "a missing file is not an error"
echo "ok   cli"
