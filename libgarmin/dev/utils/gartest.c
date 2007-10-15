#include <stdio.h>
#include <stdarg.h>
#include "libgarmin.h"

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

static struct gar * load(char *file)
{
	struct gar *g;
	g = gar_init(NULL, logfn);
	if (!g)
		return NULL;
	gar_img_load(g, file, 1);
	return g;
}

int main(int argc, char **argv)
{
	struct gar *gar;
	struct gar_rect r;
	if (argc < 2) {
		fprintf(stderr, "%s garmin.img\n", argv[0]);
		return -1;
	}
/*
garmin_rgn.c:63:1|Boundaries - 
North: 43.716080C, East: 29.987290C, South: 43.688679C, West: 29.932551C pnt:0, idxpnt:0

*/
	r.lulat = 43.706080;
	r.lulong = 29.942551;
	r.rllat = 43.698679;
	r.rllong = 29.977290;
	gar = load(argv[1]);
	gar_find_subfiles(gar, &r);
	r.lulat = 44.281147;
	r.lulong = 22.274888;
	r.rllat = 42.787006;
	r.rllong = 24.032700;
	gar_find_subfiles(gar, &r);
	r.lulat = 24.281147;
	r.lulong = 12.274888;
	r.rllat = 22.787006;
	r.rllong = 14.032700;
	gar_find_subfiles(gar, &r);
/*
43.892183C, East: 22.952971C, South: 43.638103C,
 West: 22.643273C
*/
	r.lulat = 43.892183;
	r.lulong = 22.643273;
	r.rllat = 43.638103;
	r.rllong = 22.952971;
	gar_find_subfiles(gar, &r);
	gar_free(gar);
	return 0;
}
