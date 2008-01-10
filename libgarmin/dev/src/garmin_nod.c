/*
    Copyright (C) 2008  Alexander Atanasov      <aatanasov@gmail.com>

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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "GarminTypedef.h"
#include "garmin_fat.h"
#include "garmin_rgn.h"
#include "garmin_net.h"
#include "garmin_nod.h"

/* Node flags */
#define NF_ARCS			0x40		// have arcs and links
#define NF_16BITDELTAS		0x20		// 16 bit coordinate deltas
#define NF_RESTRICTIONS		0x10		// have restrictions
#define NF_BOUND		0x08		// bound/exit node
/* Last 3 bits unknown  */
#define	NF_UNK1			0x04
#define NF_UNK2			0x02
#define NF_UNK3			0x01

/* Arcs/Links flags */
#define LF_END			0x8000
#define LF_IDXPTR		0x4000
#define LF_NEWROAD		0x0080
#define LF_COLOR		0x0040
#define LF_CURVE		0x0020
#define LF_RC_MASK		0x0007		// road class

struct nod_roadptr {
	u_int24_t off;
	u_int8_t b1;
	u_int8_t b2;
} __attribute((packed));

/* central point */
struct central_point {
	u_int8_t	crestr;
	u_int24_t	lng;
	u_int24_t	lat;
	u_int8_t	croads;
	u_int8_t	cidxs;
} __attribute((packed));

static void gar_put_cpoint(struct cpoint *p);

#define DEBUGNODES
// #define DEBUG
#ifdef DEBUG
static void print_buf(char *pref, unsigned char *a, int s)
{
	char buf[4096];
	int i,sz = 0;
	for (i=0; i < s; i++) {
		sz += sprintf(buf+sz, "%02X ",a[i]);
	}
	log(11, "%s :%s\n", pref, buf);
}
#else
static void print_buf(char *pref, unsigned char *a, int s) {}
#endif

#ifdef DEBUGNODES
static void nodelogseq(struct node *node)
{
	FILE *fp;
	char buf[512];
	sprintf(buf, "nodes/path.txt");
	if (node->nodeid == 0)
		fp = fopen(buf, "w+");
	else
		fp = fopen(buf, "a+");
	if (!fp)
		return;
	fprintf(fp, "%d\n", node->offset);
	fclose(fp);
}

static void nodetrunklog(struct node *node)
{
	FILE *fp;
	char buf[512];
	sprintf(buf, "nodes/%d-%d.txt", node->offset,node->nodeid);
	fp = fopen(buf, "w");
	if (!fp)
		return;
	fclose(fp);
}

static void nodelogsource(struct node *node, unsigned char *cp, int l)
{
	FILE *fp;
	char buf[512];
	int i;
	int m = l < 40 ? l : 40; 
	nodelogseq(node);
	sprintf(buf, "nodes/%d-%d.txt", node->offset,node->nodeid);
	fp = fopen(buf, "a+");
	if (!fp)
		return;
	for (i = 0; i < m; i++) {
		fprintf(fp, "%02X ", *cp++);
	}
	fprintf(fp, "\n");
	fclose(fp);

}
static void nodelog(struct node *node, char *fmt, ...)
{
	FILE *fp;
	char buf[512];
	va_list ap;
	sprintf(buf, "nodes/%d-%d.txt", node->offset, node->nodeid);
	fp = fopen(buf, "a+");
	if (!fp)
		return;
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	fclose(fp);
}

#define LN(y...) nodelog(node,  ## y)
#define LB(x, y) log(11, "NOD data %s:%x\n", y, x)
#else
static void nodelogsource(struct node *node, unsigned char *cp, int l) {}
static void nodetrunklog(struct node *node) {}
#define LN(y...) do {} while(0)
#define LB(x, y) do {} while(0)

#endif

static struct grapharc*
gar_alloc_grapharc(struct node *dst, unsigned char ridx, unsigned char rc, unsigned color)
{
	struct grapharc *arc;
	arc = calloc(1, sizeof(*arc));
	if (!arc)
		return NULL;
	arc->dest = dst;
	arc->roadidx = ridx;
	arc->roadclass = rc;
	arc->islink = 1;
	arc->color = color;
	return arc;
}

