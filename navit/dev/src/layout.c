#include <glib.h>
#include <string.h>
#include <stdio.h>
#include "layout.h"
#include "cfg.h"
#include "debug.h"
#include "color.h"
#include "callback.h"
#include "menu.h"
#include "navit.h"

static list_head(llayouts);
static list_head(llayers);

#define LAYOUT_CFG	"layout.conf"
#define LAYERS_CFG	"layers.conf"

struct layout * layout_new(const char *name)
{
	struct layout *l;

	l = g_new0(struct layout, 1);
	if (l) {
		l->name = g_strdup(name);
		list_append(&l->l, &llayouts);
	}
	// setup default colors
	l->bgcolor = (struct color) { 0xffff, 0xefef, 0xb7b7, 0xffff };
	l->textbg = (struct color) { 0x0000, 0x0000, 0x0000 };
	l->textfg = (struct color) { 0xffff, 0xffff, 0xffff };
	return l;
}

struct layout *layout_find(const char *name)
{
	struct layout *l;
	list_for_entry(l, &llayouts, l) {
		if (!strcmp(name, l->name))
			return l;
	}
	return NULL;
}

struct layer * layer_new(const char *name, int details)
{
	struct layer *l;

	l = g_new0(struct layer, 1);
	l->name = g_strdup(name);
	l->details = details;
	list_append(&l->l, &llayers);
	return l;
}

static struct layer *layer_find(const char *name)
{
	struct layer *l;
	list_for_entry(l, &llayers, l) {
		if (!strcmp(name, l->name))
			return l;
	}
	return NULL;
}

void layout_add_layer(struct layout *layout, struct layer *layer)
{
	layout->layers = g_list_append(layout->layers, layer);
}

struct itemtype * itemtype_new(int order_min, int order_max)
{
	struct itemtype *itm;

	itm = g_new0(struct itemtype, 1);
	itm->order_min=order_min;
	itm->order_max=order_max;
	return itm;
}

void itemtype_add_type(struct itemtype *this, enum item_type type)
{
	this->type = g_list_append(this->type, GINT_TO_POINTER(type));
}


void layer_add_itemtype(struct layer *layer, struct itemtype * itemtype)
{
	layer->itemtypes = g_list_append(layer->itemtypes, itemtype);

}

void itemtype_add_element(struct itemtype *itemtype, struct element *element)
{
	itemtype->elements = g_list_append(itemtype->elements, element);
}

struct element *
polygon_new(struct color *color)
{
	struct element *e;
	e = g_new0(struct element, 1);
	e->type=element_polygon;
	e->color=*color;

	return e;
}

static struct element *
new_polygon(char *colorname)
{
	struct element *e;
	e = g_new0(struct element, 1);
	e->type=element_polygon;
	e->colorname=strdup(colorname);

	return e;
}

struct element *
polyline_new(struct color *color, int width, int directed)
{
	struct element *e;
	
	e = g_new0(struct element, 1);
	e->type=element_polyline;
	e->color=*color;
	e->u.polyline.width=width;
	e->u.polyline.directed=directed;

	return e;
}

static struct element *
new_polyline(char *color)
{
	struct element *e;

	e = g_new0(struct element, 1);
	e->type=element_polyline;
	e->colorname = strdup(color);

	return e;
}

static void
set_width(struct element *e, int w)
{
	if (e->type == element_polyline)
		e->u.polyline.width = w;
	else if (e->type == element_circle)
		e->u.circle.width = w;
}

static void
set_directed(struct element *e, int d)
{
	if (e->type == element_polyline)
		e->u.polyline.directed=d;
}

struct element *
circle_new(struct color *color, int radius, int width, int label_size)
{
	struct element *e;

	e = g_new0(struct element, 1);
	e->type=element_circle;
	e->color=*color;
	e->label_size=label_size;
	e->u.circle.width=width;
	e->u.circle.radius=radius;

	return e;
}

static struct element *
new_circle(char *colorname)
{
	struct element *e;

	e = g_new0(struct element, 1);
	e->type=element_circle;
	e->colorname = strdup(colorname);

	return e;
}

static void
set_radius(struct element *e, int r)
{
	if (e->type == element_circle)
		e->u.circle.radius = r;
}

static void
set_labelsize(struct element *e, int s)
{
	e->label_size = s;
}

struct element *
label_new(int label_size)
{
	struct element *e;

	e = g_new0(struct element, 1);
	e->type=element_label;
	e->label_size=label_size;

	return e;
}

struct element *
icon_new(const char *src)
{
	struct element *e;

	e = g_malloc0(sizeof(*e)+strlen(src)+1);
	e->type=element_icon;
	e->u.icon.src=(char *)(e+1);
	strcpy(e->u.icon.src,src);

	return e;
}

struct element *
image_new(void)
{
	struct element *e;

	e = g_malloc0(sizeof(*e));
	e->type=element_image;

	return e;
}

