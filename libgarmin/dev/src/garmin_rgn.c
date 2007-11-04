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

#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_rgn.h"
#include "garmin_fat.h"
#include "garmin_lbl.h"
#include "garmin_net.h"
#include "garmin_subdiv.h"
#include "garmin_order.h"
#include "geoutils.h"
#include "array.h"

static int gar_load_points_overview(struct gar_subfile *sub, struct hdr_tre_t *tre)
{
	ssize_t off;
	struct gimg *g = sub->gimg;
	u_int8_t *prec;
	u_int8_t *rp;
	int rc,sz,i;

	if (!tre->tre6_offset) {
		log(11, "No polygon overview records\n");
		return 0;
	}
	off = gar_subfile_offset(sub, "TRE");
	off += tre->tre6_offset;
	if (lseek(g->fd, off, SEEK_SET) != off) {
		log(1, "Error can not seek to %zd\n", off);
		return -1;
	}
	rc = tre->tre6_size/tre->tre6_rec_size;
	log(11, "Have %d point overview records recsize=%d\n", rc, tre->tre6_rec_size);
	if (rc == 0)
		return 1;
	sub->fpoint = calloc(rc, sizeof(struct mlfilter));
	if (!sub->fpoint)
		return -1;
	sub->nfpoint = rc;
	prec = calloc(rc, (tre->tre6_rec_size));
	rc = read(g->fd, prec, tre->tre6_size);
	if (rc != tre->tre6_size) {
		log(1, "Error reading polygon overviews: %d\n", rc);
		free(prec);
		return -1;
	}
	rp = prec;
	sz = tre->tre6_size;
	i = 0;
	while (sz > 0) {
		log(15, "type:%02X maxlevel: %d subtype: %02X\n",
			*rp, *(rp+1), tre->tre6_rec_size > 2 ? *(rp+2) : 0);
		sub->fpoint[i].type = (*rp << 8) | (tre->tre6_rec_size > 2 ? *(rp+2) : 0);
		sub->fpoint[i].maxlevel = *(rp+1);
		i++;
		rp+= tre->tre6_rec_size;
		sz-= tre->tre6_rec_size;
	}
	free(prec);
	return 1;
}

static int gar_load_polygons_overview(struct gar_subfile *sub, struct hdr_tre_t *tre)
{
	ssize_t off;
	struct gimg *g = sub->gimg;
	u_int8_t *prec;
	u_int8_t *rp;
	int rc,sz,i;

	if (!tre->tre5_offset) {
		log(11, "No polygon overview records\n");
		return 0;
	}
	off = gar_subfile_offset(sub, "TRE");
	off += tre->tre5_offset;
	if (lseek(g->fd, off, SEEK_SET) != off) {
		log(1, "Error can not seek to %zd\n", off);
		return -1;
	}
	rc = tre->tre5_size/tre->tre5_rec_size;
	log(11, "Have %d polygon overview records recsize=%d\n", rc, tre->tre5_rec_size);
	if (rc == 0)
		return 1;
	sub->fpolygone = calloc(rc, sizeof(struct mlfilter));
	if (!sub->fpolygone)
		return -1;
	sub->nfpolygone = rc;
	prec = calloc(rc, (tre->tre5_rec_size));
	rc = read(g->fd, prec, tre->tre5_size);
	if (rc != tre->tre5_size) {
		log(1, "Error reading polygon overviews: %d\n", rc);
		free(prec);
		return -1;
	}
	rp = prec;
	sz = tre->tre5_size;
	i = 0;
	while (sz > 0) {
		log(15, "type:%02X maxlevel: %d unknown: %02X\n",
			*rp, *(rp+1), tre->tre5_rec_size > 2 ? *(rp+2) : 0);
		sub->fpolygone[i].type = *rp;
		sub->fpolygone[i].maxlevel = *(rp+1);
		i++;
		rp+= tre->tre5_rec_size;
		sz-= tre->tre5_rec_size;
	}
	free(prec);
	return 1;
}

