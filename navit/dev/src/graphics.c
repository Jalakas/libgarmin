#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "debug.h"
#include "string.h"
#include "point.h"
#include "graphics.h"
#include "projection.h"
#include "map.h"
#include "coord.h"
#include "transform.h"
#include "plugin.h"
#include "profile.h"
#include "mapset.h"
#include "layout.h"
#include "route.h"
#include "util.h"
#include "list.h"
#include "globals.h"

#define GC_BG	0
#define GC_TEXTBG	1
#define GC_TEXTFG	2

struct graphics
{
	struct graphics_priv *priv;
	struct graphics_methods meth;
	struct graphics_font *font[16];
	struct graphics_gc *gc[3];
	struct layout *layout;
	int ready;
};

struct displaytype {
	list_t l;
	unsigned int type;
	list_t litems;
};

struct displayitem {
	list_t l;
	struct item item;
	char *label;
	int displayed;
	int count;
	struct point pnt[0];
};

static struct displaytype *
alloc_displaytype(unsigned int type)
{
	struct displaytype *t;

	t = malloc(sizeof(*t));
	if (!t)
		return NULL;
	list_init(&t->l);
	list_init(&t->litems);
	t->type = type;
	return t;
}

static void
free_displaytype_items(struct displaytype *dt)
{
	struct displayitem *di;
	list_for_first_entry(di, &dt->litems, l) {
		list_remove(&di->l);
		if (! di->displayed && di->item.type < type_line) 
			dbg(1,"warning: item '%s' not displayed\n", item_to_name(di->item.type));
		free(di);
	}
}

#define TYPES_HASH_TAB_SIZE     256
#define TYPE_HASH(type)       (((type) * 2654435769UL)& (TYPES_HASH_TAB_SIZE-1))

struct displaylist {
	list_t types[TYPES_HASH_TAB_SIZE];
};


static struct displaytype *
display_list_get_type(struct displaylist *dl, unsigned int type)
{
	unsigned hash = TYPE_HASH(type);
	struct displaytype *dt;
	list_for_entry(dt, &dl->types[hash], l) {
		if (type == dt->type)
			return dt;
	}
	dt = alloc_displaytype(type);
	if (dt) {
		list_append(&dt->l, &dl->types[hash]);
		return dt;
	}
	return NULL;
}

struct graphics *
graphics_overlay_new(struct graphics *parent, struct point *p, int w, int h)
{
	struct graphics *this_;
	this_=g_new0(struct graphics, 1);
	this_->priv=parent->meth.overlay_new(parent->priv, &this_->meth, p, w, h);
	return this_;
}


void
graphics_init(struct graphics *this_)
{
	this_->gc[GC_BG]=graphics_gc_new(this_);
	graphics_gc_set_background(this_->gc[GC_BG], &(struct color) { 0xffff, 0xefef, 0xb7b7 });
	graphics_gc_set_foreground(this_->gc[GC_BG], &(struct color) { 0xffff, 0xefef, 0xb7b7 });
	this_->gc[GC_TEXTBG]=graphics_gc_new(this_);
	graphics_gc_set_background(this_->gc[GC_TEXTBG], &(struct color) { 0x0000, 0x0000, 0x0000 });
	graphics_gc_set_foreground(this_->gc[GC_TEXTBG], &(struct color) { 0xffff, 0xffff, 0xffff });
	this_->gc[GC_TEXTFG]=graphics_gc_new(this_);
	graphics_gc_set_background(this_->gc[GC_TEXTFG], &(struct color) { 0xffff, 0xffff, 0xffff });
	graphics_gc_set_foreground(this_->gc[GC_TEXTFG], &(struct color) { 0xffff, 0xffff, 0xffff });
	this_->meth.background_gc(this_->priv, this_->gc[GC_BG]->priv);
	// FIXME: layout is set twice 
	// this is called after set_layout from config 
	// so complete the setup here
	graphics_set_layout(this_, this_->layout);
}

void *
graphics_get_data(struct graphics *this_, char *type)
{
	return (this_->meth.get_data(this_->priv, type));
}

void
graphics_register_resize_callback(struct graphics *this_, void (*callback)(void *data, int w, int h), void *data)
{
	this_->meth.register_resize_callback(this_->priv, callback, data);
}

