#include <string.h>
#include <stdio.h>
#include "log.h"

struct tf_log {
	struct log *log;
	int id;
	FILE *fp;
};

static const char *tfheader = "type=track\n";

//sprintf(buffer,"%f %f type=trackpoint\n", pos_attr.u.coord_geo->lng, pos_attr.u.coord_geo->lat);

#define TFLOG_CFG	"logger.conf"

static struct attr logattrs[] = {
	{ .type = attr_data,		.flags = 0, .u.str = NULL,	},
	{ .type = attr_overwrite,	.flags = 0, .u.num = 0,		},
	{ .type = attr_flush_size,	.flags = 0, .u.num = 0,		},
	{ .type = attr_flush_time,	.flags = 0, .u.num = 0,		},
	NULL,
};

static void set_attr(struct attr *attr, enum attr_type type, void *data)
{
	int i = 0;
	while(attr[i]) {
		if (attr[i]->type == type) {
			attr_data_set(attr[i], data);
			return;
		}
		i++;
	}
}

static int load_config()
{
	struct navit_cfg *cfg;
	struct cfg_varval *var, *file;
	struct cfg_category *cat = NULL;
	struct log *log;
	int id;
	int overwrite, flush_size, flush_time;

	cfg = navit_cfg_load(TFLOG_CFG);
	if (!cfg)
		return -1;
	while ((cat = navit_cfg_cats_walk(cfg, cat))) {
		var = navit_cat_find_var(cat, "type");
		if (var) {
			if (!strcmp(cfg_var_value(var), "textfile")) {
				file = navit_cat_find_var(cat, "file");
				if (file) {
					
					log_set_header(log, tfheader, strlen(tfheader));
				}
			}
		}
	}


}

static int tflog_load(void)
{
	ENTER();
	return M_OK;
}

static int tflog_reconfigure(void)
{
	ENTER();
	return M_OK;
}

static int tflog_unload(void)
{
	ENTER();
	return M_OK;
}

NAVIT_MODULE(tflog, test_load,test_reconfigure,test_unload);
