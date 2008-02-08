#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "config.h"
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "log.h"
#include "callback.h"
#include "plugin.h"
#include "list.h"
#include "vehicle.h"
#include "color.h"

static list_head(lvehicles);

struct vehicle {
	list_t l;
	unsigned int id;
	struct vehicle_priv *priv;
	struct vehicle_methods meth;
	struct callback_list *cbl;
	// move to modules
	struct log *nmea_log, *gpx_log;
	char *name;
	int active;
	int update;
	int update_curr;
	int follow;
	int follow_curr;
	struct coord coord;
	int dir;
	int speed;
	struct color c;
	struct menu *menu;
	struct cursor *cursor;
	struct attr callback;
};

static void
vehicle_log_nmea(struct vehicle *this_, struct log *log)
{
}

static void
vehicle_log_gpx(struct vehicle *this_, struct log *log)
{
	struct attr pos_attr;
	char buffer[256];
	if (!this_->meth.position_attr_get)
		return;
	if (!this_->meth.position_attr_get(this_->priv, attr_position_coord_geo, &pos_attr))
		return;
	sprintf(buffer,"<trkpt lat=\"%f\" lon=\"%f\" />\n", pos_attr.u.coord_geo->lat, pos_attr.u.coord_geo->lng);
	log_write(log, buffer, strlen(buffer));
}


static void
vehicle_log_textfile(struct vehicle *this_, struct log *log)
{
	struct attr pos_attr;
	char buffer[256];
	if (!this_->meth.position_attr_get)
		return;
	if (!this_->meth.position_attr_get(this_->priv, attr_position_coord_geo, &pos_attr))
		return;
	sprintf(buffer,"%f %f type=trackpoint\n", pos_attr.u.coord_geo->lng, pos_attr.u.coord_geo->lat);
	log_write(log, buffer, strlen(buffer));
}

static int
vehicle_add_log(struct vehicle *this_, struct log *log,
		struct attr **attrs)
{
	struct attr *type;
	struct callback *cb;
	type = attr_search(attrs, NULL, attr_type);
	if (!type)
		return 1;
	if (!strcmp(type->u.str, "nmea")) {
		cb=callback_new_2(callback_cast(vehicle_log_nmea), this_, log);
	} else if (!strcmp(type->u.str, "gpx")) {
		char *header =
		    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<gpx version=\"1.0\" creator=\"Navit http://navit.sourceforge.net\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.topografix.com/GPX/1/0\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n<trk>\n<trkseg>\n";
		char *trailer = "</trkseg>\n</trk>\n</gpx>\n";
		log_set_header(log, header, strlen(header));
		log_set_trailer(log, trailer, strlen(trailer));
		cb=callback_new_2(callback_cast(vehicle_log_gpx), this_, log);
	} else if (!strcmp(type->u.str, "textfile")) {
		char *header = "type=track\n";
		log_set_header(log, header, strlen(header));
		cb=callback_new_2(callback_cast(vehicle_log_textfile), this_, log);
	} else
		return 1;
	callback_list_add(this_->cbl, cb);
	return 0;
}

struct vehicle *
vehicle_new(struct attr **attrs)
{
	struct vehicle *v;
	struct attr *name,*update,*follow,*color,*active,*source, *id;
	struct vehicle_priv *(*vehicletype_new) (struct vehicle_methods *
						 meth,
						 struct callback_list *
						 cbl,
						 struct attr ** attrs);
	char *type, *colon;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	if (!source) {
		dbg(0, "no source\n");
		return NULL;
	}

	type = g_strdup(source->u.str);
	colon = index(type, ':');
	if (colon)
		*colon = '\0';
	dbg(1, "source='%s' type='%s'\n", source->u.str, type);

	vehicletype_new = plugin_get_vehicle_type(type);
	if (!vehicletype_new) {
		dbg(0, "invalid type\n");
		return NULL;
	}
	v = g_new0(struct vehicle, 1);
	v->cbl = callback_list_new();
	v->priv = vehicletype_new(&v->meth, v->cbl, attrs);
	if (!v->priv) {
		dbg(0, "vehicletype_new failed\n");
		callback_list_destroy(v->cbl);
		g_free(v);
		return NULL;
	}
	v->update = 1;
	v->follow = 0;
	v->active = 0;
	if ((name=attr_search(attrs, NULL, attr_name)))
		v->name=g_strdup(name->u.str);
	else
		v->name=g_strdup("Noname");

	if ((id=attr_search(attrs, NULL, attr_vehicle_id)))
		v->id=id->u.num;
	if ((update=attr_search(attrs, NULL, attr_update)))
		v->update=update->u.num;
	if ((follow=attr_search(attrs, NULL, attr_follow)))
		v->follow=follow->u.num;
	if ((color=attr_search(attrs, NULL, attr_color))) 
		v->c=*(color->u.color);
	if ((active=attr_search(attrs, NULL, attr_active)) && active->u.num) {
		v->active = 1;
//		navit_set_vehicle(this_, v);
	}

	dbg(1, "leave\n");

	return v;
}

int
vehicle_position_attr_get(struct vehicle *this_, enum attr_type type,
			  struct attr *attr)
{
	if (this_->meth.position_attr_get)
		return this_->meth.position_attr_get(this_->priv, type,
						     attr);
	return 0;
}

int
vehicle_set_attr(struct vehicle *this_, struct attr *attr,
		 struct attr **attrs)
{
	if (this_->meth.set_attr)
		return this_->meth.set_attr(this_->priv, attr, attrs);
	return 0;
}

int
vehicle_add_attr(struct vehicle *this_, struct attr *attr,
		 struct attr **attrs)
{
	switch (attr->type) {
	case attr_callback:
		callback_list_add(this_->cbl, attr->u.callback);
		break;
	case attr_log:
		return vehicle_add_log(this_, attr->u.log, attrs);
	default:
		return 0;
	}
	return 1;
}

int
vehicle_remove_attr(struct vehicle *this_, struct attr *attr)
{
	switch (attr->type) {
	case attr_callback:
		callback_list_remove(this_->cbl, attr->u.callback);
		break;
	default:
		return 0;
	}
	return 1;
}

void
vehicle_destroy(struct vehicle *this_)
{
	callback_list_destroy(this_->cbl);
	g_free(this_);
}