void
graphics_register_button_callback(struct graphics *this_, void (*callback)(void *data, int pressed, int button, struct point *p), void *data)
{
	this_->meth.register_button_callback(this_->priv, callback, data);
}

void
graphics_register_motion_callback(struct graphics *this_, void (*callback)(void *data, struct point *p), void *data)
{
	this_->meth.register_motion_callback(this_->priv, callback, data);
}

struct graphics_font *
graphics_font_new(struct graphics *gra, int size)
{
	struct graphics_font *this_;

	this_=g_new0(struct graphics_font,1);
	this_->priv=gra->meth.font_new(gra->priv, &this_->meth, size);
	return this_;
}

struct graphics_gc *
graphics_gc_new(struct graphics *gra)
{
	struct graphics_gc *this_;

	this_=g_new0(struct graphics_gc,1);
	this_->priv=gra->meth.gc_new(gra->priv, &this_->meth);
	return this_;
}

void
graphics_gc_destroy(struct graphics_gc *gc)
{
	gc->meth.gc_destroy(gc->priv);
	g_free(gc);
}

void
graphics_gc_set_foreground(struct graphics_gc *gc, struct color *c)
{
	gc->meth.gc_set_foreground(gc->priv, c);
}

void
graphics_gc_set_background(struct graphics_gc *gc, struct color *c)
{
	gc->meth.gc_set_background(gc->priv, c);
}

void
graphics_gc_set_linewidth(struct graphics_gc *gc, int width)
{
	gc->meth.gc_set_linewidth(gc->priv, width);
}

struct graphics_image *
graphics_image_new(struct graphics *gra, char *path)
{
	struct graphics_image *this_;

	this_=g_new0(struct graphics_image,1);
	this_->priv=gra->meth.image_new(gra->priv, &this_->meth, path, &this_->width, &this_->height, &this_->hot);
	if (! this_->priv) {
		g_free(this_);
		this_=NULL;
	}
	return this_;
}

void graphics_image_free(struct graphics *gra, struct graphics_image *img)
{
	if (gra->meth.image_free)
		gra->meth.image_free(gra->priv, img->priv);
	g_free(img);
}

void
graphics_draw_restore(struct graphics *this_, struct point *p, int w, int h)
{
	this_->meth.draw_restore(this_->priv, p, w, h);
}

void
graphics_draw_mode(struct graphics *this_, enum draw_mode_num mode)
{
	this_->meth.draw_mode(this_->priv, mode);
}

void
graphics_draw_lines(struct graphics *this_, struct graphics_gc *gc, struct point *p, int count)
{
	this_->meth.draw_lines(this_->priv, gc->priv, p, count);
}

void
graphics_draw_circle(struct graphics *this_, struct graphics_gc *gc, struct point *p, int r)
{
	this_->meth.draw_circle(this_->priv, gc->priv, p, r);
}


void
graphics_draw_rectangle(struct graphics *this_, struct graphics_gc *gc, struct point *p, int w, int h)
{
	this_->meth.draw_rectangle(this_->priv, gc->priv, p, w, h);
}


#include "attr.h"
#include "popup.h"
#include <stdio.h>

#if 0
static void
popup_view_html(struct popup_item *item, char *file)
{
	char command[1024];
	sprintf(command,"firefox %s", file);
	system(command);
}

static void
graphics_popup(struct display_list *list, struct popup_item **popup)
{
	struct item *item;
	struct attr attr;
	struct map_rect *mr;
	struct coord c;
	struct popup_item *curr_item,*last=NULL;
	item=list->data;
	mr=map_rect_new(item->map, NULL, NULL, 0);
	printf("id hi=0x%x lo=0x%x\n", item->id_hi, item->id_lo);
	item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
	if (item) {
		if (item_attr_get(item, attr_name, &attr)) {
			curr_item=popup_item_new_text(popup,attr.u.str,1);
			if (item_attr_get(item, attr_info_html, &attr)) {
				popup_item_new_func(&last,"HTML Info",1, popup_view_html, g_strdup(attr.u.str));
			}
			if (item_attr_get(item, attr_price_html, &attr)) {
				popup_item_new_func(&last,"HTML Preis",2, popup_view_html, g_strdup(attr.u.str));
			}
			curr_item->submenu=last;
		}
	}
	map_rect_destroy(mr);
}
#endif

