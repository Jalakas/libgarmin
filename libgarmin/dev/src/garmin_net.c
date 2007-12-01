#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_fat.h"
#include "garmin_rgn.h"
#include "garmin_net.h"
#include "garmin_lbl.h"

struct gar_nod_info {
	off_t nodoff;
	u_int32_t nod1_offset;
	u_int32_t nod1_length;
	u_int32_t nod2_offset;
	u_int32_t nod2_length;
	u_int32_t nod3_offset;
	u_int32_t nod3_length;
	u_int8_t nod3_recsize;
};

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
};

#define RFL_UNKNOWN0		(1<<0)
#define RFL_ONEWAY		(1<<1)
#define RFL_LOCKTOROAD		(1<<2)
#define RFL_UNKNOWN3		(1<<3)
#define RFL_STREETADDRINFO	(1<<4)
#define RFL_ADDRONRIGHT		(1<<5)
#define RFL_NODINFO		(1<<6)
#define RFL_MAJORHW		(1<<7)

struct street_addr_info {
	u_int8_t	flags;
	u_int8_t	*field1;
	u_int8_t	*field2;
	u_int8_t	*field3;
};

/* 
 * There is no bitstream at the end of this as the PDF says in the maps i have
 */
struct road_info {
	off_t labels[10];
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
	u_int32_t nod_offset;
};

static void gar_free_nod(struct gar_nod_info *nod)
{
	free(nod);
}

static struct gar_nod_info *gar_init_nod(struct gar_subfile *sub)
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
	log(11, "len b1 %0x b2 %0x 3 %0x 4 %0x, unk3=%0x, unk4=%0x unk5=%0x zterm1=%x\n",
			nod.b1, nod.b2, nod.b3, nod.b4,nod.unknown3, nod.unknown4,
			nod.unknown5, nod.zeroterm1);
	if (nod.hsub.length > 63) {
		log(11, "nod bond2offset=%d len=%d u1off=%d len=%d u2off=%d len=%d\n",
			nod.bond2offset, nod.bond2lenght,
			nod.u1offset, nod.u1lenght,
			nod.u2offset, nod.u2lenght);
	}
	return n;
}

static void print_buf(char *pref, unsigned char *a, int s)
{
	char buf[4096];
	int i,sz = 0;
	for (i=0; i < s; i++) {
		sz += sprintf(buf+sz, "%02X ",a[i]);
	}
	log(11, "%s :%s\n", pref, buf);
}

struct nod1_data {
	u_int8_t	b[12];
	// pointer to other nod1 data
} __attribute((packed));

static void * gar_read_nod1(struct gar_subfile *sub, off_t offset, int flag)
{
	struct gimg *gimg = sub->gimg;
	struct nod1_data nd;
	int x,y;
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
	x = *(u_int32_t *)&nd.b[ci] & 0xffffff;
	x = SIGN3B(x);
	y = *(u_int32_t *)&nd.b[ci+3] & 0xffffff;
	y = SIGN3B(y);
	log(11, "nod1: %ld x=%d(%f) y=%d(%f)\n", offset,
		x, GARDEG(x), y, GARDEG(y));
	return NULL;
}

static void * gar_read_nod2(struct gar_subfile *sub, off_t offset)
{
	struct gimg *gimg = sub->gimg;
	struct nod_road_data nrd;
	u_int32_t n1;
	int flag = 0;
	int valid = 1;
	off_t o = sub->net->nod->nodoff + sub->net->nod->nod2_offset + offset;
	if (glseek(gimg, o, SEEK_SET) != o) {
		log(1, "NET: Error can not seek to %ld\n", o);
		return NULL;
	}
	if (gread(gimg, &nrd, sizeof(struct nod_road_data)) < 0)
		return NULL;
	n1 = *(u_int32_t*)nrd.nod1off & 0x00FFFFFF;
	log(11, "n2: %ld rc %d:%d 1:%d 7:%d n1 %d nodes %d[%04X] b3 [%02X]\n",
		offset,
		SPEEDCLASS(nrd.flags),
		ROADTYPE(nrd.flags),
		NODTYPE(nrd.flags),
		CHARINFO(nrd.flags),
		n1, 
		nrd.nodes, nrd.nodes, nrd.b3);
	/* Some of this is a bool flag passed to read_nod1 
	 * flag is bit0 in b3
	 */
	flag = nrd.b3 & 1;
	/*
	 * if (flag)
	 * 	nod1size = 4 + 4*nrd.b3;
	 * else
	 * 	
	 * 	nod1 elements reference other nod1 elements!
	 * 	or are chained
	 * 	read_nod1
	 * 		read_one
	 * 			read_nod1
	 */
	if (!NODTYPE(nrd.flags)) {
		log(1, "NOD2: FIXME bit0\n");
		if (nrd.nodes) {
			if (nrd.b3) {
				
			}
		}
	}
	if (CHARINFO(nrd.flags)) {
		log(1, "NOD2: FIXME bit7\n");
		/* Have some more char (?) data before nod1ptr */
	}
	gar_read_nod1(sub, n1, flag);
	return NULL;
}

