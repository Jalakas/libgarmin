#define NOTIFY_GPS	1

struct gps_data {
	unsigned int active;
	unsigned int sourceid;
	struct coord_geo geo;
	double speed;
	double direction;
	double height;
	int status;
	int sats;
	int sats_used;
};

#define NOTIFY_ROUTE	2

int listen_for(int what, notify_fn callback);
void notify(int what, void *data);
