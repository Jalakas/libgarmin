int gar_init_lbl(struct gar_subfile *sub);
void gar_free_lbl(struct gar_subfile *sub);
int gar_get_lbl(struct gar_subfile *sub, off_t offset, int type, u_int8_t *buf, int buflen);
int gar_init_srch(struct gar_subfile *sub, int what);
void gar_free_srch(struct gar_subfile *sub);
