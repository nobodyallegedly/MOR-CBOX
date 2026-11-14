#!/usr/bin/env python3
"""Regenerate tests/MUSTS.md: every sentence of the text with MUST or MUST NOT,
and the test case(s) for it. Run from the repository root after `make build/test_mor`.
The sentences are cut from spec/agreements-draft-19.md; the mapping is kept here."""
import re, subprocess, sys

CASES = {
 1: 'g1_2_under_other_terms', 2: 'g1_2_one_level', 3: 'g1_4_no_version_qualified', 4: 'g1_4_fork_receiver',
 5: 'g2_1_fingerprints', 6: 'g2_2_not_part', 7: 'g3_1_authors', 8: 'g3_2_authors_input', 9: 'g4_2_role_needs_in_force',
 10: 'g4_6_qualified', 11: 'g4_6_qualified', 12: 'g4_7_fork_of_terms', 13: 'g4_7_answers_from_shared',
 14: 'g4_7_answers_from_shared', 15: 'g4_9_never_ended', 16: 'g4_11_c_not_valid, g4_11_c_time_ground',
 17: 'g4_11_i_no_lock', 18: 'g5_3_own_name_only', 19: 'g5_4_replaced_power_place, g5_4_replaced_power_citing',
 20: 'g5_5_delegation', 21: 'g5_6_powers_cannot_touch_judicial', 22: 'g6_1_previous', 23: 'g6_2_line_fork',
 24: 'g7_2_place', 25: 'g7_3_as_shown', 26: 'g7_3_as_shown', 27: 'g7_4_one_place_for_all', 28: 'g8_who_said_it',
 29: 'g8_who_said_it, g11_7_buyers_leg', 30: 'g10_2_fillings, g11_3_readable_without_cmip',
 31: 'g9_1_unchecked, g7_2_place', 32: 'g9_2_where_a_verifier_stops, g5_powers_through_powers',
 33: 'g10_1_role_fields', 34: 'g10_2_fillings', 35: 'g10_5_new_identities', 36: 'g10_6_not_under_another_condition',
 37: 'g10_6_rights', 38: 'g10_6_rights', 39: 'g10_6_rights', 40: 'g11_3_readable_without_cmip', 41: 'g11_4_sequence',
 42: 'g11_4_ties', 43: 'g11_4_d_silence', 44: 'g11_3_readable_without_cmip', 45: 'g11_6_call_reads',
 46: 'g11_6_call_reads', 47: 'g11_6_data_path', 48: 'g11_6_data_path', 49: 'g11_6_b_majority', 50: 'g11_7_buyers_leg',
 51: 'g11_8_contest', 52: 'g11_8_contest', 53: 'g12_2_built_on_tip',
 54: 'g12_2_built_on_tip, g4_7_fork_of_terms, g6_2_line_fork', 55: 'g12_3_must_name_every_tip',
 56: 'g12_4_power_to_resolve', 57: 'g12_5_disputed', 58: 'g12_8_client', 59: 'g13_1_read_against_named',
 60: 'g7_4_one_place_for_all', 61: 'g7_4_one_place_for_all, g7_3_as_shown', 62: 'g14_baker, g14_succession',
 63: 'g14_baker', 64: 'g14_5_same_work', 65: 'g14_5_same_work',
}

text = open('spec/agreements-draft-19.md').read()
rule, rows = '-', []
for line in text.split('\n'):
    m = re.match(r'\s*- (G\d+\.\d+)', line)
    if m:
        rule = m.group(1)
    m2 = re.match(r'\s*- \(([a-z])\)', line)
    sub = m2.group(1) if m2 else ''
    for s in re.split(r'(?<=[.:;]) (?=[A-Z])', line):
        if 'MUST' not in s or s.startswith('MUST and MAY') or 'Each grammar rule is given' in s \
           or 'breaks a MUST' in s or '"should not"' in s:
            continue  # sentences about the word MUST, not rules
        s = re.sub(r'^\s*- (G\d+\.\d+ |\([a-z]\) )?', '', s.strip())
        rows.append((rule + ('(' + sub + ')' if sub else ''), s.replace('|', '\\|')))
if len(rows) != len(CASES):
    sys.exit('the text holds %d MUST sentences, the mapping %d' % (len(rows), len(CASES)))
src = open('tests/test_mor.c').read()
for v in CASES.values():
    for n in v.split(', '):
        if 'static void ' + n + '(void)' not in src:
            sys.exit('no such case: ' + n)
out = ['# Every sentence with MUST or MUST NOT, and the test case for it', '',
 "For Nobody, allegedly. There are no test vectors in the box: every case below was written from the text alone, and is the builder's reading of it (see `QUESTIONS.md`). A case that passes shows that the code does what the builder read, not that the reading is right.", '',
 "`spec/agreements-draft-19.md` holds %d sentences with MUST or MUST NOT in its grammar rules (some hold two). Each has at least one case in `tests/test_mor.c`. Two of them bind someone other than a verifier, and are tested only as far as a verifier's own behaviour goes: G3.2 binds a cMIP (the test shows that `authors` is handed to a rule in a socket), and G12.8 binds a client (the test shows that the verifier never hands out a tip as resolved)." % len(rows), '',
 'This file is made by `tests/gen_musts.py`.', '',
 '## Part 1. Sentence by sentence', '', '| # | Grammar rule | The sentence | Test case(s) |', '|---|---|---|---|']
for i, (r, s) in enumerate(rows, 1):
    out.append('| %d | %s | %s | %s |' % (i, r, s, ', '.join('`%s`' % c for c in CASES[i].split(', '))))
out += ['', '## Part 2. Case by case', '',
 'Every case of the test program, with the sentence it cites (`build/test_mor --list` prints this table). Cases beyond the %d sentences test other sentences of the text: defaults, answers, and the words of section 2.' % len(rows), '']
out.append(subprocess.run(['./build/test_mor', '--list'], capture_output=True, text=True, check=True).stdout)
open('tests/MUSTS.md', 'w').write('\n'.join(out))