static int gar_load_polylines_overview(struct gar_subfile *sub, struct hdr_tre_t *tre)
{
	ssize_t off;
	struct gimg *g = sub->gimg;
	u_int8_t *prec;
	u_int8_t *rp;
	int rc,sz,i;

	if (!tre->tre4_offset) {
		log(11, "No polylines overview records\n");
		return 0;
	}
	off = gar_subfile_offset(sub, "TRE");
	off += tre->tre4_offset;
	if (lseek(g->fd, off, SEEK_SET) != off) {
		log(1, "Error can not seek to %zd\n", off);
		return -1;
	}
	rc = tre->tre4_size/tre->tre4_rec_size;
	log(11, "Have %d polylines overview records recsize=%d\n", rc, tre->tre4_rec_size);
	if (rc == 0)
		return 1;
	sub->fpolyline = calloc(rc, sizeof(struct mlfilter));
	if (!sub->fpolyline)
		return -1;
	sub->nfpolyline = rc;
	prec = calloc(rc, (tre->tre4_rec_size));
	rc = read(g->fd, prec, tre->tre4_size);
	if (rc != tre->tre4_size) {
		log(1, "Error reading polylines overviews: %d\n", rc);
		free(prec);
		return -1;
	}
	rp = prec;
	sz = tre->tre4_size;
	i = 0;
	while (sz > 0) {
		log(15, "type:%02X maxlevel: %d unknown: %02X\n",
			*rp, *(rp+1), tre->tre4_rec_size > 2 ? *(rp+2) : 0);
		sub->fpolyline[i].type = *rp;
		sub->fpolyline[i].maxlevel = *(rp+1);
		rp+= tre->tre4_rec_size;
		sz-= tre->tre4_rec_size;
	}
	free(prec);
	return 1;
}

static int gar_load_ml_subdata(struct gar_subfile *sub, struct gar_maplevel *ml)
{
	struct gar_subdiv *gsub;
	unsigned int c,p = 0;
	unsigned int i = ga_get_count(&ml->subdivs);

	for (c = 0; c < i; c++) {
		gsub = ga_get(&ml->subdivs, c);
		if (gar_load_subdiv(sub, gsub)<0)
			return -1;
		p++;
	}
	log(11,"Loaded %d subdivs\n", p);
	return 0;
}

static int gar_load_subdivs_data(struct gar_subfile *sub)
{
	int i;
	int c = 0;
	for (i=0; i < sub->nlevels; i++) {
		if (gar_load_ml_subdata(sub, sub->maplevels[i]) < 0) {
			return -1;
		}
		c++;
	}
	log(19,"Loaded %d maplevels\n", c);
	return 0;
}

static void gar_parse_subdiv(struct gar_subdiv *gsub, struct tre_subdiv_t *sub)
{
	u_int32_t cx,cy;
	u_int32_t width, height;

	gsub->terminate = sub->terminate;
	gsub->rgn_start    = (*(u_int32_t*)(sub->rgn_offset)) & 0x00FFFFFF;
	log(15, "rgn_start: %04X terminate=%d\n", gsub->rgn_start, gsub->terminate);
	/* In the imgformat points and POIs are swapped*/
	gsub->haspoints  = !!(sub->elements & 0x10);
	gsub->hasidxpoints = !!(sub->elements & 0x20);
	gsub->haspolylines = !!(sub->elements & 0x40);
	gsub->haspolygons = !!(sub->elements & 0x80);

	cx = *(unsigned int *)sub->center_lng & 0x00FFFFFF;
	gsub->icenterlng = SIGN3B(cx);
	cy = *(unsigned int *)sub->center_lat & 0x00FFFFFF;
	gsub->icenterlat = SIGN3B(cy);
	width	= sub->width & 0x7fff;
	height	= sub->height;
	width  <<= gsub->shift;
	height <<= gsub->shift;

	gsub->north = gsub->icenterlat + height;
	gsub->south = gsub->icenterlat - height;
	gsub->east  = gsub->icenterlng + width;
	gsub->west  = gsub->icenterlng - width;
	if (gsub->south > gsub->north || gsub->west > gsub->east)
		log(1, "Invalid Subdiv North: %fC, East: %fC, South: %fC, West: %fC cx=%d cy=%d\n",
			GARDEG(gsub->north),
			GARDEG(gsub->east),
			GARDEG(gsub->south),
			GARDEG(gsub->west),
			gsub->icenterlng, gsub->icenterlat);
	log(11, "Subdiv North: %fC, East: %fC, South: %fC, West: %fC cx=%d cy=%d\n",
		GARDEG(gsub->north),
		GARDEG(gsub->east),
		GARDEG(gsub->south),
		GARDEG(gsub->west),
		gsub->icenterlng, gsub->icenterlat);
}

