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
	off_t labels[4];
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
			if (sub->czips < 255)
				size = 1;
			else
				size = 2;
			f1 = malloc(size);
			if (!f1)
				goto out_err;
			if (gread(gimg, f1, size) != size)
				goto out_err;
		} else {
			log(1, "NET: Error f1 type=1\n");
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
			if (sub->cicount < 255)
				size = 1;
			else
				size = 2;
			f2 = malloc(size);
			if (!f2)
				goto out_err;
			if (gread(gimg, f2, size) != size)
				goto out_err;
		} else {
			log(1, "NET: Error f2 type=1\n");
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
			log(1, "NET: Error f3 type=%d\n", fl);
		}
	}
	sai = malloc(sizeof(*sai));
	if (!sai)
		goto out_err;
	sai->flags = flags;
	sai->field1 = f1;
	sai->field2 = f2;
	sai->field3 = f3;
out_err:
	if (f1)
		free(f1);
	if (f2)
		free(f2);
	if (f3)
		free(f3);
	return NULL;
}

static struct road_info *gar_parse_road_info(struct gar_subfile *sub, off_t offset)
{
	struct gimg *gimg = sub->gimg;
	off_t o;
	int lblidx = 0;
	off_t labels[4];
	char buf[12];
	u_int32_t segs[100];
	int segidx = 0;
	int rc;
	u_int32_t i;
	int seg = 0;
	u_int8_t flags;
	u_int32_t road_len;

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
		if (seg) {
			if (segidx < 100)
				segs[segidx++] = i;
			else {
				log(1, "NET: Error to many segments!\n");
				return NULL;
			}
			seg = 0;
		} else {
			if (i&(1<<22))
				seg = 1;
			labels[lblidx++] = i & 0x3FFFFF;
			if (i&(1<<23))
				break;
		}
	}
	rc = gread(gimg, &flags, sizeof(u_int8_t));
	if (rc!=1)
		return NULL;
	rc = gread(gimg, buf, 3);
	if (rc != 3)
		return NULL;
	road_len = *(u_int32_t*)buf;
	// FIXME
	return NULL;
}

int gar_net_parse_sorted(struct gar_subfile *sub)
{
	unsigned char buf[3];
	int i, rc;
	unsigned int val;
	int lblidx, c = 0;
	int roadptr;
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
		c++;
	}
	log(11, "Total %d roads\n", c);
	return c;
}