static void
gar_update_grapharc(struct grapharc *arc, unsigned link, unsigned len, unsigned heading, char c1, char c2)
{
	arc->len = len;
	arc->heading = heading;
	arc->curve[0] = c1;
	arc->curve[1] = c2;
	arc->islink = link;
}

static void gar_free_grapharc(struct grapharc *arc)
{
	free(arc);
}

static struct node *
gar_alloc_node(u_int32_t offset)
{
	struct node *n;
	n = calloc(1, sizeof(*n));
	if (!n)
		return NULL;
	list_init(&n->l);
	list_init(&n->lc);
	n->offset = offset;
	nodetrunklog(n);
	return n;
}

static struct node *
gar_graph_add_node(struct gar_graph *graph, struct node *node)
{
	unsigned hash = NODE_HASH(node->offset);

	list_append(&node->l, &graph->lnodes[hash]);
	node->nodeid = graph->totalnodes;
	graph->totalnodes++;
	return node;
}

static struct node *
gar_find_node(struct gar_graph *graph, u_int32_t offset)
{
	struct node *n;
	unsigned hash = NODE_HASH(offset);

	list_for_entry(n, &graph->lnodes[hash], l) {
		if (n->offset == offset)
			return n;
	}

	return NULL;
}

struct node* 
gar_get_node(struct gar_graph *graph, u_int32_t offset)
{
	struct node *node;
	node = gar_find_node(graph, offset);
	if (node) {
//		log(11, "NOD Already read: %d\n", offset);
		return node;
	}
	node = gar_alloc_node(offset);
	if (node)
		gar_graph_add_node(graph, node);
	return node;
}

static void 
gar_free_node(struct node *n)
{
	int i;
	if (n->arcs)
		free(n->arcs);
	if (n->cpoint)
		gar_put_cpoint(n->cpoint);
	list_remove_init(&n->l);
	list_remove_init(&n->lc);
	free(n);
}


void gar_free_nod(struct gar_nod_info *nod)
{
	free(nod);
}

struct gar_nod_info *gar_init_nod(struct gar_subfile *sub)
{
	off_t off;
	int rc;
	struct gimg *gimg = sub->gimg;
	struct gar_nod_info *n;
	struct hdr_nod_t nod;

	off = gar_subfile_offset(sub, "NOD");
	if (!off) {
		log(11,"No NOD file\n");
		return NULL;
	}
	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "NOD: Error can not seek to %ld\n", off);
		return NULL;
	}
	rc = gread(gimg, &nod, sizeof(struct hdr_nod_t));
	if (rc != sizeof(struct hdr_nod_t)) {
		log(1, "NOD: Can not read header\n");
		return NULL;
	}
	if (strncmp("GARMIN NOD", nod.hsub.type,10)) {
		log(1, "NOD: Invalid header type: [%s]\n", nod.hsub.type);
		return NULL;
	}
	n = malloc(sizeof(*n));
	if (!n)
		return n;
	n->nodoff = off;
	n->nod1_offset = nod.nod1offset;
	n->nod1_length = nod.nod1length;
	n->cpalign = nod.cpalign;
	n->roadptrsize = nod.roadptrsize;
	n->nodbits = nod.nodbits;
	n->nod2_offset = nod.nod2offset;
	n->nod2_length = nod.nod2length;
	n->nod3_offset = nod.bondoffset;
	n->nod3_length = nod.bondlength;
	n->nod3_recsize = nod.bondrecsize;
	log(11, "nod hdrlen=%d\n", nod.hsub.length);
	log(11, "len nod1=%d, nod2=%d, nod3=%d\n",
			nod.nod1length, nod.nod2length, nod.bondlength);
	log(11, "off nod1=%d, nod2=%d, nod3=%d\n",
			nod.nod1offset, nod.nod2offset, nod.bondoffset);
	log(11, "hdr cpal %d b1 %0x b2 %0x 3 %0x 4 %0x, unk3=%0x, roadptrsize=%0x unk5=%0x zterm1=%x\n",
			nod.cpalign, nod.nodbits, nod.b2, nod.b3, nod.unknown3,
			nod.unknown3, nod.roadptrsize,
			nod.unknown5, nod.zeroterm1);
	if (nod.hsub.length > 63) {
		log(11, "nod bond2offset=%d len=%d u1off=%d len=%d u2off=%d len=%d\n",
			nod.bond2offset, nod.bond2lenght,
			nod.u1offset, nod.u1lenght,
			nod.u2offset, nod.u2lenght);
	}
	return n;
}

