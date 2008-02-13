#include <stdlib.h>
#include <limits.h>
#include <dlfcn.h>
#include <string.h>
#include "debug.h"
#include "globals.h"
#include "cfg.h"
#include "module.h"
#include "list.h"

#define MODULES_CFG "modules.conf"
#define MF_ACTIVE	(1<<0)
#define MF_LAZY		(1<<1)
#define MF_GLOBAL	(1<<2)
#define MF_ONDEMAND	(1<<3)

struct module {
	list_t l;
	char *alias;
	char *name;
	int flags;
	void *handle;
	struct navit_module *mod;
};

static int autoload;
static list_head(lmodules);

static struct module *navit_module_alloc(char *name, int active)
{
	struct module *mod;
	mod = calloc(1, sizeof(*mod));
	if (!mod)
		return mod;
	mod->name = strdup(name);
	if (!mod->name) {
		free(mod);
		return NULL;
	}
	if (active)
		mod->flags |= MF_ACTIVE;
	return mod;
}

static void set_module_flag(char *name, int flag)
{
	struct module *m;

	list_for_entry(m, &lmodules, l) {
		if (!strcmp(m->name, name)) {
			m->flags |= flag;
			return;
		}
	}
}

static int navit_load_module(struct module *m)
{
	char path[PATH_MAX];
	int flags;
	void *handle;
	if (m->flags & MF_GLOBAL)
		flags = RTLD_GLOBAL;
	else
		flags = RTLD_LOCAL;
	flags |= RTLD_NOW;
	debug(5, "Loading: %s\n", m->name);
	if (fromsrcdir)
		sprintf(path, "%s/modules/.libs/%s", navit_libdir, m->name);
	else
		sprintf(path, "%s/modules/%s", navit_libdir, m->name);
	handle = dlopen(path, flags);
	if (!handle) {
		debug(0, "Error loading module: %s(%s)\n", m->name,dlerror());
		m->flags &= ~MF_ACTIVE;
		return -1;
	}
	m->handle = handle;
	m->mod = dlsym(handle, "navit_module");
	return 0;
}

int navit_request_module(const char *name)
{
	struct module *m;
	list_for_entry(m, &lmodules, l) {
		if (!strcmp(m->name, name) || (m->alias && !strcmp(m->alias,name)))
			return navit_load_module(m);
	}
	return -1;
}

static void navit_modules_load(void)
{
	struct module *m;


	list_for_entry(m, &lmodules, l) {
		if (!(m->flags & MF_ACTIVE) || m->flags&MF_ONDEMAND)
			continue;
		navit_load_module(m);
	}
}

static void navit_modules_start(void)
{
	struct module *m;

	list_for_entry(m, &lmodules, l) {
		if (!(m->flags&MF_ACTIVE) || m->flags&MF_ONDEMAND)
			continue;
		if (m->mod->module_load) {
			m->mod->module_load();
		} else
			debug(0, "Warning no module_load function!\n");
	}
}

void navit_modules_init(void)
{
	struct navit_cfg *cfg;
	struct module *mod;
	struct cfg_varval *var = NULL;
	struct cfg_category *cat = NULL;
	int active;

	debug(0, "Loading modules ...\n");
	cfg = navit_cfg_load(MODULES_CFG);
	if (!cfg) {
		debug(0, "No modules configured\n");
		return;
	}
	cat = navit_cfg_find_cat(cfg, "general");
	if (cat) {
		var = navit_cat_find_var(cat, "autoload");
		if (var)
			autoload = cfg_var_true(var);
	}
	cat = navit_cfg_find_cat(cfg, "modules");
	if (!cat) {
		debug(0, "No modules defined\n");
		goto out;
	}
	var = NULL;
	while ((var = navit_cat_vars_walk(cat, var))!=NULL) {
		if (!strcasecmp(cfg_var_name(var), "load"))
			active = 1;
		else
			active = 0;
		mod = navit_module_alloc(cfg_var_value(var), active);
		if (mod)
			list_append(&mod->l, &lmodules);
	}
	cat = navit_cfg_find_cat(cfg, "ondemand");
	if (cat) {
		var = NULL;
		while ((var = navit_cat_vars_walk(cat, var))!=NULL) {
			mod = navit_module_alloc(cfg_var_value(var), 1);
			if (mod) {
				mod->alias = strdup(cfg_var_name(var));
				mod->flags |= MF_ONDEMAND;
				list_append(&mod->l, &lmodules);
			}
		}
	}

	cat = navit_cfg_find_cat(cfg, "global");
	if (cat) {
		var = NULL;
		while ((var = navit_cat_vars_walk(cat, var))!=NULL) {
			set_module_flag(cfg_var_value(var), MF_GLOBAL);
		}
	}
	navit_modules_load();
	navit_modules_start();
out:
	navit_cfg_free(cfg);
}
