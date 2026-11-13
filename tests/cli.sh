#!/bin/sh
# cli.sh - runs the command-line tool (built with the sanitizers) end to end.
set -eu
BIN=${1:-build/mor-agreements-san}
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
fail() { echo "FAIL cli: $1"; exit 1; }

# seal: build two acts by hand, naming one by the fingerprint the tool prints
K1=$("$BIN" stubs | awk '$3=="k1.lines"{print $1}')
printf 'mor-act 1\ntype piece\nsigner nobody\ndata words for people\nsig\n' | "$BIN" seal > "$T/piece"
WORDS=$(awk '/^# fingerprint/{print $3}' "$T/piece")
printf 'mor-act 1\ntype terms\nsigner alice\nprevious none\nparty signer alice\nauthor signer alice\nbody %s %s\nsig\n' "$WORDS" "$K1" | "$BIN" seal > "$T/terms"
ROOT=$(awk '/^# fingerprint/{print $3}' "$T/terms")
printf 'mor-act 1\ntype statement\nsigner alice\nagreement %s\nterms %s\nkind assent\nprevious %s\nsig\n' "$ROOT" "$ROOT" "$ROOT" | "$BIN" seal > "$T/assent"
ASSENT=$(awk '/^# fingerprint/{print $3}' "$T/assent")
cat "$T/piece" "$T/terms" > "$T/set1"
cat "$T/set1" "$T/assent" > "$T/set2"

"$BIN" verify "$T/set1" | grep -q 'Terms in force: not shown \[as shown\]' || fail "not shown without the assent"
"$BIN" verify "$T/set2" | grep -q "Terms in force: version .* shown \[as shown\]" || fail "shown with the assent"
"$BIN" verify --no-plugins "$T/set2" | grep -q 'Terms in force: version' || fail "default in_force needs no plug-in"
"$BIN" order --a "$ROOT" --b "$ASSENT" "$T/set2" | grep -q 'a came before b' || fail "order from citing"
"$BIN" bound --signer alice --act "$ASSENT" "$T/set2" | grep -q '^shown' || fail "bound by own assent"
"$BIN" bound --signer bob --act "$ASSENT" "$T/set2" | grep -q '^not shown' || fail "no one else is bound"
"$BIN" standing --signer alice "$T/set2" | grep -q 'forks standing 0' || fail "standing"
"$BIN" before-paying --root "$ROOT" "$T/set2" | grep -q 'before paying' || fail "before-paying"
# damaged bytes are rejected, never read
sed 's/party signer alice/party signer mallory/' "$T/set2" > "$T/bad"
"$BIN" verify "$T/bad" | grep -q 'the signature does not verify' || fail "damaged act rejected"
# the kept examples
for f in examples/*.mor; do "$BIN" verify "$f" > /dev/null || fail "example $f"; done
"$BIN" verify examples/click.mor | grep -q 'leg: doer sam -> signer bob .*: mismatched' || fail "click example"
"$BIN" verify examples/fork.mor | grep -q 'FORK of Terms' || fail "fork example"
if "$BIN" nonsense >/dev/null 2>&1; then fail "unknown command"; fi
echo "cli: ok"
