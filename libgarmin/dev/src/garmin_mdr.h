struct gar_mdr {
	unsigned int mdroff;
	u_int32_t idxfiles_offset;
	u_int32_t idxfiles_len;
};

int gar_init_mdr(struct gimg *g);
int gar_mdr_get_files(struct gmap *files, struct gimg *g);

