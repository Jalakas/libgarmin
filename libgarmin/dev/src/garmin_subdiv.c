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
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "libgarmin.h"
#include "libgarmin_priv.h"
#include "bsp.h"
#include "garmin_rgn.h"
#include "garmin_lbl.h"
#include "garmin_subdiv.h"

struct sign_info_t {
	u_int32_t sign_info_bits;
	int x_has_sign;
	int nx;
	int y_has_sign;
	int ny;
	int extrabit;
};

static void init_si(struct sign_info_t *si)
{
	si->sign_info_bits = 2;
	si->x_has_sign = 1;
	si->nx = 0;
	si->y_has_sign = 1;
	si->ny = 0;
}

//----------------------------------------------------------------------
// signinfor parsing shamelessly stolen from imgdecode
//----------------------------------------------------------------------

static int _bits_per_coord (u_int8_t base, int is_signed, int extra_bit)
{
	int n= 2;

	if ( base <= 9 ) n+= base;
	else n+= (2*base-9);

	if ( is_signed ) ++n;
	if ( extra_bit ) ++n;

	return n;
}

static void bits_per_coord (u_int8_t base, u_int8_t bfirst, int extra_bit,
	int *blong, int *blat, struct sign_info_t *si)
{
	int x_sign_same, y_sign_same;
	int sign_bit;

	// Get bits per longitude

	si->extrabit = !!extra_bit;
	x_sign_same= bfirst & 0x1;
	if ( x_sign_same ) {
		si->x_has_sign = 0;
		si->nx = bfirst & 0x02;
		sign_bit= 0;
		si->sign_info_bits++;
	} else {
		sign_bit= 1;
		si->x_has_sign = 1;
	}

	*blong= _bits_per_coord(base&0x0F, sign_bit, extra_bit);
	// Get bits per latitude

	if ( x_sign_same ) 
		y_sign_same= bfirst & 0x04;
	else 
		y_sign_same= bfirst & 0x02;

	if ( y_sign_same ) {
		si->y_has_sign = 0;
		si->ny = x_sign_same ? bfirst & 0x08 : bfirst & 0x04;
		sign_bit= 0;
		si->sign_info_bits++;
	} else {
		sign_bit= 1;
		si->y_has_sign = 1;
	}

	*blat= _bits_per_coord(base>>4, sign_bit, extra_bit);
}

static int count_bits(unsigned int a)
{
	int bc = 0;
	int i;
	for (i=0; i < 32; i++)
		if (a&(1<<i))
			bc++;
	return bc;
}

#if 0
static void print_bits(unsigned int a)
{
	int i;
	int sp = 0;
	for (i = 31; i >= 0; i--) {
		if (!sp) {
			if (a & (1<<i))
				sp = 1;
		}
		if (sp) {
			log(25,"%d", !!(a & (1<<i)));
		}
	}
	log(25,"\n");
}
#endif

static int bs_get_extra(int x, int eb)
{
	int b;
	b = count_bits(x);
//	printf("bits:%d x = %d\n", b, x);
	if (!eb) {
		if (b > 1) {
			if ((b&1))
				x -= 1;
			else
				x += 1;
		} else if (0){
			if (x < (1<<3))
				x -= 1;
		}
	} else {
		if ( b > 1) {
			if ((b&1))
				x += 1;
			else
				x -= 1;
		}
	}
	b = count_bits(x);
//	printf("x bits:%d x= %d\n", b,x);
	return x;
}

static int bs_get_extra_y(int x, int eb)
{
	int b;
	b = count_bits(x);
//	printf("bits:%d y = %d\n", b, x);
	if (!eb) {
		if (b > 1) {
			if (!(b&1))
				x -= 1;
			else
				x += 1;
		} else if (0){
			if (x < (1<<3))
				x -= 1;
		}
	} else {
		if ( b > 1) {
			if ((b&1))
				x += 1;
			else
				x -= 1;
		}
	}
	b = count_bits(x);
//	printf("y bits:%d y= %d\n", b,x);
	return x;
}

