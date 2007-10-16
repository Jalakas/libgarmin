#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "plugin.h"
#include "data.h"
#include "projection.h"
#include "map.h"
#include "maptype.h"
#include "item.h"
#include "attr.h"
#include "coord.h"
#include "transform.h"
#include <stdio.h>
#include "attr.h"
#include "coord.h"
#include <libgarmin.h>
#include "garmin.h"
#include "gar2navit.h"


static int map_id;

struct map_priv {
	int id;
	char *filename;
	struct gar2nav_conv *conv;
	struct gar *g;
};

struct map_rect_priv {
	int id;
	struct coord_rect r;
	char *label;		// FIXME: Register all strings for searches
	int limit;
	struct map_priv *mpriv;
	struct gmap *gmap;
	struct gobject *cobj;
	struct gobject *objs;
	struct item item;
	unsigned int last_coord;
	void *last_itterated;
	struct coord last_c;
	void *last_oattr;
	unsigned int last_attr;
};

int garmin_debug = 10;
void 
logfn(char *file, int line, int level, char *fmt, ...)
{
	va_list ap;
	if (level > garmin_debug)
		return;
	va_start(ap, fmt);
	fprintf(stdout, "%s:%d:%d|", file, line, level);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}
// need a base map and a map
struct gscale {
	char *label;
	float scale;
	int bits;
};

static struct gscale mapscales[] = {
	{"7000 km", 70000.0, 8}
	,{"5000 km", 50000.0, 8}
	,{"3000 km", 30000.0, 9}
	,{"2000 km", 20000.0, 9}
	,{"1500 km", 15000.0, 10}
	,{"1000 km", 10000.0, 10}
	,{"700 km", 7000.0, 11}
	,{"500 km", 5000.0, 11}
	,{"300 km", 3000.0, 13}
	,{"200 km", 2000.0, 13}
	,{"150 km", 1500.0, 13}
	,{"100 km", 1000.0, 14}
	,{"70 km", 700.0, 15}
	,{"50 km", 500.0, 16}
	,{"30 km", 300.0, 16}
	,{"20 km", 200.0, 17}
	,{"15 km", 150.0, 17}
	,{"10 km", 100.0, 18}
	,{"7 km", 70.0, 18}
	,{"5 km", 50.0, 19}
	,{"3 km", 30.0, 19}
	,{"2 km", 20.0, 20}
	,{"1.5 km", 15.0, 22}
	,{"1 km", 10.0, 24}
	,{"700 m", 7.0, 24}
	,{"500 m", 5.0, 24}
	,{"300 m", 3.0, 24}
	,{"200 m", 2.0, 24}
	,{"150 m", 1.5, 24}
	,{"100 m", 1.0, 24}
	,{"70 m", 0.7, 24}
	,{"50 m", 0.5, 24}
	,{"30 m", 0.3, 24}
	,{"20 m", 0.2, 24}
	,{"15 m", 0.1, 24}
	,{"10 m", 0.15, 24}
};

static int 
garmin_object_label(struct gobject *o, struct attr *attr)
{
	struct map_rect_priv *mr = o->priv_data;
	if (!mr) {
		dlog(1, "Error object do not have priv_data!!\n");
		return 0;
	}
	mr->label = gar_get_object_lbl(o);
#warning FIXME Process label and give only the visible part
	if (mr->label) {
	//	dlog(1, "Label[%d]:%s\n", o->type, mr->label);
		attr->u.str = mr->label;
		return 1;
	}
	return 0;
}


static struct map_search_priv *gmap_search_new(struct map_priv *map, struct item *item, struct attr *search, int partial)
{
	return NULL;
}

static void gmap_search_destroy(struct map_search_priv *ms)
{
}

static struct item *gmap_search_get_item(struct map_search_priv *ms)
{
	return NULL;
}


/* Assumes that only one item will be itterated at time! */
static void 
coord_rewind(void *priv_data)
{
	struct gobject *g = priv_data;
	struct map_rect_priv *mr = g->priv_data;
	mr->last_coord = 0;
};

