struct gar_subfile;
int gar_init_net(struct gar_subfile *sub);
void gar_free_net(struct gar_subfile *sub);
off_t gar_net_get_lbl_offset(struct gar_subfile *sub, off_t offset, int idx);
int gar_net_parse_sorted(struct gar_subfile *sub);
