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


#define NODE_HASH_TAB_SIZE	256      /* must be power of 2 */
#define NODE_HASH(offset)	(((offset) *2654435769UL)& (NODE_HASH_TAB_SIZE-1))

struct gar_graph {
	struct gar_subfile *sub;
	struct node *pos;
	struct node *dest;
	// All nodes
	unsigned int totalnodes;
	unsigned valid;
	list_t lnodes[NODE_HASH_TAB_SIZE];
	list_t lqueue;
	list_t lnclass[8];
	/* Links to upper lower classes */
	list_t lnposclassup[8];
	list_t lndestclassup[8];
	list_t lnposclassdown[8];
	list_t lndestclassdown[8];
};

struct grapharc;

struct node {
	list_t l;
	list_t lc;
	unsigned nodeid;
	u_int32_t offset;
	struct gcoord c;
//	u_int32_t centralpoint;
//	u_int32_t roadptr;
//	struct nod_node_info ni;
	u_int8_t complete:1,
		posreach:1,
		dstreach:1,
		class:3;
	char narcs;
	struct grapharc *arcs[10];	// FIXME make this allocated
};

struct grapharc {
	struct node *dest;
	unsigned int len;
	u_int32_t roadptr;	// or struct gar_road *road;
	unsigned char roadclass;
	unsigned char heading;
	unsigned char curve[2];
	unsigned islink:1, color:1, have_curve:1;
};

struct gar_nod_info {
	off_t nodoff;
	u_int32_t nod1_offset;
	u_int32_t nod1_length;
	u_int8_t blockalign;
	u_int32_t nod2_offset;
	u_int32_t nod2_length;
	u_int32_t nod3_offset;
	u_int32_t nod3_length;
	u_int8_t nod3_recsize;
};

struct gar_nod_info *gar_init_nod(struct gar_subfile *sub);
void gar_free_nod(struct gar_nod_info *nod);

struct gar_graph *gar_read_graph(struct gar_subfile *sub, int fromclass, u_int32_t from, int toclass, u_int32_t to);
int gar_update_graph(struct gar_graph *g , int frclass, u_int32_t from);
struct node* gar_get_node(struct gar_graph *graph, u_int32_t offset);
int gar_graph2tfmap(struct gar_graph *g, char *filename);

struct gar_road_nod *gar_read_nod2(struct gar_subfile *sub, u_int32_t offset);
void gar_free_road_nod(struct gar_road_nod *nod);