static void 
attr_rewind(void *priv_data)
{
	struct gobject *g = priv_data;
	struct map_rect_priv *mr = g->priv_data;
	mr->last_attr = 0;
};

static int 
point_coord_get(void *priv_data, struct coord *c, int count)
{
	struct gobject *g = priv_data;
	struct map_rect_priv *mr = g->priv_data;
	struct gcoord gc;
	if (!count)
		return 0;
	if (g != mr->last_itterated) {
		mr->last_itterated = g;
		mr->last_coord = 0;
	}

	if (mr->last_coord > 0)
		return 0;

	gar_get_object_coord(mr->gmap, g, &gc);
	c->x = gc.x;
	c->y = gc.y;
	mr->last_coord++;
//	dlog(1,"point: x=%d y=%d\n", c->x, c->y);
	// dlog(1, "point: x=%f y=%f\n", GARDEG(c->x), GARDEG(c->y));
	return 1;
}

static int 
poly_coord_get(void *priv_data, struct coord *c, int count)
{
	struct gobject *g = priv_data;
	struct map_rect_priv *mr = g->priv_data;
	int ndeltas = 0, total = 0;
	struct gcoord dc;

	if (!count)
		return 0;

	if (g != mr->last_itterated) {
		mr->last_itterated = g;
		mr->last_coord = 0;
	}
	ndeltas = gar_get_object_deltas(g);
	if (mr->last_coord > ndeltas + 1)
		return 0;
	while (count --) {
		if (mr->last_coord == 0) {
			gar_get_object_coord(mr->gmap, g, &dc);
			mr->last_c.x = dc.x;
			mr->last_c.y = dc.y;
		} else {
			if (!gar_get_object_dcoord(mr->gmap, g, mr->last_coord - 1, &dc)) {
				mr->last_coord = ndeltas + 2;
				return total;
			}
			mr->last_c.x += dc.x;
			mr->last_c.y += dc.y;
		}
		c->x = mr->last_c.x;
		c->y = mr->last_c.y;
		ddlog(1, "poly: x=%f y=%f\n", GARDEG(c->x), GARDEG(c->y));
//		dlog(1,"poly: x=%d y=%d\n", c->x, c->y);
		c++;
		total++;
		total++;
		mr->last_coord ++;
	}
	return total;
}

// for _any we must return one by one must extend gobject - bad
static int 
point_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct gobject *g = priv_data;
	struct map_rect_priv *mr = g->priv_data;
	switch (attr_type) {
	case attr_any:
			if (g != mr->last_oattr) {
				mr->last_oattr = g;
				mr->last_attr = 0;
			}
			if (mr->last_attr > 0)
				return 0;
			mr->last_attr++;
	case attr_label:
		attr->type = attr_label;
		return garmin_object_label(g, attr);
	case attr_limit:
		return 0;
	default:
		dlog(1, "Dont know about attribute %d[%04X] yet\n", attr_type,attr_type);
	}

	return 0;
}

static struct item_methods methods_garmin_point = {
	coord_rewind,
	point_coord_get,
	attr_rewind,
	point_attr_get,
};

static struct item_methods methods_garmin_poly = {
	coord_rewind,
	poly_coord_get,
	attr_rewind,	// point_attr_rewind,
	point_attr_get,	// poly_attr_get,
};

static struct item *
garmin_poi2item(struct map_rect_priv *mr, struct gobject *o, unsigned char otype)
{
	int subtype;

	subtype = gar_object_subtype(o);
	mr->item.id_hi = otype << 8 | subtype;
	if (mr->mpriv->conv)
		mr->item.type = g2n_get_type(mr->mpriv->conv, G2N_POINT, mr->item.id_hi);
	mr->item.meth = &methods_garmin_point;
	return &mr->item;
}