static struct gar_subdiv *gar_subdiv_alloc(struct gar_subfile *subf)
{
	struct gar_subdiv *gsub = calloc(1, sizeof(*gsub));
	if (gsub) {
		ga_init(&gsub->points, 1, 256);
		ga_init(&gsub->pois, 1, 256);
		ga_init(&gsub->polylines, 1, 512);
		list_init(&gsub->lpolygons);
		gsub->subfile = subf;
	}
	return gsub;
}

static ssize_t gar_get_rgnoff(struct gar_subfile *sub, ssize_t *l)
{
	ssize_t rgnoff = gar_subfile_offset(sub, "RGN");
	struct hdr_rgn_t rgnhdr;
	int rc;
	struct gimg *g = sub->gimg;
	if (lseek(g->fd, rgnoff, SEEK_SET) != rgnoff) {
		log(1, "Error can not seek to %zd\n", rgnoff);
		return 0;
	}
	rc = read(g->fd, &rgnhdr, sizeof(struct hdr_rgn_t));
	if (rc == sizeof(struct hdr_rgn_t)) {
		if (strncmp("GARMIN RGN", rgnhdr.hsub.type, 10)) {
			log(1, "RGN: Invalid header type: [%s]\n",
				rgnhdr.hsub.type);
			return 0;
		}
		*l = rgnhdr.length;
		return rgnoff+rgnhdr.offset;
	}
	return 0;
}

static int gar_load_ml_subdivs(struct gar_subfile *subf, struct gar_maplevel *ml, 
		struct hdr_tre_t *tre, struct gar_subdiv **gsub_prev,
		ssize_t rgnoff, int last)
{
	int i;
	struct gimg *g = subf->gimg;
	struct tre_subdiv_next_t subn;
	struct tre_subdiv_t subl;
	int s = sizeof(struct tre_subdiv_next_t);
	int s1 = sizeof(struct tre_subdiv_t);
	struct gar_subdiv *gsub = NULL;

	if (!last) {
		log(11, "Reading level:%d reading:%d\n",
				ml->ml.level, ml->ml.nsubdiv);
		for (i=0; i < ml->ml.nsubdiv; i++) {
			if (read(g->fd, &subn, s) != s) {
				log(1, "Error reading subdiv %d of maplevel:%d\n",
						i, ml->ml.level);
				return -1;
			}
			gsub = gar_subdiv_alloc(subf);
			if (!gsub) {
				log(1, "Can not allocate subdivision!\n");
				return -1;
			}
			gsub->n = subf->subdividx++;	// index in all subdivs
			gsub->next = subn.next;
			if (ml->ml.bits < 24)
				gsub->shift = 24 - ml->ml.bits;
			gar_parse_subdiv(gsub, &subn.tresub);
			log(15, "idx: %d start: %04X next_rgn: %04X(%d)\n",
			gsub->n, gsub->rgn_start,gsub->next,gsub->next);

			gsub->rgn_start   += rgnoff;
			// skip if this is the first entry
			if (*gsub_prev){
				(*gsub_prev)->rgn_end = gsub->rgn_start;
				log(15,"prev start:%04X end: %04X size: %d\n",
					(*gsub_prev)->rgn_start, (*gsub_prev)->rgn_end,
					(*gsub_prev)->rgn_end - (*gsub_prev)->rgn_start);
				if ((*gsub_prev)->rgn_start > (*gsub_prev)->rgn_end) {
					log(10,"invalid start and end\n");
				}
			}
			ga_append(&ml->subdivs, gsub);
			*gsub_prev = gsub;
			gsub->rgn_end = 0;
		}
	} else {	// level 0 are shorter
		log(11, "Reading level:%d reading:%d\n",
				ml->ml.level, ml->ml.nsubdiv);
		for (i=0; i < ml->ml.nsubdiv; i++) {
			if (read(g->fd, &subl, s1) != s1) {
				log(1, "Error reading subdiv %d of maplevel:%d\n",
						i, ml->ml.level);
				return -1;
			}
			gsub = gar_subdiv_alloc(subf);
			if (!gsub) {
				log(1, "Can not allocate subdivision!\n");
				return -1;
			}
			gsub->n = subf->subdividx++;	// index in the level not in all
			gsub->next = 0;
			if (ml->ml.bits < 24)
				gsub->shift = 24 - ml->ml.bits;
			gar_parse_subdiv(gsub, &subl);
			gsub->rgn_start   += rgnoff;
			gsub->rgn_end = 0;
			// skip if this is the first entry
			if (*gsub_prev){
				(*gsub_prev)->rgn_end = gsub->rgn_start;
			}
			ga_append(&ml->subdivs, gsub);
			*gsub_prev = gsub;
		}
	}
	return 0;
}

