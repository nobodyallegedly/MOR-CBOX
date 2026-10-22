#include "text14.h"

#include "identity.h"

int mor_is_decision(mor_world *w, const uint8_t c[32], const uint8_t id[32])
{
	const mor_act *d = mor_world_get(w, id);
	if (!d)
		return -1;
	if (mor_is_chain_act(w, d))
		return mor_hash_eq(d->type == 0 ? d->id : d->signer, c);
	/* a record (type 17), a grant (9) or a revocation (10) of the collective */
	if (mor_world_is_mir(w, d, MOR_AGREEMENTS) && (d->type == 17 || d->type == 9 || d->type == 10))
		return mor_hash_eq(d->signer, c);
	return 0;
}

int mor_text14(mor_world *w, const mor_act *a)
{
	if (a->acks)
		return MOR_ST_INVALID;
	if (!a->objects)
		return MOR_ST_VALID;
	if (a->objects->v != 1 || !a->signer)
		return MOR_ST_INVALID;
	const uint8_t *chain = a->objects->items[0].items[0].data, *pred = a->objects->items[0].items[1].data;
	if (!mor_hash_eq(chain, a->signer))
		return MOR_ST_INVALID;
	const mor_act *b = a->binding ? mor_world_get(w, a->binding) : NULL;
	if (!b)
		return MOR_ST_UNKNOWN;
	if (mor_world_is_mir(w, b, MOR_AGREEMENTS)) {
		/* Signed with a grant key: the one object names the grant its binding names. */
		if (b->type == 9)
			return mor_hash_eq(pred, b->id) ? MOR_ST_VALID : MOR_ST_INVALID;
		/* Signed with a holder's member key: [collective, version], the founding
		 * or clone the binding's signature names, never the signature act. */
		if (b->type == 1 && mor_pl(b, 1))
			return mor_hash_eq(pred, mor_pl_hash(b, 0)) ? MOR_ST_VALID : MOR_ST_INVALID;
		return MOR_ST_INVALID;
	}
	if (!mor_is_chain_act(w, b))
		return MOR_ST_INVALID;
	/* Signed with the signer's own key: only a collective, naming its own decision. */
	const mor_cbor *agr;
	int coll = mor_identity_agreement(w, a->signer, &agr);
	if (coll < 0)
		return MOR_ST_UNKNOWN;
	if (coll == 0)
		return MOR_ST_INVALID;
	int d = mor_is_decision(w, a->signer, pred);
	if (d < 0)
		return MOR_ST_UNKNOWN;
	return d ? MOR_ST_VALID : MOR_ST_INVALID;
}