static struct item *
garmin_pl2item(struct map_rect_priv *mr, struct gobject *o, unsigned char otype)
{
	mr->item.id_hi = otype;
	if (mr->mpriv->conv)
		mr->item.type = g2n_get_type(mr->mpriv->conv, G2N_POLYLINE, otype);
	mr->item.meth = &methods_garmin_poly;
	return &mr->item;
}

static struct item *
garmin_pg2item(struct map_rect_priv *mr, struct gobject *o, unsigned char otype)
{
	mr->item.id_hi = otype;
	if (mr->mpriv->conv)
		mr->item.type = g2n_get_type(mr->mpriv->conv, G2N_POLYGONE, otype);
	mr->item.meth = &methods_garmin_poly;
	return &mr->item;
}

static struct item *
garmin_obj2item(struct map_rect_priv *mr, struct gobject *o)
{
	unsigned char otype;
	otype = gar_obj_type(o);
	mr->item.type = type_none;
	switch (o->type) {
		case GO_POINT:
		case GO_POI:
			return garmin_poi2item(mr, o, otype);
		case GO_POLYLINE:
			return garmin_pl2item(mr, o, otype);
		case GO_POLYGON:
			return garmin_pg2item(mr, o, otype);
		default:
			dlog(1, "Unknown garmin object type:%d\n",
				o->type);
	}
	return NULL;
}

static struct item *gmap_rect_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	struct gobject *o;
	dlog(1, "id_hi=%04X id_lo=%04X\n", id_hi, id_lo);
	if (!mr->objs) {
		dlog(1, "Error no objects\n");
		return NULL;
	}
	o = mr->objs;
	while (o) {
		if (id_lo == (int)o->gptr) {
			mr->item.id_hi = (int)mr;
			mr->item.id_lo = (int)o->gptr;
			mr->item.priv_data = o;
			mr->item.type = type_none;
			o->priv_data = mr;
			return garmin_obj2item(mr, o);
		}
		o = o->next;
	}
	dlog(1, "Can not find object\n");
	return NULL;
}

static struct item *
gmap_rect_get_item(struct map_rect_priv *mr)
{
	struct gobject *o;
	if (!mr->objs)
		return NULL;
	if (!mr->cobj)
		return NULL;
		// mr->cobj = mr->objs;
	o = mr->cobj;
//	dlog(1, "gi:o=%p\n", o);
	mr->cobj = mr->cobj->next;
	if (o) {
		mr->item.id_hi = (int)mr;
		mr->item.id_lo = (int)o->gptr;
		mr->item.priv_data = o;
		mr->item.type = type_none;
		o->priv_data = mr;
		if (!garmin_obj2item(mr, o))
			return NULL;
		return &mr->item;
	}
	return NULL;
}

#define max(a,b) ((a) > (b) ? (a) : (b))

static int 
get_level(struct map_selection *sel)
{
	int l;
	l = max(sel->order[layer_town], sel->order[layer_street]);
	l = max(l, sel->order[layer_poly]);
	return l;
}

