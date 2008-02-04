#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_fat.h"
#include "garmin_rgn.h"
#include "garmin_mdr.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "GarminTypedef.h"

struct hdr_mdr_t
{
	struct hdr_subfile_part_t hsub;
	u_int32_t hdr1;	// some kind of header
	u_int32_t hdr2;
	////////////////////
	// images
	u_int32_t offset1;
	u_int32_t length1;
	u_int16_t unknown11;
	u_int32_t unknown12;
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

	u_int32_t unknown101;

	u_int32_t offset11;
	u_int32_t length11;
	u_int16_t unknown111;
	u_int32_t unknown112;

	u_int32_t offset12;
	u_int32_t length12;
	u_int16_t unknown121;
	u_int32_t unknown122;

	u_int32_t offset13;
	u_int32_t length13;
	u_int16_t unknown131;
	u_int32_t unknown132;
	
	u_int32_t offset14;
	u_int32_t length14;
	u_int16_t unknown141;
	u_int32_t unknown142;

	u_int32_t offset15;
	u_int32_t length15;
	u_int8_t unknown151;

	u_int32_t offset16;
	u_int32_t length16;
	u_int16_t unknown161;
	u_int32_t unknown162;

	// second block?
	u_int32_t offset17;
	u_int32_t length17;
	u_int32_t unknown171;

	u_int32_t offset18;
	u_int32_t length18;
	u_int16_t unknown181;
	u_int32_t unknown182;

	u_int32_t offset20;
	u_int32_t length20;
	u_int16_t unknown201;
	u_int32_t unknown202;
	u_int32_t offset21;
	u_int32_t length21;
	u_int16_t unknown211;
	u_int32_t unknown212;
	u_int32_t offset22;
	u_int32_t length22;
	u_int16_t unknown221;
	u_int32_t unknown222;
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
	u_int16_t unknown271;
	u_int32_t unknown272;
	u_int32_t offset28;
	u_int32_t length28;
	u_int16_t unknown281;
	u_int32_t unknown282;
	u_int32_t offset29;
	u_int32_t length29;
	u_int16_t unknown291;
	u_int32_t unknown292;
	u_int32_t offset30;
	u_int32_t length30;
	u_int16_t unknown301;
	u_int32_t unknown302;
	u_int32_t offset31;
	u_int32_t length31;
	u_int16_t unknown311;
	u_int32_t unknown312;
	u_int32_t offset32;
	u_int32_t length32;
	u_int16_t unknown321;
	u_int32_t unknown322;
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
	u_int16_t unknown351;
	u_int32_t unknown352;
	u_int32_t offset36;
	u_int32_t length36;
	u_int32_t offset37;
	u_int32_t length37;
	u_int16_t unknown371;
	u_int32_t unknown372;
	u_int32_t offset38;
	u_int32_t length38;
	u_int16_t unknown381;
	u_int32_t unknown382;
	u_int32_t offset39;
	u_int32_t length39;
	u_int16_t unknown391;
	u_int32_t unknown392;
	u_int32_t offset40;
	u_int32_t length40;
	u_int16_t unknown401;
	u_int32_t unknown402;
	u_int32_t offset41;
	u_int32_t length41;
	u_int16_t unknown411;
	u_int32_t unknown412;
	// len = 568
} __attribute((packed));


int gar_init_mdr(struct gimg *g)
{
	int fcnt;
	int i,rc;
	struct gar_mdr *m;
	char **files;
	struct hdr_mdr_t mdr;

	m = calloc(1, sizeof(*m));
	if (!m)
		return -1;
	files = gar_file_get_subfiles(g, &fcnt, "MDR");
	if (files) {
		if (fcnt > 1) {
			log(1, "Warning: More than one MDR is not supported\n");
		}
		for (i=0; i < fcnt; i++) {
			ssize_t off = gar_file_offset(g, files[i]);

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
			m->mdroff = gar_file_baseoffset(g, files[i]);
			m->idxfiles_offset = mdr.offset1;
			m->idxfiles_len = mdr.length1;
			g->mdr = m;
			break;
		}
		free(files);
	}
	return 1;
}

