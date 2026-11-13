#!/usr/bin/env python3
"""Checks that every sentence with MUST or MUST NOT in the two texts is cited by a test,
and that every citation is a verbatim quotation of the texts."""
import re, subprocess, sys

def norm(s):
    return ' '.join(s.split())

text = norm(open('spec/agreements-draft-17.md').read() + '\n' + open('spec/report-block-2.md').read())
cites = []
for line in subprocess.run(['build/test-san', '--cites'], capture_output=True, text=True, check=True).stdout.splitlines():
    name, cite = line.split('\t', 1)
    for c in cite.split(' || '):
        cites.append((name, norm(c)))

bad = [(n, c) for n, c in cites if c not in text]
for n, c in bad:
    print(f'NOT A QUOTATION ({n}): {c}')

# sentences that say how the word MUST is used, and state no duty
META = ('MUST and MAY are used as in RFC 2119', '"should not", written MUST NOT')

sentences = re.split(r'(?<=[.:;])\s+(?=[A-Z*(`\-])', text)
must = [s for s in sentences if 'MUST' in s and not any(m in s for m in META)]
missing = []
for s in must:
    if not any('MUST' in c and (c in s or s in c) for _, c in cites):
        missing.append(s)
for s in missing:
    print('NO TEST CITES: ' + s[:200])
print(f'{len(must)} sentences with MUST or MUST NOT; {len(must) - len(missing)} cited by a test; {len(cites)} citations, {len(bad)} not verbatim')
sys.exit(1 if bad or missing else 0)
