#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "GarminTypedef.h"

struct hdr_mdr_t
{
	struct hdr_subfile_part_t hsub;
	u_int32_t offset1;	// some kind of header
	u_int32_t length1;
	////////////////////
	u_int32_t offset2;
	u_int32_t length2;
	u_int16_t unknown21;
	u_int32_t unknown22;
	u_int32_t offset3;
	u_int32_t length3;
	u_int16_t unknown31;
	u_int32_t unknown32;
	u_int32_t offset4;
	u_int32_t length4;
	u_int16_t unknown41;
	u_int32_t unknown42;
	u_int32_t offset5;
	u_int32_t length5;
	u_int16_t unknown51;
	u_int32_t unknown52;
	u_int32_t offset6;
	u_int32_t length6;
	u_int16_t unknown61;
	u_int32_t unknown62;
	u_int32_t offset7;
	u_int32_t length7;
	u_int16_t unknown71;
	u_int32_t unknown72;
	u_int32_t offset8;
	u_int32_t length8;
	u_int16_t unknown81;
	u_int32_t unknown82;
	u_int32_t offset9;
	u_int32_t length9;
	u_int16_t unknown91;
	u_int32_t unknown92;
	u_int32_t offset10;
	u_int32_t length10;
	u_int16_t unknown101;
	u_int32_t unknown102;
	u_int32_t offset11;
	u_int32_t length11;
	u_int32_t unknown111;
	u_int32_t offset12;
	u_int32_t length12;
	u_int32_t unknown121;
	u_int16_t unknown122;
	u_int32_t offset13;
	u_int32_t length13;
	u_int32_t unknown131;
	u_int16_t unknown132;
	u_int32_t offset14;
	u_int32_t length14;
	u_int32_t unknown141;
	u_int16_t unknown142;
	u_int32_t offset15;
	u_int32_t length15;
	u_int32_t unknown151;
	u_int16_t unknown152;
	u_int32_t offset16;
	u_int32_t length16;
//	u_int32_t unknown161;
//	u_int16_t unknown162;
	///////////////
//	u_int8_t unknownbl1[8];
/* 4	u_int32_t offset17;
   4	u_int32_t length17;
   4	u_int32_t unknown171;
   2	u_int16_t unknown172; */
	u_int32_t offset18;
	u_int32_t length18;
	u_int32_t unknown181;
	u_int8_t unknown182;
	u_int32_t offset19;
	u_int32_t length19;
	u_int32_t unknown191;
	u_int16_t unknown192;
	u_int32_t offset20;
	u_int32_t length20;
	u_int32_t unknown201;
	u_int16_t unknown202;
	u_int32_t offset21;
	u_int32_t length21;
	u_int32_t unknown211;
	u_int16_t unknown212;
	u_int32_t offset22;
	u_int32_t length22;
	u_int32_t unknown221;
	u_int16_t unknown222;
	u_int32_t offset23;
	u_int32_t length23;
	u_int32_t unknown231;
	u_int16_t unknown232;
	u_int32_t offset24;
	u_int32_t length24;
	u_int32_t unknown241;
	u_int16_t unknown242;
	u_int32_t offset25;
	u_int32_t length25;
	u_int32_t unknown251;
	u_int16_t unknown252;
	u_int32_t offset26;
	u_int32_t length26;
	u_int32_t unknown261;
	u_int16_t unknown262;
	u_int32_t offset27;
	u_int32_t length27;
	u_int32_t unknown271;
	u_int16_t unknown272;
	u_int32_t offset28;
	u_int32_t length28;
	u_int32_t unknown281;
	u_int16_t unknown282;
	u_int32_t offset29;
	u_int32_t length29;
	u_int32_t unknown291;
	u_int16_t unknown292;
	u_int32_t offset30;
	u_int32_t length30;
	u_int32_t unknown301;
	u_int16_t unknown302;
	u_int32_t offset31;
	u_int32_t length31;
	u_int32_t unknown311;
	u_int16_t unknown312;
	u_int32_t offset32;
	u_int32_t length32;
	u_int32_t unknown321;
	u_int16_t unknown322;
//	u_int8_t unknownbl21[8];
	u_int32_t offset33;
	u_int32_t length33;
//	u_int32_t unknown331;
//	u_int16_t unknown332;
//	u_int8_t unknownbl2[6];
/*	u_int32_t offset34;
	u_int32_t length34;
	u_int32_t unknown341;
	u_int16_t unknown342; */
	u_int32_t offset35;
	u_int32_t length35;
	u_int32_t unknown351;
	u_int16_t unknown352;
	u_int32_t offset36;
	u_int32_t length36;
	u_int32_t offset37;
	u_int32_t length37;
	u_int32_t unknown371;
	u_int16_t unknown372;
	u_int32_t offset38;
	u_int32_t length38;
	u_int32_t unknown381;
	u_int16_t unknown382;
	u_int32_t offset39;
	u_int32_t length39;
	u_int32_t unknown391;
	u_int16_t unknown392;
	u_int32_t offset40;
	u_int32_t length40;
	u_int32_t unknown401;
	u_int16_t unknown402;
	u_int32_t offset41;
	u_int32_t length41;
	u_int32_t unknown411;
	u_int16_t unknown412;
	// len = 568
	
} __attribute((packed));