static int bs_get_long_lat(struct bsp *bp, struct sign_info_t *si, int bpx, int bpy, struct gpoly *gp, int shift, int dl)
{
	u_int32_t tmp;
	u_int32_t reg;
	u_int32_t xmask = ~0;
	u_int32_t ymask = ~0;
	u_int32_t xsign, xsign2;
	u_int32_t ysign, ysign2;
	int i,x,y,j;
	int scasex, scasey, scase = 0;
	int total = 0;

	bpy -= si->extrabit;
	xmask = (xmask << (32-bpx)) >> (32-bpx);
	ymask = (ymask << (32-bpy)) >> (32-bpy);
	xsign  = (1 << (bpx - 1));
	ysign  = (1 << (bpy - 1));
	xsign2  = xsign<<1;
	ysign2  = ysign<<1;

	reg = bsp_get_bits(bp, si->sign_info_bits);
	j = 0;
	for (i=0; i < gp->npoints; i++) {
		scasex =0;
		x = 0;
		if (si->extrabit)
			reg = bsp_get_bits(bp, 1);
		reg = bsp_get_bits(bp, bpx);
		if (reg==-1)
			goto out;
		if (si->extrabit) {
			if (reg&1) {
				j = 1 - j;
				//printf("EB:%d\n",j);
			}
		}
		if (si->x_has_sign) {
			tmp = 0;
			if (si->extrabit && reg) {
				reg>>=1;
				reg = bs_get_extra(reg, j);
			}

			while (1) {
				tmp = reg & xmask;
				if(tmp != xsign)
					break;
				scasex++;
				x += tmp - 1;
				reg = bsp_get_bits(bp, bpx);
				if (reg==-1)
					goto out;
			}
			if (tmp < xsign)
				x += tmp;
			else {
				x = tmp - (xsign << 1 ) - x;
			//	x = tmp - x - xsign;
			//	x = - (tmp-x);
			}
		} else {
			x = reg & xmask;
			if (si->extrabit && x) {
				x>>=1;
				x = bs_get_extra(x, j);
			}
			if(si->nx)
				x = -x;
		}
		reg = bsp_get_bits(bp, bpy);
		if (reg==-1)
			goto out;
		scasey = 0;
		y = 0;
		if (si->y_has_sign) {
			tmp = 0;
			if (si->extrabit && reg) {
				reg = bs_get_extra_y(reg, j);
			}
			while (1) {
				tmp = reg & ymask;
				if(tmp != ysign)
					break;
				scasey++;
				y += tmp - 1;
				reg = bsp_get_bits(bp, bpy);
				if (reg==-1)
					goto out;
			}
			if (tmp < ysign)
				y += tmp;
			else {
				y = tmp - (ysign << 1) - y;
			}
		} else {
			y = reg & ymask;
			if (si->extrabit && y) {
				y = bs_get_extra_y(y, j);
			}
			if(si->ny)
				y = -y;
		}
#if 0
		if (scasex || scasey) {
			log(dl, "deltas: %d sc=%d x=%d(%d), sc=%d y=%d(%d)\n", i, 
			scasex, x, bpx-si->x_has_sign, scasey, y, bpy-si->y_has_sign);
			if (scasex) {
				if (abs(x) < (1<<(bpx-si->x_has_sign)))
					log(dl, "ERROR: X %d is less than %d\n",
						x,
						(1<<(bpx-si->x_has_sign)));
			}
			if (scasey) {
				if (abs(y) < (1<<(bpy-si->y_has_sign)))
					log(dl, "ERROR: Y %d is less than %d\n",
						y,
						(1<<(bpy-si->y_has_sign)));
			}
		}
#endif
		if (x == 0 && y == 0) {
			log(15, "Point %d have zero deltas eb=%d\n", total, si->extrabit);
//			break;
		}
		if (x || y) {
			gp->deltas[i].x = x << (shift);
			gp->deltas[i].y = y << (shift);
			//log(dl, "x=%d, y=%d\n", gp->deltas[i].x, gp->deltas[i].y); 
			total ++;
		}
	}
out:
	gp->extrabit = si->extrabit;
	gp->scase = scase;
	return total;
}

