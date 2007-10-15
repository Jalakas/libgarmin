/* Can read up to 32 bits */
struct bspfd {
	int fd;
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
int bsp_get_bits(struct bsp *bp, int bits);
void bsp_fd_init(struct bspfd *bp, int fd);
int bsp_fd_get_bits(struct bspfd *bp, int bits);
