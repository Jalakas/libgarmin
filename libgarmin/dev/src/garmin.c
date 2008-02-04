/*
    Copyright (C) 2007  Alexander Atanasov <aatanasov@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
   
*/

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#define __USE_GNU
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "version.h"
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_fat.h"
#include "garmin_rgn.h"
#include "garmin_lbl.h"
#include "garmin_tdb.h"
#include "garmin_mdr.h"

log_fn glogfn;
int gar_debug_level = 1;

#ifdef DEBUG
void gar_print_buf(char *pref, unsigned char *a, int s)
{
	char buf[4096];
	int i,sz = 0;
	for (i=0; i < s; i++) {
		sz += sprintf(buf+sz, "%02X ",a[i]);
	}
	log(11, "%s :%s\n", pref, buf);
}
#else
void gar_print_buf(char *pref, unsigned char *a, int s) {}
#endif

void gar_log_file_date(int l, char *pref, struct hdr_subfile_part_t *h)
{
	log(l, "%s: %04d-%02d-%02d %02d:%02d:%02d\n",
		pref, h->year, h->month, h->day,
		h->hour, h->min, h->sec);
}

static void inline gcheckfd(struct gimg *g)
{
	if (g->fd == -1) {
		g->fd = open(g->file, OPENFLAGS);
		if (g->fd < 0) {
			log(1, "Error can not open:[%s] errno=%d(%s)\n",
				g->file, errno, strerror(errno));
		}
	}
}

int gopen(struct gimg *g)
{
	if (g->fd == -1)
		g->fd = open(g->file, OPENFLAGS);
	return g->fd;
}

int gclose(struct gimg *g)
{
	int fd = g->fd;
	g->fd = -1;
	if (fd != -1)
		return close(fd);
	return 0;
}

off_t glseek(struct gimg *g, off_t offset, int whence)
{
	gcheckfd(g);
	return lseek(g->fd, offset, whence);
}

ssize_t gread(struct gimg *g, void *buf, size_t count)
{
	ssize_t rc;
	gcheckfd(g);
	rc = read(g->fd, buf, count);
	if (rc > 0 && g->xor) {
		ssize_t i;
		for (i=0; i < rc; i++)
			((unsigned char *)buf)[i] ^= g->xor;
	}
	return rc;
}

ssize_t gread_safe(struct gimg *g, void *buf, size_t count)
{
	ssize_t rc;
	int err;
	off_t osave;
	gcheckfd(g);
	osave = lseek(g->fd, 0, SEEK_CUR);
	rc = read(g->fd, buf, count);
	if (rc > 0 && g->xor) {
		ssize_t i;
		for (i=0; i < rc; i++)
			((unsigned char *)buf)[i] ^= g->xor;
	}
	err = errno;
	lseek(g->fd, osave, SEEK_SET);
	errno = err;
	return rc;
}

ssize_t gwrite(struct gimg *g, void *buf, size_t count)
{
	gcheckfd(g);
	if (g->xor) {
		ssize_t i;
		for (i=0; i < count; i++)
			((unsigned char *)buf)[i] ^= g->xor;
	}
	return write(g->fd, buf, count);
}

static struct gimg *gimg_alloc(struct gar *gar, char *file)
{
	struct gimg *g;

	g = calloc(1, sizeof(*g));
	if (!g)
		return NULL;
	g->gar = gar;
	if (file) {
		g->file = strdup(file);
		if (!g->file) {
			free(g);
			return NULL;
		}
	}
	list_init(&g->lfatfiles);
	list_init(&g->lsubfiles);
	return g;
}

struct gar *gar_init_cfg(char *tbd, log_fn l, struct gar_config *cfg)
{
	struct gar *gar;
	char modename[50] = "";

	if (cfg->opm == OPM_DUMP) {
		log(1, "Data dumping not implemented\n");
		return NULL;
	}

	if (cfg->opm != OPM_PARSE && cfg->opm != OPM_GPS) {
		log(1, "Unknown op mode: %d\n", cfg->opm);
		return NULL;
	}

	gar = calloc(1, sizeof(*gar));
	if (!gar)
		return NULL;

	if (l) {
		gar->logfn = l;
		glogfn = l;
	}

	gar->cfg = *cfg;
	if (gar->cfg.opm == OPM_GPS)
		strcpy(modename, "GPS Backend");
	else if (gar->cfg.opm == OPM_PARSE)
		strcpy(modename, "Parser");
	else if (gar->cfg.opm == OPM_DUMP)
		strcpy(modename, "Data dumper");
	gar_debug_level = cfg->debuglevel;
	log(1, "%s initializing as %s\n", LIBVERSION, modename);
	list_init(&gar->limgs);
#if 0
	log(1, "struct gimg=%d\n", sizeof(struct gimg));
	log(1, "struct gar_subfile=%d\n", sizeof(struct gar_subfile));
	log(1, "struct gar_subdiv=%d\n", sizeof(struct gar_subdiv));
#endif
	if (0) {
	struct rlimit rlmt;

	rlmt.rlim_cur = 300 * 1024 * 1024;
	rlmt.rlim_max = 300 * 1024 * 1024;
	if(setrlimit(RLIMIT_AS, &rlmt))
		log(1, "setrlimit DATA failed (%s)\n",
			strerror(errno));
	}

