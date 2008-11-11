#define RFL_UNKNOWN0		(1<<0)
#define RFL_ONEWAY		(1<<1)
#define RFL_LOCKTOROAD		(1<<2)
#define RFL_UNKNOWN3		(1<<3)
#define RFL_STREETADDRINFO	(1<<4)
#define RFL_ADDRONRIGHT		(1<<5)
#define RFL_NODINFO		(1<<6)
/* This or some of the unknow flags is high congestion probability */
#define RFL_MAJORHW		(1<<7)


#define ROADS_HASH_TAB_SIZE	128      /* must be power of 2 */
#define ROAD_HASH(offset)	(((offset) * 40503)& (ROADS_HASH_TAB_SIZE-1))

struct gar_net_info {
	off_t netoff;
	u_int32_t net1_offset;
	u_int32_t net1_length;
	u_int8_t  net1_addr_shift;
	u_int32_t net2_offset;
	u_int32_t net2_length;
	u_int16_t net2_addr_shift;
	u_int32_t net3_offset;
	u_int32_t net3_length;
	struct gar_nod_info *nod;
	int roads_loaded;
	list_t lroads[ROADS_HASH_TAB_SIZE];
};

struct gar_subfile;

struct gar_road {
	list_t l;
	struct gar_subfile *sub;
	off_t offset;
	u_int32_t nod_offset;
	off_t labels[4];
	int sr_cnt;
	u_int32_t *sr_offset;
	u_int8_t road_flags;
	u_int8_t road_len;
	int rio_cnt;
	u_int8_t *rio;
	int ri_cnt;
	u_int32_t *ri;
	u_int8_t hnb;
	struct street_addr_info *sai;
	struct gar_road_nod *nod;
};


int gar_init_net(struct gar_subfile *sub);
void gar_free_net(struct gar_subfile *sub);
off_t gar_net_get_lbl_offset(struct gar_subfile *sub, off_t offset, int idx);
// int gar_net_parse_sorted(struct gar_subfile *sub);
int gar_load_roadnetwork(struct gar_subfile *sub);
struct gar_road *gar_get_road(struct gar_subfile *sub, off_t offset);
void gar_free_road(struct gar_road *ri);
int gar_match_sai(struct street_addr_info *sai, unsigned int zipid, unsigned int rid, unsigned int cid, unsigned int num);
void gar_sai2searchres(struct street_addr_info *sai, struct gar_search_res *res);
struct gar_road *gar_get_road_by_id(struct gar_subfile *sub, int sidx, int idx);
void gar_log_road_info(struct gar_road *ri);