static int gar_load_subdivs(struct gar_subfile *sub, struct hdr_tre_t *tre)
{
	int i;
	ssize_t off;
	struct gar_subdiv *gsub_prev = NULL;
	ssize_t rgnlen = sub->rgnlen;
	ssize_t rgnoff = sub->rgnoffset;
	int last = 0;
	struct gimg *g = sub->gimg;

	off = gar_subfile_offset(sub, "TRE");
	off += tre->tre2_offset;
	if (lseek(g->fd, off, SEEK_SET) != off) {
		log(1, "Error can not seek to %zd\n", off);
		return -1;
	}

	for (i=0; i < sub->nlevels; i++) {
		if (i == sub->nlevels - 1)
			last = 1;
		if (gar_load_ml_subdivs(sub, sub->maplevels[i], tre, &gsub_prev, rgnoff, last) < 0) {
			// return -1;
		}
		ga_trim(&sub->maplevels[i]->subdivs);
	}
	if (gsub_prev) {
		gsub_prev->rgn_end = rgnoff + rgnlen;
//		log(10, "RGNEND: %04X\n",gsub_prev->rgn_end); 
	}

	return sub->nlevels;
}

static struct gar_maplevel *gar_alloc_maplevel(int base)
{
	struct gar_maplevel *ml;
	ml = calloc(1, sizeof(*ml));
	if (!ml)
		return NULL;
	ga_init(&ml->subdivs, base, 1024);
	return ml;
}

static int gar_load_maplevels(struct gar_subfile *sub, struct hdr_tre_t *tre)
{
	ssize_t off;
	struct gar_maplevel *ml;
	int s = sizeof(struct tre_map_level_t);
	u_int32_t nlevels;
	struct gimg *g = sub->gimg;
	int i;
	int totalsubdivs = 0;
	int base = 1;

	off = gar_subfile_offset(sub, "TRE");
	off += tre->tre1_offset;
	if (lseek(g->fd, off, SEEK_SET) != off) {
		log(1, "Error can not seek to %zd\n", off);
		return -1;
	}
	nlevels = tre->tre1_size / s;
	log(10, "Have %d levels\n", nlevels);
	sub->maplevels = calloc(nlevels, sizeof(struct tre_map_level_t *));
	if (!sub->maplevels) {
		log(1, "Error can not allocate map levels!\n");
		return -1;
	}
	sub->nlevels = nlevels;
	for (i = 0; i < nlevels; i++) { 
		ml = gar_alloc_maplevel(base);
		if (!ml) {
			log(1, "Error can not allocate map level!\n");
			return -1;
		}
		if (read(g->fd, &ml->ml, s) != s) {
			log(1, "Error reading map level %d\n", i);
			return -1;
		}
		log(10, "ML[%d] level=%d inherited=%d bits:%d nsubdiv=%d unkn=[%d %d %d]\n",
			i, ml->ml.level, ml->ml.inherited, ml->ml.bits, ml->ml.nsubdiv,
			ml->ml.bit4,ml->ml.bit5,ml->ml.bit6); 
		sub->maplevels[i] = ml;
		/* FIXME: The document states that
		 * all subdivs are indexed from 1
		 * BUT in the POI records:
		 * 'this two byte integer implies that there can not be
		 * more than 65535 subdivisions in a map level'
		 * and it's not clear in which map level
		 */
		totalsubdivs += ml->ml.nsubdiv;
		base += ml->ml.nsubdiv;
	}
	return totalsubdivs;
}

