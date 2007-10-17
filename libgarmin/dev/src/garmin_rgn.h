#include "list.h"
#include "GarminTypedef.h"

#define SIGN2B(x) (((x)>=0x8000)   ? (x)-0x10000 : (x))
#define SIGN3B(x) (((x)>=0x800000) ? (x)-0x1000000 : (x))

struct gimg;
int gar_load_subfiles(struct gimg *g);

struct gar_subdiv {
	list_t l;
	struct gar_subfile *subfile;
	u_int32_t n;
	u_int16_t next;	// section of next level
	struct gar_subdiv *nextptr;
	unsigned terminate :1;
	u_int32_t rgn_start;
	u_int32_t rgn_end;
	unsigned haspoints :1,
		hasidxpoints :1,
		haspolylines :1,
		haspolygons;
	u_int32_t icenterlng;
	u_int32_t icenterlat;
	int north; // north boundary of area covered by this subsection
	int east;  // east boundary of area covered by this subsection
	int south; // south boundary of area covered by this subsection
	int west;  // west boundary of area covered by this subsection
	u_int32_t	shift;
	u_int32_t	level;
	void *strtable;	// TODO
	list_t lpoints;
	int pointidx;
	list_t lpois;
	list_t lpolylines;
	list_t lpolygons;
};

struct gar_maplevel {
	struct tre_map_level_t ml;
	list_t lsubdivs;
};

struct mlfilter {
	unsigned short type;
	unsigned char maxlevel;
};

struct region_def {
	int country;
	char *name;
};

struct city_def {
	int point_idx;
	int subdiv_idx;
	int region_idx;
	char *label;
};

struct zip_def {
	char *code;
};

struct gar_net_info;

struct gar_subfile {
	list_t l;
	struct gimg *gimg;
	char *mapid;
	struct gar_lbl_t *lbl;
	struct gar_net_info *net;
	int transparent:1,
	    basemap:1;
	double north;
	double east;
	double south;
	double west;
	double area;
	int subdividx;
	int nlevels;
	struct gar_maplevel **maplevels;
	char **countries;
	int ccount;
	struct region_def **regions;
	int rcount;
	struct city_def **cities;
	int cicount;
	struct zip_def **zips;
	int czips;
	struct mlfilter *fpoint;
	int nfpoint;
	struct mlfilter *fpolyline;
	int nfpolyline;
	struct mlfilter *fpolygone;
	int nfpolygone;
};

