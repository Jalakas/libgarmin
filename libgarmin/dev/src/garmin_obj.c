/*
    Copyright (C) 2007  Alexander Atanasov	<aatanasov@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
    
    
*/

#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_rgn.h"
#include "garmin_lbl.h"
#include "garmin_order.h"
#include "geoutils.h"

static struct gobject *gar_alloc_object(int type, void *obj)
{
	struct gobject *o;
	o = calloc(1, sizeof(*o));
	if (!o)
		return o;
	o->type = type;
	o->gptr = obj;
	return o;
}

void gar_free_objects(struct gobject *g)
{
	struct gobject *gn;
	while (g) {
		gn = g->next;
		free(g);
		g = gn;
	}
}

static int gar_is_point_visible(struct gar_subfile *gsub, int level, struct gpoint *gp)
{
	int i;
	int type = (gp->type << 8 ) || gp->subtype;
	if (!gsub->fpoint)
		return 1;	// no filter all is visible
	for (i=0; i < gsub->nfpoint; i++) {
		if (gsub->fpoint[i].type == type) {
			if (gsub->fpoint[i].maxlevel >= level)
				return 1;
			else
				return 0;
		}
	}
	return 0;
}

static int gar_is_line_visible(struct gar_subfile *gsub, int level, struct gpoly *gp)
{
	int i;
	int type = gp->type;
	if (!gsub->fpolyline)
		return 1;	// no filter all is visible
	for (i=0; i < gsub->nfpolyline; i++) {
		if (gsub->fpolyline[i].type == type) {
			if (gsub->fpolyline[i].maxlevel >= level)
				return 1;
			else
				return 0;
		}
	}
	return 0;
}

static int gar_is_pgone_visible(struct gar_subfile *gsub, int level, struct gpoly *gp)
{
	int i;
	int type = gp->type;
	if (!gsub->fpolygone)
		return 1;	// no filter all is visible
	for (i=0; i < gsub->nfpolygone; i++) {
		if (gsub->fpolygone[i].type == type) {
			if (gsub->fpolygone[i].maxlevel >= level)
				return 1;
			else
				return 0;
		}
	}
	return 0;
}

static struct gar_subdiv *gar_find_subdiv_by_idx(struct gar_subfile *gsub, 
						int fromlevel, int idx)
{
	int i = fromlevel;
	struct gar_subdiv *d;
	for (; i < gsub->nlevels; i++) {
		list_for_entry(d, &gsub->maplevels[i]->lsubdivs, l) {
			if (d->n == idx) {
				log(15, "Found in level %d\n", i);
				return d;
			}
		}
	}

	return NULL;
}

static struct gobject *gar_get_subdiv_objs(struct gar_subdiv *gsd, int *count, struct gar_subfile *gsub, int level, int start)
{
	struct gobject *first = NULL, *o = NULL, *p;
	struct gar_subdiv *gss, *gs;
	struct gpoint *gp;
	struct gpoly *gpoly;
	int objs = 0, i;

	log(15, "subdiv:%d cx=%d cy=%d north=%d west=%d south=%d east=%d\n",
		gsd->n, gsd->icenterlng, gsd->icenterlat, gsd->north, gsd->west,
		gsd->south, gsd->east); 
	list_for_entry(gpoly, &gsd->lpolygons, l) {
		/* Do not return definition areas */
		if (gpoly->type == 0x4b)
			continue;
		if (!start && !gar_is_pgone_visible(gsub, level, gpoly))
			continue;
		p = gar_alloc_object(GO_POLYGON, gpoly);
		if (!p)
			goto out_err;
		if (first) {
			o->next = p;
			o = p; 
		} else
			o = first = p;
		objs++;
	}
	list_for_entry(gpoly, &gsd->lpolylines, l) {
		if (!start && !gar_is_line_visible(gsub, level, gpoly))
			continue;
		p = gar_alloc_object(GO_POLYLINE, gpoly);
		if (!p)
			goto out_err;
		if (first) {
			o->next = p;
			o = p; 
		} else
			o = first = p;
		objs++;
	}