int gar_init_net(struct gar_subfile *sub)
{
	struct gimg *gimg = sub->gimg;
	off_t off;
	struct gar_net_info *ni;
	struct hdr_net_t net;
	int rc;

	off = gar_subfile_offset(sub, "NET");
	if (!off) {
		log(11,"No NET file\n");
		return 0;
	}
	log(11, "NET initializing ...\n");
	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "NET: Error can not seek to %ld\n", off);
		return -1;
	}
	rc = gread(gimg, &net, sizeof(struct hdr_net_t));
	if (rc != sizeof(struct hdr_net_t)) {
		log(1, "NET: Can not read header\n");
		return -1;
	}

	if (strncmp("GARMIN NET", net.hsub.type,10)) {
		log(1, "NET: Invalid header type: [%s]\n", net.hsub.type);
		return -1;
	}

	ni = malloc(sizeof(*ni));
	if (!ni)
		return -1;
	ni->netoff = off;
	ni->net1_offset = net.net1_offset;
	ni->net1_length = net.net1_length;
	ni->net1_addr_shift = net.net1_addr_shift;
	ni->net2_offset = net.net2_offset;
	ni->net2_length = net.net2_length;
	ni->net2_addr_shift = net.net2_addr_shift;
	ni->net3_offset = net.net3_offset;
	ni->net3_length = net.net3_length;
	ni->nod = gar_init_nod(sub);;
	sub->net = ni;
	log(11, "len net1=%d, net2=%d, net3=%d\n",
		ni->net1_length, ni->net2_length,
		ni->net3_length);
	return 1;
}

void gar_free_net(struct gar_subfile *sub)
{
	if (!sub->net)
		return;
	if (sub->net->nod)
		gar_free_nod(sub->net->nod);
	free(sub->net);
	sub->net = NULL;
}

off_t gar_net_get_lbl_offset(struct gar_subfile *sub, off_t offset, int idx)
{
	struct gimg *gimg = sub->gimg;
	off_t o;
	char buf[12];
	char *cp;
	int rc;
	u_int32_t i;

	if (!sub->net || idx > 3)
		return 0;

	o = sub->net->netoff + sub->net->net1_offset + (offset << sub->net->net1_addr_shift);
	if (glseek(gimg, o, SEEK_SET) != o) {
		log(1, "NET: Error can not seek to %ld\n", o);
		return 0;
	}
	rc = gread(gimg, buf, sizeof(buf));
	if (rc > 3) {
		cp = buf;
		i = *(u_int32_t*)cp;
		if (i&0x400000) {
			cp+=6;
			i = *(u_int32_t*)cp;
		}
		i &= 0x3FFFFF;
		return i;
	} else
		return 0;
}

static void gar_free_addr_info(struct street_addr_info *sai)
{
	if (sai->field1)
		free(sai->field1);
	if (sai->field2)
		free(sai->field2);
	if (sai->field3)
		free(sai->field3);
	free(sai);
}