static int
garmin_get_selection(struct map_rect_priv *map, struct map_selection *sel)
{
	struct gar_rect r;
	struct gmap *gm;
	struct gobject **glast = NULL;
	int rc;
	int level = 15;	/* max level for maps, overview maps can have bigger
			   levels we do not deal w/ them
			*/
	int flags = 0;
	if (sel && sel->order[layer_town] == 0 && sel->order[layer_poly] == 0
		&& sel->order[layer_street]) {
		// FIXME: use max zoom level
		flags = GO_GET_ROUTABLE;
		level = 18;
	} else if (sel)
		flags = GO_GET_SORTED;

	if (sel) {
		r.lulat = sel->rect.lu.y;
		r.lulong = sel->rect.lu.x;
		r.rllat = sel->rect.rl.y;
		r.rllong = sel->rect.rl.x;
		level = get_level(sel);
		printf("Looking level=%d for %f %f %f %f\n",
			level, r.lulat, r.lulong, r.rllat, r.rllong);
	}
	gm = gar_find_subfiles(map->mpriv->g, sel ? &r : NULL);
	if (!gm) {
		dlog(1, "Can not find map data\n");
		return -1;
	} 
	map->gmap = gm;
	glast = &map->objs;
	while (*glast) {
		if ((*glast)->next) {
			*glast = (*glast)->next;
		} else
			break;
	}
	rc = gar_get_objects(gm, level, sel ? &r : NULL, glast, flags);
	if (rc < 0) {
		dlog(1, "Error loading objects\n");
		return -1;
	}
	map->cobj = map->objs;
	dlog(1, "Loaded %d objects\n", rc);
	return rc;
}
// Can not return NULL, navit segfaults
static struct map_rect_priv *
gmap_rect_new(struct map_priv *map, struct map_selection *sel)
{
	struct map_selection *ms = sel;
	struct map_rect_priv *mr;

	mr = calloc(1, sizeof(*mr));
	if (!mr)
		return mr;
	mr->mpriv = map;
	if (!sel) {
		garmin_get_selection(mr, NULL);
	} else {
		while (ms) {
			dlog(1, "order town:%d street=%d poly=%d\n",
				sel->order[layer_town],
				sel->order[layer_street],
				sel->order[layer_poly]);
			if (garmin_get_selection(mr, ms) < 0) {
			//	free(mr);
			//	return NULL;
			}
			ms = ms->next;
		}
	}
	return mr;
}

static void
gmap_rect_destroy(struct map_rect_priv *mr)
{
	dlog(1,"destroy maprect\n");
	if (mr->gmap)
		gar_free_gmap(mr->gmap);
	if (mr->objs)
		gar_free_objects(mr->objs);
	if (mr->label)
		free(mr->label);
	free(mr);
}

static void
gmap_destroy(struct map_priv *m)
{
	dlog(1, "garmin_map_destroy\n");
	if (m->g)
		gar_free(m->g);
	if (m->filename)
		free(m->filename);
	free(m);
}

static struct map_methods map_methods = {
	projection_garmin,
	NULL,
//	"iso8859-1",	// update from the map
	gmap_destroy,	//
	gmap_rect_new,
	gmap_rect_destroy,
	gmap_rect_get_item,
	gmap_rect_get_item_byid,
	gmap_search_new,
	gmap_search_destroy,
	gmap_search_get_item,
};

static struct map_priv *
gmap_new(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *m;
	struct attr *data;
	char buf[PATH_MAX];

	data=attr_search(attrs, NULL, attr_data);
	if (! data)
		return NULL;
	m=g_new(struct map_priv, 1);
	m->id=++map_id;
	m->filename = strdup(data->u.str);
	if (!m->filename) {
		g_free(m);
		return NULL;
	}
	m->g = gar_init(NULL, logfn);
	if (!m->g) {
		g_free(m->filename);
		g_free(m);
		return NULL;
	}
	// we want the data now, later we can load only what's necessery
	if (gar_img_load(m->g, m->filename, 1) < 0) {
		gar_free(m->g);
		g_free(m->filename);
		g_free(m);
		return NULL;
	}
	snprintf(buf, sizeof(buf), "%s.types", m->filename);
	dlog(1, "Looking for types in %s\n", buf); 
	m->conv = g2n_conv_load(buf);
	if (!m->conv) {
		char *cp;
		strcpy(buf, m->filename);
		cp = strrchr(buf ,'/');
		if (cp) {
			cp ++;
			*cp = '\0';
			strcat(buf, "garmintypes.txt");
			dlog(1, "Looking for types in %s\n", buf);
			m->conv = g2n_conv_load(buf);
		}
	}
	if (!m->conv) {
		dlog(1, "Failed to load map types\n");
	}
	*meth=map_methods;
	return m;
}

void
plugin_init(void)
{
	plugin_register_map_type("garmin", gmap_new);
}