static int gar_read_mdr(struct gimg *g, char *file)
{
	int rc;
	struct hdr_mdr_t mdr;
	ssize_t off = gar_file_offset(g, file);
	if (glseek(g, off, SEEK_SET) != off) {
		log(1, "Error seeking to %zd\n", off);
		return -1;
	}
	rc = gread(g, &mdr, sizeof(mdr));
	if (rc < 0) {
		log(1, "Error reading MDR header\n");
		return -1;
	}
	log(1, "HDR len =%d our=%d\n", mdr.hsub.length, sizeof(mdr));
	log(1, "o1: %x %d\n", mdr.offset1, mdr.length1);
	log(1, "o2: %d %d %d %d\n", mdr.offset2, mdr.length2, mdr.unknown21, mdr.unknown22);
	log(1, "o3: %d %d %d %d\n", mdr.offset3, mdr.length3, mdr.unknown31, mdr.unknown32);
	log(1, "o4: %d %d %d %d\n", mdr.offset4, mdr.length4, mdr.unknown41, mdr.unknown42);
	log(1, "o5: %d %d %d %d\n", mdr.offset5, mdr.length5, mdr.unknown51, mdr.unknown52);
	log(1, "o6: %d %d %d %d\n", mdr.offset6, mdr.length6, mdr.unknown61, mdr.unknown62);
	log(1, "o7: %d %d %d %d\n", mdr.offset7, mdr.length7, mdr.unknown71, mdr.unknown72);
	log(1, "o8: %d %d %d %d\n", mdr.offset8, mdr.length8, mdr.unknown81, mdr.unknown82);
	log(1, "o9: %d %d %d %d\n", mdr.offset9, mdr.length9, mdr.unknown91, mdr.unknown92);
	log(1, "o10: %d %d %d %d\n", mdr.offset10, mdr.length10, mdr.unknown101, mdr.unknown102);
	log(1, "o11: %d %d %d %d\n", mdr.offset11, mdr.length11, mdr.unknown111,mdr.unknown111);
	log(1, "o12: %d %d\n", mdr.offset12, mdr.length12/*, mdr.unknown121, mdr.unknown122*/);
	log(1, "o13: %d %d %d %d\n", mdr.offset13, mdr.length13, mdr.unknown131, mdr.unknown132);
	log(1, "o14: %d %d %d %d\n", mdr.offset14, mdr.length14, mdr.unknown141, mdr.unknown142);
	log(1, "o15: %d %d %d %d\n", mdr.offset15, mdr.length15, mdr.unknown151, mdr.unknown152);
	log(1, "o16: %d %d\n", mdr.offset16, mdr.length16/*, mdr.unknown161, mdr.unknown162*/);
//	log(1, "o17: %x %x\n", mdr.offset17, mdr.length17);
	log(1, "o18: %d %d %d %d\n", mdr.offset18, mdr.length18, mdr.unknown181, mdr.unknown182);
	log(1, "o19: %d %d %d %d\n", mdr.offset19, mdr.length19, mdr.unknown191, mdr.unknown192);
	log(1, "o20: %d %d %d %d\n", mdr.offset20, mdr.length20, mdr.unknown201, mdr.unknown202);
	log(1, "o21: %d %d %d %d\n", mdr.offset21, mdr.length21, mdr.unknown211, mdr.unknown212);
	log(1, "o22: %d %d %d %d\n", mdr.offset22, mdr.length22, mdr.unknown221, mdr.unknown222);
	log(1, "o23: %d %d %d %d\n", mdr.offset23, mdr.length23, mdr.unknown231, mdr.unknown232);
	log(1, "o24: %d %d %d %d\n", mdr.offset24, mdr.length24, mdr.unknown241, mdr.unknown242);
	log(1, "o25: %d %d %d %d\n", mdr.offset25, mdr.length25, mdr.unknown251, mdr.unknown252);
	log(1, "o26: %d %d %d %d\n", mdr.offset26, mdr.length26, mdr.unknown261, mdr.unknown262);
	log(1, "o27: %d %d %d %d\n", mdr.offset27, mdr.length27, mdr.unknown271, mdr.unknown272);
	log(1, "o28: %d %d %d %d\n", mdr.offset28, mdr.length28, mdr.unknown281, mdr.unknown282);
	log(1, "o29: %d %d\n", mdr.offset29, mdr.length29);
	log(1, "o30: %d %d %d %d\n", mdr.offset30, mdr.length30, mdr.unknown301, mdr.unknown302);
	log(1, "o31: %d %d %d %d\n", mdr.offset31, mdr.length31, mdr.unknown311, mdr.unknown312);
	log(1, "o32: %d %d\n", mdr.offset32, mdr.length32);
	log(1, "o33: %d %d\n", mdr.offset33, mdr.length33/*, mdr.unknown331, mdr.unknown332*/);
//	log(1, "o34: %d %d\n", mdr.offset34, mdr.length34);
	log(1, "o35: %d %d\n", mdr.offset35, mdr.length35);
	log(1, "o36: %d %d\n", mdr.offset36, mdr.length36);
	log(1, "o37: %d %d\n", mdr.offset37, mdr.length37);
	log(1, "o38: %d %d\n", mdr.offset38, mdr.length38);
	log(1, "o39: %d %d\n", mdr.offset39, mdr.length39);
	log(1, "o40: %d %d\n", mdr.offset40, mdr.length40);
	log(1, "o41: %d %d\n", mdr.offset41, mdr.length41);
//	log(1, "o42: %d %d\n", mdr.offset42, mdr.length42);
//	log(1, "o43: %d %d\n", mdr.offset43, mdr.length43);
}

