/*
    Copyright (C) 2007  Alexander Atanasov	<aatanasov@gmail.com>

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

#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_fat.h"
#include "garmin_rgn.h"

/*
   Load FAT structure and ASSUME that all files are continious
 */


int gar_file_get_subfiles(struct gimg *g, char *out[], int size)
{
	const static char subext[] = "TRE";
	struct fat_entry *fe;
	char *cp;
	int cf = 0;
	list_for_entry(fe, &g->lfatfiles, l) {
		cp = strrchr(fe->filename,'.');
		if (!cp)
			continue;
		cp ++;
		if (!strcmp(cp, subext)) {
			out[cf++] = fe->filename;
		}
		if (cf == size)
			break;
	}
	return cf;
}

ssize_t gar_subfile_offset(struct gar_subfile *sub, char *ext)
{
	struct fat_entry *fe;
	struct gimg *g = sub->gimg;
	char fn[20];
	sprintf(fn,"%s.%s", sub->mapid, ext);
	list_for_entry(fe, &g->lfatfiles, l) {
		if (!strcmp(fe->filename, fn))
			return fe->offset;
	}

	return 0;
}

ssize_t gar_file_size(struct gimg *g, char *ext)
{
	struct fat_entry *fe;
	char *cp;
	list_for_entry(fe, &g->lfatfiles, l) {
		cp = strrchr(fe->filename,'.');
		if (!cp)
			continue;
		cp ++;
		if (!strcmp(cp, ext))
			return fe->size;
	}

	return 0;
}

struct fat_entry *gar_fat_get_fe_by_name(struct gimg *g, char *name)
{
	struct fat_entry *fe;

	list_for_entry(fe, &g->lfatfiles, l) {
		if (!strcmp(fe->filename, name))
			return fe;
	}
	return NULL;
}

static int gar_add_fe(struct gimg *g, struct FATblock_t *fent)
{
	struct fat_entry *fe, *fe1;
	char *cp;

	if(!strncmp(fent->name,"MAPSOURC",8) || !strncmp(fent->name,"SENDMAP2", 8)) {
		log(7, "Skipping MAPSOURCE or SENDMAP file\n");
		return 0;
	}

	if (fent->name[0] == '.') {
		log(10,"Skipping root directory\n");
		return 0;
	}
	fe = calloc(1, sizeof(*fe));
	if (!fe)
		return -1;
	memcpy(fe->filename, fent->name, 8);
	cp = strchr(fe->filename, ' ');
	if (!cp)
		cp = fe->filename + 7;
	*cp++ = '.';
	memcpy(cp, fent->type, 3);
	cp += 3;
	*cp = '\0';
	fe->size = fent->size;
	fe->offset = fent->blocks[0] * g->blocksize;
	log(11, "File: [%s] size:[%ld] offset:[%ld/%08X]\n", fe->filename,
		fe->size, fe->offset,fe->offset);
	fe1 = gar_fat_get_fe_by_name(g, fe->filename);
	if (fe1) {
		// check to see if the offset is less
		if (fe1->offset > fe->offset) {
			log(10, "Correcting offset for [%s]\n", fe->filename);
			fe1->offset = fe->offset;
		}
		if (fe1->size < fe->size)
			fe1->size = fe->size;
		free(fe);
	} else {
		log(11, "Creating FAT file:[%s]\n", fe->filename);
		list_append(&fe->l, &g->lfatfiles);
	}
	return 0;
}

int gar_load_fat(struct gimg *g)
{
	struct FATblock_t fent;
	ssize_t s = sizeof(struct FATblock_t);
	int count = 0;
	int rc;
	struct fat_entry *fe;

	
	/* Read dummy FAT entries first */
	while ((rc = read(g->fd, &fent, s)) == s) {
		if (fent.flag != 0x00)
			break;
		count ++;
	}
	do {
		if(fent.flag != 0x01)
			break;
		gar_add_fe(g, &fent);
		count ++;
	} while ((rc = read(g->fd, &fent, s)) == s);
	
	if (!count) {
		log(1, "Failed to read FAT\n");
		return 0;
	}
	log(1, "FAT Directory\n");
	list_for_entry(fe, &g->lfatfiles, l) {
		log(1,"%s %d\n",fe->filename,fe->size);
	}

	return count * s;
}

int gar_fat_file2fd(struct gimg *g, char *name, int fd)
{
	struct fat_entry *fe;
	int sz, rc;
	char buf[4096];
	fe = gar_fat_get_fe_by_name(g, name);
	if (!fe) {
		log(1, "%s not found in image\n", name);
		return -1;
	}
	sz = fe->size;
	lseek(g->fd, fe->offset, SEEK_SET);
	while (sz) {
		rc = read(g->fd, buf, sz > sizeof(buf) ? sizeof(buf) : sz);
		if (rc < 0) {
			log(1, "Error reading file\n");
			return -1;
		}
		write(fd, buf, rc);
		sz -= rc;
	}
	return 0;
}