static void
xdisplay_free(struct displaylist *displaylist)
{
	int i;
	struct displaytype *dt;
	for (i=0; i < TYPES_HASH_TAB_SIZE; i++) {
		list_for_entry(dt, &displaylist->types[i], l) {
			free_displaytype_items(dt);
		}
	}
}

void
display_add(struct displaylist *displaylist, struct item *item, int count, struct point *pnt, char *label)
{
	struct displayitem *di;
	struct displaytype *dt;
	int len;
	char *p;

	len=sizeof(*di)+count*sizeof(*pnt);
	if (label)
		len+=strlen(label)+1;

	p=malloc(len);

	di=(struct displayitem *)p;
	di->displayed=0;
	p+=sizeof(*di)+count*sizeof(*pnt);
	di->item=*item;
	if (label) {
		di->label=p;
		strcpy(di->label, label);
	} else 
		di->label=NULL;
	di->count=count;
	memcpy(di->pnt, pnt, count*sizeof(*pnt));

	dt = display_list_get_type(displaylist, item->type);
	if (dt) {
		list_append(&di->l, &dt->litems);
	}
}


static void
label_line(struct graphics *gra, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font, struct point *p, int count, char *label)
{
	int i,x,y,tl;
	double dx,dy,l;
	struct point p_t;

	tl=strlen(label)*400;
	for (i = 0 ; i < count-1 ; i++) {
		dx=p[i+1].x-p[i].x;
		dx*=100;
		dy=p[i+1].y-p[i].y;
		dy*=100;
		l=(int)sqrtf((float)(dx*dx+dy*dy));
		if (l > tl) {
			x=p[i].x;
			y=p[i].y;
			if (dx < 0) {
				dx=-dx;
				dy=-dy;
				x=p[i+1].x;
				y=p[i+1].y;
			}
			x+=(l-tl)*dx/l/200;
			y+=(l-tl)*dy/l/200;
			x-=dy*45/l/10;
			y+=dx*45/l/10;
			p_t.x=x;
			p_t.y=y;
	#if 0
			printf("display_text: '%s', %d, %d, %d, %d %d\n", label, x, y, dx*0x10000/l, dy*0x10000/l, l);
	#endif
			gra->meth.draw_text(gra->priv, fg->priv, bg->priv, font->priv, label, &p_t, dx*0x10000/l, dy*0x10000/l);
		}
	}
}


static void
xdisplay_draw_elements(struct graphics *gra, struct displaylist *displaylist, struct itemtype *itm)
{
	struct element *e;
	GList *es,*types;
	enum item_type type;
	struct graphics_gc *gc = NULL;
	struct graphics_image *img;
	struct point p;
	struct displaytype *dt;
	struct displayitem *di;
	char icon[PATH_MAX];
	es=itm->elements;
	while (es) {
		e=es->data;
		types=itm->type;
		while (types) {
			type=GPOINTER_TO_INT(types->data);
			dt = display_list_get_type(displaylist,  type);
			if (!dt)
				continue;
			if (gc)
				graphics_gc_destroy(gc);
			gc=NULL;
			img=NULL;
			list_for_entry(di, &dt->litems,l) {
				di->displayed=1;
				if (! gc) {
					gc=graphics_gc_new(gra);
					gc->meth.gc_set_foreground(gc->priv, &e->color);
				}
				switch (e->type) {
				case element_polygon:
					gra->meth.draw_polygon(gra->priv, gc->priv, di->pnt, di->count);
					break;
				case element_polyline:
					if (e->u.polyline.width > 1) 
						gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
					gra->meth.draw_lines(gra->priv, gc->priv, di->pnt, di->count);
					break;
				case element_circle:
					if (e->u.circle.width > 1) 
						gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
					gra->meth.draw_circle(gra->priv, gc->priv, &di->pnt[0], e->u.circle.radius);
					if (di->label && e->label_size) {
						p.x=di->pnt[0].x+3;
						p.y=di->pnt[0].y+10;
						if (! gra->font[e->label_size])
							gra->font[e->label_size]=graphics_font_new(gra, e->label_size*20);
						gra->meth.draw_text(gra->priv, gra->gc[GC_TEXTFG]->priv, gra->gc[GC_TEXTBG]->priv, gra->font[e->label_size]->priv, di->label, &p, 0x10000, 0);
					}
					break;
				case element_label:
					if (di->label) {
						if (! gra->font[e->label_size])
							gra->font[e->label_size]=graphics_font_new(gra, e->label_size*20);
						label_line(gra, gra->gc[GC_TEXTFG], gra->gc[GC_TEXTBG], gra->font[e->label_size], di->pnt, di->count, di->label);
					}
					break;
				case element_icon:
					if (!img) {
						sprintf(icon, "%s/xpm/%s", navit_sharedir, e->u.icon.src);
						img=graphics_image_new(gra, icon);
						if (! img)
							g_warning("failed to load icon '%s'\n", e->u.icon.src);
					}
					if (img) {
						p.x=di->pnt[0].x - img->hot.x;
						p.y=di->pnt[0].y - img->hot.y;
						gra->meth.draw_image(gra->priv, gra->gc[GC_BG]->priv, &p, img->priv);
						graphics_image_free(gra, img);
						img = NULL;
					}
					break;
				case element_image:
					printf("image: '%s'\n", di->label);
					gra->meth.draw_image_warp(gra->priv, gra->gc[GC_BG]->priv, di->pnt, di->count, di->label);
					break;
				default:
					printf("Unhandled element type %d\n", e->type);
				
				}
			}
			types=g_list_next(types);
		}
		es=g_list_next(es);
	}
	if (gc)
		graphics_gc_destroy(gc);
}