static void gar_copy_source(u_int8_t *dp, u_int8_t *ep, u_int8_t **dst, int *len)
{
	int l;
	int i;
	u_int8_t *s;
	l = ep - dp;
	s = malloc(l);
	if (!s)
		return;
	*len = l;
	i = 0;
	while(dp!=ep) {
		s[i] = *dp;
		i++;
		dp++;
	}
	*dst = s;
}

static int gar_parse_poly(u_int8_t *dp, u_int8_t *ep, struct gpoly **ret, int line, int *ok, int cshift)
{
	struct sign_info_t si;
	struct bsp bp;
	u_int8_t *sp = dp;
	u_int32_t total_bytes = 10;
	u_int16_t bs_len;
	u_int8_t bs_info;
	u_int32_t lbloffset;
	struct gpoly *gp;
	int bpx;	// bits per x
	int bpy;	// bits per y
	int two_byte,cnt;
	int extra_bit = 0;
	int dl = 15;

	*ok = 0;
	*ret = NULL;
	init_si(&si);
	gp = calloc(1, sizeof(*gp));
	if (!gp)
		return -1;
	list_init(&gp->l);
//	if (line)
//		gar_copy_source(dp,ep, &gp->source, &gp->slen);
	gp->type = *dp;
	two_byte = gp->type & 0x80;

	if (line) {
		gp->line = 1;
		gp->dir = gp->type & 0x40;
		gp->type &= 0x3F;
	} else {
		gp->type &= 0x7F;
	}
	if (0 &&(gp->type == 0x21 || gp->type == 0x20 || gp->type == 0x22))
		dl = 1;
	dp++;
	lbloffset = *(u_int32_t *)dp;
	lbloffset &= 0x00FFFFFF;
	extra_bit = !!(lbloffset & 0x400000);
	if (line) {
		gp->netlbl = !!(lbloffset & 0x800000);
		if (gp->netlbl) {
		// FIXME read .NET file
		}
	}
	lbloffset = *(u_int32_t *)dp;
	gp->lbloffset = lbloffset & 0x3FFFFF;
	dp+=3;
	gp->c.x = *(u_int16_t *)dp;
	dp+=2;
	gp->c.y = *(u_int16_t *)dp;
	dp+=2;
	gp->c.y = SIGN2B(gp->c.y);
	gp->c.x = SIGN2B(gp->c.x);
	gp->c.y<<=cshift;
	// XXX Check if we have to shift for the extrabit
	gp->c.x<<=cshift;
	log(dl, "shift=%d lat=%f, long=%f\n",
		cshift,
		GARDEG(gp->c.y),
		GARDEG(gp->c.x));

	if (two_byte) {
		bs_len = *(u_int16_t*)dp;
		dp+=2;
		total_bytes += bs_len + 1;
	} else {
		bs_len = *dp;
		dp++;
		total_bytes += bs_len;
	}
	if (dp+bs_len > ep) {
		log(1, "Invalid poligon line:%d len: %d max is %d two_byte=%d bslen=%02X\n", 
		line, bs_len,
		ep - sp, two_byte, bs_len);
		free(gp);
		return total_bytes;
	}
	if (!bs_len) {
		log(1, "Empty poligon definition\n");
		free(gp);
		return total_bytes;
	}
	bs_info = *dp;
	dp++;
	if (extra_bit)
		dl = 11;
#ifdef DBGEXTRA
	if (extra_bit) {
			u_int8_t *t = dp;
			int j;
		log(1, "extra bit bs_len=%d\n", bs_len);
			for (j=0; j < bs_len; j++)
				log(1,"%02X\n", *t++);
//		return total_bytes;
	}
#endif
	bits_per_coord(bs_info, *dp, extra_bit, &bpx, &bpy, &si);
	log(dl ,"%d/%d bits per long/lat  len=%d extra_bit=%d\n", 
		bpx, bpy, bs_len, extra_bit);
//	cnt = 2 + ((bs_len*8)-si.sign_info_bits)/(bpx+bpy-2*si.extrabit);
	cnt = 2 + ((bs_len*8)-si.sign_info_bits)/(bpx+bpy/*+si.extrabit*/);
	log(dl, "Total coordinates: %d\n", cnt);
	gp->deltas = calloc(cnt, sizeof(struct gcoord));
	if (!gp->deltas) {
		log(1, "Can not allocate polygon points!\n");
		free(gp);
		return total_bytes;
	}
	gp->npoints = cnt;
	if ((cnt == 1 && bs_len) || !bs_len) {
		log(1, "Error have %d points but  datalen is %d, bpx=%d, bpy=%d si.sign_info_bits=%d\n",cnt, bs_len, bpx, bpy,si.sign_info_bits);
		log(1, "dp=%02X extra_bit=%d bs_info=%02X\n", *dp,extra_bit,bs_info);
		{
			u_int8_t *t = dp;
			int j;
			for (j=0; j < bs_len; j++)
				log(1,"%02X\n", *t++);
		}
		free(gp->deltas);
		free(gp);
		return total_bytes;
	}
	if (bs_len) {
		bsp_init(&bp, dp, bs_len);
		gp->npoints = bs_get_long_lat(&bp, &si, bpx, bpy, gp, cshift,dl);
	}
	log(dl, "Total real coordinates: %d\n", gp->npoints);
	if (gp->npoints < 1) {
		log(17, "Ignoring polydefinition w/ %d points\n",
			gp->npoints);
		log(17, "Have %d points datalen is %d, bpx=%d, bpy=%d si.sign_info_bits=%d extrabit=%d\n",cnt, bs_len, bpx, bpy,si.sign_info_bits, si.extrabit);
		free(gp->deltas);
		free(gp);
		return total_bytes;
	}
	*ok = 1;
	*ret = gp;
	return total_bytes;
}