struct nod1_data {
	u_int8_t	b[12];
	// pointer to other nod1 data
} __attribute((packed));

struct nod_node {
	u_int8_t	b1;
	u_int24_t	c1;
	u_int24_t	c2;
	u_int8_t	b2;
	u_int8_t	b3;
};

static void *gar_read_nod1node(struct gar_subfile *sub, off_t offset, unsigned char idx)
{
	off_t off;
	struct nod_node n;
	u_int32_t lng;
	u_int32_t lat;

	off = 1 + idx + (offset >> sub->net->nod->cpalign);
	off <<= sub->net->nod->cpalign;
	if (glseek(sub->gimg, off, SEEK_SET) != off) {
		log(1, "NET: Error can not seek to %ld\n", off);
		return NULL;
	}
	if (gread(sub->gimg, &n, sizeof(n)) < 0)
		return NULL;
	lng = *(u_int32_t *)&n.c1 & 0xffffff;
	lat = *(u_int32_t *)&n.c2 & 0xffffff;
	log(11, "nod1 off %ld 1 %x c1 %f[%x] c2 %f[%x] 2 %x 3 %x\n",
		off, n.b1, GARDEG(lng), lng, GARDEG(lat), lat, n.b2, n.b3);
	return NULL;
}

static void * gar_read_nod1(struct gar_subfile *sub, off_t offset, int flag)
{
	struct gimg *gimg = sub->gimg;
	struct nod1_data nd;
	unsigned int x,y;
	int ci= 2;
	char buf[128];
	off_t o = sub->net->nod->nodoff + sub->net->nod->nod1_offset + offset;
	if (glseek(gimg, o, SEEK_SET) != o) {
		log(1, "NET: Error can not seek to %ld\n", o);
		return NULL;
	}
	if (gread(gimg, &nd, sizeof(nd)) < 0)
		return NULL;
	sprintf(buf, "nod1 %ld", offset);
	print_buf(buf, (unsigned char *)&nd, sizeof(nd));
	x = *(u_int16_t *)&nd.b[ci] & 0xffff;
	x <<= 16;
	x >>= 8;
	x = SIGN3B(x);
	y = *(u_int16_t *)&nd.b[ci+2] & 0xffff;
	y <<= 16;
	y >>= 8;
	y = SIGN3B(y);
	log(11, "nod1: %ld x=%d(%f) y=%d(%f)\n", offset,
		x, GARDEG(x), y, GARDEG(y));
	gar_read_nod1node(sub, offset+1, nd.b[0]);
	return NULL;
}
// the bitstream at the end
// if nodptr
// and next != 0
// bit count
// bitmap which deltas from the sd:idx are covered

struct nod_road_data {
	u_int8_t	flags;
	u_int24_t	nod1off;
	u_int16_t	bmlen;
} __attribute((packed));


void gar_free_road_nod(struct gar_road_nod *nod)
{
	free(nod);
}

struct gar_road_nod *gar_read_nod2(struct gar_subfile *sub, u_int32_t offset)
{
	struct gimg *gimg = sub->gimg;
	struct nod_road_data nrd;
	struct gar_road_nod *nr = NULL;
	u_int32_t n1 = -1;
	off_t o = sub->net->nod->nodoff + sub->net->nod->nod2_offset + offset;
	if (glseek(gimg, o, SEEK_SET) != o) {
		log(1, "NET: Error can not seek to %ld\n", o);
		return NULL;
	}
	if (gread(gimg, &nrd, sizeof(struct nod_road_data)) < 0)
		return NULL;
	if (HAVENODES(nrd.flags)) {
		int len = 0;
		n1 = *(u_int32_t*)nrd.nod1off & 0x00FFFFFF;
		if (nrd.bmlen)
			len = (7+nrd.bmlen)/8;
		nr = calloc(1, sizeof(*nr)+len);
		if (!nr)
			return NULL;
		nr->nodesoff = n1;
		nr->flags = nrd.flags;
		nr->bmlen = nrd.bmlen;
		gread(gimg, &nr->bitmap[0], len);
	} else {
		nr = calloc(1, sizeof(*nr));
		if (!nr)
			return NULL;
		nr->flags = nrd.flags;
	}
	log(11, "n2: %d sc %d rc %d havenodes:%d 7:%d nodes at %d bmlen %d bits\n",
		offset,
		SPEEDCLASS(nrd.flags),
		ROADTYPE(nrd.flags),
		HAVENODES(nrd.flags),
		CHARINFO(nrd.flags),
		n1, 
		nrd.bmlen);
	return nr;
}