	return gar;
}

struct gar *gar_init(char *tdb, log_fn l)
{
	struct gar_config cfg;
	memset(&cfg, 0, sizeof(struct gar_config));
	cfg.opm = OPM_GPS;
	cfg.debuglevel = 10;
	return gar_init_cfg(tdb, l, &cfg);
}

void gar_free(struct gar *g)
{
	if (g->gmap)
		free(g->gmap);
	log(1, "Implement me\n");
}

static int gar_load_img_hdr(struct gimg *g, unsigned int *dataoffset, 
				unsigned int *blocksize, unsigned int *fatoffset)
{
	int rc;
	struct hdr_img_t hdr;
	rc = gread(g, &hdr, sizeof(struct hdr_img_t));
	if (rc < 0) {
		log(7, "Read error: %d(%s)\n", errno, strerror(errno));
		return -1;
	}
	if (rc != sizeof(struct hdr_img_t)) {
		log(7, "Error reading header want %d got %d\n",
			sizeof(struct hdr_img_t), rc);
		return -1;
	}
	if (hdr.xorByte != 0) {
		log(1, "Please, xor the file key:%02X, use garxor\n",
					hdr.xorByte);
		return -1;
	}
	if (strncmp(hdr.signature,"DSKIMG",6)) {
		log(1, "Invalid signature: [%s]\n", hdr.signature);
		return -1;
	}
	if (strncmp(hdr.identifier,"GARMIN",6)) {
		log(1, "Invalid identifier: [%s]\n", hdr.identifier);
		return -1;
	}
	log(17, "Fat offset: %d hdr:%d\n", hdr.fat_offset, sizeof(hdr));
	log(15, "File: [%s]\n", g->file);
	log(10, "Desc1:[%s]\n", hdr.desc1);
	log(10, "Desc2:[%s]\n", hdr.desc2);
	*blocksize = get_blocksize(&hdr);
	log(15, "Blocksize: %u\n", *blocksize);
	*dataoffset = hdr.dataoffset;
	*fatoffset = hdr.fat_offset;
	log(15, "Dataoffset: %u[%08X]\n", *dataoffset, *dataoffset);
	return 1;
}

static void gar_register_gmp_files(struct gimg *g, char *gmpfile)
{
	struct hdr_gmp_t gmp;
	char buf[20], *cp;
	char buf1[20];
	int rc;
	off_t gmpoff;
	strcpy(buf, gmpfile);
	cp = strchr(buf, '.');
	if (!cp)
		return;
	*cp = '\0';
	gmpoff = gar_file_offset(g, gmpfile);
	log(10, "GMP %s at %ld\n", buf, gmpoff);
	glseek(g, gmpoff, SEEK_SET);
	rc = gread(g, &gmp, sizeof(gmp));
	if (rc != sizeof(gmp)) {
		log(1, "Error reading GMP at %ld\n", gmpoff);
		return;
	}
	gar_log_file_date(11, "GMP Created", &gmp.hsub);
	log(11, "GMP type:[%s] len=%d vs %d\n", gmp.hsub.type, gmp.hsub.length, sizeof(gmp));
	log(11, "GMP TRE at %d\n", gmp.tre_offset);
	log(11, "GMP RGN at %d\n", gmp.rgn_offset);
	log(11, "GMP LBL at %d\n", gmp.lbl_offset);
	log(11, "GMP NET at %d\n", gmp.net_offset);
	log(11, "GMP NOD at %d\n", gmp.nod_offset);
	log(11, "GMP UN1 at %d\n", gmp.unknown1);
	log(11, "GMP UN2 at %d\n", gmp.unknown2);
	sprintf(buf1, "%s.TRE", buf);
	gar_fat_add_file(g, buf1, gmp.tre_offset + gmpoff);
	sprintf(buf1, "%s.RGN", buf);
	gar_fat_add_file(g, buf1, gmp.rgn_offset + gmpoff);
	sprintf(buf1, "%s.LBL", buf);
	gar_fat_add_file(g, buf1, gmp.lbl_offset + gmpoff);
	sprintf(buf1, "%s.NET", buf);
	gar_fat_add_file(g, buf1, gmp.net_offset + gmpoff);
	sprintf(buf1, "%s.NOD", buf);
	gar_fat_add_file(g, buf1, gmp.nod_offset + gmpoff);
}

