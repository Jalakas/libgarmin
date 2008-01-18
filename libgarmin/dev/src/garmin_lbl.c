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
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "garmin_fat.h"
#include "garmin_lbl.h"
#include "garmin_rgn.h"
#include "garmin_net.h"
#include "bsp.h"
// process .LBL 

static const char str6tbl1[] = {
	' ','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R'
	,'S','T','U','V','W','X','Y','Z'
	,0,0,0,0,0
	,'0','1','2','3','4','5','6','7','8','9'
	,0,0,0,0,0,0
};

static const char str6tbl2[] = {
	//@   !   "   #   $   %   &    '   (   )   *   +   ,   -   .   /
	'@','!','"','#','$','%','&','\'','(',')','*','+',',','-','.','/'
	,0,0,0,0,0,0,0,0,0,0
	//:   ;   <   =   >   ?
	,':',';','<','=','>','?'
	,0,0,0,0,0,0,0,0,0,0,0
	//[    \   ]   ^   _
	,'[','\\',']','^','_'
};


static const char str6tbl3[] = {
    '`','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r',
's','t','u','v','w','x','y','z'
};


static int gar_lbl_decode6(struct bspfd *bp, u_int8_t *out, ssize_t len)
{
	int c;
	unsigned char ch;
	u_int8_t *cp;
	int sz = 0;
	cp = out;
	while ((c = bsp_fd_get_bits(bp,6)) > -1) {
		log(20,"got c:%d[%02X]\n",c,c);
		if (c > 0x2f)
			break;
		ch = str6tbl1[c];
		if (ch == 0) {
			if (c == 0x1C) {
				c = bsp_fd_get_bits(bp,6);
				if (c < 0)
					break;
				ch = str6tbl2[c];
				*cp++ = ch;
				sz++;
			} else if (c == 0x1B) {
				c = bsp_fd_get_bits(bp,6);
				if (c < 0)
					break;
				ch = str6tbl3[c];
				*cp++ = ch;
				sz++;
			} else if (c == 0x1D) {	// delimiter for formal name
				*cp++ = '|';
				sz++;
			} else if (c == 0x1E) {
				*cp++ = '_';	// hide previous symbols
				sz++;
			} else if (c == 0x1F) {
				*cp++ = '^';	// hide next symbols
				sz++;
			} else if (c >= 0x20 && c <= 0x2F ) {
				*cp++ = '@';
				sz++;
			}
		} else {
			*cp++ = ch;
			sz++;
		}
		if (sz >= len-1)
			break;
	}
	*cp = '\0';
	return sz;
}

static int gar_lbl_decode8(struct bspfd *bp, u_int8_t *out, ssize_t len)
{
	int c, sz = 0;
	unsigned char *cp = out;
	while ((c = bsp_fd_get_bits(bp,8)) > -1) {
		if (!c)
			break;
		if (c == 0x1A) {
			*cp++ = '$';
			sz++;
		} else if (c == 0x1B) {
			*cp++ = '#';
			sz++;
		} else if (c == 0x1C) {
			*cp++ = '~';
			sz++;
		} else if (c == 0x1D) {	// delimiter for formal name
			*cp++ = '|';
			sz++;
		} else if (c == 0x1E) {
			*cp++ = '_';	// hide previous symbols
			sz++;
		} else if (c == 0x1F) {
			*cp++ = '^';	// hide next symbols
			sz++;
		} else if (c>= 0x01 && c<=0x06) {
			*cp++ = '@';
			sz++;
		} else {
			*cp++ = c;
			sz ++;
		}
		if (sz >=len-1)
			break;
	}
	*cp = '\0';
	return sz;
}

static int gar_lbl_decode9(struct bspfd *bp, u_int8_t *out, ssize_t len)
{
	return gar_lbl_decode8(bp, out, len);
}

static int gar_lbl_decode10(struct bspfd *bp, u_int8_t *out, ssize_t len)
{
	return gar_lbl_decode8(bp, out, len);
}