static void
xdisplay_draw_layer(struct displaylist *displaylist, struct graphics *gra, struct layer *lay, int order)
{
	GList *itms;
	struct itemtype *itm;

	itms=lay->itemtypes;
	while (itms) {
		itm=itms->data;
		if (order >= itm->order_min && order <= itm->order_max) 
			xdisplay_draw_elements(gra, displaylist, itm);
		itms=g_list_next(itms);
	}
}

static void
xdisplay_draw(struct displaylist *displaylist, struct graphics *gra, int order)
{
	struct layer *lay;
	GList *lays;

	if (!gra->layout) {
		debug(0, "Error no layout is set\n");
		return;
	}
	lays=gra->layout->layers;
	while (lays) {
		lay=lays->data;
		xdisplay_draw_layer(displaylist, gra, lay, order);
		lays=g_list_next(lays);
	}
}

extern void *route_selection;

static void
do_draw_map(struct displaylist *displaylist, struct transformation *t, struct map *m, int order)
{
	enum projection pro;
	struct map_rect *mr;
	struct item *item;
	int conv,count,max=16384;
	struct point pnt[max];
	struct coord ca[max];
	struct attr attr;
	struct map_selection *sel;

	pro=map_projection(m);
	conv=map_requires_conversion(m);
	sel=transform_get_selection(t, pro, order);
	if (route_selection)
		mr=map_rect_new(m, route_selection);
	else
		mr=map_rect_new(m, sel);
	if (! mr) {
		map_selection_destroy(sel);
		return;
	}
	while ((item=map_rect_get_item(mr))) {
		count=item_coord_get(item, ca, item->type < type_line ? 1: max);
		if (item->type >= type_line && count < 2) {
			dbg(1,"poly from map has only %d points\n", count);
			continue;
		}
		if (item->type < type_line) {
			if (! map_selection_contains_point(sel, &ca[0])) {
				dbg(1,"point not visible\n");
				continue;
			}
		} else if (item->type < type_area) {
			if (! map_selection_contains_polyline(sel, ca, count)) {
				dbg(1,"polyline not visible\n");
				continue;
			}
		} else {
			if (! map_selection_contains_polygon(sel, ca, count)) {
				dbg(1,"polygon not visible\n");
				continue;
			}
		}
		if (count == max) 
			dbg(0,"point count overflow\n", count);
		count=transform(t, pro, ca, pnt, count, 1);
		if (item->type >= type_line && count < 2) {
			dbg(1,"poly from transform has only %d points\n", count);
			continue;
		}
		if (!item_attr_get(item, attr_label, &attr))
			attr.u.str=NULL;
		if (conv && attr.u.str && attr.u.str[0]) {
			char *str=map_convert_string(m, attr.u.str);
			display_add(displaylist, item, count, pnt, str);
			map_convert_free(str);
		} else
			display_add(displaylist, item, count, pnt, attr.u.str);
	}
	map_rect_destroy(mr);
	map_selection_destroy(sel);
}

