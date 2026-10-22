/* The running summary (Envelopes, Part three, "The running summary"). */
#ifndef MOR_MMR_H
#define MOR_MMR_H

#include <stddef.h>
#include <stdint.h>

/* An incremental Merkle mountain range: at most 64 peaks. */
typedef struct mor_mmr {
	uint8_t peak[64][32];
	unsigned height[64];
	unsigned npeaks;
	uint64_t n;
} mor_mmr;

void mor_mmr_init(mor_mmr *m);
void mor_mmr_add(mor_mmr *m, const uint8_t id[32]);
/* S(n): the root, peaks bagged right to left; 32 zero bytes when empty. */
void mor_mmr_root(const mor_mmr *m, uint8_t out[32]);

#endif
