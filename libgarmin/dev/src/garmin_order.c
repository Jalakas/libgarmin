#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "list.h"
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_order.h"

#ifdef STANDALONE
#undef log
#define log(n,x...) fprintf(stdout, ## x)
#endif

struct gar_objprio_def {
	unsigned char objid;
	unsigned char prio;
};

/* Default polygon draw order 1 is lowest prio */
static struct gar_objprio_def def_poly_order[] = {
{ 0x01,1 },
{ 0x02,1 },
{ 0x03,1 },
{ 0x04,1 },
{ 0x05,1 },
{ 0x06,1 },
{ 0x07,1 },
{ 0x08,3 },
{ 0x09,1 },
{ 0x0a,2 },
{ 0x0b,2 },
{ 0x0c,2 },
{ 0x0d,2 },
{ 0x0e,2 },
{ 0x13,2 },
{ 0x14,2 },
{ 0x15,2 },
{ 0x16,2 },
{ 0x17,3 },
{ 0x18,3 },
{ 0x19,3 },
{ 0x1a,4 },
{ 0x1e,2 },
{ 0x1f,2 },
{ 0x20,2 },
{ 0x28,1 },
{ 0x29,1 },
{ 0x32,1 },
{ 0x3b,1 },
{ 0x3c,7 },
{ 0x3d,7 },
{ 0x3e,7 },
{ 0x3f,7 },
{ 0x40,7 },
{ 0x41,7 },
{ 0x42,7 },
{ 0x43,7 },
{ 0x44,4 },
{ 0x45,2 },
{ 0x46,2 },
{ 0x47,2 },
{ 0x48,3 },
{ 0x49,4 },
{ 0x4a,8 },
{ 0x4b,8 },
{ 0x4c,5 },
{ 0x4d,5 },
{ 0x4e,5 },
{ 0x4f,5 },
{ 0x50,3 },
{ 0x51,6 },
{ 0x52,4 },
{ 0x53,5 },
};

int gar_init_draworder(struct gar_objdraworder *o, int objtype)
{
	o->maxprio = 0;
	o->objtype = objtype;
	return 1;
}

int gar_free_draworder(struct gar_objdraworder *o)
{
	return 1;
}

int gar_get_draw_prio(struct gar_objdraworder *d, unsigned char objid)
{
	return d->order[objid];
}

int gar_del_draw_prio(struct gar_objdraworder *d, unsigned char objid, 
			unsigned char prio)
{
	d->order[objid] = 0;
	return 0;
}

int gar_add_draw_prio(struct gar_objdraworder *d, unsigned char objid, 
			unsigned char prio)
{
	if (prio > d->maxprio)
		d->maxprio = prio;
	d->order[objid] = prio;
	return 1;
}

int gar_set_default_poly_order(struct gar_objdraworder *od)
{
	int i;

	for (i = 0; i < sizeof(def_poly_order)/sizeof(def_poly_order[0]); i++)
		if (gar_add_draw_prio(od, def_poly_order[i].objid,
				 def_poly_order[i].prio) < 0)
			return -1;
	return 0;
}

static int inline gar_get_obj_prio(struct gobject *o, struct gar_objdraworder *od)
{
	int objid = -1;
	switch (o->type) {
		case GO_POINT:
			objid = ((struct gpoint *)o->gptr)->type;
			break;
		case GO_POLYLINE:
		case GO_POLYGON:
			objid = ((struct gpoly *)o->gptr)->type;
			break;
	}

	if (objid == -1)
		return 0;

	return gar_get_draw_prio(od, objid);
}
static int gar_count_objects(struct gobject *objs)
{
	int c = 0;
	while (objs) {
		c++;
		objs = objs->next;
	}
	return c;
}
static struct gobject *gar_do_order(struct gobject *objs, struct gar_objdraworder *od)
{
	struct gobject *ol, *on = NULL;
	struct gobject *prios[od->maxprio+1];
	struct gobject *last[od->maxprio+1];
	int i;

	for (i=0; i < od->maxprio + 1; i++) {
		last[i] = prios[i] = NULL;
	}

