/* Canonical text (Text MIR, Part two, rules 7 to 13). */
#ifndef MOR_TEXT_H
#define MOR_TEXT_H

#include <stddef.h>
#include <stdint.h>

#include "arena.h"

/* The six rules, numbered as Part two lists them (Text rules 7 to 12 are
 * the vectors' rules 1 to 6). */
enum mor_text_rule {
	MOR_TEXT_CANONICAL = 0,
	MOR_TEXT_UTF8 = 1,      /* Text rule 7: valid UTF-8, no U+FEFF */
	MOR_TEXT_LINEBREAK = 2, /* Text rule 8: no CR, U+2028, U+2029 */
	MOR_TEXT_CONTROL = 3,   /* Text rule 9: no C0 (save LF), DEL or C1 */
	MOR_TEXT_TRAILING = 4,  /* Text rule 10: no trailing space, no final LF */
	MOR_TEXT_NONCHAR = 5,   /* Text rule 11: no noncharacters */
	MOR_TEXT_NFC = 6,       /* Text rule 12: NFC under Unicode 17.0 */
	MOR_TEXT_NOMEM = -1,
};

/* Returns MOR_TEXT_CANONICAL, or the first rule broken (rules 7 to 11 checked
 * in order over the whole string, NFC last), or MOR_TEXT_NOMEM. The arena is
 * used for scratch and restored before returning. */
int mor_text_check(const uint8_t *s, size_t n, mor_arena *scratch);

/* The Unicode version of the normalization tables linked in. */
const char *mor_text_unicode_version(void);

#endif