static int gar_lbl_decode16(struct bspfd *bp, u_int8_t *out, ssize_t len)
{
	int c, sz = 0;
	unsigned char *cp = out;
	char d[64];
	int i;
//	*(cp+sz) = '\0';
//	return sz;

	while ((c = bsp_fd_get_bits(bp,8)) > -1) {
#if 0
		if (c == 0x1A) {
			*cp++ = '$';
			sz++;
		} else if (c == 0x1B) {
			*cp++ = '#';
			sz++;
		} else if (c == 0x1C) {
			*cp++ = '~';
			sz++;
		} else if (c == 0x1D) {	// delimiter for formal name
			*cp++ = '|';
			sz++;
		} else if (c == 0x1E) {
			*cp++ = '_';	// hide previous symbols
			sz++;
		} else if (c == 0x1F) {
			*cp++ = '^';	// hide next symbols
			sz++;
		} else if (c>= 0x01 && c<=0x06) {
			*cp++ = '@';
			sz++;
		} else {
			*cp++ = c;
			sz ++;
		}
#endif
/*		*cp++ = c>>8;
		sz++;
		if (sz >=len-1)
			break;
		*cp++ = c&0xff;
		sz++;
		*/
		sz += snprintf(cp+sz, len-sz, "%02x", c);
		if (sz >=len-1 || sz > 64)
			break;
		if (c == 1)
			break;
	}
	*(cp+sz) = '\0';
	return sz;
}

static struct gar_lbl_t *gar_alloc_lbl(void)
{
	return calloc(1, sizeof(struct gar_lbl_t));
}


static u_int32_t gar_lbl_offset(struct gar_subfile *sub ,u_int32_t offlbl, int type)
{
	u_int32_t off1, off = 0xFFFFFFFF;
	switch(type) {
		case L_LBL:
			if (sub->lbl)
				off = sub->lbl->offset + sub->lbl->lbl1off + ( offlbl << sub->lbl->addrshift);
			break;
		case L_POI:
			if (sub->lbl) {
				struct gimg *gimg = sub->gimg;
				char b[3];
				int rc;
				off = sub->lbl->offset + sub->lbl->lbl6off +  offlbl; 
				if (glseek(gimg, off, SEEK_SET) != off) {
					log(1, "LBL: Error can not seek to %zd\n", off);
					return 0xFFFFFFFF;
				}
				rc = gread(gimg, b, 3);
				if (rc!=3)
					return 0xFFFFFFFF;
				off = *(u_int32_t *)b;
				off &= 0x3FFFFF;
				off = sub->lbl->offset + sub->lbl->lbl1off + ( off << sub->lbl->addrshift);
			}
			break; 
		case L_NET:
			off1 = gar_net_get_lbl_offset(sub, offlbl, 0);
			if (off1) {
				off = sub->lbl->offset + sub->lbl->lbl1off + (off1 << sub->lbl->addrshift);
			}
			break;
		default:
			log(1, "Unknown label type: %d\n", type);
	}
	return off;
}

int gar_get_lbl(struct gar_subfile *sub, off_t offset, int type, u_int8_t *buf, int buflen)
{
	u_int32_t off;
	struct bspfd bp;
	struct gimg *gimg = sub->gimg;
	*buf = '\0';
	if (!sub->lbl || !sub->lbl->decode)
		return 0;
	if (offset == 0)
		return 0;

	off = gar_lbl_offset(sub, offset, type);

	if (off == 0xFFFFFFFF)
		return 0;

	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %zd\n", off);
		return -1;
	}

	bsp_fd_init(&bp, gimg);
	return sub->lbl->decode(&bp, buf, buflen);
}