	ol = objs;
	while (objs) {
		on = objs->next;
		i = gar_get_obj_prio(objs, od);
		if (prios[i])
			objs->next = prios[i];
		else {
			objs->next = NULL;
			last[i] = objs;
		}
		prios[i] = objs;
		objs=on;
	}
	if (prios[0]) {
		log(1, "Unknown order prio[0] = %d\n",
			gar_count_objects(prios[0]));
	}
	ol = last[0];
	if (ol)
		on = prios[0];
	for (i=1; i < od->maxprio + 1; i++) {
		if (ol)
			ol->next = prios[i];
		else if (!on)
			on = prios[i];
		if (last[i])
			ol = last[i];
	}
	return on;
}

/*
 Objects list to sort, draw order, where to place the sorted objects
 head = 1, at head, = 0 at tail
 unknown objects are ordered in front, prio=0
 as per pseudo docs they should not be drawn
 Polygons are sorted in front
 TODO:
 Order Polylines after polygons
 and points after polylines
 */
struct gobject *gar_order_objects(struct gobject *objs, struct gar_objdraworder *od,
				int head)
{
	struct gobject *toorder = NULL, *otmp, *ostart, *otmp1;
	struct gobject *order = NULL, *oprev = NULL;
	int type = od->objtype;
	int incnt,outcnt;

	incnt = gar_count_objects(objs);
	ostart = otmp = objs;
	while (otmp) {
		if (otmp->type == type) {
			// remove from list
			if (oprev) {
				oprev->next = otmp->next;
			} else {
				ostart = otmp->next;
			}
			
			// add to order list
			if (order) {
				order->next = otmp;
				order = order->next;
			} else {
				order = toorder = otmp;
			}
			// oprev stay the same
			otmp1 = otmp->next;
			otmp->next = NULL;
			otmp = otmp1;
		} else { 
			oprev = otmp;
			otmp = otmp->next;
		}
	}
	if (!toorder)
		return ostart;

	otmp1 = gar_do_order(toorder, od);
	if (head) {
		oprev = otmp1;
		while (oprev->next)
			oprev = oprev->next;
		oprev->next = ostart;
		order = otmp1;
	} else {
		oprev = ostart;
		while (oprev->next)
			oprev = oprev->next;
		oprev->next = otmp1;
		order = ostart;
	}
	outcnt = gar_count_objects(order);
	if (incnt!=outcnt)
	log(1, "Inobjects:%d outobjects: %d lost:%d\n",
		incnt, outcnt, incnt-outcnt);
	return order;
}

#ifdef STANDALONE
static int gar_show_prios(struct gar_objdraworder *d)
{
	int i;
	for (i=0; i < 256; i++)
		fprintf(stderr, "%d = %02X\n", d->order[i], i);
	printf("maxprio=%d\n", d->maxprio);
	return 0;
}

void print_dummy(struct gobject *d)
{
	int cnt = 0;
	while (d) {
		cnt ++;
		printf("type=%d objid=%02X\n", d->type, ((struct gpoint *)d->gptr)->type);
		d = d->next;
	}
	printf("Total: %d\n", cnt);
}

struct gobject *make_dummy(int cnt)
{
	struct gobject *ret = NULL, *go , *prev;

	while (cnt) {
		go = calloc(1, sizeof(*go));
		go->type = 4;
		go->gptr = calloc(1, sizeof(struct gpoint));
		if (cnt < 10)
		((struct gpoint *)go->gptr)->type = 0x32;
		else
		((struct gpoint *)go->gptr)->type = 0x01;
		if (!ret)
			prev = ret = go;
		else {
			prev->next = go;
			prev = go;
		}
		cnt --;
	}
	return ret;
}

int main(int argc, char **argv)
{
	struct gar_objdraworder od;
	struct gobject *g;
	gar_init_draworder(&od,4);
	gar_set_default_poly_order(&od);
	gar_show_prios(&od);
	g = make_dummy(20);
	printf("input:\n");
	print_dummy(g);
	g =  gar_order_objects(g, &od, 1); 
	printf("result:\n");
	print_dummy(g);
	gar_free_draworder(&od);
}
#endif