static struct street_addr_info* gar_parse_addr_info(struct gar_subfile *sub)
{
	u_int8_t flags, size;
	u_int8_t fl;
	u_int8_t *f1 = NULL, *f2 = NULL, *f3 = NULL;
	struct street_addr_info *sai;
	struct gimg *gimg = sub->gimg;
	if (gread(gimg, &flags, sizeof(u_int8_t)) < 0)
		return NULL;
	fl = flags >> 2;
	fl &= 3;
	if (fl != 3) {
		if (fl == 0) {
			if (gread(gimg, &size, sizeof(u_int8_t)) < 0)
				return NULL;
			f1 = malloc(size+1);
			if (!f1)
				goto out_err;
			if (gread(gimg, &f1[1], size) != size)
				goto out_err;
			f1[0] = size;
		} else if (fl == 2) {
			if (sub->czips < 256)
				size = 1;
			else
				size = 2;
			f1 = malloc(size);
			if (!f1)
				goto out_err;
			if (gread(gimg, f1, size) != size)
				goto out_err;
		} else {
			unsigned char buf[3];
			log(1, "NET: Error f1 type=1\n");
			gread(gimg, buf, 3);
			log(1, "Data: [%02X][%02X]\n",
				buf[0], buf[1]);
		}
	}
	fl = flags >> 4;
	fl &= 3;
	if (fl != 3) {
		if (fl == 0) {
			if (gread(gimg, &size, sizeof(u_int8_t)) < 0)
				return NULL;
			f2 = malloc(size+1);
			if (!f2)
				goto out_err;
			if (gread(gimg, &f2[1], size) != size)
				goto out_err;
			f2[0] = size;
		} else if (fl == 2) {
			if (sub->cicount < 256)
				size = 1;
			else
				size = 2;
			f2 = malloc(size);
			if (!f2)
				goto out_err;
			if (gread(gimg, f2, size) != size)
				goto out_err;
		} else if (fl == 1) {
			log(1, "NET: Error f2 type=1\n");
			/* Not in the PDF, the guess is that its idx/sdidx */
			f2 = malloc(3);
			gread(gimg, f2, 3);
		}
	}
	fl = flags >> 6;
	fl &= 3;
	if (fl != 3) {
		if (fl == 0) {
			if (gread(gimg, &size, sizeof(u_int8_t)) < 0)
				return NULL;
			f3 = malloc(size+1);
			if (!f3)
				goto out_err;
			if (gread(gimg, &f3[1], size) != size)
				goto out_err;
			f3[0] = size;
		} else if (fl == 2) {
			if (sub->rcount < 256)
				size = 1;
			else
				size = 2;
			f3 = malloc(size);
			if (!f3)
				goto out_err;
			if (gread(gimg, f3, size) != size)
				goto out_err;
		} else {
			
			unsigned char buf[3];
			log(1, "NET: Error f3 type=%d\n", fl);
			gread(gimg, buf, 3);
			log(1, "Data: [%02X][%02X][%02X]\n",
				buf[0], buf[1], buf[2]);
		}
	}
	sai = malloc(sizeof(*sai));
	if (!sai)
		goto out_err;
	sai->flags = flags;
	sai->field1 = f1;
	sai->field2 = f2;
	sai->field3 = f3;
	return sai;
out_err:
	if (f1)
		free(f1);
	if (f2)
		free(f2);
	if (f3)
		free(f3);
	return NULL;
}

static void gar_log_road_info(struct gar_subfile *sub, struct road_info *ri)
{
	int i;
	int idx;
	int sdidx;
	unsigned char buf[2048];
	int sz;
	struct gobject *o;
	log(11, "Labels at %ld %ld %ld %ld\n",
		ri->labels[0],ri->labels[1],ri->labels[2],ri->labels[3]);
	if (ri->labels[0]) {
		gar_get_lbl(sub, ri->labels[0], L_LBL, buf, 1024);
		log(11, "L[0]=%s\n", buf);
	}
	if (ri->labels[1]) {
		gar_get_lbl(sub, ri->labels[1], L_LBL, buf, 1024);
		log(11, "L[1]=%s\n", buf);
	}
	if (ri->labels[2]) {
		gar_get_lbl(sub, ri->labels[2], L_LBL, buf, 1024);
		log(11, "L[2]=%s\n", buf);
	}
	if (ri->labels[3]) {
		gar_get_lbl(sub, ri->labels[3], L_LBL, buf, 1024);
		log(11, "L[3]=%s\n", buf);
	}

	if (ri->sr_cnt) {
		if (!sub->net->net2_length)
			log(1, "Error have segmented offsets but segments section is empty\n");
		log(11, "Segmented roads offsets:\n");
		for (i=0; i < ri->sr_cnt; i++)
			log(11, "Seg at:%d\n", ri->sr_offset[i]);
	}
	log(11, "road_flags=%d road_len=%d hnb=%d "
		"unk0:%d, oneway:%d, lock:%d, unk3:%d, ar:%d, mhw:%d\n",
		ri->road_flags, ri->road_len, ri->hnb,
		!!(ri->road_flags&RFL_UNKNOWN0),
		!!(ri->road_flags&RFL_ONEWAY),
		!!(ri->road_flags&RFL_LOCKTOROAD),
		!!(ri->road_flags&RFL_UNKNOWN3),
		!!(ri->road_flags&RFL_ADDRONRIGHT),
		!!(ri->road_flags&RFL_MAJORHW));

	sz = 0;
	for (i=0; i < ri->rio_cnt; i++) {
		sz += sprintf(buf + sz, "%d %d ", i, ri->rio[i]);
	}
	log(11, "segments per level: %s\n", buf);
	sz = 0;
	for (i=0; i < ri->ri_cnt; i++) {
		idx = ri->ri[i] & 0xff;
		sdidx = ri->ri[i] >> 8;
		sdidx &= 0xFFFF;
		// 8 bits idx, 16 bits subdiv
		sz += snprintf(buf + sz, sizeof(buf) - sz, "%d i:%d sd:%d ", i, idx, sdidx);
		/* this is the road on the differnet map levels */
		o = gar_get_subfile_object_byidx(sub, sdidx, idx, GO_POLYLINE);
		if (o) {
			if (i==0) {
				char *cp = gar_object_debug_str(o);
				if (cp) {
					log(11, "%s\n", cp);
					free(cp);
				}
			}
#if 0
			cp = gar_get_object_lbl(o);
			if (cp) {
				log(11, "LBL=%s\n", cp);
				free(cp);
			}
#endif
			gar_free_objects(o);
		} else {
			log(1, "NET: Error can not find object\n");
		}
	}
	log(11, "segments:%s\n", buf);
	if (ri->sai) {
		log(11, "Have street address info\n");
		// gar_log_sai(ri->sai);
	}
	if (ri->road_flags & RFL_NODINFO) {
		gar_read_nod2(sub, ri->nod_offset);
//		log(11, "nod data at %u\n", ri->nod_offset);
	}
}

