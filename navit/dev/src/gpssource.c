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
	list_append(&source->l, &lsources);
	return 1;
}

void gps_unregister_source(struct gps_source *source)
{
	list_remove(&source->l);
}

void gps_source_data(struct gps_source *src, struct gps_data *data)
{
	if (src->flags & GS_ACTIVE) {
		notify(NOTIFY_ACTIVEGPS, data);
	} else {
		notify(NOTIFY_GPS, data);
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
