#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <malloc.h>


#define log(x, y ...) fprintf(stdout, ## y)
unsigned char d1[] = {
0x08, 0x00, 0x00, 0x00, 0xd0, 0x01, 0xe6, 0xfe, 0x03, 0x01, 0xab, 0x7a, 0xb1
};

unsigned char d2[] = {
0x05, 0x40, 0x07, 0x00, 0xbc, 0x01, 0x85, 0x00, 0x03, 0x57, 0x6d, 0x12, 0x0a
};

struct gpoly {
	u_int8_t type;
	unsigned dir:1,
		netlbl:1,
		line:1;
	double dlng;
	double dlat;
	u_int32_t lbloffset;

};

struct sign_info_t {
	u_int32_t sign_info_bits;
	int x_has_sign;
	int nx;
	int y_has_sign;
	int ny;
};

static void init_si(struct sign_info_t *si)
{
	si->sign_info_bits = 2;
	si->x_has_sign = 1;
	si->nx = 0;
	si->y_has_sign = 1;
	si->ny = 0;
}

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
	int *blong, int *blat,  struct sign_info_t *si)
{
	int x_sign_same, y_sign_same;
	int sign_bit;

	// Get bits per longitude


	x_sign_same= bfirst & 0x1;

	if ( x_sign_same ) {
		si->x_has_sign = 0;
		si->nx = bfirst & 0x02;
		si->sign_info_bits++;
		sign_bit= 0;
	} else {
		sign_bit= 1;
		si->x_has_sign = 1;
	}
	*blong= _bits_per_coord(base&0x0F, sign_bit, extra_bit);
	// Get bits per latitude

	if ( x_sign_same ) y_sign_same= bfirst & 0x04;
	else y_sign_same= bfirst & 0x02;

	if ( y_sign_same ) {
		si->y_has_sign = 0;
		si->ny = x_sign_same ? bfirst & 0x08 : bfirst & 0x04;
		sign_bit= 0;
		si->sign_info_bits++;
	} else {
		sign_bit= 1;
		si->y_has_sign = 1;
	}

	*blat= _bits_per_coord((base&0xF0)>>4, sign_bit, extra_bit);
}


struct bsp {
	u_int8_t *data;
	u_int8_t *cb;
	int cbit;
};

static void bsp_init(struct bsp *bp, u_int8_t *data)
{
	bp->cbit = 0;
	bp->data = data;
	bp->cb = bp->data; 
}

static int inline bsp_get_bits(struct bsp *bp, int bits)
{
	u_int32_t ret = 0;
	int i;
	
	for (i=0; i < bits; i++) {
		if (bp->cbit == 8) {
			bp->cbit = 0;
			bp->cb++;
		}
		ret |= (!!(*bp->cb & (1<<bp->cbit))) << i;
		bp->cbit ++;
	}
	return ret;
}

static int bs_get_mask(int bits)
{
	int a = 0,i;
	for (i =0; i < bits; i++)
		a |= (1<<i);
	return a;
}

static int inline bs_get_long_lat(int count, struct bsp *bp, struct sign_info_t *si, int bpx, int bpy)
{
	int i, s;
	int x, y, nw;
	int xmask, ymask;
	int symask = (1 << (bpy -1));
	int sxmask = (1 << (bpx -1));

	log(10, "Sign info bits:%d\n",si->sign_info_bits);
	log(10, "X has sign=%d, nx=%d\n", si->x_has_sign, si->nx);
	log(10, "Y has sign=%d, ny=%d\n", si->y_has_sign, si->ny);
	xmask = bs_get_mask(bpx-1);
	ymask = bs_get_mask(bpy-1);
	x = bsp_get_bits(bp, si->sign_info_bits);
	for (i=0; i < count; i++) {
		x = bsp_get_bits(bp, bpx);
		if (si->x_has_sign) {
			s = x & sxmask;
			if (s) {
				x &= ~sxmask;
				if (!x) {
					/* FIXME: Is it possible to have nested special cases ?*/
					nw = bsp_get_bits(bp, bpx);
					if (nw&sxmask) {
						nw &= ~sxmask;
						nw = - (xmask ^ (nw - 1));
					}
					x = nw << 1;
				} else {
					x = bpx - 1 - x;
				}
			}
		} else if (si->nx) {
			x = -x;
		}
		y = bsp_get_bits(bp, bpy);
		if (si->y_has_sign) {
			s = y & symask;
			if (s) {
				y &= ~symask;
				if (!y) {
					nw = bsp_get_bits(bp, bpy);
					if (nw&symask) {
						nw &= ~symask;
						nw = - (ymask ^ (nw - 1));
					}
					y = nw << 1;
					// special case!
				} else {
					y &= ~symask;
					y = - (ymask ^ (y-1));
				}
			}
		} else if (si->ny) {
			y = -y;
		}
		log(10, "lat=%d, long=%d\n", y, x);
	}
	return 0;
}

static int parse(unsigned char *dp, int line)
{

	struct bsp bp;
	struct sign_info_t si;
	u_int32_t total_bytes = 10;
	u_int16_t bs_len;
	u_int8_t bs_info;
	u_int32_t lbloffset;
	struct gpoly *gp;
	int bpx;	// bits per x
	int bpy;	// bits per y
	int two_byte;
	int extra_bit = 0;

	init_si(&si);
	gp = calloc(1, sizeof(*gp));
	if (!gp)
		return -1;
	gp->type = *dp;
	two_byte = gp->type & 0x80;

	if (line) {
		gp->line = 1;
		gp->dir = gp->type & 0x40;
		gp->type &= 0x3F;
	} else {
		gp->type &= 0x7F;
	}

	lbloffset = *(u_int32_t *)dp;
	lbloffset &= 0x00FFFFFF;
	extra_bit = lbloffset & 0x400000;
	gp->lbloffset = lbloffset & 0x3FFFFF;
	if (line) {
		gp->netlbl = !!(lbloffset & 0x800000);
		if (gp->netlbl) {
		// FIXME read .NET file
		}
	}
	dp+=4;
	gp->dlng = *(u_int16_t *)dp;
	dp+=2;
	gp->dlat = *(u_int16_t *)dp;
	dp+=2;
	if (two_byte) {
		bs_len = *(u_int16_t*)dp;
		dp+=2;
		total_bytes += bs_len + 1;
	} else {
		bs_len = *dp++;
		total_bytes += bs_len;
	}
	bs_info = *dp;
	log(10, "Base %u/%u bits per long/lat\n",
		bs_info&0x0F, (bs_info&0xF0)>>4);
	dp++;
	bits_per_coord(bs_info, *dp, extra_bit, &bpx, &bpy, &si);
	log(10,"%d/%d bits per long/lat  len=%d\n",
		bpx, bpy, bs_len);
	log(10, "Total coordinates: %d\n", (bs_len*8)/(bpx+bpy));
	bsp_init(&bp, dp);
	bs_get_long_lat((bs_len*8)/(bpx+bpy), &bp, &si, bpx, bpy);
	return total_bytes;

}

int main(int argc, char **argv)
{
	parse(d1,0);
	parse(d2,1);
	return 0;
}
