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

struct net_data {
	off_t labels[4];
	u_int32_t sr_offset[4];
	u_int8_t road_flags;
	u_int8_t road_len;
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