static void gar_enqueue_node(struct gar_graph *graph, struct node *node)
{
	log(12, "NOD enqueue:%d %d\n", node->offset, node->complete);
	list_remove_init(&node->lc);
	list_append(&node->lc, &graph->lqueue);
}

static int gar_read_node(struct gar_graph *graph, struct node *from, struct node *node, int class)
{
	if (node->complete)
		return 1;
	return 0;
}

static int gar_process_queue(struct gar_graph *graph, int pos, int class)
{
	struct node *n;
	unsigned nread = 0;
	int reach = 0;
	while (!list_empty(&graph->lqueue)) {
		n = list_entry(graph->lqueue.n, struct node, lc);
		list_remove_init(&n->lc);
		if (n->class == class) {
			if (gar_read_node(graph, NULL, n, class) < 0) {
				// error
				log(1, "NOD Error reading node %d\n", n->offset);
				return -1;
			}
			if (pos) {
				n->posreach = 1;
			} else {
				n->dstreach = 1;
			}
			list_append(&n->lc, &graph->lnclass[class]);
			reach += (n->posreach&&n->dstreach);
			nread ++;
		} else if (n->class > class) {
			if (pos) {
				n->posreach = 1;
				list_append(&n->lc, &graph->lnposclassup[class]);
			} else {
				n->dstreach = 1;
				list_append(&n->lc, &graph->lndestclassup[class]);
			}
		} else if (n->class < class) {
			if (pos) {
				n->posreach = 1;
				list_append(&n->lc, &graph->lnposclassdown[class]);
			} else {
				n->dstreach = 1;
				list_append(&n->lc, &graph->lndestclassdown[class]);
			}
		}
	}
	return reach;
}

static int gar_load_pos_class(struct gar_graph *graph, int class)
{
	if (graph->pos->class == class) {
		list_append(&graph->pos->lc, &graph->lqueue);
	} else {
		struct node *n, *ns;
		list_for_entry_safe(n, ns, &graph->lnposclassup[class - 1], lc) {
			list_remove_init(&n->lc);
			list_append(&n->lc, &graph->lqueue);
		}
	}
	return gar_process_queue(graph, 1, class);
}

static int gar_load_dest_class(struct gar_graph *graph, int class)
{
	if (graph->dest->class == class) {
		list_append(&graph->dest->lc, &graph->lqueue);
	} else {
		struct node *n, *ns;
		list_for_entry_safe(n, ns, &graph->lndestclassup[class - 1], lc) {
			list_remove_init(&n->lc);
			list_append(&n->lc, &graph->lqueue);
		}
	}
	return gar_process_queue(graph, 0, class);
}

static int gar_load_next_class(struct gar_graph *graph)
{
	int connected = 0;
	int curclass;
	struct node *pos = graph->pos, *dest = graph->dest;

	curclass = pos->class;
	if (graph->dest) {
		if (pos->class < dest->class) {
			curclass = pos->class;
			while(curclass < dest->class) {
				connected = gar_load_pos_class(graph, curclass);
				if (connected)
					goto done;
				curclass++;
				if (curclass > 7)
					break;
			}
		} else if (pos->class > dest->class) {
			curclass = dest->class;
			while(curclass < pos->class) {
				connected = gar_load_dest_class(graph, curclass);
				if (connected)
					goto done;
				curclass++;
				if (curclass > 7)
					break;
			}
		} 
	}

	while (!connected) {
		connected = gar_load_pos_class(graph, curclass);
		if (connected)
			goto done;
		if (graph->dest) {
			connected = gar_load_dest_class(graph, curclass);
			if (connected)
				goto done;
		}
		curclass++;
		if (curclass > 7)
			break;
	}
	connected = 0;
done:
	if (connected <=0) {
		// failed to build the graph
	}
	return connected;
}

