#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "list.h"
#include "cfg.h"
#include "notify.h"
#include "gpssource.h"


static list_head(lsources);
static int source_id;

#define GS_ACTIVE	(1<<0)


struct gps_source *gps_source_alloc(char *name)
{
	struct gps_source *s;
	s = calloc(1, sizeof(*s));
	s->name = strdup(name);
	if (!s->name) {
		free(s);
		return NULL;
	}
	return s;
}

void gps_source_free(struct gps_source *s)
{
	if (!list_empty(&s->l)) {
		debug(0, "ERROR: Freeing registered source");
	}
	free(s->name);
	free(s);
}

int gps_register_source(struct gps_source *source)
{
	if (!source->id)
		source->id = ++source_id;
	debug(10, "Registered source:id=%d [%s]\n",
		source->id, source->name);
	list_append(&source->l, &lsources);
	return 1;
}

void gps_unregister_source(struct gps_source *source)
{
	debug(10, "Unregistered source:id=%d [%s]\n",
		source->id, source->name);
	list_remove(&source->l);
}

void gps_source_data(struct gps_source *src, struct gps_data *data)
{
	debug(11, "Data from: [%d:%s]\n", src->id, src->name);
//extern struct navit *global_navit;
//	navit_draw(global_navit);
	if (src->flags & GS_ACTIVE) {
		notify(NOTIFY_GPS, GPS_ACTIVE, data);
	} else {
		notify(NOTIFY_GPS, GPS_DATA, data);
	}
}

int gps_get_source_id(char *name)
{
	struct gps_source *gs;

	list_for_entry(gs, &lsources, l) {
		if (!strcmp(gs->name, name))
			return gs->id;
	}
	return -1;
}

void gps_set_source_active(int id)
{
	struct gps_source *gs;
	list_for_entry(gs, &lsources, l) {
		gs->flags &= ~GS_ACTIVE;
		if (gs->id == id)
			gs->flags |= GS_ACTIVE;
	}
}