int gar_mdr_get_files(struct gmap *files, struct gimg *g)
{
	struct gar_mdr *m = g->mdr;
	int i, cnt = m->idxfiles_len/4;
	struct gar_subfile *sub;
	unsigned int *id;
	char buf[m->idxfiles_len];
	unsigned int off;
	int idx = 0;

	off = g->mdr->mdroff + m->idxfiles_offset;
	if (glseek(g, off, SEEK_SET) != off) {
		log(1, "Error seeking to %d\n", off);
		return -1;
	}
	i = gread(g, &buf, m->idxfiles_len);
	if (i!=m->idxfiles_len) {
		log(1, "Error reading indexed files\n");
		return -1;
	}
	id = (unsigned int *)&buf[0];
	files->subs = calloc(cnt, sizeof(struct gar_subfile *));
	if (!files->subs)
		return -1;
	files->subfiles = cnt;
	for (i = 0; i < cnt; i++) {
		sub = gar_find_subfile_byid(g, *id);
		if (!sub) {
			log(1, "Error can not find id %d\n", *id);
		} else {
			files->subs[idx++] = sub;
		}
		id++;
	}
	files->lastsub = idx;
	return idx;

}

#ifdef STANDALONE
static void print_buf(char *pref, unsigned char *a, int s)
{
	char buf[4096];
	int i,sz = 0;
	for (i=0; i < s; i++) {
		sz += sprintf(buf+sz, "%02X ",a[i]);
	}
	log(1, "%s :%s\n", pref, buf);
}

static void print_bufa(char *pref, unsigned char *a, int s)
{
	char buf[4096];
	int i,sz = 0;
	int inasc = 0;
	for (i=0; i < s; i++) {
		if (isprint(a[i])) {
			sz += sprintf(buf+sz, "%s%c", inasc ? "" : "[", a[i]);
			inasc = 1;
		} else {
			sz += sprintf(buf+sz, "%s%02X ", inasc ? "]" : "",a[i]);
			inasc = 0;
		}
	}
	log(1, "%s :%s\n", pref, buf);
}

struct shead {
//	unsigned char b1;
	unsigned char head[6];
} __attribute((packed));

struct search_entry {
	unsigned char text[3];
	u_int8_t b1;
	u_int8_t b2;
	u_int8_t b3;
} __attribute((packed));

static int gar_log_o17(struct gimg *g, u_int32_t base, u_int32_t offset, u_int32_t len)
{
	int i = 0, rc, sz = 0, bp = 0;
	off_t off = base+offset;
	unsigned char buf[1024];
	int dl = gar_debug_level;
	char pref[10];
	struct shead head;
	struct search_entry ent;

	if (!len)
		return 0;
	if (glseek(g, off, SEEK_SET) != off) {
		log(1, "Error seeking to %zd\n", off);
		return -1;
	}
	gread(g, &head, sizeof(head));
	sprintf(pref, "header");
	print_buf(pref, (char *)&head, sizeof(head));
	sz = sizeof(head);
	while (sz < len) {
		rc = gread(g, &ent, sizeof(ent));
		if ( rc < 0)
			break;
		log(1, "%x %02X %02X %02X [%.3s]\n", sz, ent.b1, ent.b2,ent.b3,ent.text);
		sz += rc;
		i++;
	}
	log(1, "Done %d len:%d sz=%d\n",i, len, sz);
}

static int gar_log_cstrings(struct gimg *g, u_int32_t base, u_int32_t offset, u_int32_t len)
{
	int i = 0, rc, sz = 0, bp = 0;
	off_t off = base+offset;
	unsigned char buf[1024];
	int dl = gar_debug_level;
	char pref[10];
	if (!len)
		return 0;
	if (glseek(g, off, SEEK_SET) != off) {
		log(1, "Error seeking to %zd\n", off);
		return -1;
	}
	sprintf(pref, "lbl");
	do {
		rc = gread(g, buf + bp, sizeof(buf) - bp);
		print_buf(pref, buf, rc);
//		log(1, "STR:[%s] len:[%d]\n", buf, strlen(buf));
//		memmove(buf, buf + strlen(buf) + 1, strlen(buf));
//		bp = 
		i++;
		sz += rc;
	} while (sz < len);
	log(1, "Done %d len:%d rs:%d\n",i);
	return i;
	
}