static struct gar_subfile *gar_alloc_subfile(struct gimg *g, char *mapid)
{
	struct gar_subfile *sub;
	sub = calloc(1, sizeof(*sub));
	if (!sub)
		return NULL;
	sub->gimg = g;
	sub->mapid = strdup(mapid);
	if (!sub->mapid) {
		free(sub);
		return NULL;
	}
	list_init(&sub->l);
	sub->subdividx = 1;
	return sub;
}

static void gar_free_region_def(struct region_def *rd)
{
	if (rd->name)
		free(rd->name);
	free(rd);
}

static void gar_free_city_def(struct city_def *cd)
{
	if (cd->label)
		free(cd->label);
	free(cd);
}

static void gar_free_zip_def(struct zip_def *zd)
{
	if (zd->code)
		free(zd->code);
	free(zd);
}

static void gar_free_subfile(struct gar_subfile *f)
{
	int i;
	if (f->mapid)
		free(f->mapid);
	if (f->maplevels) {
		for (i = 0; i < f->nlevels; i++) {
			free(f->maplevels[i]);
		}
		free(f->maplevels);
	}
	if (f->fpoint)
		free(f->fpoint);
	if (f->fpolyline)
		free(f->fpolyline);
	if (f->fpolygone)
		free(f->fpolygone);

	if (f->countries) {
		for (i = 0; i < f->ccount; i++) {
			if (f->countries[i])
				free(f->countries[i]);
		}
	}

	if (f->regions) {
		for (i = 0; i < f->rcount; i++) {
			if (f->regions[i])
				gar_free_region_def(f->regions[i]);
		}
		free(f->regions);
	}

	if (f->cities) {
		for (i = 0; i < f->cicount; i++) {
			if (f->cities[i])
				gar_free_city_def(f->cities[i]);
		}
		free(f->cities);
	}

	if (f->zips) {
		for (i = 0; i < f->czips; i++) {
			if (f->zips[i])
				gar_free_zip_def(f->zips[i]);
		}
		free(f->zips);
	}

	free(f);
}

static void gar_calculate_zoom_levels(struct gimg *g)
{
	int nbits[25];
	int j,i, fb = 0, lb = 0;
	struct gar_subfile *sub;
	struct gar_maplevel *ml;

	for (i = 0; i < 25; i++)
		nbits[i] = 0;
	list_for_entry(sub, &g->lsubfiles, l) {
		for (i = 0; i < sub->nlevels; i++) {
			ml = sub->maplevels[i];
			if (ml->ml.inherited)
				continue;
			nbits[ml->ml.bits] = 1;
		}
	}
	i = 0;
	for (j = 0; j < 25; j++) {
		if (nbits[j]) {
			i++;
			if (!fb)
				fb = j;
			lb = j;
		}
	}
	log(1, "Have %d levels base bits=%d bits=%d\n", i, fb, lb - fb);
	g->basebits = fb;
	g->zoomlevels = lb - fb;
	g->minlevel = 100;
	g->maxlevel = 0;

	list_for_entry(sub, &g->lsubfiles, l) {
		if (sub->basemap) {
			for (i = 0; i < sub->nlevels; i++) {
				ml = sub->maplevels[i];
				if (ml->ml.inherited)
					continue;
				if (g->minlevel > ml->ml.level)
					g->minlevel = ml->ml.level;
				if (g->maxlevel < ml->ml.level)
					g->maxlevel = ml->ml.level;
			}
		}
	}
	log(1, "Minlevel=%d Maxlevel=%d\n", g->minlevel, g->maxlevel);
}