static void
do_draw(struct displaylist *displaylist, struct transformation *t, GList *mapsets, int order)
{
	struct mapset *ms;
	struct map *m;
	struct mapset_handle *h;

	if (! mapsets)
		return;
	ms=mapsets->data;
	h=mapset_open(ms);
	while ((m=mapset_next(h, 1))) {
		do_draw_map(displaylist, t, m, order);
	}
	mapset_close(h);
}

int
graphics_ready(struct graphics *this_)
{
	return this_->ready;
}

void
graphics_displaylist_draw(struct graphics *gra, struct displaylist *displaylist, struct transformation *trans)
{
	int order=transform_get_order(trans);
	gra->meth.draw_mode(gra->priv, draw_mode_begin);
	xdisplay_draw(displaylist, gra, order);
	gra->meth.draw_mode(gra->priv, draw_mode_end);
}

void
graphics_displaylist_move(struct displaylist *displaylist, int dx, int dy)
{
	struct displayitem *di;
	struct displaytype *dt;
	int i,j;
	for (i=0; i < TYPES_HASH_TAB_SIZE; i++) {
		list_for_entry(dt, &displaylist->types[i], l) {
			list_for_entry(di, &dt->litems, l) {
				for (j = 0 ; j < di->count ; j++) {
					di->pnt[j].x+=dx;
					di->pnt[j].y+=dy;
				}
			}
		}
	}
}

void
graphics_set_layout(struct graphics *gra, struct layout *layout)
{
	gra->layout = layout;
	if (gra->gc[GC_BG]) {
		graphics_gc_set_background(gra->gc[GC_BG], &layout->bgcolor);
		graphics_gc_set_foreground(gra->gc[GC_BG], &layout->bgcolor);
		graphics_gc_set_background(gra->gc[GC_TEXTBG], &layout->textbg);
		graphics_gc_set_foreground(gra->gc[GC_TEXTBG], &layout->textfg);
		graphics_gc_set_background(gra->gc[GC_TEXTFG], &layout->textbg);
		graphics_gc_set_foreground(gra->gc[GC_TEXTFG], &layout->textfg);
		gra->meth.background_gc(gra->priv, gra->gc[GC_BG]->priv);
	}
}

void
graphics_draw(struct graphics *gra, struct displaylist *displaylist, GList *mapsets, struct transformation *trans)
{
	int order=transform_get_order(trans);

	dbg(1,"enter");

#if 0
	printf("scale=%d center=0x%x,0x%x mercator scale=%f\n", scale, co->trans->center.x, co->trans->center.y, transform_scale(co->trans->center.y));
#endif
	
	xdisplay_free(displaylist);
	dbg(1,"order=%d\n", order);


#if 0
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_begin(co->data_window[i]);	
	}
#endif
	profile(0,NULL);
	do_draw(displaylist, trans, mapsets, order);
//	profile(1,"do_draw");
	graphics_displaylist_draw(gra, displaylist, trans);
	profile(1,"xdisplay_draw");
	profile(0,"end");
  
#if 0
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_end(co->data_window[i]);
	}
#endif
	gra->ready=1;
}


void display_list_for_each(struct displaylist *displaylist, void (*cb_fn)(struct displayitem *di, void *data), void *data)
{
	struct displayitem *di;
	int i;
	struct displaytype *dt;
	for (i=0; i < TYPES_HASH_TAB_SIZE; i++) {
		list_for_entry(dt, &displaylist->types[i], l) {
			list_for_entry(di, &dt->litems, l) {
				cb_fn(di, data);
			}
		}
	}
}
struct displaylist *
graphics_displaylist_new(void)
{
	struct displaylist *ret=g_new(struct displaylist, 1);
	int i;

	for (i=0; i < TYPES_HASH_TAB_SIZE; i++) {
		list_init(&ret->types[i]);
	}
	return ret;
}

struct item *
graphics_displayitem_get_item(struct displayitem *di)
{
	return &di->item;
}

char *
graphics_displayitem_get_label(struct displayitem *di)
{
	return di->label;
}

