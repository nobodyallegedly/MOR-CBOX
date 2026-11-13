#ifndef MOR_INTERNAL_H
#define MOR_INTERNAL_H

#define MOR_MAX_AGR (4 * MOR_MAX_ACTS)

#include "mor.h"

typedef struct mor_blk {
    struct mor_blk *next;
    size_t used, cap;
} mor_blk;

/* the place (or anchoring) through which acts are seen for one agreement (G7) */
typedef struct {
    const mor_act *place; /* NULL: every act held is seen */
    mor_qual qual;
    bool cannot; /* a place is named and gives no answer (G9) */
} mor_view;

struct mor_set {
    mor_blk *blocks;
    mor_options opt;
    mor_act **acts;
    int n_acts;
    mor_act **by_fp; /* sorted by fingerprint */
    mor_reject *rejects;
    int n_rejects;
    mor_agreement **agr;
    int n_agr;
    mor_act **st_by_terms; /* Statements sorted by the Terms they name */
    int n_st;
    mor_act **tr_by_repl; /* Terms sorted by the Terms they replace */
    int n_tr;
    mor_act **topo; /* every act after the acts it names */
    int n_topo;
    const mor_act **stack;
    mor_line_fork *lforks;
    int n_lforks;
    mor_view view; /* the view in use while a plug-in is asked */
    int depth;
    bool evaluated;
    bool unsettled;
    unsigned epoch;
    unsigned *seen; /* per act index, for walks */
};

/* decode.c */
int mor_decode(mor_set *s, const unsigned char *bytes, size_t len);

/* set.c */
mor_act *mor_find_mut(const mor_set *s, const mor_fp *fp);
const char *mor_strdup_n(mor_set *s, const char *p, size_t n);
int mor_fp_cmp(const mor_fp *a, const mor_fp *b);
bool mor_ref_eq(const mor_ref *a, const mor_ref *b);
const char *mor_short(mor_set *s, const mor_fp *fp);
const char *mor_ref_str(mor_set *s, const mor_ref *r);

/* grammar.c */
int mor_named(const mor_act *a, const mor_fp **out, int max);
int mor_st_range(const mor_set *s, const mor_fp *terms, int *end);
unsigned mor_restmark(const mor_set *s, const mor_act *a);
mor_tri mor_spot_fillers(mor_set *s, const mor_act *terms, const mor_party *role, const mor_view *v, const char **who, int *n);
void mor_spot_eval(mor_set *s, const mor_act *terms, const mor_party *role, const mor_view *v, mor_spot *out);
mor_agreement *mor_agr(mor_set *s, const mor_fp *root);
void mor_agr_eval(mor_set *s, mor_agreement *a);
mor_view mor_view_for(mor_set *s, const mor_act *terms);
bool mor_usable(const mor_set *s, const mor_act *a, const mor_view *v);
int mor_pos(const mor_view *v, const mor_act *a);
mor_tri mor_assent_in_name(mor_set *s, const mor_act *t, const mor_ref *name, const mor_view *v);
mor_tri mor_check_power(mor_set *s, const mor_act *x, const char *kind, const char **why);
mor_tri mor_bound_signer(mor_set *s, const char *p, const mor_act *u);
mor_tri mor_k4_call(mor_set *s, const mor_rule *rule, mor_k4_q q, const mor_act *terms, const mor_act *subject, const mor_view *v,
                    mor_k4_out *out);
bool mor_is_version(const mor_agreement *a, const mor_act *t);
mor_tri mor_k1_read(mor_set *s, const mor_act *terms, mor_k1_out *out);
const mor_party *mor_party_named(const mor_act *t, mor_ref_kind k, const char *name);
int mor_order3(mor_set *s, const mor_view *v, const mor_act *a, const mor_act *b); /* -1 a first, 1 b first, 0 none shown */

#endif