static int debug = 10;
static void logfn(char *file, int line, int level, char *fmt, ...)
{
	va_list ap;
	if (level > debug)
		return;
	va_start(ap, fmt);
	fprintf(stdout, "%s:%d:%d|", file, line, level);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

static struct gar * load(char *file)
{
	struct gar *g;
	struct gar_config cfg;
	cfg.opm = OPM_PARSE;
	// FIXME: make cmdline arg
	cfg.debugmask = 0; // DBGM_LBLS | DBGM_OBJSRC;
	cfg.debuglevel = debug;
	g = gar_init_cfg(NULL, logfn, &cfg);
	if (!g)
		return NULL;
	if (gar_img_load(g, file, 1) > 0)
		return g;
	else {
		gar_free(g);
		return NULL;
	}
}

static int usage(char *pn)
{
	fprintf(stderr, "%s [-d level] garmin.img\n", pn);
	return -1;
}

int main(int argc, char **argv)
{
	struct gar *gar;
	struct gar_subfile *sub;
	struct gimg *g;
	char **files;
	int fcnt;
	int i;
	if (argc < 2) {
		return usage(argv[0]);
	}
	gar = load(argv[1]);
	if (!gar)
		return -1;
	g = gar_get_dskimg(gar, NULL);
	files = gar_file_get_subfiles(g, &fcnt, "MDR");
	if (files) {
		for (i=0; i < fcnt; i++) {
			gar_read_mdr(g, files[i]);
		}
	}
}