	list_for_entry(gp, &gsd->lpoints, l) {
		if (!start && !gar_is_point_visible(gsub, level, gp))
			continue;
		p = gar_alloc_object(GO_POINT, gp);
		if (!p)
			goto out_err;
		if (first) {
			o->next = p;
			o = p; 
		} else
			o = first = p;
		objs++;
	}
	list_for_entry(gp, &gsd->lpois, l) {
		if (!start && !gar_is_point_visible(gsub, level, gp))
			continue;
		p = gar_alloc_object(GO_POI, gp);
		if (!p)
			goto out_err;
		if (first) {
			o->next = p;
			o = p; 
		} else
			o = first = p;
		objs++;
	}
	if (gsd->next) {
		log(15, "Must load %d subdiv\n", gsd->next);
		if (gsd->next == gsd->n) {
			log(1, "%d points to itself\n", gsd->next);
			goto done;
		}
		if (gsd->nextptr)
			gss = gsd->nextptr;
		else {
			gss = gar_find_subdiv_by_idx(gsub, level, gsd->next);
			gsd->nextptr = gss;
		}
		if (gss) {
			list_for_entry(gs, gss->l.p, l) {
				log(15, "Loading subdiv: %d\n", gs->n);
				p = gar_get_subdiv_objs(gs, &i, gsub, level, 0);
				if (p) {
					objs += i;
					if (first) {
						o = p;
						while (o->next)
							o = o->next;
						o->next = first;
					}
					first = p;
				}
				if (gs->terminate)
					break;
			}
		}
	}
done:
	*count = objs;
	return first;
out_err:
	if (first)
		gar_free_objects(first);
	*count = 0;
	return NULL;
}

static int gar_subdiv_visible(struct gar_subdiv *sd, struct gar_rect *rect)
{
	struct gar_rect sr;
	sr.lulat = sd->north;
	sr.lulong = sd->west;
	sr.rllat = sd->south;
	sr.rllong = sd->east;
	return gar_rects_intersectboth(&sr, rect);
}

// FIXME: This is very very slow
struct gobject *gar_get_object(struct gar *gar, void *ptr)
{
	struct gimg *g;
	struct gar_subfile *sub;
	struct gar_subdiv *gsd;
	struct gar_maplevel *ml;
	struct gpoint *gp;
	struct gpoly *gpoly;

	int i;
	list_for_entry(g, &gar->limgs,l) {
		list_for_entry(sub, &g->lsubfiles, l) {
			for (i=0; i < sub->nlevels; i++) {
				ml = sub->maplevels[i];
				list_for_entry(gsd, &ml->lsubdivs, l) {
					list_for_entry(gpoly, &gsd->lpolygons, l) {
						if (gpoly == ptr) {
							return gar_alloc_object(GO_POLYGON, gpoly);
						}
					}
					list_for_entry(gpoly, &gsd->lpolylines, l) {
						if (gpoly == ptr) {
							return gar_alloc_object(GO_POLYLINE, gpoly);
						}
					}
					list_for_entry(gp, &gsd->lpoints, l) {
						if (gp == ptr) {
							return gar_alloc_object(GO_POINT, gp);
						}
					}
					list_for_entry(gp, &gsd->lpois, l) {
						if (gp == ptr) {
							return gar_alloc_object(GO_POI, gp);
						}
					}
				}
			}
		}
	}
	return NULL;
}

int gar_get_objects(struct gmap *gm, int level, struct gar_rect *rect, 
			struct gobject **ret, int flags)
{
	struct gobject *first = NULL, *o = NULL, *p;
	struct gar_maplevel *ml;
	struct gar_subdiv *gsd;
	struct gar_subfile *gsub;
	int objs = 0;
	int lvl = level;
	int bits,i,j;
	int nsub = 0;
	int lvlmax = 0;
	int nbits[25];
	int fb = 0;

	gsub = gm->subs[0];
	if (!gsub)
		return -1;
	for (nsub = 0; nsub < 25; nsub++)
		nbits[nsub] = 0;
	for (nsub = 0; nsub < gm->subfiles ; nsub++) {
		for (i = 0; i < gm->subs[nsub]->nlevels; i++) {
			ml = gm->subs[nsub]->maplevels[i];
			if (ml->ml.inherited)
				continue;
			nbits[ml->ml.bits] = 1;
		}
	}
	i = 0;
	for (nsub = 0; nsub < 25; nsub++) {
		if (nbits[nsub]) {
			i++;
			if (!fb)
				fb = nsub;
		}
	}
	log(1, "Have %d levels base bits=%d\n", i, fb);
	lvl = fb + (i/18.0) * lvl - 1;
	log(1, "Level %d requested, scaling to %d levels=%d\n",
			level, i, lvl);
#if 0
	if (1 || lvl >= gsub->nlevels) {
		lvl = (gsub->nlevels/18.0) * lvl;
		if (lvl == gsub->nlevels)
			lvl = gsub->nlevels -1;
		log(1, "Level %d requested, scaling to %d levels=%d\n",
			level, gsub->nlevels, lvl);
	}
#endif
//	if (gsub->maplevels[lvl]->ml.inherited)
//		lvl++;
//	ml = gsub->maplevels[lvl];
//	bits = ml->ml.bits;
	bits = lvl;
	log(7, "Level in=%d => Level = %d bits = %d\n", level, lvl, bits);
	if (rect) {
		log(15, "Rect: lulong=%f lulat=%f rllong=%f rllat=%f\n",
			rect->lulong, rect->lulat, rect->rllong, rect->rllat);
	}
	for (nsub = 0; nsub < gm->subfiles ; nsub++) {
		gsub = gm->subs[nsub];
		for (i = 0; i < gsub->nlevels; i++) {
			ml = gsub->maplevels[i];
			if (ml->ml.inherited)
				continue;
			if (ml->ml.bits < bits)
				continue;
			list_for_entry(gsd, &ml->lsubdivs, l) {
				if (rect && !gar_subdiv_visible(gsd, rect))
					continue;
				p = gar_get_subdiv_objs(gsd, &j, gsub, ml->ml.level, 1);
				if (p) {
					objs += j;
					if (first) {
						o = p;
						while (o->next)
							o = o->next;
						o->next = first;
					}
					first = p;
				}
			}
			break;
		}
	}
	if ((flags&GO_GET_SORTED) && gm->draworder)
		*ret = gar_order_objects(first, gm->draworder, 1);
	else
		*ret = first;
	return objs;
}

