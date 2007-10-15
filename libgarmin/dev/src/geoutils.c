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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "geoutils.h"

#ifdef STANDALONE
#undef log
#define log(x, y ...) fprintf(stdout, ## y)
#endif

void gar_rect_log(int l, char *pref,struct gar_rect *r)
{
	char buf[1024];
	if (!r) {
		return;
	}
	sprintf(buf,"%s%slulat=%f, lulong=%f, rllat=%f, rllong=%f\n",
		pref, pref ? ":" : "", r->lulat, r->lulong, r->rllat, r->rllong);
	log(l, buf);
}
int gar_rect_contains(struct gar_rect *r1, double lat, double lon)
{
	int ret = 0;
	if (r1->lulat <= lat && lat <= r1->rllat)
		ret = 3;
	else if (r1->lulat >= lat && r1->rllat >= lat)
		ret = 3;

	if (r1->lulong <= lon && lon <= r1->rllong)
		ret ++;
	if (r1->lulong >= lon && r1->rllong >= lon)
		ret ++;

#if 0
	if (ret !=4 ) {
		gar_rect_log(1,"rect", r1);
		log(1,"lat=%f, lon=%f, ret=%d\n", lat, lon, ret);
		if (r1->lulong <= lon)
			log(1, "lulong %f <= lon %f\n", r1->lulong, lon);
		if (r1->lulong >= lon)
			log(1, "lulong %f >= lon %f\n", r1->lulong, lon);
		if (r1->rllong <= lon)
			log(1, "rllong %f <= lon %f\n", r1->rllong, lon);
		if (r1->rllong >= lon)
			log(1, "rllong %f >= lon %f\n", r1->rllong, lon);
		
		
	} else {
		gar_rect_log(1,"contains rect", r1);
	}
#endif
	return ret ==  4;
}

int gar_rects_overlaps(struct gar_rect *r2, struct gar_rect *r1)
{
	if (r1->lulat >= r2->rllat)
		return 0;
	if (r1->rllat <= r2->lulat)
		return 0;
	if (r1->lulong <= r2->rllong)
		return 0;
	if (r1->rllong >= r2->lulong)
		return 0;

	return 1;
}

int gar_rects_intersect(struct gar_rect *r2, struct gar_rect *r1)
{
	if (gar_rect_contains(r2, r1->lulat, r1->lulong))
		return 1;
	if (gar_rect_contains(r2, r1->rllat, r1->rllong))
		return 1;
	if (gar_rect_contains(r2, r1->lulat, r1->rllong))
		return 1;
	if (gar_rect_contains(r2, r1->rllat, r1->lulong))
		return 1;

	return 0;
}

int gar_rects_intersectboth(struct gar_rect *r2, struct gar_rect *r1)
{
	return gar_rects_intersect(r2,r1) || gar_rects_intersect(r1,r2); 
}

#ifdef STANDALONE
/*
 North: 41.622133C
 West: 23.706822C 
 		  East: 23.917301C
 		  South: 41.395862C
 
 */
int main(int argc, char **argv)
{
	struct gar_rect r;
	struct gar_rect r1;
	int rc;
	r.lulat = 41.622133;
	r.lulong = 23.706822;
	r.rllat = 41.395862;
	r.rllong = 23.917301;

	gar_rect_log(1,"r", &r);
	rc = gar_rect_contains(&r, 41.612133, 23.806822);
	printf("contains: %d must be 1\n", rc);
	rc = gar_rect_contains(&r, 41.612133, 24.806822);
	printf("contains: %d must be 0\n", rc);
	rc = gar_rect_contains(&r, 42.612133, 23.806822);
	printf("contains: %d must be 0\n", rc);
	r1.lulat = 41.622133;
	r1.lulong = 23.706822;
	r1.rllat = 41.3958622;
	r1.rllong = 23.917301;
	gar_rect_log(1,"r1", &r1);
	printf("overlap: %d\n", gar_rects_overlaps(&r, &r1));
	printf("intersect: %d\n", gar_rects_intersect(&r, &r1));

	r1.lulat = 41.263315;
	r1.lulong = 24.003882;
	r1.rllat = 45.802112;
	r1.rllong = 22.292740;
	gar_rect_log(1,"r1", &r1);
	printf("overlap: %d\n", gar_rects_overlaps(&r, &r1));
	printf("intersect: %d\n", gar_rects_intersect(&r, &r1));

	if (argc > 1) {
		r1.lulat = atof(argv[1]);
		r1.lulong = atof(argv[2]);
		r1.rllat = atof(argv[3]);
		r1.rllong = atof(argv[4]);
		gar_rect_log(1,"r1", &r1);
		printf("overlap: %d\n", gar_rects_overlaps(&r, &r1));
		printf("intersect: %d\n", gar_rects_intersect(&r, &r1));
	}

	return 0;
}
#endif