static int
get_order(char *value, int *min, int *max)
{
	const char  *pos;
	int ret;

	*min = 0;
	*max = 18;
	pos = strchr(value, '-');
	if (!pos) {
		ret = sscanf(value,"%d",min);
		*max = *min;
	} else if (pos == value) {
		ret = sscanf(value,"-%d",max);
	} else {
		ret = sscanf(value,"%d-%d", min, max);
	}
	return ret;
}

static void layer_add_itemtypes(struct layer *l, struct itemtype *it, char *types)
{
	char *saveptr=NULL, *tok, *type_str, *str;
	enum item_type itype;

	type_str = strdup(types);
	str = type_str;
	layer_add_itemtype(l, it);
	while ((tok=strtok_r(str, ",", &saveptr))) {
		itype=item_from_name(tok);
		itemtype_add_type(it, itype);
		str=NULL;
	}
	g_free(type_str);

}

static int elements_activate(struct color_scheme *cs, struct itemtype *it)
{
	GList *es;
	struct element *e;
	struct color *c;

	es=it->elements;
	while (es) {
		e=es->data;
		if (e->type == element_label ||
			e->type == element_icon || 
			e->type == element_image) {
			es=g_list_next(es);
			continue;
		}
		
		if (!e->colorname) {
			debug(0, "Element have no color name! type=%d\n", e->type);
			es=g_list_next(es);
			continue;
		}
		if (*e->colorname != '#') {
			if (cs) {
				c = cs_lookup_color(cs, e->colorname);
				if (c) {
					e->color = *c;
				} else {
					debug(0, "WARNING! Can not find color [%s]\n",
						e->colorname);
				}
			} else {
				debug(0, "No color scheme defined for color:[%s]\n",
					e->colorname);
			}
		} else {
			parse_color(e->colorname, &e->color);
		}
		es=g_list_next(es);
	}
	return 1;
}

static int layer_activate(struct color_scheme *cs, struct layer *layer)
{
	GList *itms;
	struct itemtype *itm;

	itms=layer->itemtypes;
	while (itms) {
		itm = itms->data;
		elements_activate(cs, itm);
		itms=g_list_next(itms);
	}
	return 1;
}

int layout_activate(struct layout *layout)
{
	GList *lays;
	struct layer *lay;
	struct color_scheme *cs = NULL;

	if (layout->colorscheme)
		cs = cs_lookup(layout->colorscheme);
	lays=layout->layers;
	while (lays) {
		lay=lays->data;
		layer_activate(cs, lay);
		lays=g_list_next(lays);
	}
	return 1;
}


static int layers_init(void)
{
	struct navit_cfg *cfg;
	struct cfg_varval *var = NULL;
	struct cfg_category *cat = NULL;
	struct layer *layer;
	struct itemtype *it = NULL;
	struct element *e = NULL;
	char *type = NULL;
	char *order = NULL;
	int min, max;


	debug(0, "Layers initializing ...\n");

	cfg = navit_cfg_load(LAYERS_CFG);
	if (!cfg)
		return -1;
	while ((cat = navit_cfg_cats_walk(cfg, cat))) {
		layer = layer_new(cfg_cat_name(cat), 0);
		if (!layer)
			goto out_err;
		var = NULL;
		order = NULL;
		while ((var = navit_cat_vars_walk(cat, var))) {
			if (!strcmp(cfg_var_name(var), "order")) {
				order = cfg_var_value(var);
				if (!get_order(order, &min, &max)) {
					debug(0, "Invalid order:[%s]\n", order);
					goto out_err;
				}
#if 0
				it = itemtype_new(min, max);
				if (!it) {
					debug(0, "Can not create type\n");
					goto out_err;
				}
#endif
			} else if (!strcmp(cfg_var_name(var), "type")) {
				if (!order) {
					debug(0, "order is required before type\n");
					goto out_err;
				}
#if 1
				it = itemtype_new(min, max);
				if (!it) {
					debug(0, "Can not create type\n");
					goto out_err;
				}
#endif
				type = cfg_var_value(var);
				debug(10, "types=%s order=%d-%d\n",
					type, min, max);
				layer_add_itemtypes(layer, it, type);
				e = NULL;
			} else if (!strcmp(cfg_var_name(var), "polygon")) {
				if (!it) {
					debug(0, "type required before polygon\n");
					goto out_err;
				}
				e = new_polygon(cfg_var_value(var));
				if (e)
					itemtype_add_element(it, e);
			} else if (!strcmp(cfg_var_name(var), "polyline")) {
				if (!it) {
					debug(0, "type required before polyline\n");
					goto out_err;
				}
				e = new_polyline(cfg_var_value(var));
				if (e)
					itemtype_add_element(it, e);
			} else if (!strcmp(cfg_var_name(var), "circle")) {
				if (!it) {
					debug(0, "type required before circle\n");
					goto out_err;
				}
				e = new_circle(cfg_var_value(var));
				if (e)
					itemtype_add_element(it, e);
			} else if (!strcmp(cfg_var_name(var), "label")) {
				if (!it) {
					debug(0, "type required before label\n");
					goto out_err;
				}
				if (e && e->type == element_circle) { 
					set_labelsize(e, cfg_var_intvalue(var));
				} else {
					e = label_new(cfg_var_intvalue(var));
					if (e)
						itemtype_add_element(it, e);
				}
			} else if (!strcmp(cfg_var_name(var), "icon")) {
				if (!it) {
					debug(0, "type required before icon\n");
					goto out_err;
				}
				e = icon_new(cfg_var_value(var));
				if (e)
					itemtype_add_element(it, e);
			} else if (!strcmp(cfg_var_name(var), "image")) {
				if (!it) {
					debug(0, "type required before image\n");
					goto out_err;
				}
				e = image_new();
				if (e)
					itemtype_add_element(it, e);
			} else if (!strcmp(cfg_var_name(var), "width")) {
				if (e) {
					set_width(e, cfg_var_intvalue(var));
				} else {
					debug(0, "Error width and no element\n");
				}
			} else if (!strcmp(cfg_var_name(var), "radius")) {
				if (e) {
					set_radius(e, cfg_var_intvalue(var));
				} else {
					debug(0, "Error radius and no element\n");
				}
			} else if (!strcmp(cfg_var_name(var), "directed")) {
				if (e) {
					set_directed(e, cfg_var_true(var));
				} else {
					debug(0, "Error directed and no element\n");
				}
			} else {
				debug(0, "Unknown keyword:[%s]\n",cfg_var_name(var)); 
			}
		}
	}
	navit_cfg_free(cfg);
	return 1;
out_err:
	navit_cfg_free(cfg);
	return -1;
}

