#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "GarminTypedef.h"

#define GAR4DEG(x) ((double)(x)* 360.0 / (1<<31))

int gar_parse_tdb(char *file)
{
	int fd, rc;
	u_int16_t s,t;
	u_int8_t b;
	char *buf, *cp;
	unsigned char *uc;
	struct tdb_block block;
	int version = 0;
	int sz,c;
	float north, south, east, west;
	fd = open(file, O_RDONLY);
	if (fd <0) {
		return -1;
	}
	while (read(fd, &block, sizeof(struct tdb_block)) == 
				sizeof(struct tdb_block)) {
		printf("Block type: %02X size=%d\n", block.id, block.size);
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
				printf("ProductID: %d\n", *(u_int16_t *)cp);
				printf("Unknown: %d\n", *(u_int16_t *)(cp+2));
				printf("TDB Version: %.2f\n", (*(u_int16_t *)(cp+4))/100.0);
				version = (*(u_int16_t *)(cp+4))/100.0;
				printf("Map Series Name: [%s]\n", cp+16);
				cp+=16+strlen(cp+16) + 1;
				printf("Version: %.2f\n", (*(u_int16_t *)cp)/100.0);
				printf("Map Family: [%s]\n", cp+2);
				printf("Left bytes: %d\n", block.size - (cp - buf));
				break;
			case TDB_COPYRIGHT:
				printf("Map copyrights:\n");
				while (cp < buf+block.size) {
					printf("%s\n", cp+4);
					cp+=4+strlen(cp+4) + 1;
				}
				printf("Left bytes: %d\n", block.size - (cp - buf));
				break;
			case TDB_TRADEMARK:
				printf("Map TradeMarks:\n");
				while (cp < buf+block.size) {
					printf("[%02X][%02X][%02X]%s\n",
						*cp, *(cp+1), *(cp+2), cp + 3);
					cp+=3+ strlen(cp+3) + 1;
				}
				printf("Left bytes: %d\n", block.size - (cp - buf));
				break;
			case TDB_REGIONS:
				printf("Map Regions:\n");
				while (cp < buf+block.size) {
					printf("[%02X][%02X]%s\n", *cp, *(cp+1),cp+2);
					cp+=2+strlen(cp+2) + 1;
				}
				printf("Left bytes: %d\n", block.size - (cp - buf));
				break;
			case TDB_BASEMAP:
				if (version == 3) {
					printf("BaseMap number: %08u\n", *(u_int32_t *)cp);
					printf("Parent map: %08u\n", *(u_int32_t *)cp+4);
				} else if (version == 4) {
					uc = cp;
					printf("BaseMap number: [%02X][%02X][%02X][%02X]\n", *uc, *(uc+1), *(uc+2), *(uc+3));
					printf("Parent map: [%02X][%02X][%02X][%02X]\n", *(uc+4), *(uc+5), *(uc+6), *(uc+7));
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
					printf("Unknown version:%d\n", version);
				}
				printf("North: %f\n", north);
				printf("West: %f\n", west);
				printf("South: %f\n", south);
				printf("East: %f\n", east);
				cp+= 0x18;
				if (cp < buf+block.size) {
					printf("Descr:[%s]\n", cp);
					cp += strlen(cp) + 1;
				}
				printf("Left bytes: %d\n", block.size - (cp - buf));
				break;
			case TDB_DETAILMAP:
				printf("DetailMap number: %08u\n", *(u_int32_t *)cp);
				printf("Parent map: %08u\n", *(u_int32_t *)cp+4);
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
					printf("Unknown version:%d\n", version);
				}
				printf("North: %f\n", north);
				printf("West: %f\n", west);
				printf("South: %f\n", south);
				printf("East: %f\n", east);
				cp+= 0x18;
				if (cp < buf+block.size) {
					printf("Descr:[%s]\n", cp);
					cp += strlen(cp) + 1;
				}
				t = *(u_int16_t*)cp;
				printf("blocks: %04X\n", *(u_int16_t*)cp);
				cp+=2;
				s = *(u_int16_t*)cp;
				printf("data: %04X\n", *(u_int16_t*)cp);
				cp+=2;
				while (s--) {
					printf("size: %08u\n", *(u_int32_t*)cp);
					cp+=4;
				}
				if (0 && version == 4) {
					printf("file:[%s]\n", cp);
					cp += strlen(cp) + 1;
				}
				printf("term: %02X\n", *cp);
				cp++;
				printf("Left bytes: %d\n", block.size - (cp - buf));
				break;
				
			default:
				printf("Unknown block 0x%02X\n", block.id);
		}
		free(buf);
	}
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
	return gar_parse_tdb(argv[1]);
}
#endif
