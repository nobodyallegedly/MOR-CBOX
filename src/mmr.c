#include "mmr.h"

#include <string.h>

#include "crypto.h"

void mor_mmr_init(mor_mmr *m)
{
	memset(m, 0, sizeof *m);
}

static void node(uint8_t out[32], const uint8_t l[32], const uint8_t r[32])
{
	mor_tagged_hash2(out, "MOR/mmr-node", l, 32, r, 32);
}

void mor_mmr_add(mor_mmr *m, const uint8_t id[32])
{
	uint8_t cur[32];
	unsigned h = 0;
	mor_tagged_hash(cur, "MOR/mmr-leaf", id, 32);
	while (m->npeaks && m->height[m->npeaks - 1] == h) {
		node(cur, m->peak[m->npeaks - 1], cur);
		m->npeaks--;
		h++;
	}
	memcpy(m->peak[m->npeaks], cur, 32);
	m->height[m->npeaks] = h;
	m->npeaks++;
	m->n++;
}

void mor_mmr_root(const mor_mmr *m, uint8_t out[32])
{
	uint8_t acc[32];
	if (!m->npeaks) {
		memset(out, 0, 32);
		return;
	}
	memcpy(acc, m->peak[m->npeaks - 1], 32);
	for (unsigned i = m->npeaks - 1; i > 0; i--)
		node(acc, m->peak[i - 1], acc);
	memcpy(out, acc, 32);
}