static int gar_log_imgs(struct gimg *g, u_int32_t base, u_int32_t offset,
		u_int32_t len, u_int16_t recsize)
{
	int i,rc;
	off_t off = base+offset;
	unsigned char buf[recsize];
	int dl = gar_debug_level;
	char pref[10];
	if (!len)
		return;
	gar_debug_level = 13;
	if (glseek(g, off, SEEK_SET) != off) {
		log(1, "Error seeking to %zd\n", off);
		return -1;
	}
	for (i=0; i < len/recsize; i++) {
		rc = gread(g, buf, recsize);
		if (rc != recsize) {
			log(1, "Error reading record %d\n", i);
		}
		log(1, "IMG: %d/I%08X\n", *(u_int32_t*)buf,*(u_int32_t*)buf);
	}
	gar_debug_level = dl;
	log(1, "Done %d len:%d rs:%d\n",i, len, recsize);
	return i;
	
}

static int gar_log_recs(struct gimg *g, u_int32_t base, u_int32_t offset,
		u_int32_t len, u_int16_t recsize)
{
	int i,rc;
	off_t off = base+offset;
	unsigned char buf[recsize];
	int dl = gar_debug_level;
	char pref[10];
	if (!len)
		return;
	gar_debug_level = 13;
	if (glseek(g, off, SEEK_SET) != off) {
		log(1, "Error seeking to %zd\n", off);
		return -1;
	}
	for (i=0; i < len/recsize; i++) {
		rc = gread(g, buf, recsize);
		if (rc != recsize) {
			log(1, "Error reading record %d\n", i);
		}
		sprintf(pref, "%d", i);
		print_buf(pref, buf, recsize);
	}
	gar_debug_level = dl;
	log(1, "Done %d len:%d rs:%d\n",i, len, recsize);
	return i;
	
}

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
	log(1, "hdr: %08X %08X\n", mdr.hdr1, mdr.hdr2);
	log(1, "o1: %d %d %d %d\n", mdr.offset1, mdr.length1, mdr.unknown11, mdr.unknown12);
//	recsz ok if more than 256 images 2b img idx
//	gar_log_recs(g, off, mdr.offset1, mdr.length1, mdr.unknown11);
	gar_log_imgs(g, off, mdr.offset1, mdr.length1, mdr.unknown11);
	
	log(1, "o2: %d %d %d %d\n", mdr.offset2, mdr.length2, mdr.unknown21, mdr.unknown22);

	gar_log_recs(g, off, mdr.offset2, mdr.length2, mdr.unknown21);

	log(1, "o3: %d %d %d %d\n", mdr.offset3, mdr.length3, mdr.unknown31, mdr.unknown32);
	// 
//	gar_log_recs(g, off, mdr.offset3, mdr.length3, mdr.unknown31);
	
//	log(1, "o4: %d %d %d %d\n", mdr.offset4, mdr.length4, mdr.unknown41, mdr.unknown42);
//	lots of ok 3 bytes - indexed by ... ?
//	gar_log_recs(g, off, mdr.offset4, mdr.length4, mdr.unknown41);

	log(1, "o5: %d %d %d %d\n", mdr.offset5, mdr.length5, mdr.unknown51, mdr.unknown52);
//	size = 8 ok cities 1b map 1/2b idx 8A 08 80 00 00
//	gar_log_recs(g, off, mdr.offset5, mdr.length5, mdr.unknown51);

	log(1, "o6: %d %d %d %d\n", mdr.offset6, mdr.length6, mdr.unknown61, mdr.unknown62);
//	ok sz 
	gar_log_recs(g, off, mdr.offset6, mdr.length6, mdr.unknown61);

	log(1, "o7: %d %d %d %d\n", mdr.offset7, mdr.length7, mdr.unknown71, mdr.unknown72);
