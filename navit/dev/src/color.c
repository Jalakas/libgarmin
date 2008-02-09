#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cfg.h"
#include "list.h"
#include "color.h"
#include "debug.h"

#define COLORS_CFG	"colors.conf"

static list_head(lschemes);

struct color_def {
	list_t l;
	char *name;
	struct color color;
};

struct color_scheme {
	list_t l;
	char *name;
	list_t lcolors;
};

struct color_scheme *cs_lookup(char *name)
{
	struct color_scheme *cs;

	list_for_entry(cs, &lschemes, l) {
		if (!strcmp(cs->name, name))
			return cs;
	}
	return NULL;
}

struct color *cs_lookup_color(struct color_scheme *cs, char *name)
{
	struct color_def *cd;
	list_for_entry(cd, &cs->lcolors, l) {
		if (!strcmp(cd->name, name))
			return &cd->color;
	}
	return NULL;
}

int cs_parse_color(char *def, struct color *color)
{
	int r,g,b,a;

	if(strlen(def)==7){
		sscanf(def,"#%02x%02x%02x", &r, &g, &b);
		color->r = (r << 8) | r;
		color->g = (g << 8) | g;
		color->b = (b << 8) | b;
		color->a = (65535);
	} else if(strlen(def)==9){
		sscanf(def,"#%02x%02x%02x%02x", &r, &g, &b, &a);
		color->r = (r << 8) | r;
		color->g = (g << 8) | g;
		color->b = (b << 8) | b;
		color->a = (a << 8) | a;
	} else {
		debug(0,"Invalid color definition [%s] has unknown format\n",
				def);
		return -1;
	}
	return 1;
}

int cs_resolve_color(char *scheme, char *name, struct color *color)
{
	struct color_scheme *cs;
	struct color *c;

	if (*name == '#')
		return cs_parse_color(name, color);
	cs = cs_lookup(scheme);
	if (!cs)
		return -1;
	c = cs_lookup_color(cs, name);
	if (c) {
		*color = *c;
		return 1;
	}
	return -1;
}

static struct color_def *cs_add_color_def(struct color_scheme *cs, char *name, 
							char *value)
{
	struct color_def *cd;
	cd = calloc(1,sizeof(*cd));
	if (!cd)
		return cd;
	cd->name = strdup(name);
	if (!cd->name) {
		free(cd);
		return NULL;
	}
	if (cs_parse_color(value, &cd->color) < 0) {
		free(cd->name);
		free(cd);
		return NULL;
	}
	list_append(&cd->l, &cs->lcolors);
	return cd;
}

static struct color_scheme *cs_alloc(char *name)
{
	struct color_scheme *cs;
	cs = calloc(1, sizeof(*cs));
	if (!cs)
		return NULL;
	list_init(&cs->lcolors);
	cs->name = strdup(name);
	if (!cs->name) {
		free(cs);
		return NULL;
	}
	list_append(&cs->l, &lschemes);
	return cs;
}

int cscheme_init(void)
{
	struct navit_cfg *cfg;
	struct color_scheme *cs;
	struct cfg_varval *var = NULL;
	struct cfg_category *cat = NULL;
	cfg = navit_cfg_load(COLORS_CFG);
	if (!cfg) {
		debug(5, "No color schemes defined\n");
		return -1;
	}
	debug(5, "Color schemes initializing ...\n");
	while ((cat = navit_cfg_cats_walk(cfg, cat))) {
		cs = cs_alloc(cfg_cat_name(cat));
		if (!cs)
			goto out_err;
		var = NULL;
		while((var = navit_cat_vars_walk(cat, var))) {
			cs_add_color_def(cs, cfg_var_name(var), cfg_var_value(var));
		}
	}
	navit_cfg_free(cfg);
	return 0;
out_err:
	navit_cfg_free(cfg);
	return -1;
}

