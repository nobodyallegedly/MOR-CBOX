#include "verdict.h"

#include "identity.h"

int mor_verdict(mor_world *w, const uint8_t id[32])
{
	const mor_act *a = mor_world_get(w, id);
	if (!a)
		return MOR_ST_UNKNOWN;
	int st = mor_status_act(w, a, 0);
	if (st == MOR_ST_INVALID)
		return st;
	if (mor_world_mir_of(w, a->spec) == MOR_MIR_NONE)
		return MOR_ST_UNKNOWN;
	return st;
}
