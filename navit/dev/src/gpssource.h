#include "coord.h"

struct gps_source {
	list_t l;
	unsigned int flags;
	char *name;
	unsigned int id;
	void *priv;
};

#define GPS_NONE	0
#define GPS_NO_FIX	1
#define GPS_2D		2
#define GPS_3D		3

struct gps_data {
	int fix;
	double	time;
	unsigned int sourceid;
	char *name;
	struct coord_geo geo;
	double speed;
	double direction;
	double height;
	double climb;
	int status;
	int sats;
	int sats_used;
};

int gps_register_source(struct gps_source *source);
void gps_unregister_source(struct gps_source *source);
void gps_source_data(struct gps_source *src, struct gps_data *data);
int gps_get_source_id(char *name);
void gps_set_source_active(int id);

struct gps_source *gps_source_alloc(char *name);
void gps_source_free(struct gps_source *s);