void gar_free_graph(struct gar_graph *g)
{
	int i;
	struct node *n, *ns;
	for (i = 0; i < NODE_HASH_TAB_SIZE; i++) {
		list_for_entry_safe(n, ns,&g->lnodes[i],l) {
			gar_free_node(n);
		}
	}
	if (!list_empty(&g->lqueue)) {
		log(1, "NOD error read queue not empty\n");
	}
	if (!list_empty(&g->lcpoints)) {
		log(1, "NOD error still have central points\n");
	}
	free(g);
}

static struct gar_graph *gar_alloc_graph(struct gar_subfile *sub)
{
	struct gar_graph *g;
	int i;

	g = calloc(1, sizeof(*g));
	if (!g)
		return g;
	g->sub = sub;
	// All central points
	list_init(&g->lcpoints);
	// All nodes
	for (i = 0; i < NODE_HASH_TAB_SIZE; i++)
		list_init(&g->lnodes[i]);
	list_init(&g->lqueue);
	for (i = 0; i < 8; i++) {
		list_init(&g->lnclass[i]);
		list_init(&g->lnposclassup[i]);
		list_init(&g->lnposclassdown[i]);
		list_init(&g->lndestclassup[i]);
		list_init(&g->lndestclassdown[i]);
	}
	return g;
};

struct gar_graph * gar_read_graph(struct gar_subfile *sub, int frclass, u_int32_t from, int dstclass, u_int32_t to)
{
	struct gar_graph *g;
	struct node *pos, *dest;

	g = gar_alloc_graph(sub);
	if (!g)
		return NULL;
	pos = gar_get_node(g, from);
	pos->class = frclass;
	g->pos  = pos;
	if (dstclass != 1000) {
		dest = gar_get_node(g, to);
		dest->class = dstclass;
		g->dest = dest;
	}
	g->valid = gar_load_next_class(g);
	return g;
}

int gar_update_graph(struct gar_graph *g , int frclass, u_int32_t from)
{
	// TBD
	// walk all nodes and reset pos/dst reachable
	// or use grpah version update when visiting
	// then reload what's needed
	// move freeing of unused(not pos and not dst reachable) to another function
	return 0;
}

int gar_graph2tfmap(struct gar_graph *g, char *filename)
{
	struct node *n;
	FILE *tfmap;
	int i,h,j=0;
	tfmap = fopen(filename, "w");
	if (!tfmap)
		return -1;
	for (h=0; h < NODE_HASH_TAB_SIZE; h++) {
		list_for_entry(n, &g->lnodes[h], l) {
			// FIXME
			fprintf(tfmap, "garmin:0x%x 0x%x type=rg_point debug=\"%d arcs\" label=\"%d-%d\"\n\n",
				n->c.x , n->c.y, n->narcs, n->nodeid, n->offset);
			for (i=0; i < n->narcs; i++) {
				struct grapharc *arc = &n->arcs[i];
				 if (arc->islink){
					 log(15, "NOD link to %d\n", arc->dest->offset);
					 continue;
				}
				fprintf(tfmap, "type=grapharc lenght=%d  head=%d label=\"%d->%d\"\n", 
					arc->len, arc->heading, n->offset, arc->dest->offset);
				fprintf(tfmap, "garmin:0x%x 0x%x\n",
					n->c.x, n->c.y);
				fprintf(tfmap, "garmin:0x%x 0x%x\n",
					arc->dest->c.x, arc->dest->c.y);
				fprintf(tfmap, "\n");
			}
			fprintf(tfmap, "\n");
			j++;
		}
	}
	fclose(tfmap);
	log(1, "NOD Total nodes written: %d\n", j);
	return j;
}

struct roadptr {
	u_int32_t off;
	// check size when accessing b1/b2
	u_int8_t b1;
	u_int8_t b2;
};

struct cpoint {
	list_t l;
	// FIXME need to add sub file if graph is shared 
	u_int32_t offset;
	unsigned refcnt;
	u_int8_t	crestr;
	u_int32_t	lng;
	u_int32_t	lat;
	u_int8_t	croads;	// roads count of unknown4=size records
	u_int8_t	cidxs;	// indexes ?
	u_int8_t	rpsize;
	u_int8_t	*roads;
	u_int8_t	*idx;
	u_int8_t	*restr;
};