static int gar_subfile_have_bits(struct gar_subfile *sub, int bits)
{
	struct gar_maplevel *ml;
	int i;

	for (i = 0; i < sub->nlevels; i++) {
		ml = sub->maplevels[i];
		if (ml->ml.inherited)
			continue;
		if (ml->ml.bits < bits)
			return 1;
	}

	return 0;
}

static int gar_check_basemap(struct gar_subfile *sub)
{
	if (1 || gar_subfile_have_bits(sub, 18)) {
		sub->basemap = 1;
		log(1, "%s selected as a basemap\n", sub->mapid);
		return 1;
	} else {
		log(1, "%s not used as basemap, no enough bits\n",
			sub->mapid);
	}
	return 0;
}
/*
 * Is it OKAY to have more than one basemap? 
 * may be if they do not overlap 
 * it's not required to have X bits to be a basemap
 */
static int gar_select_basemaps(struct gimg *g)
{
	struct gar_subfile *sub;
	int havebase = 0;
	int id;
	unsigned int minid = ~0;
	char *ptr, *eptr;

	log(1, "Selecting basemap ...\n");
	list_for_entry(sub, &g->lsubfiles, l) {
		if (*sub->mapid == 'I') {
			ptr = sub->mapid+1;
			id = strtol(ptr, &eptr, 16);
			if (!(*ptr && *eptr == '\0')) {
				havebase = gar_check_basemap(sub);
			}
			continue;
		}
		if (*sub->mapid >= 'A' && *sub->mapid <= 'Z') {
			havebase = gar_check_basemap(sub);
		}
	}
	if (havebase)
		return 1;

	list_for_entry(sub, &g->lsubfiles, l) {
		if (*sub->mapid < '0' || *sub->mapid > '9') {
			havebase = gar_check_basemap(sub);
		}
	}

	if (havebase)
		return 1;
	list_for_entry(sub, &g->lsubfiles, l) {
		id = atoi(sub->mapid);
		if (id < minid)
			minid = id;
	}
	list_for_entry(sub, &g->lsubfiles, l) {
		id = atoi(sub->mapid);
		if (id == minid) {
			havebase = gar_check_basemap(sub);
			if (havebase)
				break;
		}
	}
#if 0
	if (!havebase) {
		log(1, "Warning no basemap found guessing\n");
		list_for_entry(sub, &g->lsubfiles, l) {
			minid = atoi(sub->mapid);
			havebase = gar_check_basemap(sub);
			if (havebase)
				break;
		}
	}
#endif
	return minid;
}