int gar_init_lbl(struct gar_subfile *sub)
{
	struct hdr_lbl_t lbl;
	struct gar_lbl_t *l;
	off_t off;
	struct gimg *gimg = sub->gimg;
	int rc;

	log(11, "LBL initializing ...\n");
	off = gar_subfile_offset(sub, "LBL");
	if (!off) {
		log(1,"No LBL file\n");
		return 0;
	}
	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %ld\n", off);
		return -1;
	}
	rc = gread(gimg, &lbl, sizeof(struct hdr_lbl_t));
	if (rc != sizeof(struct hdr_lbl_t)) {
		log(1, "LBL: Can not read header\n");
		return -1;
	}
	if (strncmp("GARMIN LBL", lbl.hsub.type, 10)) {
		log(1, "LBL: Invalid header type:[%s]\n", lbl.hsub.type);
		return -1;
	}
	gar_log_file_date(11, "LBL Created", &lbl.hsub);
	l = gar_alloc_lbl();
	if (!l) {
		log(1, "LBL: Out of memory\n");
		return -1;
	}
	switch(lbl.coding) {
		case 0x06:
			log(11,"LBL: Uses 6bit coding\n");
			l->decode = gar_lbl_decode6;
			l->bits = 6;
			break;
		case 0x08:
			log(11,"LBL: Uses 8bit coding\n");
			l->decode = gar_lbl_decode8;
			l->bits = 8;
			break;
		case 0x09:
			log(11,"LBL: Uses 9bit coding\n");
			l->decode = gar_lbl_decode9;
			l->bits = 9;
			break;
		case 0x0A:
			log(11,"LBL: Uses 10bit coding\n");
			l->decode = gar_lbl_decode10;
			l->bits = 0x0a;
			break;
		case 0x0B:
			log(11,"LBL: Uses ???0bbit coding\n");
			l->decode = gar_lbl_decode16;
			l->bits = 0x0b;
			break;
		default:
			log(1,"LBL: %02X unknown coding\n",lbl.coding);
			break;
	};
	if (lbl.hsub.length > 0xAA) {
		if (1250 <= lbl.codepage && lbl.codepage <=1258) {
			sprintf(l->codepage,"Windows-%d", lbl.codepage);
		} else if (lbl.codepage == 950)
			sprintf(l->codepage,"Big5");
		else 
			sprintf(l->codepage,"ascii");
		log(11,"LBL: Uses %s encoding:%d\n", l->codepage,lbl.codepage);
	}
	l->offset = gar_subfile_baseoffset(sub, "LBL");//off;
	l->lbl1off = lbl.lbl1_offset;
	l->lbl1size = lbl.lbl1_length;
	l->lbl6off = lbl.lbl6_offset;
	l->lbl6size = lbl.lbl6_length;
	l->addrshift = lbl.addr_shift;
	l->addrshiftpoi = lbl.lbl6_addr_shift;
	l->lbl6_glob_mask = lbl.lbl6_glob_mask;
	sub->lbl = l;
	return 0;
}

void gar_free_lbl(struct gar_subfile *sub)
{
	free(sub->lbl);
	sub->lbl = NULL;
}

static int gar_get_at(struct gar_subfile *sub, off_t offset, char *buf, int buflen)
{
	u_int32_t off = offset;
	struct bspfd bp;
	struct gimg *gimg = sub->gimg;
	*buf = '\0';
	if (off == 0xFFFFFFFF)
		return -1;

	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %zd\n", off);
		return -1;
	}

	bsp_fd_init(&bp, gimg);
	if (!sub->lbl->decode)
		return 0;
	return sub->lbl->decode(&bp, (u_int8_t *)buf, buflen);
}

