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

struct road_info {
	off_t labels[10];
	int sr_cnt;
	u_int32_t *sr_offset;
	u_int8_t road_flags;
	u_int8_t road_len;
	/* count for rio and ri */
	int ri_cnt;
	u_int8_t *rio;
	u_int32_t *ri;
	struct street_addr_info *sai;
	u_int32_t nod_offset;
};

int gar_init_net(struct gar_subfile *sub)
{
	struct gimg *gimg = sub->gimg;
	off_t off;
	struct gar_net_info *ni;
	struct hdr_net_t net;
	int rc;

	log(11, "NET initializing ...\n");
	off = gar_subfile_offset(sub, "NET");
	if (!off) {
		log(11,"No NET file\n");
		return 0;
	}
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
	sub->net = ni;
	return 1;
}

void gar_free_net(struct gar_subfile *sub)
{
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
		} else {
			unsigned char buf[3];
			log(1, "NET: Error f3 type=%d\n", fl);
			gread(gimg, buf, 3);
			log(1, "Data: [%02X][%02X]\n",
				buf[0], buf[1]);
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

static void gar_log_road_info(struct road_info *ri)
{
	int i;
	int idx;
	int sdidx;
	log(11, "Labels at %ld %ld %ld %ld\n",
		ri->labels[0],ri->labels[1],ri->labels[2],ri->labels[3]);
	log(11, "Segmented roads offsets:\n");
	for (i=0; i < ri->sr_cnt; i++)
		log(11, "Seg at:%d\n", ri->sr_offset[i]);
	log(11, "road_flags=%d road_len=%d\n",
		ri->road_flags, ri->road_len);
	log(11, "segments per level:\n");
	for (i=0; i < ri->ri_cnt; i++) {
		log(11, "%d %d\n", i, ri->rio[i]);
	}
	for (i=0; i < ri->ri_cnt; i++) {
		idx = ri->ri[i] & 0xff;
		sdidx = ri->ri[i] >> 8;
		sdidx &= 0xFFFF;
		// 8 bits idx, 16 bits subdiv
		log(11, "%d i:%d sd:%d\n", i, idx, sdidx);
	}
	if (ri->sai) {
		log(11, "Have street address info\n");
		// gar_log_sai(ri->sai);
	}
	if (ri->nod_offset)
		log(11, "nod data at %u\n", ri->nod_offset);
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
	off_t o;
	int lblidx = 0;
	off_t labels[10];
	char buf[4];
	u_int32_t segs[100];
	int segidx = 0;
	int rc;
	u_int32_t i,j;
	char seg = 0;
	u_int8_t flags;
	u_int32_t road_len;
	u_int8_t rio[100];
	unsigned ri_cnt = 0;
	u_int32_t ri[100];
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
		rc = gread(gimg, buf, 3);
		if (rc != 3)
			return NULL;
		i = *(u_int32_t*)buf;
		if (i&(1<<22)) {
			seg = 1;
//			continue;
		}
		if (/*i&(1<<22)*/seg) {
			if (segidx < 100)
				segs[segidx++] = i & 0x3FFFFF;
			else {
				log(1, "NET: Error to many segments!\n");
				return NULL;
			}
//			seg = 0;
		} else {
			if (i&(1<<22)) {
				seg = 1;
				continue;
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
		rio[ri_cnt++] = tmp & 0x3f;
		if (tmp & (1<<7))
			break;
	}
	for (j=0; j < ri_cnt; j++) {
		rc = gread(gimg, buf, 3);
		if (rc != 3)
			return NULL;
		i = *(u_int32_t*)buf;
		ri[j] = i;
	}
	if (flags & RFL_STREETADDRINFO) {
		log(11, "Parsing street address\n");
		if (gread(gimg, &hnb, sizeof(u_int8_t)) < 0)
			return NULL;
		sai = gar_parse_addr_info(sub);
		if (!sai)
			return NULL;
	}
	if (flags & RFL_NODINFO) {
		if (gread(gimg, &tmp, sizeof(u_int8_t)) < 0)
			return NULL;
		if ((tmp & 3) == 1) {
			tmp = 2;
		} else if ((tmp & 3) == 2) {
			tmp = 3;
		} else {
			tmp = 0;
			log(1, "NET: Unknow nod info:%d\n", tmp);
//			return NULL;
		}
		if (tmp) {
			buf[0] = buf[1] = buf[2] = buf[3] = 0;
			if (gread(gimg, buf, tmp) < 0)
				return NULL;
			if (tmp == 3)
				nodptr = *(u_int32_t*)buf;
			else
				nodptr = *(u_int16_t*)buf;
		}
	}
	rd = calloc(1,sizeof(*rd));
	if (!rd)
		return NULL;
	if (lblidx > 3) {
		for (i=0; i < lblidx; i++)
			log(11, "LBL:%d %ld\n", i, labels[i]);
	}
	for (i=0; i < lblidx; i++)
		rd->labels[i] = labels[i];
	rd->sr_cnt = segidx; 
	rd->sr_offset = malloc(segidx * sizeof(rd->sr_offset));
	if (!rd->sr_offset) {
		free(rd);
		return NULL;
	} 
	for (i=0; i < segidx; i++)
		rd->sr_offset[i] = segs[i];
	rd->road_flags = flags;
	rd->road_len = road_len;
	rd->ri_cnt = ri_cnt;
	rd->rio = malloc(ri_cnt * sizeof(u_int8_t));
	rd->ri = malloc(ri_cnt * sizeof(u_int32_t));
	if (!rd->ri || !rd->rio) {
		free(rd->sr_offset);
		free(rd->ri);
		free(rd->rio);
		free(rd);
		return NULL;
	}
	for (i=0; i < ri_cnt; i++) {
		rd->rio[i] = rio[i];
		rd->ri[i] = ri[i];
	}
	rd->sai = sai;
	rd->nod_offset = nodptr;
	return rd;
}

int gar_net_parse_sorted(struct gar_subfile *sub)
{
	unsigned char buf[3];
	int i, rc;
	unsigned int val;
	int lblidx, c = 0;
	int roadptr;
	struct road_info *ri;
	off_t o;
	if (!sub->net->net3_offset) {
		log(1, "NET: No sorted roads defined\n");
		return 0;
	}
	o = sub->net->netoff + sub->net->net3_offset;
	if (glseek(sub->gimg, o, SEEK_SET) != o) {
		log(1, "NET: Error can not seek to %ld\n", o);
		return 0;
	}
	for (i=0; i < sub->net->net3_length/3; i++) {
		rc = gread(sub->gimg, buf, 3);
		if (rc < 0)
			break;
		val = *(unsigned int *)buf;
		lblidx = val >> 21;
		lblidx &= 3;
		roadptr = val & 0x001FFFFF;
		log(11, "lblidx=%d roadptr=%d\n", lblidx, roadptr);
		ri = gar_parse_road_info(sub, roadptr);
		if (ri) {
			gar_log_road_info(ri);
			gar_free_road_info(ri);
		} else {
			log(1, "NET: Error parsing road info\n");
		}
		c++;
	}
	log(11, "Total %d roads\n", c);
	return c;
}