int gar_load_subfiles(struct gimg *g)
{
	ssize_t off;
	int rc,j;
	struct hdr_tre_t tre;
	struct gar_subfile *sub;
	int32_t i32;
	char **imgs;
	int nimgs;
	char *cp;
	char buf[20];
	int mapsets=0;

	imgs = gar_file_get_subfiles(g, &nimgs);
	log(1, "Have %d mapsets\n", nimgs);
	if (!imgs)
		return -1;
	for (rc = 0; rc < nimgs; rc++) {
		strcpy(buf, imgs[rc]);
		cp = strchr(buf, '.');
		if (!cp)
			continue;
		*cp = '\0';
		log(1, "mapset: %s\n", buf);
	}

	for (j = 0; j < nimgs; j++) {
		strcpy(buf, imgs[j]);
		cp = strchr(buf, '.');
		if (!cp)
			continue;
		*cp = '\0';
		log(1, "Loading mapset id: %s\n", buf);
		sub = gar_alloc_subfile(g, buf);
		if (!sub)
			return -1;
		off = gar_subfile_offset(sub, "TRE");
		if (!off) {
			log(1, "Error can not find TRE file!\n");
			goto out_err;
		}
		if (lseek(g->fd, off, SEEK_SET) != off) {
			log(1, "Error can not seek to %zd\n", off);
			goto out_err;
		}
		rc = read(g->fd, &tre, sizeof(struct hdr_tre_t));
		if (rc != sizeof(struct hdr_tre_t)) {
			log(1, "Error can not read TRE header!\n");
			goto out_err;
		}
		if (strncmp("GARMIN TRE", tre.hsub.type, 10)) {
			log(1, "Invalid file type:[%s]\n", tre.hsub.type);
			goto out_err;
		}
		gar_log_file_date(1, "TRE Created", &tre.hsub);
		log(10, "TRE header: len= %u, TRE1 off=%u,size=%u TRE2 off=%u, size=%u\n",
			tre.hsub.length, tre.tre1_offset, tre.tre1_size,
			tre.tre2_offset, tre.tre2_size);
		log(19, "TRE ver=[%02X] flag=[%02X]\n",
			tre.hsub.byte0x0000000C,
			tre.hsub.flag);

		log(19, "3B-3E[%02X][%02X][%02X][%02X]\n",
			tre.byte0x0000003B_0x0000003E[0],
			tre.byte0x0000003B_0x0000003E[1],
			tre.byte0x0000003B_0x0000003E[2],
			tre.byte0x0000003B_0x0000003E[3]);
		log(19, "40-49[%02X][%02X][%02X][%02X]"
			     "[%02X][%02X][%02X][%02X]"
			     "[%02X][%02X]\n",
			tre.byte0x00000040_0x00000049[0],
			tre.byte0x00000040_0x00000049[1],
			tre.byte0x00000040_0x00000049[2],
			tre.byte0x00000040_0x00000049[3],
			tre.byte0x00000040_0x00000049[4],
			tre.byte0x00000040_0x00000049[5],
			tre.byte0x00000040_0x00000049[6],
			tre.byte0x00000040_0x00000049[7],
			tre.byte0x00000040_0x00000049[8],
			tre.byte0x00000040_0x00000049[9]);
		log(19, "54-57[%02X][%02X][%02X][%02X]\n",
			tre.byte0x00000054_0x00000057[0],
			tre.byte0x00000054_0x00000057[1],
			tre.byte0x00000054_0x00000057[2],
			tre.byte0x00000054_0x00000057[3]);
		log(19, "62-65[%02X][%02X][%02X][%02X]\n",
			tre.byte0x00000062_0x00000065[0],
			tre.byte0x00000062_0x00000065[1],
			tre.byte0x00000062_0x00000065[2],
			tre.byte0x00000062_0x00000065[3]);
		log(19, "70-73[%02X][%02X][%02X][%02X]\n",
			tre.byte0x00000070_0x00000073[0],
			tre.byte0x00000070_0x00000073[1],
			tre.byte0x00000070_0x00000073[2],
			tre.byte0x00000070_0x00000073[3]);
		
		if (tre.hsub.flag & 0x80){
			log(1, "File contains locked / encypted data. Garmin does not\n"
				"want you to use this file with any other software than\n"
				"the one supplied by Garmin.\n");
			goto out_err;
		}
		
		log(10, "POI_flags=%04X\n",tre.POI_flags);
		if (tre.POI_flags & (1<<1))
			log(10, "Show street before street number\n");
		if (tre.POI_flags & (1<<2))
			log(10, "Show Zip before city\n");
		sub->transparent = tre.POI_flags & 0x0001;
		i32 = (*(u_int32_t*)tre.northbound) & 0x00FFFFFF;
		sub->north = SIGN3B(i32);
		i32 = (*(u_int32_t*)tre.eastbound) & 0x00FFFFFF;
		sub->east = SIGN3B(i32);
		i32 = (*(u_int32_t*)tre.southbound) & 0x00FFFFFF;
		sub->south = SIGN3B(i32);
		i32 = (*(u_int32_t*)tre.westbound) & 0x00FFFFFF;
		sub->west = SIGN3B(i32);
		log(1, "Boundaries - North: %fC, East: %fC, South: %fC, West: %fC\n",
			GARDEG(sub->north),
			GARDEG(sub->east),
			GARDEG(sub->south),
			GARDEG(sub->west));
#warning calculate area  area= (pi/180)R^2 |sin(lat1)-sin(lat2)| |lon1-lon2|
		log(1, "Transparent: %s, Area: %.2f m\n", sub->transparent ? "Yes" : "No",
			sub->area);

		if (sub->east == sub->west){
			sub->east = -sub->east;
		}
		gar_init_lbl(sub);
		gar_init_net(sub);
		sub->rgnoffset = gar_get_rgnoff(sub, &sub->rgnlen);
		if (!sub->rgnoffset) {
			log(1, "Can not find RGN file\n");
			goto out_err;
		}

		if (gar_load_maplevels(sub, &tre)<0) {
			log(1, "Error loading map levels!\n");
			goto out_err;
		}
		gar_load_polylines_overview(sub, &tre);
		gar_load_polygons_overview(sub, &tre);
		gar_load_points_overview(sub, &tre);

		gar_load_subdivs(sub, &tre);
		gar_load_subdivs_data(sub);
		list_append(&sub->l, &g->lsubfiles);
		gar_init_srch(sub);
		mapsets++;
	}
	g->mapsets = mapsets;
	gar_select_basemaps(g);
	gar_calculate_zoom_levels(g);

	free(imgs);
	return 0;
out_err:
	if (imgs)
		free(imgs);
	gar_free_subfile(sub);
	return -1;
}

