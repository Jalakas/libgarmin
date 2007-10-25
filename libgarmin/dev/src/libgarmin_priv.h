#include <sys/types.h>
#include "list.h"
#include "GarminTypedef.h"
#define __USE_GNU
#include <math.h>

extern log_fn glogfn;
#define glog(g, l, x ...)	g->logfn(__FILE__, __LINE__, l, ## x)
#define log(l, x ...)	glogfn(__FILE__, __LINE__, l, ## x)

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
	char	*filetdb;
	int	tdbloaded;
	log_fn	logfn;
	list_t	limgs;
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
	u_int32_t lbl6off;
	u_int32_t lbl6size;
	u_int32_t addrshift;
	u_int32_t addrshiftpoi;
};

struct gimg {
	list_t l;
	struct gar *gar;
	char *file;
	int fd;
	ssize_t dataoffset;
	ssize_t blocksize;
	list_t lfatfiles;
	list_t lsubfiles;
	int basebits;
	int zoomlevels;
	int minlevel;
	int maxlevel;
	int mapsets;
	struct hdr_img_t hdr;
};

struct gpoint {
	list_t l;
	struct gar_subdiv *subdiv;
	u_int32_t n;
	u_int8_t type;
	u_int8_t subtype;
	u_int32_t lbloffset;
	struct gcoord c;

	unsigned is_poi :1,
		has_subtype :1;
};

struct gpoly {
	list_t l;
	struct gar_subdiv *subdiv;
	u_int32_t n;
	u_int8_t type;
	u_int32_t lbloffset;
	struct gcoord c;
	unsigned dir:1,
		netlbl:1,
		line:1,
		extrabit:1,
		scase:1;
	int npoints;
	struct gcoord *deltas;
};

void gar_log_file_date(int l, char *pref, struct hdr_subfile_part_t *h);