u_int8_t gar_obj_type(struct gobject *o)
{
	u_int8_t ret = 0;
	switch (o->type) {
	case GO_POINT:
	case GO_POI:
		ret = ((struct gpoint *)o->gptr)->type;
		break;
	case GO_POLYLINE:
	case GO_POLYGON:
		ret = ((struct gpoly *)o->gptr)->type;
		break;
	default:
		log(1, "Error unknown object type:%d\n", o->type);
	}
	return ret;
}

int gar_get_object_position(struct gobject *o, struct gcoord *ret)
{
	switch (o->type) {
	case GO_POINT:
	case GO_POI:
		ret->x = ((struct gpoint *)o->gptr)->c.x;
		ret->y = ((struct gpoint *)o->gptr)->c.y;
		return 1;
	case GO_POLYLINE:
	case GO_POLYGON:
		ret->x = ((struct gpoly *)o->gptr)->c.x;
		ret->y = ((struct gpoly *)o->gptr)->c.y;
		return 1;
	default:
		log(1, "Error unknown object type:%d\n", o->type);
	}
	return 0;
}

int gar_get_object_deltas(struct gobject *o)
{
	struct gpoly *gp;
	if  (o->type == GO_POINT || o->type == GO_POI)
		return 0;
	gp = o->gptr;
	return gp->npoints;
}

int gar_get_object_coord(struct gmap *gm, struct gobject *o, struct gcoord *ret)
{
	struct gpoint *gp;
	struct gpoly *gl;
	if  (o->type == GO_POINT || o->type == GO_POI) {
		gp = o->gptr;
		ret->x = gp->c.x;
		ret->y = gp->c.y;
		return 1;
	}
	gl = o->gptr;
	ret->x = gl->c.x;
	ret->y = gl->c.y;
	return 1;
}

int gar_get_object_dcoord(struct gmap *gm, struct gobject *o, int ndelta, struct gcoord *ret)
{
	struct gpoly *gp;
	if  (o->type == GO_POINT || o->type == GO_POI)
		return 0;
	gp = o->gptr;
	if (ndelta < gp->npoints) {
		*ret = gp->deltas[ndelta];
		return 1;
	}
	return 0;
}


char *gar_get_object_lbl(struct gobject *o)
{
	char buf[8192];
	off_t off;
	int type,rc;
	struct gar_subfile *gs = NULL;
	struct gpoint *gp;
	struct gpoly *gpl;

	type = L_LBL;
	switch (o->type) {
	case GO_POI:
	case GO_POINT:
		gp = o->gptr;
		if (gp->is_poi)
			type = L_POI;
		gs = gp->subdiv->subfile;
		off = gp->lbloffset;
		if (gp->type == 0)
			return NULL;
		break;
	case GO_POLYLINE:
	case GO_POLYGON:
		gpl = o->gptr;
		if (gpl->netlbl)
			type = L_NET;
		gs = gpl->subdiv->subfile;
		off = gpl->lbloffset;
		break;
	default:
		log(1, "Error unknown object type:%d\n", o->type);
		return NULL;
	}
	rc = gar_get_lbl(gs, off, type, (unsigned char *)buf, sizeof(buf));
	if (rc > 0) {
		log(15, "LBL: type:%d [%d] offset=%03X [%s]\n", o->type, type, off,buf);
		return strdup(buf);
	}
	return NULL;
}

int gar_get_object_intlbl(struct gobject *o)
{
	char *cp;
	int r = -1;
// FIXME don't strdup/free
	cp = gar_get_object_lbl(o);
	if (cp) {
		r = atoi(cp);
		free(cp);
	}
	return r;
}

int gar_object_subtype(struct gobject *o)
{
	struct gpoint *poi;
	u_int8_t ret = 0;
	switch (o->type) {
	case GO_POINT:
		break;
	case GO_POI:
		poi = o->gptr;
		if (poi->has_subtype)
			ret = poi->subtype;
		break;
	case GO_POLYLINE:
	case GO_POLYGON:
		break;
	default:
		log(1, "Error unknown object type:%d\n", o->type);
	}
	return ret;
}