static struct gmap *gar_alloc_gmap(void)
{
	struct gmap *gm;
	gm = calloc(1, sizeof(*gm));
	if (!gm)
		return gm;
	/* Initialize default draw order for polygons */
	gm->draworder = calloc(1, sizeof(*gm->draworder));
	gar_init_draworder(gm->draworder, 4);
	gar_set_default_poly_order(gm->draworder);
	return gm;
}

void gar_free_gmap(struct gmap *g)
{
	free(g->subs);
	free(g);
}

static int gar_find_subs(struct gmap *files, struct gimg *g, struct gar_rect *rect)
{
	struct gar_subfile *sub;
	struct gar_rect r;
	char buf[256];
	int nf = 0, idx = 0;

	idx = files->lastsub;

	list_for_entry(sub, &g->lsubfiles, l) {
		if (rect) {
			r.lulat = sub->north; //DEG(sub->north);
			r.lulong = sub->west; //DEG(sub->west);
			r.rllat = sub->south; //DEG(sub->south);
			r.rllong = sub->east; //DEG(sub->east);
			sprintf(buf, "Checking %s", sub->mapid);
			gar_rect_log(12, buf, &r);
		}
		if (!rect || gar_rects_intersectboth(rect, &r)) {
			log(1, "Found subfile %d: [%s]\n", idx, sub->mapid);
			gar_rect_log(15, "subfile", &r);
			files->subs[idx] = sub;
			idx++;
			nf++;
			if (idx == files->subfiles )
				break;
		}
	}
	log(12, "Found %d subfiles\n", nf);
	files->lastsub = idx;
	return nf;
}

// public api
struct gmap *gar_find_subfiles(struct gar *gar, struct gar_rect *rect)
{
	struct gimg *g;
	struct gmap *files;
	struct gar_subfile **rsub;
	int fnd;

	files = gar_alloc_gmap();
	if (!files)
		return NULL;
	if (rect)
		gar_rect_log(1, "looking for", rect);

	list_for_entry(g, &gar->limgs,l) {
		// FIXME for more than one image
		files->zoomlevels = g->zoomlevels;
		files->basebits = g->basebits;
		files->minlevel = g->minlevel;
		files->maxlevel = g->maxlevel;
		rsub = realloc(files->subs, (files->subfiles + g->mapsets) * sizeof(struct gar_subfile *));
		if (rsub) {
			files->subs = rsub;
			files->subfiles += g->mapsets;
		} else {
			break;
		}
		fnd = gar_find_subs(files, g, rect);
		if (fnd) {
			if (rect) {
			log(15, "Found subfile for %f %f %f %f\n",
				rect->lulat, rect->lulong,
				rect->rllat, rect->rllong);
			}
		}
	}
	if (!files->lastsub) {
		gar_free_gmap(files);
		return NULL;
	}
	return files;
}