static void gar_check_nt_map(struct gimg *g)
{
	int rc;
	char **imgs;
	int nimgs;

	imgs = gar_file_get_subfiles(g, &nimgs, "GMP");
	if (nimgs) {
		log(4, "NT Map registering files\n");
		for (rc = 0; rc < nimgs; rc++) {
			gar_register_gmp_files(g, imgs[rc]);
		}
		free(imgs);
		g->is_nt = 1;
	}
}

int gar_img_load_dskimg(struct gar *gar, char *file, int tdbbase, int data,
		double north, double east, double south, double west)
{
	struct gimg *g;
	int rc;
	unsigned int blocksize;
	unsigned int dataoffset;
	unsigned int fatoffset;
	g = gimg_alloc(gar, file);
	if (!g) {
		log(1,"Out of memory!\n");
		return -1;
	}

	g->tdbbasemap = tdbbase;
	g->file = strdup(file);
	g->fd = open(file, OPENFLAGS);
	if (g->fd < 0) {
		log(1, "Can not open file: [%s] errno=%d(%s)\n", 
				g->file, errno, strerror(errno));
		return -1;
	}
	read(g->fd, &g->xor, sizeof(g->xor));
	lseek(g->fd, 0, SEEK_SET);
	if (g->xor) {
		log(1, "Map is XORed you can use garxor to speed the reading\n");
	}

	if (gar_load_img_hdr(g, &dataoffset, &blocksize, &fatoffset) < 0) {
		log(1, "Failed to load header from: [%s]\n", g->file);
		return -1;
	}

	rc = gar_load_fat(g, dataoffset, blocksize, fatoffset);
	if (rc == 0)
		return -1;
	gar_check_nt_map(g);
	if (data) {
		// FIXME: When we have a TDB we can skip this
		// but when no TDB must load, we will keep
		// the dskimg loaded and deal w/ subfiles
		// We can use MPS file to do this
		gar_load_subfiles(g);
		log(6, "Loaded %d mapsets\n", g->mapsets);
	}
	gar_init_mdr(g);
#if 0
	g->north = north;
	g->east = east;
	g->south = south;
	g->west = west;
#endif
	list_append(&g->l, &gar->limgs);
	return 1;
}

static int gar_is_gmapsupp(char *file)
{
	int rc, fd;
	struct hdr_img_t hdr;
	unsigned char xor;
	unsigned char *cp;
	fd = open(file, OPENFLAGS);
	if (fd < 0)
		return -1;
	rc = read(fd, &hdr, sizeof(struct hdr_img_t));
	close(fd);
	if (rc < 0) {
		log(7, "Read error: %d(%s)\n", errno, strerror(errno));
		return -1;
	}
	if (rc != sizeof(struct hdr_img_t)) {
		log(11, "Error reading header want %d got %d\n",
			sizeof(struct hdr_img_t), rc);
		return 0;
	}
	if (hdr.xorByte != 0) {
		xor = hdr.xorByte;
		cp = (unsigned char *)&hdr;
		for (rc = 0; rc < sizeof(struct hdr_img_t); rc++) {
			*cp = *cp ^ xor;
			cp++;
		}
	}
	if (strncmp(hdr.signature,"DSKIMG",6)) {
		return 0;
	}
	if (strncmp(hdr.identifier,"GARMIN",6)) {
		return 0;
	}

	return 1;
}

static void gar_calc_tdblevels(struct gar *gar)
{
	int minbits = 0;
	int maxbits = 0;
	int p;
	struct gimg *g;
	list_for_entry(g, &gar->limgs, l) {
		if (g->tdbbasemap) {
			minbits = g->basebits;
		} else {
			p = g->basebits+g->zoomlevels;
			if (p > maxbits)
				maxbits = p;
		}
	}
	gar->basebits = minbits;
	gar->zoomlevels = maxbits - minbits;
}

int gar_img_load(struct gar *gar, char *file, int data)
{
	if (gar_is_gmapsupp(file) == 1) {
		log(1, "Loading %s as disk image\n", file);
		return gar_img_load_dskimg(gar, file, 0, data, 0,0,0,0);
	} else {
		int rc;
		log(1, "Loading %s as TDB\n", file);
		rc = gar_parse_tdb(gar, file, data);
		gar_calc_tdblevels(gar);
		return rc;
	}
}

struct gimg *gar_get_dskimg(struct gar *gar, char *file)
{
	struct gimg *g;

	if (gar->tdbloaded && !file)
		return NULL;

	list_for_entry(g, &gar->limgs, l) {
		if (!file)
			return g;
		if (!strcmp(file, g->file))
			return g;
	}
	return NULL;
}