static int gar_parse_point(u_int8_t *dp, struct gpoint **ret, int forcest)
{
	struct gpoint *gp;
	u_int32_t i;
	*ret = NULL;
	gp = calloc(1, sizeof(*gp));
	if (!gp)
		return -1;
	*ret = gp;
	gp->type = *dp;
	dp++;
	i = *(u_int32_t *)dp;
	i &= 0x00FFFFFF;
	if (1||forcest) {
		gp->has_subtype = !!(i & 0x800000);
	}
	gp->is_poi = !!(i & 0x400000);
//	i = *(u_int32_t *)dp;
	gp->lbloffset = i & 0x3FFFFF;
	dp+=3;
	gp->c.x = *(u_int16_t *)dp;
	dp+=2;
	gp->c.y = *(u_int16_t *)dp;
	dp+=2;
	gp->c.x = SIGN2B(gp->c.x);
	gp->c.y = SIGN2B(gp->c.y);
	if (gp->has_subtype) {
		gp->subtype = *dp;
		if (!forcest && gp->has_subtype)
			log(1, "Point w/ type:%02X subtype:%02X?!?!\n",gp->type,gp->subtype);

		*ret = gp;
		return 9;
	}
	return 8;
};
#define DMPLBLS
static void dmp_lbl(struct gar_subfile *sub, u_int32_t lbloff, int type)
{
#ifdef DMPLBLS
	struct gimg *g = sub->gimg;
	u_int8_t buf[2048];
	off_t offsave = lseek(g->fd, 0, SEEK_CUR);
	if (lbloff ==0 || lbloff == 0xFFFFFF) {
		return;
	}

	if (gar_get_lbl(sub, lbloff, type, buf, sizeof(buf)) > -1) {
			if (*buf != '^' && *buf != '\0') 
			log(9, "LBL[%04X]:[%s]\n", lbloff, buf);
	}
	lseek(g->fd, offsave, SEEK_SET);
#endif
}