int gar_init_srch(struct gar_subfile *sub, int what)
{
	struct hdr_lbl_t lbl;
	struct gimg *gimg = sub->gimg;
	off_t off, off1;
	int nc, rc;
	int idx;
	char *rb;
	char *cp;
	char buf[1024];
	off = gar_subfile_offset(sub, "LBL");
	if (!off) {
		log(1,"No LBL file\n");
		return 0;
	}

	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %ld\n", off);
		goto outerr;
	}
	rc = gread(gimg, &lbl, sizeof(struct hdr_lbl_t));
	if (rc != sizeof(struct hdr_lbl_t)) {
		log(1, "LBL: Can not read header\n");
		goto outerr;
	}

	off = gar_subfile_baseoffset(sub, "LBL");
	off1 = off;
	if (what == 0) {
		nc = lbl.lbl2_length/lbl.lbl2_rec_size;
		log(1, "%d countries defined %d\n", nc,lbl.lbl2_length);
		if (!nc)
			goto out;
		sub->countries = calloc(nc + 1, sizeof(char *));
		if (!sub->countries)
			goto outerr;
		idx = 1;
		rb = malloc(lbl.lbl2_length);
		if (!rb) {
			free(sub->countries);
			sub->countries = NULL;
			goto outerr;
		}
		off += lbl.lbl2_offset;
		if (glseek(gimg, off, SEEK_SET) != off) {
			log(1, "LBL: Error can not seek to %ld\n", off);
			goto outerr;
		}
		rc = gread(gimg, rb, lbl.lbl2_length);
		if (rc != lbl.lbl2_length) {
			log(1, "LBL: Error reading countries\n");
			free(rb);
			free(sub->countries);
			sub->countries = NULL;
			goto outerr;
		}
		cp = rb;
		while (cp < rb + lbl.lbl2_length) {
			off = *(u_int32_t *)cp;
			off &= 0x00FFFFFF;
			off <<= lbl.addr_shift;
			off += off1 + lbl.lbl1_offset;// + sizeof(struct hdr_lbl_t);
			gar_get_at(sub, off, buf, sizeof(buf));
			log(15, "LBL: CNT[%d] off=%03lX [%s]\n", idx, off, buf);
			sub->countries[idx] = strdup(buf);
			idx++;
			cp += lbl.lbl2_rec_size;
		}
		sub->ccount = idx;
		free(rb);
		//return idx;
		goto out;
	}
	nc = lbl.lbl3_length/lbl.lbl3_rec_size;
	log(1, "%d regions defined sz=%d\n", nc,lbl.lbl3_rec_size);
	if (!nc)
		goto out;
	sub->regions = calloc(nc + 1, sizeof(struct region_def *));
	if (!sub->regions)
		goto outerr;
	rb = malloc(lbl.lbl3_length);
	off = off1 + lbl.lbl3_offset;
	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %ld\n", off);
		goto outerr;
	}
	rc = gread(gimg, rb, lbl.lbl3_length);
	if (rc != lbl.lbl3_length) {
		log(1, "LBL: Error reading regions\n");
		free(rb);
		free(sub->regions);
		sub->regions = NULL;
		goto outerr;
	}
	cp = rb;
	idx = 1;
	while (cp < rb + lbl.lbl3_length) {
		off = *(u_int32_t *)(cp+2);
		off &= 0x00FFFFFF;
		off <<= lbl.addr_shift;
		off += off1 + lbl.lbl1_offset;// + sizeof(struct hdr_lbl_t);
		gar_get_at(sub, off, buf, sizeof(buf));
		log(15, "LBL: CNT[%d] off=%03lX cnt:%d [%s]\n", idx, off, *(short *)cp,buf);
		sub->regions[idx] = calloc(1, sizeof(struct region_def));
		if (!sub->regions[idx])
			break;
		sub->regions[idx]->name = strdup(buf);
		sub->regions[idx]->country = *(short *)cp;
		idx++;
		cp += lbl.lbl3_rec_size;
	}
	sub->rcount = idx;
	free(rb);
	// cities
	nc = lbl.lbl4_length/lbl.lbl4_rec_size;
	log(1, "%d cities defined sz=%d\n", nc,lbl.lbl4_rec_size);
	if (!nc)
		goto out;
	sub->cities = calloc(nc + 1, sizeof(struct city_def *));
	if (!sub->cities)
		goto outerr;
	rb = malloc(lbl.lbl4_length);
	off = off1 + lbl.lbl4_offset;
	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %ld\n", off);
		goto outerr;
	}
	rc = gread(gimg, rb, lbl.lbl4_length);
	if (rc != lbl.lbl4_length) {
		log(1, "LBL: Error reading cities\n");
		free(rb);
		free(sub->cities);
		sub->regions = NULL;
		goto outerr;
	}
	cp = rb;
	idx = 1;
	while (cp < rb + lbl.lbl4_length) {
		unsigned short tmp = *(unsigned short *)(cp+3);
		sub->cities[idx] = calloc(1, sizeof(struct city_def));
		if (!sub->cities[idx])
			break;
		sub->cities[idx]->region_idx = tmp & 0x1fff;
		if (tmp & (1<<15)) {
			sub->cities[idx]->point_idx = *(char *)cp;
			sub->cities[idx]->subdiv_idx = *(short *)(cp+1);
			log(15, "LBL: City def region %d at pointidx: %d, subdividx:%d\n",
				sub->cities[idx]->region_idx,
				sub->cities[idx]->point_idx,
				sub->cities[idx]->subdiv_idx);
		} else {
			off = *(u_int32_t *)(cp);
			off &= 0x00FFFFFF;
			off <<= lbl.addr_shift;
			off += off1 + lbl.lbl1_offset;// + sizeof(struct hdr_lbl_t);
			gar_get_at(sub, off, buf, sizeof(buf));
			log(15, "LBL: CNT[%d] off=%03lX cnt:%d region:%d [%s]\n", idx, off, *(short *)cp,sub->cities[idx]->region_idx,buf);
			sub->cities[idx]->label = strdup(buf);
		}
		idx++;
		cp += lbl.lbl4_rec_size;
	}
	free(rb);
	sub->cicount = idx;

	// lbl7 = index of POI types
	log(1,"lbl7 off=%04X size=%d, recsize=%d\n",
		lbl.lbl7_offset, lbl.lbl7_length,lbl.lbl7_rec_size);
	off = off1 + lbl.lbl7_offset;
	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %ld\n", off);
		goto outerr;
	}
	if (0) {
		unsigned char rec[lbl.lbl7_rec_size];
		idx = 0;
		while (idx < lbl.lbl7_length) {
			gread(gimg, rec, lbl.lbl7_rec_size);
			log(11, "%d: %x\n", idx, *(int *)rec);
			idx+=lbl.lbl7_rec_size;
		}
	}

	// ZIPs
	nc = lbl.lbl8_length/lbl.lbl8_rec_size;
	log(1, "%d ZIPs defined sz=%d\n", nc,lbl.lbl8_rec_size);
	if (!nc)
		goto pois;
	sub->zips = calloc(nc + 1, sizeof(struct zip_def *));
	if (!sub->zips)
		goto outerr;
	rb = malloc(lbl.lbl8_length);
	off = off1 + lbl.lbl8_offset;
	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %ld\n", off);
		goto outerr;
	}
	rc = gread(gimg, rb, lbl.lbl8_length);
	if (rc != lbl.lbl8_length) {
		log(1, "LBL: Error reading ZIPs\n");
		free(rb);
		free(sub->zips);
		sub->zips = NULL;
		goto outerr;
	}
	cp = rb;
	idx = 1;
	while (cp < rb + lbl.lbl8_length) {
		sub->zips[idx] = calloc(1, sizeof(struct zip_def));
		if (!sub->zips[idx])
			break;
		off = *(u_int32_t *)(cp);
		off &= 0x00FFFFFF;
		off <<= lbl.addr_shift;
		off += off1 + lbl.lbl1_offset;
		gar_get_at(sub, off, buf, sizeof(buf));
		log(15, "LBL: ZIP[%d] off=%03lX [%s]\n", idx, off, buf);
		sub->zips[idx]->code = strdup(buf);
		idx++;
		cp += lbl.lbl8_rec_size;
	}
	free(rb);
	sub->czips = idx;
