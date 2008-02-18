#ifndef __NAVIT_CFG_H__
#ifdef __cplusplus
extern "C" {
#endif

struct cfg_varval;
struct cfg_category;
struct navit_cfg;

struct navit_cfg *navit_cfg_alloc(void);

struct navit_cfg *navit_cfg_load(char *file);
void navit_cfg_free(struct navit_cfg *cfg);

struct cfg_category *navit_cfg_cats_walk(struct navit_cfg *cfg, struct cfg_category *c);
struct cfg_category *navit_cfg_find_cat(struct navit_cfg *cfg, char *name);
struct cfg_varval *navit_cat_find_var(struct cfg_category *cat, char *name);
struct cfg_varval *navit_cat_vars_walk(struct cfg_category *cat, struct cfg_varval *v);
char *cfg_cat_name(struct cfg_category *cat);
char *cfg_var_name(struct cfg_varval *v);
int cfg_var_true(struct cfg_varval *v);
int cfg_var_intvalue(struct cfg_varval *v);
char *cfg_var_value(struct cfg_varval *v);
struct attr;
void navit_cfg_attrs_free(struct attr **attrs);
struct attr **navit_cfg_cat2attrs(struct cfg_category *cat);


int navit_cfg_save(struct navit_cfg *cfg, char *file);

#ifdef __cplusplus
}
#endif

#define __NAVIT_CFG_H__
#endif