int gar_load_subdiv(struct gar_subfile *sub, struct gar_subdiv *gsub)
{
	ssize_t rsize;
	u_int8_t *data;
	u_int16_t *dp;
	u_int8_t *d, *e;
	int rc,i,j;
	u_int32_t objcnt;
	u_int32_t opnt = 0, oidx = 0, opline = 0, opgon = 0;
	struct gimg *g = sub->gimg;
	struct gpoint *gp;
	u_int32_t pgi = 1, pli = 1, pi = 1, poii = 1;

	// FIXME: Index for an empty subdiv?
	if (gsub->rgn_start == gsub->rgn_end)
		return 0;
	/* regison start is direct offset */
	log(15, "rgnstart: %04X, rgnend: %04X\n", gsub->rgn_start, gsub->rgn_end);
	rsize = gsub->rgn_end - gsub->rgn_start;
	if (rsize < 0) {
		log(1, "Error reading subdiv - negative size!\n");
		return -1;
	}

	log(15, "Subdiv size: %zd\n", rsize);
	if (lseek(g->fd, gsub->rgn_start, SEEK_SET) != gsub->rgn_start) {
		log(1, "Error can not seek to %d\n", gsub->rgn_start);
		return -1;
	}
	data = calloc(rsize, sizeof(u_int8_t));
	if (!data) {
		log(1, "Out of memory reading subdiv\n");
		return -1;
	}
	rc = read(g->fd, data, rsize);
	if (rc != rsize) {
		log(1, "Error reading subdiv want:%d got:%d\n", rsize, rc);
		free(data);
		return -1;
	}
	dp = (u_int16_t *)data;
	objcnt = gsub->haspoints + gsub->hasidxpoints + gsub->haspolylines +
			gsub->haspolygons;
#if 0
	if (!objcnt) {
		log(1, "Error size: %d but no objects!\n", rsize);
		free(data);
		return -1;
	}
#endif
	// FIXME: Rename the variables opnt<->oindx
	if (gsub->haspoints) {
		opnt = (objcnt - 1) * sizeof(u_int16_t);
	}
	if (gsub->hasidxpoints) {
		if (opnt)
			oidx = *dp++;
		else
			oidx = (objcnt - 1) * sizeof(u_int16_t);
	}
	if (gsub->haspolylines) {
		if (opnt || oidx)
			opline = *dp++;
		else
			opline = (objcnt - 1) * sizeof(u_int16_t);
		if (opline > rsize) {
			/* Is it possible to reuse polygons/lines ?? */
			log(7, "Correcting: Polylines offset is invalid\n");
			gsub->haspolylines = 0;
			opline = 0;
		}

	}
	if (gsub->haspolygons) {
		if (opnt || oidx || opline)
			opgon = *dp++;
		else
			opgon = (objcnt - 1) * sizeof(u_int16_t);
		if (opgon > rsize) {
			log(7, "Correcting: Polygons offset is invalid\n");
			gsub->haspolygons = 0;
			opgon = 0;
		}
	}
	log(15, "Have: %d objects: pnts: %s, idxpnts: %s, polyline:%s, poly:%s\n", 
		objcnt,
		gsub->haspoints? "yes" : "no",
		gsub->hasidxpoints? "yes" : "no",
		gsub->haspolylines? "yes" : "no",
		gsub->haspolygons? "yes" : "no");
	log(15, "@Points: %04X, IDXPNTs: %04X, PLINES: %04X, PGONS: %04X\n",
		opnt, oidx, opline, opgon);
	if (gsub->haspoints) {
		d = data + opnt;
		e = data + (oidx ? oidx : opline ? opline : opgon ? opgon : rsize);
		i = 0;
		while (d < e) {
			// decode one pnt
			rc = gar_parse_point(d, &gp, 1);
			gp->c.x <<= gsub->shift;
			gp->c.y <<= gsub->shift;
			gp->c.x += gsub->icenterlng;
			gp->c.y += gsub->icenterlat;
			gp->n = pi++;
			j = 1;
			if (gp->c.x < gsub->west || gp->c.x > gsub->east) {
				log(10, "Point[%d] out of bonds: %f, west=%f, east=%f\n",
					gp->n,
					GARDEG(gp->c.x), GARDEG(gsub->west),
					GARDEG(gsub->east));
				j = 0;
			}
			if (gp->c.y < gsub->south || gp->c.y > gsub->north) {
				log(10, "Point[%d] out of bonds: %f, south=%f, north=%f\n",
					gp->n,
					GARDEG(gp->c.y), GARDEG(gsub->south),
					GARDEG(gsub->north));
				j = 0;
			}
			if (/*gp->type != 0 && */j) {
				gp->subdiv = gsub;
				list_append(&gp->l, &gsub->lpois);
			//	dmp_lbl(sub, gp->lbloffset, L_LBL);
			} else {
				free(gp);
			}
			d+=rc;
		};
	}
	if (gsub->hasidxpoints) {
		d = data + oidx;
		e = data + (opline ? opline : opgon ? opgon : rsize);
		i = 0;
		while (d < e) {
			rc = gar_parse_point(d, &gp, 1);
			gp->c.x <<= gsub->shift;
			gp->c.y <<= gsub->shift;
			gp->c.x += gsub->icenterlng;
			gp->c.y += gsub->icenterlat;
			gp->n = poii++;
			j = 1;
			if (gp->c.x < gsub->west || gp->c.x > gsub->east) {
				log(1, "Poi[%d] out of bonds: %f, west=%f, east=%f\n",
				gp->n,
					GARDEG(gp->c.x), GARDEG(gsub->west), 
					GARDEG(gsub->east));
				j = 0;
			}
			if (gp->c.y < gsub->south || gp->c.y > gsub->north) {
				log(1, "Poi[%d] out of bonds: %f, south=%f, north=%f\n",
					gp->n,
					GARDEG(gp->c.y), GARDEG(gsub->south),
					GARDEG(gsub->north));
				j = 0;
			}
			if (/*gp->type != 0 && */j) {
				gp->subdiv = gsub;
				list_append(&gp->l, &gsub->lpoints);
			//	dmp_lbl(sub, gp->lbloffset, gp->is_poi ? L_POI : L_LBL);
			} else
				free(gp);
			d+=rc;
		};
	}
	if (gsub->haspolylines) {
		int ok = 0;
		struct gpoly *poly;
		d = data + opline;
		e = data + (opgon ? opgon : rsize);
		if (d>=data+rsize  || e > data+rsize) {
			log(1,"Invalid polyline! data=%p rsize=%d ep=%p d=%p e=%p opline=%u opgon=%u\n",
				data, rsize, data+rsize, d, e, opline, opgon);
			return objcnt-1;
		}
		while (d < e) {
			rc = gar_parse_poly(d, e, &poly, 1, &ok, gsub->shift);
			if (ok) {
				poly->n = pli++;
				poly->c.x += gsub->icenterlng;
				poly->c.y += gsub->icenterlat;
				poly->subdiv = gsub;
				list_append(&poly->l, &gsub->lpolylines);
//				dmp_lbl(sub, poly->lbloffset, L_LBL);
			}
			d+=rc;
		}
	}
	if (gsub->haspolygons) {
		int ok = 0;
		struct gpoly *poly;
		d = data + opgon;
		e = data + rsize;
		if (d>=data+rsize || e > data+rsize) {
			log(1,"Invalid polygon! data=%p d=%p e=%p opline=%04X\n",
				data, d, e, opline);
			return objcnt-1;
		}
		while (d < e) {
			rc = gar_parse_poly(d, e, &poly, 0, &ok,gsub->shift);
			if (ok) {
				poly->n = pgi++;
				poly->c.x += gsub->icenterlng;
				poly->c.y += gsub->icenterlat;
				poly->subdiv = gsub;
				list_append(&poly->l, &gsub->lpolygons);
//				dmp_lbl(sub, poly->lbloffset, L_LBL);
			}
			d+=rc;
		}
	}

	return objcnt;
}
