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
	struct gar_graph *graph;
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
	struct gar_road *proad;
	struct gar_road *droad;
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

static int gar_get_road_nodes(struct gar_route *route, struct gar_road *road ,struct node *n, struct node **ret)
{
	int j;
	int rc = 0;
	struct roadptr *rp;
	for (j=0; j < n->narcs; j++) {
		rp = gar_cp_idx2road(n->cpoint, n->arcs[j].roadidx);
		if (!rp) {
			log(1, "arc:%d do not have rp\n", j);
		} else {
			if (get_u24(rp->off) == road->offset) {
				ret[rc++] = gar_get_node(route->graph, n->arcs[j].dest->offset);
			}
		}
	}
	return rc;
}

static struct node *gar_select_node(struct gar_route *route, struct gar_road *road,
				int pos)
{
	struct node *n, *n1, *n2;
	struct node *roadnodes[512];;
	struct node *own[512];
	int nrn = 1, nown;
	struct roadptr *rp;
	int j,k,p;
	if (!road->nod)
		return NULL;
	/* This is the first road node */
	n = gar_get_node(route->graph, road->nod->nodesoff);
	if (!n)
		return NULL;
	if (!gar_read_node(route->graph, NULL, n)) {
		log(1, "Failed to read node:%d\n",road->nod->nodesoff);
		return NULL;
	}
	roadnodes[0] = n;
	nown = gar_get_road_nodes(route, road, n, own);
	log(1, "Own from %d are %d\n", n->offset, nown);
	{
		int m;
		for (m=0; m < nown; m++)
			log(1, "%d\n", own[m]->offset);
	}
	for (j=0; j < n->narcs; j++) {
		rp = gar_cp_idx2road(n->cpoint, n->arcs[j].roadidx);
		if (!rp) {
			log(1, "arc:%d do not have rp\n", j);
		} else {
			if (get_u24(rp->off) == road->offset) {
				n1 = gar_get_node(route->graph, n->arcs[j].dest->offset);
				roadnodes[nrn++] = n1;
				if (!gar_read_node(route->graph, NULL, n1)) {
					log(1, "Failed to read node:%d\n",n1->offset);
					return NULL;
				}
				nown = gar_get_road_nodes(route, road, n1, own);
				log(1, "Own from %d are %d\n", n1->offset, nown);
				{
					int m;
					for (m=0; m < nown; m++)
						log(1, "%d\n", own[m]->offset);
				}
			}
		}
	}

	for (j=1; j < nrn; j++) {
		n1 = roadnodes[j];

		log(1, "%d\n", n1->offset);
		if (!gar_read_node(route->graph, NULL, n1)) {
			log(1, "Failed to read node:%d\n",n1->offset);
			return NULL;
		}
		for (k=0; k < n1->narcs; k++) {
			rp = gar_cp_idx2road(n1->cpoint, n1->arcs[k].roadidx);
			if (!rp) {
				log(1, "arc:%d do not have rp\n", k);
			} else {
				if (get_u24(rp->off) == road->offset) {
					int add = 1;
					n2 = gar_get_node(route->graph, n1->arcs[k].dest->offset);
					for (p = 0; p < nrn; p++) {
						if (roadnodes[p] == n2) {
							add = 0;
							break;
						}
					}
					if (add)
						roadnodes[nrn++] = n2;
				}
			}
		}
	}
	log(1, "Road nodes:%d\n", nrn);
	for (j=0; j < nrn; j++) {
		log(1, "%d\n", roadnodes[j]->offset);
	}
	if (pos < nrn) {
		log(1, "Selecting node %d: %d\n", pos, roadnodes[pos]->offset);
		return roadnodes[j];
	} else {
		log(1, "have %d but pos is %d\n", nrn, pos);
	}
	return n;
}

void gar_free_route(struct gar_route *route)
{
	if (route->graph)
		gar_free_graph(route->graph);
	free(route);
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

	if (!grr->route) {
		route = calloc(1, sizeof(*route));
		route->dstidx = -1;
		route->posidx = -1;
	} else {
		route = grr->route;
	}
	if (!route)
		return NULL;

	if (route->posmapid != grr->posmapid
		|| route->posobjid != grr->posobjid) {
		proad = gar_find_road_byid(gar, grr->posmapid, grr->posobjid);
		if (proad) {
			log(1, "FROM: pos=%d ROAD:",grr->posidx);
			gar_log_road_info(proad);
			route->proad = proad;
			route->posmapid = grr->posmapid;
			route->posobjid = grr->posobjid;
			route->posidx = -1;
		} else {
			log(1, "Can not find position road\n");
		}
	}

	if (route->dstmapid != grr->dstmapid
		|| route->dstobjid != grr->dstobjid) {
		droad = gar_find_road_byid(gar, grr->dstmapid, grr->dstobjid);
		if (droad) {
			log(1, "TO: pos=%d ROAD:",grr->dstidx);
			gar_log_road_info(droad);
			route->droad = droad;
			route->dstmapid = grr->dstmapid;
			route->dstobjid = grr->dstobjid;
			route->dstidx = -1;
		} else {
			log(1, "Can not find destination road\n");
		}
	}
	if (!route->proad || !route->droad) {
		gar_free_route(route);
		return NULL;
	}
	if (route->proad->sub != route->droad->sub) {
		log(1, "Can not route across subfiles yet\n");
		gar_free_route(route);
		return NULL;
	}

	if (!route->graph) {
		route->graph = gar_alloc_graph(route->proad->sub);
	}

	if (route->posidx != grr->posidx) {
		route->posidx = grr->posidx;
		route->pnode = gar_select_node(route, route->proad,
					route->posidx);
	}
	if (route->dstidx != grr->dstidx) {
		route->dstidx = grr->dstidx;
		route->dnode = gar_select_node(route, route->droad,
				route->dstidx);
	}

	return route;
}
