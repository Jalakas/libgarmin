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
#include <sys/types.h>
#include <sys/stat.h>
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

log_fn glogfn;

void gar_log_file_date(int l, char *pref, struct hdr_subfile_part_t *h)
{
	log(l, "%s: %04d-%02d-%02d %02d:%02d:%02d\n",
		pref, h->year, h->month, h->day,
		h->hour, h->min, h->sec);
}

static void inline gcheckfd(struct gimg *g)
{
	if (g->fd == -1) {
		g->fd = open(g->file, O_RDONLY);
		if (g->fd < 0) {
			log(1, "Error can not open:[%s] errno=%d(%s)\n",
				g->file, errno, strerror(errno));
		}
	}
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
	log(1, "%s initializing ...\n", LIBVERSION);
	list_init(&gar->limgs);
	return gar;
}

struct gar *gar_init(char *tdb, log_fn l)
{
	struct gar_config cfg;
	memset(&cfg, 0, sizeof(struct gar_config));
	cfg.opm = OPM_GPS;
	return gar_init_cfg(tdb, l, &cfg);
}

void gar_free(struct gar *g)
{
	log(1, "Implement me\n");
}

static int gar_load_img_hdr(struct gimg *g, unsigned int *dataoffset, unsigned int *blocksize)
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
	log(10, "File: [%s]\n", g->file);
	log(10, "Desc1:[%s]\n", hdr.desc1);
	log(10, "Desc2:[%s]\n", hdr.desc2);
	*blocksize = get_blocksize(&hdr);
	log(10, "Blocksize: %u\n", *blocksize);
	*dataoffset = hdr.dataoffset;
	log(10, "Dataoffset: %u[%08X]\n", *dataoffset, *dataoffset);
	return 1;
}

int gar_img_load_dskimg(struct gar *gar, char *file, int tdbbase, int data,
		double north, double east, double south, double west)
{
	struct gimg *g;
	int rc;
	int blocksize;
	int dataoffset;
	g = gimg_alloc(gar, file);
	if (!g) {
		log(1,"Out of memory!\n");
		return -1;
	}

	g->tdbbasemap = tdbbase;
	g->file = strdup(file);
	g->fd = open(file, O_RDONLY);
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
	if (gar_load_img_hdr(g, &dataoffset, &blocksize) < 0) {
		log(1, "Failed to load header from: [%s]\n", g->file);
		return -1;
	}

	rc = gar_load_fat(g, dataoffset, blocksize);
	if (rc == 0)
		return -1;
	
	if (data) {
		gar_load_subfiles(g);
		log(1, "Loaded %d mapsets\n", g->mapsets);
	}
	g->north = north;
	g->east = east;
	g->south = south;
	g->west = west;
	list_append(&g->l, &gar->limgs);
	return 1;
}

static int gar_is_gmapsupp(char *file)
{
	int rc, fd;
	struct hdr_img_t hdr;
	unsigned char xor;
	unsigned char *cp;
	fd = open(file, O_RDONLY);
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

int gar_img_load(struct gar *gar, char *file, int data)
{
	if (gar_is_gmapsupp(file) == 1) {
		log(1, "Loading %s as disk image\n", file);
		return gar_img_load_dskimg(gar, file, 0, data, 0,0,0,0);
	} else {
		log(1, "Loading %s as TDB\n", file);
		return gar_parse_tdb(gar, file, data);
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