static void gar_free_road_info(struct road_info *ri)
{
	if (ri->sr_offset)
		free(ri->sr_offset);
	if (ri->rio)
		free(ri->rio);
	if (ri->ri)
		free(ri->ri);
	if (ri->sai)
		gar_free_addr_info(ri->sai);
	free(ri);
}

static struct road_info *gar_parse_road_info(struct gar_subfile *sub, off_t offset)
{
	struct road_info *rd;
	struct gimg *gimg = sub->gimg;
	off_t o,o1;
	int lblidx = 0;
	off_t labels[10];
	char buf[4];
	u_int32_t segs[101];
	int segidx = 0;
	int rc;
	u_int32_t i,j;
	char seg = 0;
	u_int8_t flags;
	u_int32_t road_len;
	u_int8_t rio[25];
	unsigned ri_cnt = 0;
	unsigned rio_cnt = 0;
	u_int32_t *ri;
	u_int8_t tmp;
	u_int8_t hnb = 0;
	struct street_addr_info *sai = NULL;
	u_int32_t nodptr = 0;

	o = sub->net->netoff + sub->net->net1_offset + (offset << sub->net->net1_addr_shift);
	if (glseek(gimg, o, SEEK_SET) != o) {
		log(1, "NET: Error can not seek to %ld\n", o);
		return NULL;
	}
	while (42) {
		buf[0] = buf[1] = buf[2] = buf[3] = 0;
		rc = gread(gimg, buf, 3);
		if (rc != 3)
			return NULL;
		i = *(u_int32_t*)buf;
/* 		if (i&(1<<22)) {
			seg = 1;
//			continue;
		} */
		if (/*i&(1<<22)*/seg) {
			if (segidx < 100)
				segs[segidx++] = i & 0x7FFFFF;
			else {
				log(1, "NET: Error to many segments!\n");
				return NULL;
			}
//			seg = 0;
		} else {
			if (i&(1<<22)) {
				seg = 1;
//				continue;
			}
			if (i & 0x3FFFFF) {
				labels[lblidx++] = i & 0x3FFFFF;
				if (lblidx == 10) {
					log(1, "NET: Error to many labels!\n");
					return NULL;
				}
			}
		}
		if (i&(1<<23))
			break;
	}
	rc = gread(gimg, &flags, sizeof(u_int8_t));
	if (rc!=1)
		return NULL;
	rc = gread(gimg, buf, 3);
	if (rc != 3)
		return NULL;
	road_len = *(u_int32_t*)buf;
	while (42) {
		if (gread(gimg, &tmp, 1) < 0)
			return NULL;
		rio[rio_cnt] = tmp & 0x3f;
		ri_cnt += rio[rio_cnt];
		rio_cnt++;
		if (tmp & (1<<7))
			break;
		if (rio_cnt == 25) {
			log(1, "NET: Too many rios\n");
			break;
		}
	}
	ri = malloc(ri_cnt * sizeof(u_int32_t));
	if (!ri)
		return NULL;
	for (j=0; j < ri_cnt; j++) {
		rc = gread(gimg, buf, 3);
		if (rc != 3)
			return NULL;
		i = *(u_int32_t*)buf;
		ri[j] = i;
	}
	if (flags & RFL_STREETADDRINFO) {
		if (gread(gimg, &hnb, sizeof(u_int8_t)) < 0)
			return NULL;
		sai = gar_parse_addr_info(sub);
		if (!sai)
			return NULL;
	}
	if (flags & RFL_NODINFO) {
		if (gread(gimg, &tmp, sizeof(u_int8_t)) < 0)
			return NULL;
		if (tmp > 2)
			log(11, "NET: FIXME nod info:%d\n", tmp);
		if ((tmp & 3) == 1) {
			tmp = 2;
		} else if ((tmp & 3) == 2) {
			tmp = 3;
		} else if ((tmp & 3) == 3) {
			tmp = 1;
		} else {
			log(1, "NET: Unknow nod info:%d\n", tmp);
			tmp = 0;
//			return NULL;
		}
		if (tmp) {
			buf[0] = buf[1] = buf[2] = buf[3] = 0;
			if (gread(gimg, buf, tmp) < 0)
				return NULL;
			if (tmp == 3)
				nodptr = *(u_int32_t*)buf & 0xffffff;
			else if (tmp == 2)
				nodptr = *(u_int16_t*)buf & 0xffff;
			else if (tmp == 1)
				nodptr = *(u_int8_t*)buf & 0xff;
		}
	}
	rd = calloc(1,sizeof(*rd));
	if (!rd)
		return NULL;
	if (lblidx > 4) {
		for (i=0; i < lblidx; i++)
			log(11, "LBL:%d %ld\n", i, labels[i]);
	}
	for (i=0; i < lblidx; i++)
		rd->labels[i] = labels[i];
	if (segidx) {
		rd->sr_cnt = segidx; 
		rd->sr_offset = malloc(segidx * sizeof(rd->sr_offset));
		if (!rd->sr_offset) {
			free(rd);
			return NULL;
		} 
		for (i=0; i < segidx; i++)
			rd->sr_offset[i] = segs[i];
	}
	rd->road_flags = flags;
	rd->road_len = road_len;
	rd->rio_cnt = rio_cnt;
	rd->rio = malloc(rio_cnt * sizeof(u_int8_t));
	rd->ri_cnt = ri_cnt;
	rd->ri = ri;
	if (!rd->ri || !rd->rio) {
		free(rd->sr_offset);
		free(rd->ri);
		free(rd->rio);
		free(rd);
		return NULL;
	}
	for (i=0; i < rio_cnt; i++) {
		rd->rio[i] = rio[i];
	}
	for (i=0; i < ri_cnt; i++) {
		rd->ri[i] = ri[i];
	}
	rd->hnb = hnb;
	rd->sai = sai;
	rd->nod_offset = nodptr;
	o1 = glseek(gimg, 0, SEEK_CUR);
	log(11, "read %ld roadptr %ld\n",  o1-o, offset);
	return rd;
}

