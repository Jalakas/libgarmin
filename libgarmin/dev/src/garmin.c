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

#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_fat.h"
#include "garmin_rgn.h"
#include "garmin_lbl.h"

log_fn glogfn;

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

struct gar *gar_init(char *tbd, log_fn l)
{
	struct gar *gar;

	gar = calloc(1, sizeof(*gar));
	if (!gar)
		return NULL;

	if (l) {
		gar->logfn = l;
		glogfn = l;
	}

	log(1, "libgarmin initializing ...\n");
	list_init(&gar->limgs);
	if (tbd) {
		gar->filetdb = strdup(tbd);
		if (!gar->filetdb) {
			log(1,"Out of memory!\n");
			free(gar);
			return NULL;
		}
	}
	return gar;
}

void gar_free(struct gar *g)
{
	log(1, "Implement me\n");
}

int gar_load_tbd(char *tdb)
{
	return -1;
}

static int gar_load_img_hdr(struct gimg *g)
{
	int rc;
	rc = read(g->fd, &g->hdr, sizeof(struct hdr_img_t));
	if (rc < 0) {
		log(7, "Read error: %d(%s)\n", errno, strerror(errno));
		return -1;
	}
	if (rc != sizeof(struct hdr_img_t)) {
		log(7, "Error reading header want %d got %d\n",
			sizeof(struct hdr_img_t), rc);
		return -1;
	}
	if (g->hdr.xorByte != 0) {
		log(1, "Please, xor the file key:%02X, use garxor\n",
					g->hdr.xorByte);
		return -1;
	}
	if (strncmp(g->hdr.signature,"DSKIMG",6)) {
		log(1, "Invalid signature: [%s]\n",g->hdr.signature);
		return -1;
	}
	if (strncmp(g->hdr.identifier,"GARMIN",6)) {
		log(1, "Invalid identifier: [%s]\n",g->hdr.identifier);
		return -1;
	}
	log(10, "File: [%s]\n", g->file);
	log(10, "Desc1:[%s]\n", g->hdr.desc1);
	log(10, "Desc2:[%s]\n", g->hdr.desc2);
	g->blocksize = get_blocksize(&g->hdr);
	log(10, "Blocksize: %u\n", g->blocksize);
	g->dataoffset = g->hdr.dataoffset;
	log(10, "Dataoffset: %u[%08X]\n", g->dataoffset,g->dataoffset);
	return 1;
}

struct gimg *gar_img_load(struct gar *gar, char *file, int data)
{
	struct gimg *g;
	int rc;
	g = gimg_alloc(gar, file);
	if (!g) {
		log(1,"Out of memory!\n");
		return NULL;
	}

	g->file = strdup(file);
	g->fd = open(file, O_RDONLY);
	if (g->fd < 0) {
		log(1, "Can not open file: [%s]\n", g->file);
		return NULL;
	}
	if (gar_load_img_hdr(g) < 0) {
		log(1, "Failed to load header from: [%s]\n", g->file);
		return NULL;
	}

	rc = gar_load_fat(g);
	if (rc == 0)
		return NULL;
	if (!g->dataoffset) {
		g->dataoffset = rc + sizeof(sizeof(struct hdr_img_t));
		log(10, "Dataoffset corrected to: %u\n", g->dataoffset);
	}
	if (data) {
		gar_load_subfiles(g);
		log(1, "Loaded %d mapsets\n", g->mapsets);
	}
	list_append(&g->l, &gar->limgs);
	return g;
}
