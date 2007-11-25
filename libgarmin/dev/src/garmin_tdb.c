#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "GarminTypedef.h"
#include "garmin_tdb.h"

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

static int gar_tdb_load_img(struct gar *gar, char *file, int basemap, int data,
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

int gar_parse_tdb(struct gar *gar, char *file, int data)
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
	fd = open(file, O_RDONLY);
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
			case TDB_HEADER:
				log(10, "ProductID: %d\n", *(u_int16_t *)cp);
				log(11, "Unknown: %d\n", *(u_int16_t *)(cp+2));
				log(1, "TDB Version: %.2f\n", (*(u_int16_t *)(cp+4))/100.0);
				version = (*(u_int16_t *)(cp+4))/100.0;
				log(1, "Map Series Name: [%s]\n", cp+16);
				cp+=16+strlen(cp+16) + 1;
				log(1, "Version: %.2f\n", (*(u_int16_t *)cp)/100.0);
				log(1, "Map Family: [%s]\n", cp+2);
				log(11, "Left bytes: %d\n", block.size - (cp - buf));
				if (version != 3 && version != 4) {
					log(1, "Unsupported TDB version\n");
					close(fd);
					return -1;
				}
				if (gar) {
					gar->tdbdir = strdup(file);
					tp = strrchr(gar->tdbdir, '/');
					if (tp)
						*tp = '\0';
					else
						*gar->tdbdir = '\0';
					gar->tdbloaded = 1;
				}
				break;
			case TDB_COPYRIGHT:
				log(1, "Map copyrights:\n");
				while (cp < buf+block.size) {
					log(1, "%s\n", cp+4);
					cp+=4+strlen(cp+4) + 1;
				}
				log(11, "Left bytes: %d\n", block.size - (cp - buf));
				break;
			case TDB_TRADEMARK:
				log(1, "Map TradeMarks:\n");
				while (cp < buf+block.size) {
					log(1, "[%02X]%s\n",
						*cp, cp + 1);
					cp+=1+ strlen(cp+1) + 1;
				}
				log(11, "Left bytes: %d\n", block.size - (cp - buf));
				break;
			case TDB_REGIONS:
				log(1, "Covered Regions:\n");
				while (cp < buf+block.size) {
					log(1, "[%02X][%02X]%s\n", *cp, *(cp+1),cp+2);
					cp+=2+strlen(cp+2) + 1;
				}
				log(1, "Left bytes: %d\n", block.size - (cp - buf));
				break;
			case TDB_BASEMAP:
				if (version == 3) {
					log(1, "BaseMap number: %08u\n", *(u_int32_t *)cp);
					log(11, "Parent map: %08u\n", *(u_int32_t *)cp+4);
					sprintf(imgname, "%08u", *(u_int32_t *)cp);
				} else if (version == 4) {
					uc = (unsigned char *)cp;
					log(1, "BaseMap number: [%02X][%02X][%02X][%02X]\n", *uc, *(uc+1), *(uc+2), *(uc+3));
					log(11, "Parent map: [%02X][%02X][%02X][%02X]\n", *(uc+4), *(uc+5), *(uc+6), *(uc+7));
				} else {
					log(1, "Unknown TDB version\n");
					break;
				}
				if (version == 3) {
					north = GAR4DEG(*(u_int32_t *)(cp+8));
					east = GAR4DEG(*(u_int32_t *)(cp+0xc));
					south = GAR4DEG(*(u_int32_t *)(cp+0x10));
					west = GAR4DEG(*(u_int32_t *)(cp+0x14));
				} else if (version == 4) {
					c = *(int32_t *)(cp+8) >>8;
					north = GARDEG(c);
					c = *(int32_t *)(cp+12) >> 8;
					east = GARDEG(c);
					c = *(int32_t *)(cp+16) >> 8;
					south = GARDEG(c);
					c = *(int32_t *)(cp+20) >> 8;
					west = GARDEG(c);
				} else {
					log(1, "Unknown TDB version:%d\n", version);
					break;
				}
				log(9, "North: %fC West: %fC South: %fC East: %fC\n",
					north, west, south, east);
				cp+= 0x18;
				if (cp < buf+block.size) {
					log(9, "Descr:[%s]\n", cp);
					cp += strlen(cp) + 1;
				}
				log(11, "Left bytes: %d\n", block.size - (cp - buf));
				if (version == 4) {
					tp = strrchr(file, '/');
					if (tp) {
						sprintf(imgname, "%s", tp+1);
					} else {
						strncpy(imgname, file, sizeof(imgname)-1);
						imgname[sizeof(imgname)-1] = '\0';
					}
					tp = strrchr(imgname, '.');
					if (tp)
						*tp = '\0';
				}
				if (version == 3) {
					havebase = 1;
					gar_tdb_load_img(gar, imgname, 1, data,
						north, east, south, west);
				} else if (version == 4) {
					havebase = 1;
					/* IN v4 looks like there are definitions pointing in the basemap */
					if (!td4bm) {
						gar_tdb_load_img(gar, imgname, 1, data,
							north, east, south, west);
						td4bm = 1;
					}
				}
				break;
			case TDB_DETAILMAP:
				log(1, "DetailMap number: %08u\n", *(u_int32_t *)cp);
				sprintf(imgname, "%08u", *(u_int32_t *)cp);
				log(11, "Parent map: %08u\n", *(u_int32_t *)cp+4);
				if (version == 3) {
					north = GAR4DEG(*(u_int32_t *)(cp+8));
					east = GAR4DEG(*(u_int32_t *)(cp+0xc));
					south = GAR4DEG(*(u_int32_t *)(cp+0x10));
					west = GAR4DEG(*(u_int32_t *)(cp+0x14));
				} else if (version == 4) {
					c = *(int32_t *)(cp+8) >>8;
					c = SIGN3B(c);
					north = GARDEG(c);
					c = *(int32_t *)(cp+12) >> 8;
					c = SIGN3B(c);
					east = GARDEG(c);
					c = *(int32_t *)(cp+16) >> 8;
					c = SIGN3B(c);
					south = GARDEG(c);
					c = *(int32_t *)(cp+20) >> 8;
					c = SIGN3B(c);
					west = GARDEG(c);
				} else {
					log(1, "Unknown TDB version:%d\n", version);
					break;
				}
				log(9, "North: %fC West: %fC South: %fC East: %fC\n",
					north, west, south, east);
				cp+= 0x18;
				if (cp < buf+block.size) {
					log(9, "Descr:[%s]\n", cp);
					cp += strlen(cp) + 1;
				}
				t = *(u_int16_t*)cp;
				log(15, "blocks: %04X\n", *(u_int16_t*)cp);
				cp+=2;
				s = *(u_int16_t*)cp;
				log(15, "data: %04X\n", *(u_int16_t*)cp);
				cp+=2;
				while (s--) {
					log(15, "size: %08u\n", *(u_int32_t*)cp);
					cp+=4;
				}
				log(15, "term: %02X\n", *cp);
				cp++;
				log(11, "Left bytes: %d\n", block.size - (cp - buf));
				gar_tdb_load_img(gar, imgname, 0, data,
					north, east, south, west);
				break;
			case TDB_TAIL:
				log(11, "TDB Tail block\n");
				for (c=0; c < block.size; c++) {
					log(11, "[%02X]\n", *(unsigned char *)cp);
					cp++;
				}
				break;
			default:
				log(1, "Unknown TDB block ID:0x%02X\n", block.id);
		}
		free(buf);
	}
	close(fd);
	return havebase;
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
	return gar_parse_tdb(NULL,argv[1],0);
}
#endif