//	size  ok = 7 streets: 1b map 3b roadptr ?? ?? ??
//	gar_log_recs(g, off, mdr.offset7, mdr.length7, mdr.unknown71);

	log(1, "o8: %d %d %d %d\n", mdr.offset8, mdr.length8, mdr.unknown81, mdr.unknown82);
	gar_log_recs(g, off, mdr.offset8, mdr.length8, mdr.unknown81);

	log(1, "o9: %d %d %d %d\n", mdr.offset9, mdr.length9, mdr.unknown91, mdr.unknown92);
//	ok sz 4
	gar_log_recs(g, off, mdr.offset9, mdr.length9, mdr.unknown91);

	log(1, "o10: %d %d %d %d\n", mdr.offset10, mdr.length10, mdr.unknown101,mdr.unknown101);
	gar_log_cstrings(g, off, mdr.offset10, mdr.length10);
	log(1, "o11: %d %d %d %d\n", mdr.offset11, mdr.length11, mdr.unknown111, mdr.unknown112);
//	size = 9 ??
//	gar_log_recs(g, off, mdr.offset11, mdr.length11, mdr.unknown111);
	log(1, "o12: %d %d %d %d\n", mdr.offset12, mdr.length12, mdr.unknown121, mdr.unknown122);
//	size = 9 ??
//	gar_log_recs(g, off, mdr.offset12, mdr.length12, mdr.unknown121);
	log(1, "o13: %d %d %d %d\n", mdr.offset13, mdr.length13, mdr.unknown131, mdr.unknown132);
	log(1, "o14: %d %d %d %d\n", mdr.offset14, mdr.length14, mdr.unknown141, mdr.unknown142);
//	size ok = 6/8 ??? 1/2b map idx
//	gar_log_recs(g, off, mdr.offset14, mdr.length14, mdr.unknown141);
	log(1, "o15: %d %d %d\n", mdr.offset15, mdr.length15, mdr.unknown151/*, mdr.unknown162*/);
	gar_log_cstrings(g, off, mdr.offset15, mdr.length15);

//	log(1, "o17: %x %x\n", mdr.offset17, mdr.length17);
	log(1, "o16: %d %d %d %d\n", mdr.offset16, mdr.length16, mdr.unknown161 /*, mdr.unknown162*/);
	log(1, "o17: %d %d %d \n", mdr.offset17, mdr.length17, mdr.unknown171/*, mdr.unknown172*/);
	// strings : 
	gar_log_o17(g, off, mdr.offset17, mdr.length17);
//	gar_log_cstrings(g, off, mdr.offset17, mdr.length17);
	log(1, "o20: %d %d %d %d\n", mdr.offset20, mdr.length20, mdr.unknown201, mdr.unknown202);
	log(1, "o21: %d %d %d %d\n", mdr.offset21, mdr.length21, mdr.unknown211, mdr.unknown212);
	log(1, "o22: %d %d %d %d\n", mdr.offset22, mdr.length22, mdr.unknown221, mdr.unknown222);
	log(1, "o23: %d %d %d %d\n", mdr.offset23, mdr.length23, mdr.unknown231, mdr.unknown232);
	log(1, "o24: %d %d %d %d\n", mdr.offset24, mdr.length24, mdr.unknown241, mdr.unknown242);
	log(1, "o25: %d %d %d %d\n", mdr.offset25, mdr.length25, mdr.unknown251, mdr.unknown252);
	log(1, "o26: %d %d %d %d\n", mdr.offset26, mdr.length26, mdr.unknown261, mdr.unknown262);
	log(1, "o27: %d %d %d %d\n", mdr.offset27, mdr.length27, mdr.unknown271, mdr.unknown272);
	log(1, "o28: %d %d %d %d\n", mdr.offset28, mdr.length28, mdr.unknown281, mdr.unknown282);
	log(1, "o29: %d %d %d\n", mdr.offset29, mdr.length29, mdr.unknown291);
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
#endif
