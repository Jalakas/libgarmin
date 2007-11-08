#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libgarmin.h"

static int debug = 15;
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
	struct gar_config cfg;
	cfg.opm = OPM_PARSE;
	// FIXME: make cmdline arg
	cfg.debugmask = 0; // DBGM_LBLS | DBGM_OBJSRC;
	g = gar_init_cfg(NULL, logfn, &cfg);
	if (!g)
		return NULL;
	if (gar_img_load(g, file, 1) > 0)
		return g;
	else {
		gar_free(g);
		return NULL;
	}
}
static int usage(char *pn)
{
	fprintf(stderr, "%s [-d level] garmin.img\n", pn);
	return -1;
}

int main(int argc, char **argv)
{
	struct gar *gar;
	struct gar_rect r;
	char *file = argv[1];
	if (argc < 2) {
		return usage(argv[0]);
	}
/*
garmin_rgn.c:63:1|Boundaries - 
North: 43.716080C, East: 29.987290C, South: 43.688679C, West: 29.932551C pnt:0, idxpnt:0

*/
	if (!strcmp(argv[1], "-d")) {
		if (argc > 3) {
			debug = atoi(argv[2]);
			fprintf(stderr, "debug level set to %d\n", debug);
			file = argv[3];
		} else {
			return usage(argv[0]);
		}
	}

	r.lulat = 43.706080;
	r.lulong = 29.942551;
	r.rllat = 43.698679;
	r.rllong = 29.977290;
	gar = load(file);
	if (!gar) {
		fprintf(stderr, "Failed to load: [%s]\n", file);
		return 0;
	}
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
