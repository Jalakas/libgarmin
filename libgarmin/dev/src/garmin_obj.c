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
#include <stdio.h>
#include <string.h>
#include "list.h"
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_rgn.h"
#include "garmin_lbl.h"
#include "garmin_net.h"
#include "garmin_order.h"
#include "garmin_subdiv.h"
#include "geoutils.h"
#include "array.h"

static void gar_ref_subdiv(struct gobject *o)
{
	struct gpoint *gp;
	struct gpoly *gl;
	struct gar_search_res *sr;
	struct gar_subdiv *sd = NULL;

	switch (o->type) {
		case GO_POINT:
			gp = o->gptr;
			sd = gp->subdiv;
			break;
		case GO_POLYLINE:
		case GO_POLYGON:
			gl = o->gptr;
			sd = gl->subdiv;
			break;
		default:
			break;
	};
	if (sd)
		gar_subdiv_ref(sd);
}

static void gar_unref_subdiv(struct gobject *o)
{
	struct gpoint *gp;
	struct gpoly *gl;
	struct gar_subdiv *sd = NULL;

	switch (o->type) {
		case GO_POINT:
			gp = o->gptr;
			sd = gp->subdiv;
			break;
		case GO_POLYLINE:
		case GO_POLYGON:
			gl = o->gptr;
			sd = gl->subdiv;
		default:
			break;
	};
	if (sd)
		gar_subdiv_unref(sd);
}

static void gar_free_search_obj(struct gar_search_res *s)
{
	gar_subfile_unref(s->sub);
	free(s);
}

static struct gar_search_res *gar_alloc_search_obj(struct gar_subfile *sub, struct gar_search_res *from)
{
	struct gar_search_res *s;
	s = calloc(1, sizeof(*s));
	if (!s)
		return s;
	if (from)
		*s = *from;
	s->sub = sub;
	s->fileid = sub->id;
	gar_subfile_ref(sub);
	return s;
}

static struct gobject *gar_alloc_object(int type, void *obj)
{
	struct gobject *o;
	o = calloc(1, sizeof(*o));
	if (!o)
		return o;
	o->type = type;
	o->gptr = obj;
	gar_ref_subdiv(o);
	return o;
}

void gar_free_objects(struct gobject *g)
{
	struct gobject *gn;
	while (g) {
		gn = g->next;
		gar_unref_subdiv(g);
		if (g->type == GO_SEARCH)
			gar_free_search_obj(g->gptr);
		free(g);
		g = gn;
	}
}

static int gar_is_point_visible(struct gar_subfile *gsub, int level, struct gpoint *gp)
{
	int i;
	int type;
	if (!gsub->fpoint)
		return 1;	// no filter all is visible
	type = (gp->type << 8 ) || gp->subtype;
	for (i=0; i < gsub->nfpoint; i++) {
		if (gsub->fpoint[i].type == type) {
			if (gsub->fpoint[i].maxlevel >= level)
				return 1;
			else
				return 0;
		}
	}
	return 1;
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
	return 1;
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
	return 1;
}

static struct gar_subdiv *gar_find_subdiv_by_idx(struct gar_subfile *gsub, 
						int fromlevel, int idx)
{
	int i = fromlevel;
	struct gar_subdiv *sd;

	for (; i < gsub->nlevels; i++) {
		sd = ga_get_abs(&gsub->maplevels[i]->subdivs, idx);
		if (sd) {
			if (idx != sd->n)
				log(1, "Error subdiv found as idx:%d real is:%d\n",
					idx, sd->n);
			log(15, "Found in level %d\n", i);
			return sd;
		}
	}

	return NULL;
}

static struct gobject *gar_get_subdiv_objs(struct gar_subdiv *gsd, int *count, int level, int start, int routable)
{
	struct gobject *first = NULL, *o = NULL, *p;
	struct gpoint *gp;
	struct gpoly *gpoly;
	struct gar_subfile *gsub = gsd->subfile;
	int objs = 0;
	int cnt, i;

