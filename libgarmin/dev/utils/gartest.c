#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libgarmin.h"

static int debug = 15;
static int debugmask = 0;
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
	//	cfg.debugmask = DBGM_LBLS;//0; // DBGM_LBLS | DBGM_OBJSRC;
	cfg.debugmask = debugmask;
	cfg.debuglevel = debug;
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
	fprintf(stderr, "%s [-d level] [-l] [-n] garmin.img\n", pn);
	fprintf(stderr, "\t-l Dump labels\n");
	fprintf(stderr, "\t-n Dump data\n");
	return -1;
}

int main(int argc, char **argv)
{
	struct gar *gar;
	struct gar_rect r;
	char *file = argv[1];
	int i = 1;
	if (argc < 2) {
		return usage(argv[0]);
	}
	while (i < argc) {
		if (*argv[i] != '-')
			break;
		if (!strcmp(argv[i], "-d")) {
			if (argc > i+1) {
				debug = atoi(argv[i+1]);
				fprintf(stderr, "debug level set to %d\n", debug);
				i++;
			} else {
				return usage(argv[0]);
			}
		} else if (!strcmp(argv[i], "-l")) {
			debugmask |= DBGM_LBLS;
		} else if (!strcmp(argv[i], "-n")) {
			debugmask |= DBGM_DUMP;
		}
		i++;
	}
	if (i >= argc)
		return usage;
	file = argv[i];

	r.lulat = 43.706080;
	r.lulong = 29.942551;
	r.rllat = 43.698679;
	r.rllong = 29.977290;
	gar = load(file);
	if (!gar) {
		fprintf(stderr, "Failed to load: [%s]\n", file);
		return 0;
	}
	gar_find_subfiles(gar, &r, 0);
	r.lulat = DEGGAR(44.281147);
	r.lulong = DEGGAR(22.274888);
	r.rllat = DEGGAR(42.787006);
	r.rllong = DEGGAR(24.032700);
	gar_find_subfiles(gar, &r,0);
	r.lulat = DEGGAR(24.281147);
	r.lulong = DEGGAR(12.274888);
	r.rllat = DEGGAR(22.787006);
	r.rllong = DEGGAR(14.032700);
	gar_find_subfiles(gar, &r,0);
/*
43.892183C, East: 22.952971C, South: 43.638103C,
 West: 22.643273C
*/
	r.lulat = DEGGAR(43.892183);
	r.lulong = DEGGAR(22.643273);
	r.rllat = DEGGAR(43.638103);
	r.rllong = DEGGAR(22.952971);
	gar_find_subfiles(gar, &r,0);
	gar_find_subfiles(gar, NULL,0);
	gar_free(gar);
	return 0;
}
