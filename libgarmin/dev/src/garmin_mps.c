#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#define __USE_GNU
#include <fcntl.h>
#include <string.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "GarminTypedef.h"
#include "garmin_tdb.h"

#define MPS_PRODUCT	0x46
#define MPS_BASEMAP	0x4c
#define MPS_UNLOCK	0x55
#define MPS_DETAILMAP	0x56

#ifdef STANDALONE
#undef log
#define log(x, y...) fprintf(stderr, ## y)
int gar_img_load_dskimg(struct gar *gar, char *a, int bm, int data,
	double north, double east, double south, double west)
{
}
#endif
#define GAR4DEG(x) ((double)(x)* 360.0 / (1<<31))

/*
 * XXX: Use TDB for lookups
 */

static int gar_mps_load_img(struct gar *gar, char *file, int basemap, int data,
		double north, double east, double south, double west)
{
	char path[4096];
	int rc;

	if (!gar)
		return 0;
	if (!gar->tdbdir) {
		log(1, "Trying to load [%s] but not TDB header seen yet\n", file);
		return -1;
	}
	sprintf(path, "%s/%s.img", gar->tdbdir, file);
	rc =  gar_img_load_dskimg(gar, path, basemap, data,
				DEGGAR(north), DEGGAR(east),
				DEGGAR(south), DEGGAR(west));
	if (rc < 0)
		log(1, "Failed to load [%s]\n", path);
	return rc;
}

int gar_parse_mps(struct gar *gar, char *file, int data)
{
	int fd, rc;
	u_int16_t s,t;
	char *buf, *cp, *tp;
	unsigned char *uc;
	struct tdb_block block;
	int version = 0;
	int c;
	int havebase = -1;
	int td4bm = 0;
	float north, south, east, west;
	char imgname[128];
	fd = open(file, OPENFLAGS);
	if (fd <0) {
		log(1, "Can not open:[%s] errno=%d(%s)\n",
				file, errno, strerror(errno));
		return -1;
	}
	while (read(fd, &block, sizeof(struct tdb_block)) == 
				sizeof(struct tdb_block)) {
		log(11,"Block type: %02X size=%d\n", block.id, block.size);
		buf = malloc(block.size);
		if (!buf) {
			break;
		}
		rc = read(fd, buf, block.size);
		if (rc != block.size)
			break;
		cp = buf;
		switch (block.id) {
			case MPS_PRODUCT:
				log(11, "ProductID: %d/%x\n", *(u_int32_t *)cp, *(u_int32_t *)cp);
				cp+=4;
				log(11, "Name: %s\n", cp);
				cp+=strlen(cp) + 1;
				break;
			case MPS_BASEMAP:
				log(11, "ProductID: %d\n", *(u_int32_t *)cp);
				cp+=4;
				// if == 0 it's a base map
				log(11, "MapID: %d\n", *(u_int32_t *)cp);
				cp+=4;
				log(11, "Name: %s\n", cp);
				cp += strlen(cp) + 1;
				log(11, "Area: %s\n", cp);
				cp += strlen(cp) + 1;
				log(11, "Product: %s\n", cp);
				cp += strlen(cp) + 1;
				log(11, "MapID: %d/%x\n", *(u_int32_t *)cp,*(u_int32_t *)cp);
				cp+=4;
				log(11, "ParentMapID?: %d/%x\n", *(u_int32_t *)cp,*(u_int32_t *)cp);
				cp+=4;
				break;
			case MPS_UNLOCK:
				// don't care
				log(11, "Unlock section\n");
				break;
			case MPS_DETAILMAP:
				log(11, "Map: %s\n", cp);
				cp+=strlen(cp)+1;
				log(11, "??: %x\n", *cp);
				cp++;
				break;
			default:
				log(1, "Unknown MPS block ID:0x%02X\n", block.id);
		}
		log(11,"size=%d read=%d\n",  block.size, cp-buf);
		free(buf);
	}
	close(fd);
	return havebase;
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
	return gar_parse_mps(NULL,argv[1],0);
}
#endif