int layout_init(void)
{
	struct navit_cfg *cfg;
	struct cfg_varval *var;
	struct cfg_category *cat = NULL;
	struct layout *layout;
	struct layer *layer; 

	debug(0, "Layout initializing ...\n");
	cfg = navit_cfg_load(LAYOUT_CFG);
	if (!cfg)
		return -1;
	layers_init();
	while ((cat = navit_cfg_cats_walk(cfg, cat))) {
		layout = layout_new(cfg_cat_name(cat));
		if (!layout) {
			debug(0, "Failed to allocate layout\n");
			goto out_err;
		}
		var = NULL;
		while ((var = navit_cat_vars_walk(cat, var))) {
			if (!strcmp(cfg_var_name(var), "colors")) {
				layout->colorscheme = strdup(cfg_var_value(var));
			} else if (!strcmp(cfg_var_name(var), "bgcolor")) {
				if (cs_resolve_color(layout->colorscheme, cfg_var_value(var),
								&layout->bgcolor) < 0) {
					debug(0, "Error can not resolve background color:[%s] in scheme [%s]\n",
						cfg_var_value(var), layout->colorscheme);
					goto out_err;
				}
			} else if (!strcmp(cfg_var_name(var), "textbg")) {
				if (cs_resolve_color(layout->colorscheme, cfg_var_value(var),
								&layout->textbg) < 0) {
					debug(0, "Error can not resolve text background color:[%s] in scheme [%s]\n",
						cfg_var_value(var), layout->colorscheme);
					goto out_err;
				}
			} else if (!strcmp(cfg_var_name(var), "textfg")) {
				if (cs_resolve_color(layout->colorscheme, cfg_var_value(var),
								&layout->textfg) < 0) {
					debug(0, "Error can not resolve text  color:[%s] in scheme [%s]\n",
						cfg_var_value(var), layout->colorscheme);
					goto out_err;
				}
			} else if (!strcmp(cfg_var_name(var), "layer")) {
				layer = layer_find(cfg_var_value(var));
				if (!layer) {
					debug(0, "WARNING! Layer %s is not defined\n",
						cfg_var_value(var));
					continue;
				}
				layout_add_layer(layout, layer);
			} else {
				debug(0, "Unknown keyword:[%s]\n",cfg_var_name(var)); 
			}
		}
	}
	navit_cfg_free(cfg);
	return 1;
out_err:
	navit_cfg_free(cfg);
	return -1;
}

static void
menu_layout_set(struct navit *nav, char *name)
{
	struct layout *l;
	l = layout_find(name);
	if (l) {
		layout_activate(l);
		navit_set_layout(nav, l);
		debug(5, "Layout set to [%s]\n", name);
	} else {
		debug(0, "Error can not find layout:[%s]\n", name);
	}
}

void layouts_menu_create(struct navit *nav, struct menu *men)
{
	struct layout *lay;
	struct callback *cb;

	list_for_entry(lay, &llayouts, l) {
		cb=callback_new_2(callback_cast(menu_layout_set), nav, (void *)lay->name);
		menu_add(men, lay->name, menu_type_menu, cb);
	}
}