int gar_net_parse_sorted(struct gar_subfile *sub)
{
	unsigned char buf[4];
	int i, rc;
	unsigned int val;
	int lblidx, c = 0, p = 0;
	unsigned int roadptr;
	struct road_info *ri;
	off_t o;
	if (!sub->net->net3_offset) {
		log(1, "NET: No sorted roads defined\n");
		return 0;
	}
	gar_subfile_ref(sub);
	o = sub->net->netoff + sub->net->net3_offset;
	if (glseek(sub->gimg, o, SEEK_SET) != o) {
		log(1, "NET: Error can not seek to %ld\n", o);
		return 0;
	}
	for (i=0; i < sub->net->net3_length/3; i++) {
		glseek(sub->gimg, o, SEEK_SET);
		rc = gread(sub->gimg, buf, 3);
		if (rc < 0)
			break;
		o = glseek(sub->gimg, 0, SEEK_CUR);
		val = *(unsigned int *)buf;
		lblidx = val >> 21;
		lblidx &= 3;
		roadptr = val & 0x003FFFFF;
		log(11, "lblidx %d roadptr %d\n", lblidx, roadptr);
		ri = gar_parse_road_info(sub, roadptr);
		if (ri) {
			gar_log_road_info(sub, ri);
			gar_free_road_info(ri);
			p++;
		} else {
			log(1, "NET: Error parsing road info\n");
		}
		c++;
	}
	log(11, "Total %d roads, %d parsed\n", c, p);
	gar_subfile_unref(sub);
	return c;
}
