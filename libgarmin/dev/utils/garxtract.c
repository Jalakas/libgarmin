#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "libgarmin.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static int debug = 10;
static void logfn(char *file, int line, int level, char *fmt, ...)
{
	va_list ap;
	if (level > debug)
		return;
	va_start(ap, fmt);
	fprintf(stdout, "%s:%d:%d|", file, line, level);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

static struct gar * get_gar(void)
{
	struct gar *g;
	g = gar_init(NULL, logfn);
	if (!g)
		return NULL;
	return g;
}

int main(int argc, char **argv)
{
	struct gimg *g;
	struct gar * gar;
	int fd;
	if (argc < 3) {
		fprintf(stderr, "%s garmin.img file\n", argv[0]);
		return -1;
	}
#warning FIXME gar_img_load returns int find the image in the list
	gar = get_gar();
	if (gar_img_load(gar, argv[1], 0) < 0) {
		fprintf(stderr, "Failed to load:[%s]\n", argv[1]);
		return 0;
	}
	g = gar_get_dskimg(gar, NULL);
	if (g) {
		fd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0660);
		if (fd > -1) {
			if (gar_fat_file2fd(g, argv[2], fd) < 0) {
				unlink(argv[2]);
			}
			close(fd);
		}
	} else {
		fprintf(stderr, "Failed to find dskimg! Run %s on a .img file\n", argv[0]);
	}
	gar_free(gar);
	return 0;
}
