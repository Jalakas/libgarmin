struct fat_entry {
	list_t l;
	char filename[13];
	off_t size;
	off_t offset;
};

typedef struct fat_block_struct fat_entry_t;
int gar_load_fat(struct gimg *g, int dataoffset, int blocksize);
struct gar_subfile;
ssize_t gar_subfile_offset(struct gar_subfile *sub, char *ext);
ssize_t gar_file_size(struct gimg *g, char *ext);
char **gar_file_get_subfiles(struct gimg *g, int *count, const char *subext);
struct fat_entry *gar_fat_get_fe_by_name(struct gimg *g, char *name);
ssize_t gar_file_offset(struct gimg *g, char *file);
int gar_fat_add_file(struct gimg *g, char *name, off_t offset);