static int
within_dist_point(struct point *p0, struct point *p1, int dist)
{
	if (p0->x == 32767 || p0->y == 32767 || p1->x == 32767 || p1->y == 32767)
		return 0;
	if (p0->x == -32768 || p0->y == -32768 || p1->x == -32768 || p1->y == -32768)
		return 0;
        if ((p0->x-p1->x)*(p0->x-p1->x) + (p0->y-p1->y)*(p0->y-p1->y) <= dist*dist) {
                return 1;
        }
        return 0;
}

static int
within_dist_line(struct point *p, struct point *line_p0, struct point *line_p1, int dist)
{
	int vx,vy,wx,wy;
	int c1,c2;
	struct point line_p;

	vx=line_p1->x-line_p0->x;
	vy=line_p1->y-line_p0->y;
	wx=p->x-line_p0->x;
	wy=p->y-line_p0->y;

	c1=vx*wx+vy*wy;
	if ( c1 <= 0 )
		return within_dist_point(p, line_p0, dist);
	c2=vx*vx+vy*vy;
	if ( c2 <= c1 )
		return within_dist_point(p, line_p1, dist);

	line_p.x=line_p0->x+vx*c1/c2;
	line_p.y=line_p0->y+vy*c1/c2;
	return within_dist_point(p, &line_p, dist);
}

static int
within_dist_polyline(struct point *p, struct point *line_pnt, int count, int dist, int close)
{
	int i;
	for (i = 0 ; i < count-1 ; i++) {
		if (within_dist_line(p,line_pnt+i,line_pnt+i+1,dist)) {
			return 1;
		}
	}
	if (close)
		return (within_dist_line(p,line_pnt,line_pnt+count-1,dist));
	return 0;
}

static int
within_dist_polygon(struct point *p, struct point *poly_pnt, int count, int dist)
{
	int i, j, c = 0;
        for (i = 0, j = count-1; i < count; j = i++) {
		if ((((poly_pnt[i].y <= p->y) && ( p->y < poly_pnt[j].y )) ||
		((poly_pnt[j].y <= p->y) && ( p->y < poly_pnt[i].y))) &&
		(p->x < (poly_pnt[j].x - poly_pnt[i].x) * (p->y - poly_pnt[i].y) / (poly_pnt[j].y - poly_pnt[i].y) + poly_pnt[i].x)) 
                        c = !c;
        }
	if (! c)
		return within_dist_polyline(p, poly_pnt, count, dist, 1);
        return c;
}

int
graphics_displayitem_within_dist(struct displayitem *di, struct point *p, int dist)
{
	if (di->item.type < type_line) {
		return within_dist_point(p, &di->pnt[0], dist);
	}
	if (di->item.type < type_area) {
		return within_dist_polyline(p, di->pnt, di->count, dist, 0);
	}
	return within_dist_polygon(p, di->pnt, di->count, dist);
}

struct graphics_provider {
	list_t l;
	char *name;
	struct graphics_ops *ops;
};

static list_head(lgraphics);

int graphics_register(char *name, struct graphics_ops *ops)
{
	struct graphics_provider *gp;
	gp = calloc(1, sizeof(*gp));
	if (!gp)
		return -1;
	gp->name = strdup(name);
	if (!gp->name) {
		free(gp);
		return -1;
	}
	gp->ops = ops;
	list_append(&gp->l, &lgraphics);
	return 1;
}

struct graphics_ops *graphics_get_byname(const char *name)
{
	struct graphics_provider *gp;
	list_for_entry(gp, &lgraphics, l) {
		if (!strcmp(gp->name, name))
			return gp->ops;
	}
	return NULL;
}

struct graphics *
graphics_new(const char *type, struct attr **attrs)
{
	struct graphics *this_;
	struct graphics_priv * (*new)(struct graphics_methods *meth, struct attr **attrs);
	struct graphics_ops *grops;

	grops = graphics_get_byname(type);
	if (!grops) {
		navit_request_module(type);
		grops = graphics_get_byname(type);
	}
	if (grops) {
		this_=g_new0(struct graphics, 1);
		if (!this_)
			return NULL;
		this_->priv=grops->graphics_new(&this_->meth, attrs);
		if (!this_->priv) {
			debug(0, "Graphics:[%s] failed to create instance\n", type);
			g_free(this_);
			return NULL;
		}
	} else {
		new=plugin_get_graphics_type(type);
		if (! new)
			return NULL;
		this_=g_new0(struct graphics, 1);
		this_->priv=(*new)(&this_->meth, attrs);
	}
	return this_;
}