pois:
	// Parse POIs
	// lbl5 = index of POIS sorted by types and names
	log(1,"lbl5 off=%04X size=%d, recsize=%d\n",
		lbl.lbl5_offset, lbl.lbl5_length,lbl.lbl5_rec_size);
	off = off1 + lbl.lbl5_offset;
	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %ld\n", off);
		goto outerr;
	}
	if (1) {
		struct gar_poi_properties *pr;
		unsigned char rec[lbl.lbl5_rec_size];
		int of,si,id;
		struct gobject *o;
		idx = 0;
		while (idx < lbl.lbl5_length) {
			off = off1 + lbl.lbl5_offset + idx;
			if (glseek(gimg, off, SEEK_SET) != off) {
				log(1, "LBL: Error can not seek to %ld\n", off);
				goto outerr;
			}
			gread(gimg, rec, lbl.lbl5_rec_size);
			of = *(int*)(rec) & 0xffffff;
			si = of >> 8;
			id = of & 0xff;
			log(11, "%d: type:%d sdidx:%d idx:%d %x\n", idx, *(u_int8_t *)(rec+3),si,id, *(int *)rec);
			o = gar_get_subfile_object_byidx(sub, si, id, GO_POINT);
			if (o) {
				char *cp = gar_object_debug_str(o);
				if (cp) {
					log(11, "poi:%s\n", cp);
					free(cp);
					cp = gar_get_object_lbl(o);
					if (cp) {
						log(11, "poi:%s\n", cp);
						free(cp);
					}
				}
				pr = gar_get_poi_properties(o->gptr);
				if (pr) {
					gar_log_poi_properties(sub, pr);
					gar_free_poi_properties(pr);
				}
				gar_free_objects(o);
			} else {
					log(11, "not found\n");
			}
			idx+=lbl.lbl5_rec_size;
		}
	}

out:
	return 0;
outerr:
	return -1;
}

