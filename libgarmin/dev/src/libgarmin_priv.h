#include "config.h"
#include <sys/types.h>
#ifdef TARGET_WIN32CE
#include "win32support.h"
#endif
#include "list.h"
#include "GarminTypedef.h"
#define __USE_GNU
#include <math.h>
#ifndef TARGET_WIN32CE
#include <errno.h>
#endif

#ifndef O_NOATIME
#define O_NOATIME 0
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif
#define OPENFLAGS (O_RDONLY|O_NOATIME|O_BINARY)

extern log_fn glogfn;
extern int gar_debug_level;
// #define glog(g, l, x ...)	g->logfn(__FILE__, __LINE__, l, ## x)
#define log(l, x ...)	do {							\
				if (l <= gar_debug_level)				\
					glogfn(__FILE__, __LINE__, l, ## x);	\
			} while(0)

#define SIGN2B(x) (((x) < 0x8000)   ?  (x) : ((x)-0x10000))
#define SIGN3B(x) (((x) < 0x800000) ?  (x) : ((x)-0x1000000))

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})


//#define TWOPI (M_PI*2)
#define TWOPI 6.283185307179586476925287
#define DEG_TO_RAD(A) ((A) * (M_PI/180.0))
//#define DEG_TO_RAD(A) ((A) * 16777216.0)
//#define RAD_TO_DEG(x) ((x) * 16777216.0)
#define RAD_TO_DEG(x) ((double)(x) * ((double)180.0/M_PI))

#define DEG(x) ((x) < 0x800000 ? (double)(x) * 360.0 / 16777216.0 : -(double)((x) - 0x100000) * 360.0 / 16777216.0)
#define RAD(x) ((x) < 0x800000 ? (double)(x) * TWOPI / 16777216.0 : -(double)((x) - 0x100000) * TWOPI / 16777216.0)
//#define DEG(x) ((x) < 0x800000 ? (double)(x) * 360.0 / (1<<24) : -(double)((x) - 0x100000) * 360.0 / (1<<24))
//#define RAD(x) ((x) < 0x800000 ? (double)(x) * TWOPI / (1<<24) : -(double)((x) - 0x100000) * TWOPI / (1<<24))

struct gar {
	struct gar_config cfg;
	char	*tdbdir;
	int	tdbloaded;
	int	basebits;
	int	zoomlevels;
	log_fn	logfn;
	list_t	limgs;
	struct gmap *gmap;
};

struct bspfd;
typedef int (*decode_fn)(struct bspfd *bp, u_int8_t *out, ssize_t len);

struct gar_lbl_t {
	decode_fn decode;
	int bits;
	char codepage[512];
	u_int32_t offset;
	u_int32_t lbl1off;
	u_int32_t lbl1size;
	u_int8_t  lbl6_glob_mask;
	u_int32_t lbl6off;
	u_int32_t lbl6size;
	u_int32_t addrshift;
	u_int32_t addrshiftpoi;
};

struct gar_mdr;

struct gimg {
	list_t l;
	struct gar *gar;
	char *file;
	int fd;
	unsigned char xor;
	int is_nt;
	list_t lfatfiles;
	list_t lsubfiles;
	int tdbbasemap;
	int basebits;
	int zoomlevels;
	int minlevel;
	int maxlevel;
	int mapsets;
	double north;
	double east; 
	double south; 
	double west;
	struct gar_mdr *mdr;
};

struct gpoint {
	struct gar_subdiv *subdiv;
	u_int16_t n;
	u_int8_t type;
	u_int8_t subtype;
	u_int32_t lbloffset;
	struct gcoord c;

	unsigned is_poi :1,
		has_subtype :1,
		is_nt:1;
#ifdef DEBUG
	unsigned char *source;
	int slen;
#endif
};

struct gpoly {
	struct gar_subdiv *subdiv;
	u_int16_t n;
	u_int16_t type;
	u_int32_t lbloffset;
	struct gcoord c;
	unsigned dir:1,
		netlbl:1,
		line:1,
		extrabit:1,
		scase:1,
		valid:1,
		is_nt:1;
	int npoints;
	struct gcoord *deltas;
	unsigned char *nodemap;
#ifdef DEBUG
	unsigned char *source;
	int slen;
#endif
};

void gar_log_file_date(int l, char *pref, struct hdr_subfile_part_t *h);
int gar_img_load_dskimg(struct gar *gar, char *file, int tdbbase, int data,
			double north, double east, double south, double west);
ssize_t gread(struct gimg *g, void *buf, size_t count);
ssize_t gread_safe(struct gimg *g, void *buf, size_t count);
ssize_t gwrite(struct gimg *g, void *buf, size_t count);
off_t glseek(struct gimg *g, off_t offset, int whence);
int gopen(struct gimg *g);
int gclose(struct gimg *g);
struct gobject *gar_get_subfile_object_byidx(struct gar_subfile *sub,
				int sdidx, int oidx, int otype);
void gar_print_buf(char *pref, unsigned char *a, int s);

struct gar_subfile *gar_subfile_get_by_mapid(struct gar *gar, unsigned int mapid);
struct gar_subdiv *gar_find_subdiv_by_idx(struct gar_subfile *gsub,
                                                int fromlevel, int idx);
