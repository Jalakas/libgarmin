struct fat_entry {
	list_t l;
	char filename[12];
	off_t size;
	off_t offset;
};

typedef struct fat_block_struct fat_entry_t;
int gar_load_fat(struct gimg *g);
struct gar_subfile;
ssize_t gar_subfile_offset(struct gar_subfile *sub, char *ext);
ssize_t gar_file_size(struct gimg *g, char *ext);
int gar_file_get_subfiles(struct gimg *g, char **out, int size);