static void gar_free_region_def(struct region_def *rd)
{
	if (rd->name)
		free(rd->name);
	free(rd);
}

static void gar_free_city_def(struct city_def *cd)
{
	if (cd->label)
		free(cd->label);
	free(cd);
}

static void gar_free_zip_def(struct zip_def *zd)
{
	if (zd->code)
		free(zd->code);
	free(zd);
}

void gar_free_srch(struct gar_subfile *f)
{
	int i;
	if (f->countries) {
		for (i = 0; i < f->ccount; i++) {
			if (f->countries[i])
				free(f->countries[i]);
		}
		f->ccount = 0;
		free(f->countries);
		f->countries = NULL;
	}

	if (f->regions) {
		for (i = 0; i < f->rcount; i++) {
			if (f->regions[i])
				gar_free_region_def(f->regions[i]);
		}
		f->rcount = 0;
		free(f->regions);
		f->regions = NULL;
	}

	if (f->cities) {
		for (i = 0; i < f->cicount; i++) {
			if (f->cities[i])
				gar_free_city_def(f->cities[i]);
		}
		f->cicount = 0;
		free(f->cities);
		f->cities = NULL;
	}

	if (f->zips) {
		for (i = 0; i < f->czips; i++) {
			if (f->zips[i])
				gar_free_zip_def(f->zips[i]);
		}
		f->czips = 0;
		free(f->zips);
		f->zips = NULL;
	}
}

#define POI_STREET_NUM		(1<<0)
#define POI_STREET		(1<<1)
#define POI_CITY		(1<<2)
#define POI_ZIP			(1<<3)
#define POI_PHONE		(1<<4)
#define POI_EXIT		(1<<5)
#define POI_TIDE_PREDICT	(1<<6)
#define POI_UNKNOW		(1<<7)

struct gar_poi_properties {
	u_int8_t	flags;
	u_int32_t	lbloff;
	char		*number;
	u_int32_t	streetoff;
	unsigned short	cityidx;
	unsigned short	zipidx;
	char		*phone;
	u_int32_t	exitoff;
	u_int32_t	tideoff;
};

void gar_log_poi_properties(struct gar_subfile *sub, struct gar_poi_properties *p)
{
	char buf[1024];
	log(11, "POI: flags:%x, lblat:%d, number=%s,city=%d,zip=%d,phone=%s\n",
		p->flags, p->lbloff, p->number?:"", p->cityidx,
		p->zipidx, p->phone?:"");
	if (p->flags&POI_CITY) {
		if (p->cityidx < sub->cicount) {
			if (sub->cities[p->cityidx]->label) {
				log(11, "POI: city=%s\n", sub->cities[p->cityidx]->label);
			} else {
				struct gobject *o;
				o = gar_get_subfile_object_byidx(sub,
				sub->cities[p->cityidx]->subdiv_idx, 
				sub->cities[p->cityidx]->point_idx, 
				GO_POINT);
				if (o) {
					char *l;
					l = gar_get_object_lbl(o);
					if (l) {
						log(11, "POI: city=%s\n", l);
						free(l);
					}
					gar_free_objects(o);
				}
			}
		} else
			log(11, "POI: invalid cityidx\n");
	}
	if (p->flags&POI_ZIP) {
		if (p->zipidx < sub->czips)
			log(11, "POI: zip=%s\n", sub->zips[p->zipidx]->code);
		else
			log(11, "POI: invalid zipidx\n");
	}
	gar_get_lbl(sub, p->lbloff, L_LBL, buf, sizeof(buf));
	log(11, "POI: label=%s\n", buf);
	if (p->flags&POI_STREET) {
		gar_get_lbl(sub, p->streetoff, L_LBL, buf, sizeof(buf));
		log(11, "POI: street=%s\n", buf);
	}
}

void gar_free_poi_properties(struct gar_poi_properties *p)
{
	if (p->number)
		free(p->number);
	if (p->phone)
		free(p->phone);
	free(p);
}

