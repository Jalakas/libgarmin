#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#define __USE_BSD
#include <sys/time.h>
#include "libgarmin.h"
#include "list.h"
#include "garmin_nod.h"

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
	fprintf(stderr, "%s [-d level] garmin.img from [to]\n", pn);
	return -1;
}

int main(int argc, char **argv)
{
	struct gar *gar;
	struct gar_graph *g;
	struct gar_subfile *sub;
	int from = -1, to = -1;
	char *file = argv[1];
	struct gmap *gm;
	struct timeval tv1, tv2,tv3;
	int i;
	if (argc < 2) {
		return usage(argv[0]);
	}
	if (!strcmp(argv[1], "-d")) {
		if (argc > 3) {
			debug = atoi(argv[2]);
			fprintf(stderr, "debug level set to %d\n", debug);
			file = argv[3];
		} else {
			return usage(argv[0]);
		}
	}
	if (argc > 4)
		from = atoi(argv[4]);
	if (argc > 5)
		to = atoi(argv[5]);
	gar = load(file);
	fprintf(stderr, "Graph from %d to %d\n", from, to);
	gm = gar_find_subfiles(gar, NULL, GO_GET_ROUTABLE);
	for (i=0; i < gm->subfiles; i++) {
		sub = gm->subs[i];
		gettimeofday(&tv1, NULL);
		if (to != -1)
			g = gar_read_graph(sub, 0, from,  0, to);
		else
			g = gar_read_graph(sub, 0, from,  1000, to);
		gettimeofday(&tv2, NULL);
		if (g) {
			char buf[512];
			sprintf(buf,"/tmp/%d-graph.txt", from);
			gar_graph2tfmap(g, buf);
			timersub(&tv2,&tv1,&tv3);
			printf("Read %d in %d.%d sec\n", g->totalnodes, tv3.tv_sec,tv3.tv_usec);
			gar_free_graph(g);
		}
		break;
	}

	gar_free(gar);
	return 0;
}
