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

#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

enum {
	L_LBL,
	L_NET,
	L_POI,
};

struct gcoord {
	int x;
	int y;
};

#define GO_POINT	1
#define GO_POI 		2
#define GO_POLYLINE	3
#define GO_POLYGON	4

struct gar_subfile;
struct gar_objdraworder;

struct gmap {
	struct gar_objdraworder *draworder;
	int subfiles;
	struct gar_subfile **subs;
	int zoomlevels;
	int basebits;
};

struct gobject {
	int type;
	void *gptr;
	void *priv_data;
	struct gobject *next;
};

struct gar_rect	{
	double lulat;
	double lulong;
	double rllat;
	double rllong;
};

struct gimg;

typedef void (*log_fn)(char *file, int line, int level, char *fmt, ...);
struct gar *gar_init(char *tbd, log_fn l);
void gar_free(struct gar *g);
int gar_load_tbd(char *tdb);
struct gimg *gar_img_load(struct gar *gar, char *file, int data);
struct gmap *gar_find_subfiles(struct gar *gar, struct gar_rect *rect);
void gar_free_gmap(struct gmap *g);
int gar_get_zoomlevels(struct gar_subfile *sub);

#define GO_GET_SORTED	(1<<0)
#define GO_GET_ROUTABLE	(1<<1)

struct gobject *gar_get_object(struct gar *gar, void *ptr);
int gar_get_objects(struct gmap *gm, int level, struct gar_rect *rect, 
			struct gobject **ret, int flags);
void gar_free_objects(struct gobject *g);
u_int8_t gar_obj_type(struct gobject *o);
int gar_get_object_position(struct gobject *o, struct gcoord *ret);
int gar_object_subtype(struct gobject *o);
int gar_get_object_dcoord(struct gmap *gm, struct gobject *o, int ndelta, struct gcoord *ret);
int gar_get_object_coord(struct gmap *gm, struct gobject *o, struct gcoord *ret);

int gar_get_object_deltas(struct gobject *o);
/* Get lbl as strdup'ed string */
char *gar_get_object_lbl(struct gobject *o);
int gar_get_object_intlbl(struct gobject *o);
int gar_object_get_draw_order(struct gobject *o);
int gar_fat_file2fd(struct gimg *g, char *name, int fd);

#define GARDEG(x) ((x) < 0x800000 ? (double)(x) * 360.0 / 16777216.0 : -(double)((x) - 0x100000) * 360.0 / 16777216.0)
#define GARRAD(x) ((x) < 0x800000 ? (double)(x) * TWOPI / 16777216.0 : -(double)((x) - 0x100000) * TWOPI / 16777216.0)

#ifdef __cplusplus
}
#endif
