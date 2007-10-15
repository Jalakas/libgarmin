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

struct road_def {
	char *labels[4];
	u_int32_t sr_offset;
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

	log(1, "NET initializing ...\n");
	off = gar_subfile_offset(sub, "NET");
	if (!off) {
		log(1,"No NET file\n");
		return 0;
	}
	if (lseek(gimg->fd, off, SEEK_SET) != off) {
		log(1, "NET: Error can not seek to %ld\n", off);
		return -1;
	}
	rc = read(gimg->fd, &net, sizeof(struct hdr_net_t));
	if (rc != sizeof(struct hdr_net_t)) {
		log(1, "NET: Can not read header\n");
		return -1;
	}

	ni = calloc(1, sizeof(*ni));
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

off_t gar_net_get_lbl_offset(struct gar_subfile *sub, off_t offset, int idx)
{
	struct gimg *gimg = sub->gimg;
	off_t o;
	char buf[12];
	char *cp;
	int rc;
	u_int32_t i;

	if (!sub->net || idx > 3)
		return 0xFFFFFFFF;

	o = sub->net->netoff + sub->net->net1_offset + (offset << sub->net->net1_addr_shift);
	if (lseek(gimg->fd, o, SEEK_SET) != o) {
		log(1, "NET: Error can not seek to %ld\n", o);
		return 0xFFFFFFFF;
	}
	rc = read(gimg->fd, buf, sizeof(buf));
	if (rc > 3) {
		cp = buf;
		i = *(u_int32_t*)cp;
		if (i&0x400000) {
			cp+=6;
			i = *(u_int32_t*)cp;
		}
		i &= 0x3FFFFF;
		if (!i)
			i = 0xFFFFFFFF;
		return i;
	} else
		return 0xFFFFFFFF;
}