static struct cpoint *gar_alloc_cp(void)
{
	struct cpoint *p;
	p = calloc(1, sizeof(*p));
	if (!p)
		return NULL;
	list_init(&p->l);
	return p;
}

static void gar_free_cpoint(struct cpoint *p)
{
	list_remove(&p->l);
	if (p->roads)
		free(p->roads);
	if (p->idx)
		free(p->idx);
	if (p->restr)
		free(p->restr);
	free(p);
}

static struct cpoint *gar_read_cp(struct gar_graph *graph, u_int32_t offset)
{
	struct gimg *gimg = graph->sub->gimg;
	struct cpoint *p;
	struct central_point n;
	int rc;
	unsigned i;

	glseek(gimg, offset, SEEK_SET);
	rc = gread(gimg, &n, sizeof(struct central_point));
	if (rc != sizeof(struct nod_node)) {
		log(1, "NOD: Error reading cpoint\n");
		return NULL;
	}
	p = gar_alloc_cp();
	if (!p)
		return NULL;
	if (n.croads) {
		i = n.croads*graph->sub->net->nod->roadptrsize;
		p->roads = malloc(i);
		if (!p->roads)
			goto out_err;
		p->rpsize = graph->sub->net->nod->roadptrsize;
		p->croads = n.croads;
		rc = gread(gimg, p->roads,i);
		if (rc != i)
			goto out_err;
		i = (n.cidxs) * 3;
		p->cidxs = n.cidxs;
		p->idx = malloc(i);
		if (!p->idx)
			goto out_err;
		rc = gread(gimg, p->idx,i);
		if (rc != i)
			goto out_err;
	}
	p->crestr = n.crestr;
	if (p->crestr) {
		p->restr = malloc(p->crestr);
		if (!p->restr)
			goto out_err;
		rc = gread(gimg, p->restr,p->crestr);
		if (rc != p->crestr)
			goto out_err;
	}
	p->lng = *(u_int32_t *) n.lng & 0xffffff;
	p->lat = *(u_int32_t *) n.lat & 0xffffff;
	p->offset = offset;
	log(11, "CPNT: %d at %f/%f\n", p->offset, GARDEG(p->lng), GARDEG(p->lat));
	return p;
out_err:
	gar_free_cpoint(p);
	return NULL;
}

static struct cpoint *gar_lookup_cpoint(struct gar_graph *graph, u_int32_t offset)
{
	struct cpoint *p;
	list_for_entry(p, &graph->lcpoints, l) {
		if (p->offset == offset)
			return p;
	}
	return NULL;
}

static struct cpoint *gar_get_cpoint(struct gar_graph *graph, u_int32_t offset,
		 int8_t idx)
{
	struct cpoint *p;
	u_int32_t off = 1 + idx + (offset >> graph->sub->net->nod->cpalign);
	off <<= graph->sub->net->nod->cpalign;
	off += graph->sub->net->nod->nod1_offset + graph->sub->net->nod->nodoff;
	p = gar_lookup_cpoint(graph, off);
	if (!p) {
		p = gar_read_cp(graph, off);
		if (!p)
			return NULL;
		list_append(&p->l, &graph->lcpoints);
	}
	log(15, "CPNT:%d\n", p->offset);
	p->refcnt++;
	return p;
}

static void gar_put_cpoint(struct cpoint *p)
{
	p->refcnt --;
	if (p->refcnt == 0) {
		gar_free_cpoint(p);
	}
}

static u_int32_t gar_cp_idx2off(struct cpoint *p, u_int8_t idx)
{
	u_int32_t i;
#ifdef DEBUG
	if (idx >= p->cidxs) {
		log(1, "NOD Error idx %d not valid max %d\n",
				idx, p->cidxs);
	}
#endif
	i = *(u_int32_t*)(p->idx+3*idx);
	return i & 0xFFFFFF;
}

static struct roadptr *gar_cp_idx2road(struct cpoint *p, u_int8_t idx)
{
	int i;
	i = idx*p->rpsize;
#ifdef DEBUG
	if (i > p->croads*p->rpsize) {
		log(1, "NOD Error roadidx %d not valid max %d\n",
				idx, p->croads*p->rpsize);
	}
#endif
	return (struct roadptr *)&p->roads[idx*p->rpsize];
}
