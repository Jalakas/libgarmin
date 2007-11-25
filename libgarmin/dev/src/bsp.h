struct gimg;
/* Can read up to 32 bits */
struct bspfd {
	struct gimg *g;
	u_int8_t buf[4096];
	int datalen;
	u_int8_t *cb;
	u_int8_t *ep;
	int cbit;
};


struct bsp {
	u_int8_t *data;
	u_int8_t *ep;
	u_int8_t *cb;
	int cbit;
};
void bsp_init(struct bsp *bp, u_int8_t *data, u_int32_t len);
void bsp_fd_init(struct bspfd *bp, struct gimg *g);
int bsp_fd_get_bits(struct bspfd *bp, int bits);

// LSB to MSB
static int inline bsp_get_bits(struct bsp *bp, int bits)
{
	u_int32_t ret = 0;
	int i;
	
	for (i=0; i < bits; i++) {
		if (bp->cbit == 8) {
			bp->cbit = 0;
			bp->cb++;
		}
		if (bp->cb >= bp->ep)
			return -1;
		if (*bp->cb & (1<<bp->cbit))
			ret |= 1 << i;
		bp->cbit ++;
	}
	return ret;
}