static int gar_decode_base11(unsigned char *cp, char *out, int l)
{
	int sz = 0;
	int a, b;
	int done = 0;
	int rb = 0;
	*cp &= 0x7f;
	do {
		done = (*cp) & 0x80;
		*cp &= 0x7f;
		a = (*(cp+rb))/11;
		b = (*(cp+rb))%11;
		if (a!=10&&b!=10) {
			sz += sprintf(out+sz, "%d%d", a, b);
		} else {
			if (a!=10)
				sz += sprintf(out+sz, "%d ", a);
			else if (b!=10)
				sz += sprintf(out+sz, " %d", b);
			else
				sz += sprintf(out+sz, " ");
		}
		if (sz >= l-3)
			break;
		rb++;
		cp++;
	} while (!done);
	return rb;
}
static unsigned char gar_get_setbit(unsigned char b, int bit)
{
	int set=0;
	int i;
	for (i=0; i < 8; i++) {
		if (b&(1<<i)) {
			if (set==bit)
				return i;
			set++;
		}
	}
	return 8;
}

static unsigned char gar_mask_properties(unsigned char glob,unsigned char mask)
{
	int bits=0,i;
	unsigned char ret = glob;
	for (i=0; i < 8; i++) {
		if (glob&(1<<i))
			bits++;
	}
	mask &= 0xFF >> (8-bits);
	for (i=0; i < bits; i++) {
		if (!(mask&(1<<i))) {
			ret &= ~(1<<gar_get_setbit(glob, i));
		}
	}
	return ret;
}

struct gar_poi_properties *gar_get_poi_properties(struct gpoint *poi)
{
	struct gimg *gimg = poi->subdiv->subfile->gimg;
	struct gar_poi_properties *p;
	u_int32_t off;
	u_int8_t fl;
	int tmp;
	char buf[256];
	char l[1024];
	unsigned char *cp = buf;
	if (!poi->is_poi)
		return NULL;
	off = poi->subdiv->subfile->lbl->offset + poi->subdiv->subfile->lbl->lbl6off + poi->lbloffset;
	if (glseek(gimg, off, SEEK_SET) != off) {
		log(1, "LBL: Error can not seek to %zd\n", off);
		return NULL;
	}
	fl = poi->subdiv->subfile->lbl->lbl6_glob_mask;
	log(12, "POI global properties: %x\n", fl);
	tmp = gread(gimg, buf, sizeof(buf));
	if (tmp < 0) {
		log(1, "LBL: Error reading poi properties\n");
		return NULL;
	}
	p = calloc(1, sizeof(*p));
	if (!p)
		return NULL;
	tmp = *(int *)cp&0xFFFFFF;
	cp+=3;
	p->lbloff = tmp & 0x3fffff;
	if (tmp & (1<<23)) {
		log(12, "Partial properties: %x\n", *cp);
//		p->flags = fl-*(signed char *)cp++;//gar_mask_properties(fl, *cp++);
		fl = p->flags = gar_mask_properties(fl, *cp++);
		log(12, "Partial properties: %x\n", p->flags);
	} else
		p->flags = fl;
	if (fl & POI_STREET_NUM) {
		if (*cp&0x80) {
			// base11
			cp += gar_decode_base11(cp, l, sizeof(l));
			p->number = strdup(l);
		} else {
			// ptr to lbl
			tmp = *(int *)cp&0x3FFFFF;
			if (gar_get_lbl(poi->subdiv->subfile, L_LBL, tmp, (unsigned char*)l, sizeof(l)))
				p->number = strdup(l); 
			cp+=3;
		}
	}

	if (fl & POI_STREET) {
		// 3 bytes ptr to lbl1
		p->streetoff = *(int *)cp & 0x3fffff;
		cp+=3;
	}

	if (fl & POI_CITY) {
		if (poi->subdiv->subfile->cicount < 256)
			p->cityidx = *cp++;
		else {
			p->cityidx = *(u_int16_t*)cp;
			cp+=2;
		}
	}

	if (fl & POI_ZIP) {
		if (poi->subdiv->subfile->czips < 256)
			p->zipidx = *cp++;
		else {
			p->zipidx = *(u_int16_t*)cp;
			cp+=2;
		}
	}

	if (fl & POI_PHONE) {
		cp += gar_decode_base11(cp, l, sizeof(l));
		p->phone = strdup(l);
	}
	if (fl & POI_EXIT) {
		// ptr to lblX exits
		p->exitoff = *(int *)cp & 0xffffff;
		cp+=3;
	}
	if (fl & POI_TIDE_PREDICT) {
		// ptr to lblX tide
		p->tideoff = *(int *)cp & 0xffffff;
		cp+=3;
	}
	return p;
}


