int gar_init_lbl(struct gar_subfile *sub);
void gar_free_lbl(struct gar_subfile *sub);
int gar_get_lbl(struct gar_subfile *sub, off_t offset, int type, u_int8_t *buf, int buflen);
int gar_init_srch(struct gar_subfile *sub, int what);
void gar_free_srch(struct gar_subfile *sub);

struct gar_poi_properties;
void gar_free_poi_properties(struct gar_poi_properties *p);
struct gar_poi_properties *gar_get_poi_properties(struct gpoint *poi);
void gar_log_poi_properties(struct gar_subfile *sub, struct gar_poi_properties *p);


#define POI_STREET_NUM		(1<<0)
#define POI_STREET		(1<<1)
#define POI_CITY		(1<<2)
#define POI_ZIP			(1<<3)
#define POI_PHONE		(1<<4)
#define POI_EXIT		(1<<5)
#define POI_TIDE_PREDICT	(1<<6)
#define POI_UNKNOW		(1<<7)

struct gar_poi_properties {
	u_int8_t	flags;
	u_int32_t	lbloff;
	char		*number;
	u_int32_t	streetoff;
	unsigned short	cityidx;
	unsigned short	zipidx;
	char		*phone;
	u_int32_t	exitoff;
	u_int32_t	tideoff;
};

