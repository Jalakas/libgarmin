#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "globals.h"
#include "debug.h"
#include "cfg.h"
#include "item.h"
#include "attr.h"

struct cfg_varval {
	char *name;
	char *value;
	struct cfg_varval *next;
};

struct cfg_category {
	char *name;
	struct cfg_varval *vars;
	struct cfg_category *next;
};

struct navit_cfg {
	struct cfg_category *cats;
};

struct navit_cfg *navit_cfg_alloc(void)
{
	struct navit_cfg *c;
	return calloc(1, sizeof(*c));
}

static void cfg_free_var(struct cfg_varval *v)
{
	free(v->name);
	free(v->value);
	free(v);
}

static void cfg_free_category(struct cfg_category *cat)
{
	struct cfg_varval *v, *var = cat->vars;
	while (var) {
		v = var->next;
		cfg_free_var(var);
		var = v;
	}
	free(cat);
}

void navit_cfg_free(struct navit_cfg *cfg)
{
	struct cfg_category *c, *cat = cfg->cats;
	while (cat) {
		c = cat->next;
		cfg_free_category(cat);
		cat = c;
	}
}

static void cat_add_var(struct cfg_category *cat, struct cfg_varval *v)
{
	struct cfg_varval *vars;
	vars = cat->vars;
	/* Do not prepend to keep the order */
	if (!vars) {
		cat->vars = v;
		return;
	}
	while (vars->next)
		vars = vars->next;
	vars->next = v;
}

static struct cfg_varval *cfg_alloc_var(char *name, char *value)
{
	struct cfg_varval *v;
	v = calloc(1, sizeof(*v));
	if (!v)
		return v;
	v->name = strdup(name);
	v->value = strdup(value);
	if (!v->name || !v->value) {
		free(v->name);
		free(v->value);
		free(v);
		return NULL;
	}
	return v;
}

static void trimr(char *str)
{
	char *cp = str;
	cp += strlen(cp) - 1;
	while(*cp && isspace(*cp)) {
		*cp = '\0';
		cp --;
	}
}

static int process_line(struct cfg_category *cat, char *line)
{
	char name[4096];
	char value[4096];
	int rc;
	struct cfg_varval *v;

	rc = sscanf(line, "%[^ =] = %[^\n]", name, value);
	if (rc != 2) {
		debug(0, "Invalid line:%d\n",rc);
		return -1;
	}
	trimr(value);
	v = cfg_alloc_var(name, value);
	if (v) {
		cat_add_var(cat, v);
		return 1;
	}
	return -1;
}

static void cfg_add_category(struct navit_cfg *cfg, struct cfg_category *c)
{
	struct cfg_category *cat;

	cat = cfg->cats;
	/* Do not prepend to keep the order */
	if (!cat) {
		cfg->cats = c;
		return;
	}
	while (cat->next)
		cat = cat->next;
	cat->next = c;
}

static struct cfg_category * process_category(struct navit_cfg *cfg, char *line)
{
	char *name = line;
	char *cp;
	struct cfg_category *cat;
	cp = strchr(name, ']');
	if (!cp)
		return NULL;
	*cp = '\0';
	name++;
	cat = calloc(1, sizeof(*cat));
	if (!cat) {
		debug(1, "Can not allocate category:[%s]\n", name);
		return NULL;
	}
	cat->name = strdup(name);
	cfg_add_category(cfg, cat);
	return cat;
}

struct navit_cfg *navit_cfg_load(char *file)
{
	FILE *fp;
	struct navit_cfg *cfg;
	struct cfg_category *cat = NULL;
	char buf[4096];
	int lineno = 0;
	char *cp;

	if (*file == '/') {
		fp = fopen(file, "r");
	} else {
		sprintf(buf, "%s/etc/%s", navit_prefix, file);
		fp = fopen(buf, "r");
	}
	if (!fp)
		return NULL;
	cfg = navit_cfg_alloc();
	if (!cfg)
		goto out_err;
	while (fgets(buf, sizeof(buf), fp)) {
		lineno++;
		cp = buf;
		cp += strspn(cp, " ");
		if (*cp == '#' || *cp == ';' || *cp == '/' || *cp == '\0' || *cp == '\n') {
			// skip comments and blank lines
			continue;
		} else if (*cp == '[') {
			cat = process_category(cfg, cp);
			if (!cat) {
				goto out_err_line;
			}
		} else if (cat) {
			if (process_line(cat, cp) < 0) {
				goto out_err_line;
			}
		} else {
			debug(0, "No category\n");
			goto out_err_line;
		}
	}
	fclose(fp);
	return cfg;
out_err_line:
	debug(0, "Error at line %d:[%s]\n", lineno, cp);
out_err:
	fclose(fp);
	if (cfg)
		navit_cfg_free(cfg);
	return NULL;
}

struct cfg_category *navit_cfg_find_cat(struct navit_cfg *cfg, char *name)
{
	struct cfg_category *c = cfg->cats;
	while (c) {
		if (!strcasecmp(c->name, name))
			return c;
		c = c->next;
	}
	return NULL;
}

struct cfg_category *navit_cfg_cats_walk(struct navit_cfg *cfg, struct cfg_category *c)
{
	if (!c)
		return cfg->cats;
	else
		return c->next;
}

struct cfg_varval *navit_cat_find_var(struct cfg_category *cat, char *name)
{
	struct cfg_varval *v = cat->vars;

	while (v) {
		if (!strcmp(v->name, name))
			return v;
		v = v->next;
	}
	return NULL;
}

struct cfg_varval *navit_cat_vars_walk(struct cfg_category *cat, struct cfg_varval *v)
{
	if (!v)
		return cat->vars;
	else
		return v->next;
}

int cfg_var_true(struct cfg_varval *v)
{
	if (!strcasecmp(v->value, "yes") || !strcasecmp(v->value, "enabled")
		|| !strcasecmp(v->value, "true") || !strcasecmp(v->value, "1"))
			return 1;
	return 0;
}

int cfg_var_intvalue(struct cfg_varval *v)
{
	return strtol(v->value,NULL,0);
}

char *cfg_var_value(struct cfg_varval *v)
{
	return v->value;
}

char *cfg_var_name(struct cfg_varval *v)
{
	return v->name;
}

int navit_cfg_save(struct navit_cfg *cfg, char *file)
{
	return -1;
}

char *cfg_cat_name(struct cfg_category *cat)
{
	return cat->name;
}

void navit_cfg_attrs_free(struct attr **attrs)
{
	int i=0;
	while(attrs[i]) {
		attr_free(attrs[i]);
		i++;
	}
	free(attrs);
}

struct attr **navit_cfg_cat2attrs(struct cfg_category *cat)
{
	int c = 0;
	struct cfg_varval *v;
	struct attr **ret;
	v = cat->vars;
	while(v) {
		c++;
		v = v->next;
	}
	if (!c)
		return NULL;
	ret = calloc(c+1, sizeof(struct attr *));
	if (!ret)
		return NULL;
	c = 0;
	v = cat->vars;
	while(v) {
		ret[c] = attr_new_from_text(v->name, v->value);
		if (ret[c])
			c++;
		v = v->next;
	}
	return ret;
}
