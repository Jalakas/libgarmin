#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "cfg.h"
#include "debug.h"
#include "module.h"
#include "list.h"
#include "notify.h"
#include "gpssource.h"

static list_head(ltflogs);

#define TFL_TEXTFILE	1
#define TFL_GPX		2
#define TFL_NMEA	3

struct tf_log {
	list_t	l;
	struct log *log;
	char *name;
	int type;
	int sourceid;
};

static const char *tfheader = "type=track\n";
static const char *gpxheader = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<gpx version=\"1.0\" creator=\"Navit http://navit.sourceforge.net\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n<trk>\n<trkseg>\n";
static const char *gpxtrailer = "</trkseg>\n</trk>\n</gpx>\n";

static int tf_gps_data(unsigned int group, int mask, void *priv, void *data)
{
	struct tf_log *tfl = priv;
	struct gps_data *gps = data;
	char buf[256];
	int rc = 0;

	if (tfl->type == TFL_NMEA) {
		rc = sprintf(buf, "%s\n", (char *)data);
	} else {
		if (gps->fix < 2)
			return 1;
		if (tfl->type == TFL_TEXTFILE) {
			rc = sprintf(buf,"%f %f type=trackpoint\n", 
				gps->geo.lat, gps->geo.lng);
		} else if (tfl->type == TFL_GPX) {
			rc = sprintf(buf,"<trkpt lat=\"%f\" lon=\"%f\" />\n", 
				gps->geo.lat, gps->geo.lng);
		}
	}
	if (rc>0)
		log_write(tfl->log, buf, rc);
	return 1;
}


static int tf_add_log(struct log *log, char *name, int id, int type)
{
	struct tf_log *tfl;
	tfl = calloc(1, sizeof(tfl));
	if (!tfl) {
		return -1;
	}
	tfl->name = strdup(name);
	if (!tfl->name) {
		free(tfl);
		return -1;
	}
	tfl->log = log;
	tfl->sourceid = id;
	tfl->type = type;
	debug(5, "Added %s log\n", name);
	list_append(&tfl->l, &ltflogs);
	if (type != TFL_NMEA)
		listen_for(NOTIFY_GPS, GPS_DATA, tf_gps_data, tfl);
	else
		listen_for(NOTIFY_GPS, GPS_NMEA, tf_gps_data, tfl);
	return 1;
}

//sprintf(buffer,"%f %f type=trackpoint\n", pos_attr.u.coord_geo->lng, pos_attr.u.coord_geo->lat);

#define TFLOG_CFG	"logger.conf"

static int load_config(int reload)
{
	struct navit_cfg *cfg;
	struct cfg_varval *var;
	struct cfg_category *cat = NULL;
	struct log *log;
	int id, type;
	struct attr **attrs;

	cfg = navit_cfg_load(TFLOG_CFG);
	if (!cfg)
		return -1;
	while ((cat = navit_cfg_cats_walk(cfg, cat))) {
		log = NULL;
		var = navit_cat_find_var(cat, "source");
		id = gps_get_source_id(cfg_var_value(var));
		if (id == -1)
			id = cfg_var_intvalue(var);
		if (!id) {
			debug(0, "ERROR: Can not find source:[%s]\n",
				cfg_var_value(var));
			continue;
		}
		var = navit_cat_find_var(cat, "type");
		if (var) {
			if (!strcmp(cfg_var_value(var), "textfile")) {
				type = TFL_TEXTFILE;
				attrs = navit_cfg_cat2attrs(cat);
				if (attrs) {
					log = log_new(attrs);
					navit_cfg_attrs_free(attrs);
					if (log) {
						log_set_header(log, tfheader, strlen(tfheader));
					}
				}
			} else if (!strcmp(cfg_var_value(var), "gpx")) {
				type = TFL_GPX;
				attrs = navit_cfg_cat2attrs(cat);
				if (attrs) {
					log = log_new(attrs);
					navit_cfg_attrs_free(attrs);
					if (log) {
						log_set_header(log, gpxheader, strlen(gpxheader));
						log_set_trailer(log, gpxtrailer, strlen(gpxtrailer));
					}
				}
			} else if (!strcmp(cfg_var_value(var), "nmea")) {
				type = TFL_NMEA;
				attrs = navit_cfg_cat2attrs(cat);
				if (attrs) {
					log = log_new(attrs);
					navit_cfg_attrs_free(attrs);
				}
			} else {
				debug(0, "ERROR: Unknown log type:[%s]\n",
						cfg_var_value(var));
					continue;
			}
			if (log) {
				if (tf_add_log(log, cfg_cat_name(cat), id, type) < 0) {
					log_destroy(log);
					debug(0, "ERROR: Adding log %s\n",
						cfg_cat_name(cat));
				}
			}
		}
	}
	navit_cfg_free(cfg);
	return 1;
}

static int tflog_load(void)
{
	ENTER();
	load_config(0);
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

NAVIT_MODULE(tflog, tflog_load,tflog_reconfigure,tflog_unload);
