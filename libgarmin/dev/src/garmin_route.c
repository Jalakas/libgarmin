/*
    Copyright (C) 2007,2008  Alexander Atanasov      <aatanasov@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301  USA
*/

#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "GarminTypedef.h"
#include "garmin_rgn.h"
#include "garmin_net.h"
#include "garmin_nod.h"
#include "garmin_lbl.h"

struct gar_route {
	struct gar_graph *g;
	struct node *pnode;
	unsigned int posmapid;
	unsigned int posobjid;
	int posidx;
	struct node *dnode;
	unsigned int dstmapid;
	unsigned int dstobjid;
	int dstidx;
	int route_flags;
	int vehicle_type;
};

#if 0
struct gar_route_request {
	struct gar_route *route;
	unsigned int posmapid;
	unsigned int posobjid;
	int posidx;
	unsigned int dstmapid;
	unsigned int dstobjid;
	int dstidx;
	int route_flags;
	int vehicle_type;
};
#endif

static struct gar_road *gar_find_road_byid(struct gar *gar, int mapid, int objid)
{
	struct gar_subfile *sub;
	struct gar_road *gr;
	int sdidx, idx;
	sub = gar_subfile_get_by_mapid(gar, mapid);
	if (!sub)
		return NULL;
	log(1, "Found subfile:[%d/%x]\n", sub->id, sub->id);
	sdidx = objid >> 16;
	idx = (objid >> 8) & 0xFF;
	gr = gar_get_road_by_id(sub, sdidx, idx);
	return gr;
}

struct gar_route *gar_route(struct gar *gar, struct gar_route_request *grr)
{
	struct gar_route *route = NULL;
	struct gar_road *proad;
	struct gar_road *droad;
	log(1, "RT: request %s from %d:%d:%d to %d:%d:%d\n", grr->route ?  "update" : "new",
			grr->posmapid,
			grr->posobjid,
			grr->posidx,
			grr->dstmapid,
			grr->dstobjid,
			grr->dstidx
			);
	proad = gar_find_road_byid(gar, grr->posmapid, grr->posobjid);
	if (proad) {
		log(1, "FROM: pos=%d ROAD:",grr->posidx);
		gar_log_road_info(proad);
	}
	droad = gar_find_road_byid(gar, grr->dstmapid, grr->dstobjid);
	if (droad) {
		log(1, "TO: pos=%d ROAD:",grr->dstidx);
		gar_log_road_info(droad);
	}
	if (grr->route) {
		// update route
	} else {
		// create new route
	}
	return route;
}