	log(15, "subdiv:%d cx=%d cy=%d north=%d west=%d south=%d east=%d\n",
		gsd->n, gsd->icenterlng, gsd->icenterlat, gsd->north, gsd->west,
		gsd->south, gsd->east);
	if (!gsd->loaded) {
		if (gar_load_subdiv_data(gsub, gsd) < 0)
			goto out_err;
	}
	if (start) {
		if (!routable) {
			cnt = ga_get_count(&gsd->polygons);
			for (i=0; i < cnt; i++) {
				gpoly = ga_get(&gsd->polygons, i);
				/* Do not return definition areas and backgrounds */
				if (gpoly->type == 0x4a || gpoly->type == 0x4b)
					continue;
				if (!gar_is_pgone_visible(gsub, level, gpoly))
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
		}
		cnt = ga_get_count(&gsd->polylines);
		for (i=0; i < cnt; i++) {
			gpoly = ga_get(&gsd->polylines, i);
			if (!gar_is_line_visible(gsub, level, gpoly))
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
		if (!routable) {
			cnt = ga_get_count(&gsd->points);
			for (i=0; i < cnt; i++) {
				gp = ga_get(&gsd->points, i);
				if (!gar_is_point_visible(gsub, level, gp))
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
		}
	}

	// Fixme we can go down to get more POIs
	// This code is dissabled
#if 0
	if (0 && gsd->next) {
		struct gar_subdiv *gss;
		struct gar_subdiv *gs;
		int i;

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
				p = gar_get_subdiv_objs(gs, &i, level, 0, routable);
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
#endif
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

static void *gar_subfile_get_byidx(struct gar_subfile *sub,
				int sdidx, int oidx, int otype)
{
	int i,j;
	struct gar_subdiv *sd;
	struct gar_maplevel *ml;
	void *obj = NULL;
	/* FIXME: This can be improved */
	for(i=0; i < sub->nlevels; i++) {
		ml = sub->maplevels[i];
		sd = ga_get_abs(&ml->subdivs, sdidx);
		if (sd) {
			if (!sd->loaded) {
				if (gar_load_subdiv_data(sub, sd) < 0)
					goto out;
			}
			switch(otype) {
			case GO_POINT:
				obj = ga_get_abs(&sd->points, oidx);
				goto out;
			case GO_POLYLINE:
				obj = ga_get_abs(&sd->polylines, oidx);
				goto out;
			case GO_POLYGON:
				obj = ga_get_abs(&sd->polygons, oidx);
				goto out;
			default:
				log(1, "Unknown object type: %d mapid:%s\n",
					otype, sub->mapid);
			}
			break;
		}
	}
	j = 0;
	for(i=0; i < sub->nlevels; i++)
		j += sub->maplevels[i]->ml.nsubdiv;
	log(1, "Can not find subdiv: %d have %d\n", sdidx, j);
	return NULL;
out:
	if (obj)
		return obj;
	else {
		if (otype == GO_POLYLINE) {
			int size = ga_get_count(&sd->polylines);
			log(1, "Can not find idx:%d sdidx:%d, have maxidx:%d\n",
						oidx, sdidx, size);
		}
		if (otype == GO_POINT) {
			int size = ga_get_count(&sd->points);
			log(1, "Can not find idx:%d sdidx:%d, have maxidx:%d\n",
						oidx, sdidx, size);
		}
	}
	return NULL;
}


struct gobject *gar_get_subfile_object_byidx(struct gar_subfile *sub,
				int sdidx, int oidx, int otype)
{
	int i,j;
	struct gar_subdiv *sd;
	struct gar_maplevel *ml;
	void *obj = NULL;
	/* FIXME: This can be improved */
	for(i=0; i < sub->nlevels; i++) {
		ml = sub->maplevels[i];
		sd = ga_get_abs(&ml->subdivs, sdidx);
		if (sd) {
			if (!sd->loaded) {
				if (gar_load_subdiv_data(sub, sd) < 0)
					goto out;
			}
			switch(otype) {
			case GO_POINT:
				obj = ga_get_abs(&sd->points, oidx);
				goto out;
			case GO_POLYLINE:
				obj = ga_get_abs(&sd->polylines, oidx);
				goto out;
			case GO_POLYGON:
				obj = ga_get_abs(&sd->polygons, oidx);
				goto out;
			default:
				log(1, "Unknown object type: %d mapid:%s\n",
					otype, sub->mapid);
			}
			break;
		}
	}
	j = 0;
	for(i=0; i < sub->nlevels; i++)
		j += sub->maplevels[i]->ml.nsubdiv;
	log(1, "Can not find subdiv: %d have %d\n", sdidx, j);
	return NULL;
out:
	if (obj)
		return gar_alloc_object(otype, obj);
	else {
		if (otype == GO_POLYLINE) {
			int size = ga_get_count(&sd->polylines);
			log(1, "Can not find idx:%d sdidx:%d, have maxidx:%d\n",
						oidx, sdidx, size);
		}
		if (otype == GO_POINT) {
			int size = ga_get_count(&sd->points);
			log(1, "Can not find idx:%d sdidx:%d, have maxidx:%d\n",
						oidx, sdidx, size);
		}
	}
	return NULL;
}

struct gobject *gar_get_object_by_id(struct gar *gar, unsigned int mapid,
					unsigned int objid)
{
	unsigned int sdidx;
	unsigned int otype;
	unsigned int oidx;
	struct gimg *g;
	struct gar_subfile *sub;
	struct gobject *go = NULL;
	sdidx = objid >> 16;
	otype = objid & 0xFF;
	oidx = (objid >> 8) & 0xFF;
	log(1, "Looking for sdidx: %d otype:%d oidx: %d in %d\n",
		sdidx, otype, oidx, mapid);
	list_for_entry(g, &gar->limgs,l) {
		list_for_entry(sub, &g->lsubfiles, l) {
			if (sub->id == mapid) {
				if (!sub->loaded) {
					if (gar_load_subfile_data(sub)<0)
						return NULL;
				}
				go = gar_get_subfile_object_byidx(sub,
						sdidx, oidx, otype);
				break;
			}
		}
	}
	return go;
}

// FIXME: This is very very slow
// This function is obsolete now by gar_get_object_by_id
struct gobject *gar_get_object(struct gar *gar, void *ptr)
{
	struct gimg *g;
	struct gar_subfile *sub;
	struct gar_subdiv *gsd;
	struct gar_maplevel *ml;
	struct gpoint *gp;
	struct gpoly *gpoly;
	int c, i, j;
	int cnt, k;

	list_for_entry(g, &gar->limgs,l) {
		list_for_entry(sub, &g->lsubfiles, l) {
			for (i=0; i < sub->nlevels; i++) {
				ml = sub->maplevels[i];
				c = ga_get_count(&ml->subdivs);
				for (j = 0; j < c; j++) {
					gsd = ga_get(&ml->subdivs, j);
					cnt = ga_get_count(&gsd->polygons);
					for (k=0; k < cnt; k++) {
						gpoly = ga_get(&gsd->polygons, k);
						if (gpoly == ptr) {
							return gar_alloc_object(GO_POLYGON, gpoly);
						}
					}
					cnt = ga_get_count(&gsd->polylines);
					for (k=0; k < cnt; k++) {
						gpoly = ga_get(&gsd->polylines, k);
						if (gpoly == ptr) {
							return gar_alloc_object(GO_POLYLINE, gpoly);
						}
					}
					cnt = ga_get_count(&gsd->points);
					for (k=0; k < cnt; k++) {
						gp = ga_get(&gsd->points, k);
						if (gp == ptr) {
							return gar_alloc_object(GO_POINT, gp);
						}
					}
				}
			}
		}
	}
	return NULL;
}
static void gar_debug_objects(struct gobject *o);

/* TODO: Handle special chars  */
static int gar_match(char *needle, char *str, int type)
{
	switch (type) {
		case GM_EXACT:
			return !strcasecmp(str, needle);
		case GM_START:
			return !strncasecmp(str, needle, strlen(needle));
		case GM_ANY:
			return !!strstr(str, needle);
	}
	return 0;
}


static int gar_get_search_objects(struct gmap *gm, struct gobject **ret,
					struct gar_search *s)
{
	int i, rc = 0;
	int nsub;
	struct gar_search_res *so;
	struct gar_search_res *from = NULL;
	struct gar_subfile *gsub;
	struct gobject *first = NULL, *o = NULL, *ot;

	from = &s->fromres;
	if (from) {
		log(1, "Search from cid: %d rid: %d cid:%d\n",
				from->countryid, from->regionid,
				from->cityid);
	}
	switch (s->type) {
		case GS_COUNTRY:
			break;
		case GS_REGION:
			break;
		case GS_CITY:
			break;
		case GS_ZIP:
			break;
		case GS_ROAD:
			break;
		case GS_INTERSECT:
			log(1, "Intersection search not implemented\n");
			return 0;
			break;
		case GS_HOUSENUMBER:
			log(1, "House Number search not implemented\n");
			return 0;
			break;
		case GS_POI:
			log(1, "POI search not implemented\n");
			return 0;
			break;
		default:
			log(1, "Error unknow search type:%d\n", s->type);
			return 0;
	}
	for (nsub = 0; nsub < gm->lastsub; nsub++) {
		gsub = gm->subs[nsub];
		if (!gsub->loaded) {
			// FIXME: error handle
			// FIXME: Load only the sd-s that are in the selected level
			// FIXME: Load only if have enough bits
			gar_load_subfile_data(gsub);
		}
		gar_init_srch(gsub, 0);
		gar_init_srch(gsub, 1);

		switch (s->type) {
			case GS_COUNTRY:
				for (i = 1; i < gsub->ccount; i++) {
					if (gar_match(s->needle, gsub->countries[i], s->match)) {
						log(1, "Match: %s(%d)\n", gsub->countries[i],i);
						so = gar_alloc_search_obj(gsub, from);
						if (so) {
							so->countryid = i;
							o = gar_alloc_object(GO_SEARCH, so);
							if (o) {
								rc ++;
								if (first) {
									o->next = first;
									first = o;
								} else
									first = o;
							}
						}
					}
				}
				break;
			case GS_REGION:
				for (i = 1; i < gsub->rcount; i++) {
					if ((!from->countryid || from->countryid == gsub->regions[i]->country) &&
					gar_match(s->needle, gsub->regions[i]->name, s->match)) {
						log(1, "Match: %s(%d)\n", gsub->regions[i]->name, i);
						so = gar_alloc_search_obj(gsub, from);
						if (so) {
							so->countryid = gsub->regions[i]->country;
							so->regionid = i;
							o = gar_alloc_object(GO_SEARCH, so);
							if (o) {
								rc ++;
								if (first) {
									o->next = first;
									first = o;
								} else
									first = o;
							}
						}
					}
				}
				break;
			case GS_CITY:
				for (i = 1; i < gsub->cicount; i++) {
					char *lbl = gsub->cities[i]->label;
					if (from->regionid && gsub->cities[i]->region_idx != from->regionid)
						continue;
					ot = NULL;
					if (!lbl) {
						ot = gar_get_subfile_object_byidx(gsub, 
								gsub->cities[i]->subdiv_idx,
								gsub->cities[i]->point_idx, GO_POINT);
						if (ot)
							lbl = gar_get_object_lbl(ot);
					}
					log(15, "Match: %s %s\n", s->needle, lbl);
					if (lbl && gar_match(s->needle, lbl, s->match)) {
						log(1, "Match: %s(%d)\n", lbl, i);
						so = gar_alloc_search_obj(gsub, from);
						if (so) {
							so->countryid = gsub->regions[gsub->cities[i]->region_idx]->country;
							so->regionid = gsub->cities[i]->region_idx;
							so->cityid = i;
							if (ot) {
								struct gar_poi_properties *pr;
								pr = gar_get_poi_properties(ot->gptr);
								if (pr) {
									so->zipid = pr->zipidx;
									gar_free_poi_properties(pr);
								}
							}
							o = gar_alloc_object(GO_SEARCH, so);
							if (o) {
								rc ++;
								if (first) {
									o->next = first;
									first = o;
								} else
									first = o;
							}
						}
					} else {
						if (ot)
							gar_free_objects(ot);
					}
					if (!gsub->cities[i]->label && lbl)
						free(lbl);
				}
				break;
			case GS_ROAD:
				if (gsub->net) {
					struct gar_road *r;
					char buf[256];
					int i,j;
					gar_load_roadnetwork(gsub);
					for (i=0; i < ROADS_HASH_TAB_SIZE; i++) {
						list_for_entry(r, &gsub->net->lroads[i], l) {
							if (!r->sai || (from->cityid == 0 && from->regionid == 0 && from->zipid == 0) ||  gar_match_sai(r->sai, from->zipid, from->regionid, from->cityid, 0)) {
								for(j = 0; j < 4; j++) {
									if (r->labels[j]) {
										if (gar_get_lbl(gsub, r->labels[j], L_LBL, (unsigned char *)buf, sizeof(buf))) {
											if (gar_match(s->needle, buf, s->match)) {
											so = gar_alloc_search_obj(gsub, from);
											if (so) {
												log(10, "Found road: %s\n", buf);
												if (r->sai)
													gar_sai2searchres(r->sai, so);
												// FIXME: if we have only region
												// we can get the country
												// if we have city, we can get region
												// and country
												so->roadid = r->offset;
												o = gar_alloc_object(GO_SEARCH, so);
												if (o) {
													rc++;
													if (first) {
														o->next = first;
														first = o;
													} else
														first = o;
												}
											}
											}
										}
									}
								}
							}
						}
					}
				}
				break;
		}
//		gar_free_srch(gsub);
	}
	*ret = first;
	log(1, "Returning %d matches\n", rc);
	return rc;
}


/*
 XXX Make gar_get_objects_zoom and gar_get_objects_level
 XXX to work with zoom(bits) and with levels
 */

int gar_get_objects(struct gmap *gm, int level, void *select, 
			struct gobject **ret, int flags)
{
	struct gobject *first = NULL, *o = NULL, *p;
	struct gar_maplevel *ml;
	struct gar_subdiv *gsd;
	struct gar_subfile *gsub;
	int objs = 0;
	int bits,i,j, lvlobjs,k;
	int nsub = 0;
	int basebits = 6;
	int baselevel = 0;
	int sdcount;
	int routable = flags&GO_GET_ROUTABLE;
	int prio = -1;
	struct gar_rect *rect = select;

	gsub = gm->subs[0];
	if (!gsub)
		return -1;
	if (routable) {
		bits = gm->basebits+gm->zoomlevels;
		log(7, "Looking for roads at last level: %d bits\n",
		gm->basebits+gm->zoomlevels);
		rect = NULL;
	} else if (flags&GO_GET_SEARCH) {
		return gar_get_search_objects(gm, ret, select);
	} else {
		bits = level;
		log(7, "Level =%d  bits = %d subfiles:%d\n", level, bits, gm->lastsub);
	}

	if (rect) {
		log(15, "Rect: lulong=%f lulat=%f rllong=%f rllat=%f\n",
			rect->lulong, rect->lulat, rect->rllong, rect->rllat);
	}
	basebits = gm->basebits;
	baselevel = gm->minlevel;

	log(3, "Basemap bits:%d level = %d\n", basebits, baselevel);
	if (gm->lastsub == 1 && gm->subs[0]->basemap) {
		// Check for images where no detail is available 
		if (bits > gsub->maplevels[gsub->nlevels-1]->ml.bits)
			bits = gsub->maplevels[gsub->nlevels-1]->ml.bits;
	}
	for (nsub = gm->lastsub - 1; nsub >=0  ; nsub--) {
		gsub = gm->subs[nsub];
		if (prio != -1  && prio != gsub->drawprio) {
			break;
		}
		if (routable) {
			if (!gsub->have_net)
				continue;
		}
		log(3, "Loading %s basemap:%s\n", gsub->mapid, gsub->basemap ? "yes" : "no");
		if (!gsub->loaded) {
			// FIXME: error handle
			// FIXME: Load only the sd-s that are in the selected level
			// FIXME: Load only if have enough bits
			if (gsub->maplevels[0]->ml.inherited && 
				gsub->maplevels[0]->ml.bits <= bits)
					gar_load_subfile_data(gsub);
		}
		for (i = 0; i < gsub->nlevels; i++) {
nextlvl:
			ml = gsub->maplevels[i];
			if (ml->ml.inherited) {
				continue;
				if (gsub->basemap) {
					// if it's a basemap give what we have
					continue;
				} else {
					if (bits <= ml->ml.bits)
					// if it's detail and we are not reached
					// inheritance level skip it
						break;
					else
						continue;
				}
			}
//			if (ml->ml.level < baselevel)
//					continue;
			if (ml->ml.bits < bits) {
				continue;
			}
			log(3, "Loading level:%d bits:%d\n",
				ml->ml.level, ml->ml.bits);
			lvlobjs = 0;
			sdcount = ga_get_count(&ml->subdivs);
			for (k = 0; k < sdcount; k++) {
				gsd = ga_get(&ml->subdivs, k);
				if (rect && !gar_subdiv_visible(gsd, rect))
					continue;
				p = gar_get_subdiv_objs(gsd, &j, ml->ml.level, 1, routable);
				if (p) {
					log(15, "%s subdiv:%d gave %d objects\n",
						gsub->mapid, gsd->n, j);
					//gar_debug_objects(p);
					objs += j;
					lvlobjs += j;
					if (first) {
						o = p;
						while (o->next)
							o = o->next;
						o->next = first;
					}
					first = p;
					prio = gsub->drawprio;
				}
			}
			log(10, "Total objects:%d level: %d\n", objs, lvlobjs);
			if (0 && lvlobjs < 2) {
				// do this only if submap is transparent
				if (i+1 < gsub->nlevels) {
					i++;
					// FIXME: Free parasit objects
					goto nextlvl;
				}
			}
			break;
		}
	}
	if (!first) {
		log(1, "Error no objects found\n");
		/* 
		 * FIXME: For the case where we have no detailed maps
		 * for all areas, in that case show the last level from
		 * the basemap
		 */
	}
	if ((flags&GO_GET_SORTED) && gm->draworder)
		*ret = gar_order_objects(first, gm->draworder, 1);
	else
		*ret = first;
	return objs;
}

u_int16_t gar_obj_type(struct gobject *o)
{
	u_int16_t ret = 0;
	switch (o->type) {
	case GO_POINT:
		{
		struct gpoint *point;
		point = o->gptr;
		if (point->has_subtype)
			ret |= point->subtype;
		ret |= point->type << 8;
		}
		break;
	case GO_POLYLINE:
	case GO_POLYGON:
		ret = ((struct gpoly *)o->gptr)->type;
		break;
	case GO_ROAD:
	case GO_SEARCH:
		ret = 1;
		break;
	default:
		ret = ~0;
		log(1, "Error unknown object type:%d\n", o->type);
	}
	return ret;
}

int gar_get_object_position(struct gobject *o, struct gcoord *ret)
{
	switch (o->type) {
	case GO_POINT:
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
	if  (o->type == GO_POINT)
		return 0;
	gp = o->gptr;
	if (gp->valid)
		return gp->npoints + 1;
	return 0;
}

int gar_get_object_coord(struct gmap *gm, struct gobject *o, struct gcoord *ret)
{
	struct gpoint *gp;
	struct gpoly *gl = NULL;
	if  (o->type == GO_POINT) {
		gp = o->gptr;
		ret->x = gp->c.x;
		ret->y = gp->c.y;
		return 1;
	} else if (o->type == GO_SEARCH) {
		struct gar_search_res *res;
		struct city_def *cd = NULL;
		struct gpoint *gp;
		res = o->gptr;
		if (res->roadid) {
			struct gar_road *rd = gar_get_road(res->sub, res->roadid);
			if (rd) {
			}
			return 0;
		}
		// give the most precise coord here
		if (res->cityid) {
			if (res->cityid < res->sub->cicount) {
				cd = res->sub->cities[res->cityid];
				if (cd->label) {
					cd = NULL;
				}
			}
		} 
		if (!cd && res->regionid) {
			int i;
			struct city_def *cd = NULL;
			for (i=1; i < res->sub->cicount; i++) {
				cd = res->sub->cities[i];
				if (!cd->label && cd->region_idx == res->regionid) {
					break;
				} else
					cd = NULL;
			}
		} 
		if (!cd && res->countryid) {
			int i;
			int rid = 0;
			struct city_def *cd = NULL;
			for (i=1; i < res->sub->rcount; i++) {
				if (res->sub->regions[i]->country == res->countryid) {
					rid = i;
					break;
				}
			}
			if (rid) {
				for (i=1; i < res->sub->cicount; i++) {
					cd = res->sub->cities[i];
					if (!cd->label && cd->region_idx == rid) {
						break;
					} else
						cd = NULL;
				}
			}
		}
		if (cd) {
			gp = gar_subfile_get_byidx(res->sub,
				cd->subdiv_idx, cd->point_idx,
				GO_POINT);
			if (gp) {
				ret->x = gp->c.x;
				ret->y = gp->c.y;
				return 1;
			} else {
				log(1, "Error can not find city object\n");
			}
		} else {
				log(1, "Error can not any find city\n");
			}
		return 0;
	} else if (o->type == GO_ROAD) {
		struct gar_road *rd = o->gptr;
		if (rd->rio_cnt) {
			struct gar_subdiv *sd;
			int idx, sdidx;
			if (!rd->sub->loaded) {
				if (gar_load_subfile_data(rd->sub) < 0)
					return 0;
			}
			idx = rd->ri[0] & 0xff;
			sdidx = rd->ri[0] >> 8;
			sdidx &= 0xFFFF;
			sd = ga_get_abs(&rd->sub->maplevels[rd->sub->nlevels - 1]->subdivs, sdidx);
			if (sd) {
				if (!sd->loaded) {
					if (gar_load_subdiv_data(rd->sub, sd) < 0)
						return 0;
				}
				gl = ga_get_abs(&sd->polylines, idx);
			} else {
				log(1, "Error can not find road idx/sd %d %d in level %d roadid:%d\n", idx, sdidx,rd->rio[0], rd->offset);
			}
		}
	} else {
		gl = o->gptr;
	}
	if (!gl)
		return 0;
	ret->x = gl->c.x;
	ret->y = gl->c.y;
	return 1;
}

int gar_get_object_dcoord(struct gmap *gm, struct gobject *o, int ndelta, struct gcoord *ret)
{
	struct gpoly *gp;
	if  (o->type == GO_POINT)
		return 0;
	gp = o->gptr;
	if (ndelta < gp->npoints) {
		*ret = gp->deltas[ndelta];
		return 1;
	}
	return 0;
}

int gar_is_object_dcoord_node(struct gmap *gm, struct gobject *o, int ndelta)
{
	struct gpoly *gp;
	if  (o->type == GO_POINT)
		return 0;
	gp = o->gptr;
	if (gp->nodemap && ndelta < gp->npoints) {
		return bm_is_set(gp->nodemap, ndelta);
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
	struct gar_road *rd;

	type = L_LBL;
	switch (o->type) {
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
	case GO_ROAD: {
			int i,sz = 0;
			rd = o->gptr;
			for (i=0;i < 4; i++) {
				if (rd->labels[i]) {
					sz += gar_get_lbl(rd->sub, rd->labels[i], L_LBL,
					 (unsigned char *)buf+sz, sizeof(buf)-sz);
					strcat(buf, "/");
				}
			}
			if (sz)
				return strdup(buf);
			else
				return NULL;
		}
	default:
		log(1, "Error unknown object type:%d\n", o->type);
		return NULL;
	}
	rc = gar_get_lbl(gs, off, type, (unsigned char *)buf, sizeof(buf));
	if (rc > 0) {
		log(15, "LBL: type:%d [%d] offset=%03lX [%s]\n", o->type, type, off, buf);
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

int gar_object_mapid(struct gobject *o)
{
	struct gar_subdiv *sd = NULL;
	switch (o->type) {
	case GO_POINT:
		sd = ((struct gpoint *)o->gptr)->subdiv;
		break;
	case GO_POLYLINE:
	case GO_POLYGON:
		sd = ((struct gpoly *)o->gptr)->subdiv;
		break;
	case GO_ROAD:
		return ((struct gar_road *)o->gptr)->sub->id;
	case GO_SEARCH:
		return ((struct gar_search_res *)o->gptr)->sub->id;
	default:
		log(1, "Error unknown object type:%d\n", o->type);
	}
	if (sd)
		return sd->subfile->id;
	return -1;
}

int gar_object_flags(struct gobject *o)
{
	int ret = 0;
	struct gpoly *gp;
	switch (o->type) {
	case GO_POINT:
	case GO_POLYGON:
		break;
	case GO_POLYLINE:
		gp = o->gptr;
		if (gp->dir)
			ret |= F_ONEWAY;
		if (gp->extrabit)
			ret |= F_SEGMENTED;
		break;
	default:
		log(1, "Error unknown object type:%d\n", o->type);
	}
	return ret;
}

int gar_object_index(struct gobject *o)
{
	switch (o->type) {
	case GO_POINT:
		{
			struct gpoint *pt;
			pt = o->gptr;
			if (pt->n > 255) {
				log(1, "Error more than 255 points in a subdiv are not supported\n");
			}
			return (pt->subdiv->n << 16) | (pt->n << 8) | o->type;
		}
	case GO_POLYLINE:
	case GO_POLYGON:
		{
			struct gpoly *p;
			p = o->gptr;
			if (p->n > 255) {
				log(1, "Error more than 255 polygons/lines in a subdiv are not supported\n");
			}
			return (p->subdiv->n << 16) | (p->n << 8) | o->type;
		}
	case GO_ROAD:
			return ((struct gar_road *)o->gptr)->offset;
	case GO_SEARCH:
			{
				struct gar_search_res *s = o->gptr;
				if (s->roadid)
					return s->roadid;
				if (s->cityid)
					return s->cityid;
				if (s->regionid)
					return s->regionid;
				if (s->countryid)
					return s->countryid;
				return -1;
			}
	default:
		log(1, "Error unknown object type:%d\n", o->type);
	}
	return 0;
}

static void gar_log_source(u_int8_t *src, int len)
{
	char buf[len*3+1];
	int i,sz = 0;
	for (i=0; i < len; i++)
		sz += sprintf(buf+sz, "%02X ", src[i]);
	log(1, "SRC:[%s]\n", buf);
}

char *gar_object_debug_str(struct gobject *o)
{
	struct gpoint *gp;
	struct gpoly *gl;
	struct gar_subdiv *sd = NULL;
	char buf[1024];
	char extra[100];
	u_int32_t idx = 0;
	int type=0;
	struct gcoord c;
	unsigned char *src = NULL;
	int slen = 0;

	*extra = '\0';
	switch (o->type) {
	case GO_POINT:
		gp = o->gptr;
		type = gp->type << 8 | gp->subtype;
		c = gp->c;
		idx = gp->n;
		sd = gp->subdiv;
		src = gp->source;
		slen = gp->slen;
		break;
	case GO_POLYLINE:
	case GO_POLYGON:
		gl = o->gptr;
		type = gl->type;
		c = gl->c;
		idx = gl->n;
		sd = gl->subdiv;
		sprintf(extra, " d:%u sc:%u eb:%u dt:%d",
			gl->dir, gl->scase, gl->extrabit, gl->npoints);
		src = gl->source;
		slen = gl->slen;
		break;
	default:
		return NULL;
	}
	if (src) {
		gar_log_source(src, slen);
	}
	if (sd) {
		snprintf(buf, sizeof(buf), "SF:%s SD:%d l=%d ot=%d idx=%d gt=0x%02X lng=%f lat=%f%s",
			sd->subfile->mapid, sd->n, sd->shift, o->type, idx, type, GARDEG(c.x), GARDEG(c.y), extra);
		return strdup(buf);
	}
	return NULL;
}

static void gar_debug_objects(struct gobject *o)
{
	char *cp;
	while(o) {
		cp = gar_object_debug_str(o);
		if (cp) {
			log(1, "%s\n", cp);
			free(cp);
		}
		o = o->next;
	}
}

unsigned int gar_srch_get_countryid(struct gobject *o)
{
	struct gar_search_res *s = o->gptr;
	if (o->type != GO_SEARCH)
		return 0;
	return s->countryid;
}

char *gar_srch_get_country(struct gobject *o)
{
	struct gar_subfile *sub;
	struct gar_search_res *s = o->gptr;
	unsigned id;
	if (o->type != GO_SEARCH)
		return 0;
	id = s->countryid;
	sub = s->sub;
	if (id < sub->ccount)
		return sub->countries[id];
	return NULL;
}

unsigned int gar_srch_get_regionid(struct gobject *o)
{
	struct gar_search_res *s = o->gptr;
	if (o->type != GO_SEARCH)
		return 0;
	return s->regionid;
}

char *gar_srch_get_region(struct gobject *o)
{
	struct gar_subfile *sub;
	struct gar_search_res *s = o->gptr;
	unsigned id;
	if (o->type != GO_SEARCH)
		return 0;
	id = s->regionid;
	sub = s->sub;
	if (id && id < sub->rcount)
		return sub->regions[id]->name;
	return NULL;
}

unsigned int gar_srch_get_cityid(struct gobject *o)
{
	struct gar_search_res *s = o->gptr;
	if (o->type != GO_SEARCH)
		return 0;
	return s->cityid;
}

char *gar_srch_get_city(struct gobject *o)
{
	struct gar_search_res *s = o->gptr;
	struct gobject *ot;
	unsigned id;
	char *lbl = NULL;
	if (o->type != GO_SEARCH)
		return NULL;
	id = s->cityid;
	if (!id)
		return NULL;
	if (s->sub->cities[id]->label)
		return strdup(s->sub->cities[id]->label);
	ot = gar_get_subfile_object_byidx(s->sub, 
			s->sub->cities[id]->subdiv_idx,
			s->sub->cities[id]->point_idx, GO_POINT);
	if (ot)
		lbl = gar_get_object_lbl(ot);
	return lbl;
}

unsigned int gar_srch_get_zipid(struct gobject *o)
{
	struct gar_search_res *s = o->gptr;
	if (o->type != GO_SEARCH)
		return 0;
	return s->zipid;
}

char *gar_srch_get_zip(struct gobject *o)
{
	struct gar_subfile *sub;
	struct gar_search_res *s = o->gptr;
	unsigned id;
	if (o->type != GO_SEARCH)
		return 0;
	id = s->zipid;
	sub = s->sub;
	if (id && id < sub->czips)
		return sub->zips[id]->code;
	return NULL;
}

unsigned int gar_srch_get_roadid(struct gobject *o)
{
	struct gar_search_res *s = o->gptr;
	if (o->type != GO_SEARCH)
		return 0;
	return s->roadid;
}

char *gar_srch_get_roadname(struct gobject *o)
{
	struct gar_search_res *s = o->gptr;
	struct gar_subfile *sub = s->sub;
	unsigned id;
	struct gar_road *rd;
	if (o->type != GO_SEARCH)
		return 0;
	id = s->roadid;
	rd = gar_get_road(sub, id);
	if (rd) {
		char buf[8192];
		int i,sz = 0;
		for (i=0;i < 4; i++) {
			if (rd->labels[i]) {
				if (sz) {
					strcat(buf, "/");
					sz++;
				}
				sz += gar_get_lbl(rd->sub, rd->labels[i], L_LBL,
				 (unsigned char *)buf+sz, sizeof(buf)-sz);
			}
		}
		if (sz) {
			log(1, "roadname:[%s]\n", buf);
			return strdup(buf);
		}
	} else {
		log(1, "Can not find road %d\n", id);
	}
	return strdup("");
}
