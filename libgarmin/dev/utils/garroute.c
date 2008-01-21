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

static void dump_q(struct gar_graph *graph)
{
	struct node *n;
	int d=0;
	list_for_entry(n, &graph->lqueue, lc) {
//		fprintf(stderr, "%d %d\n", n->offset, n->value);
		d++;
	}
//	fprintf(stderr, "Depth %d\n", d);
}
void enqueue_node(struct gar_graph *graph, struct node *node)
{
//	fprintf(stderr, "NOD enqueue:%d c=%d v=%d\n", node->offset, node->complete, node->value);
	if (!node->complete) {
		struct node *n;
		list_remove_init(&node->lc);
		list_for_entry(n, &graph->lqueue, lc) {
			if (n->value > node->value) {
				list_append(&node->lc, &n->lc);
				dump_q(graph);
				return;
			}
		}
		list_append(&node->lc, &graph->lqueue);
	}
	dump_q(graph);
}

int can_visit(struct node *from, struct node *to)
{
	return 1;
}

static int get_arc_heuristic(struct node *n, struct grapharc *arc)
{
	return arc->len;
}

int route(struct gar_subfile *sub, struct gar_graph *g, struct node *from, struct node *to)
{
	struct node *n;
	unsigned int iter = 0;
	unsigned int newval;
	int i;

	enqueue_node(g, from);
	from->value = 0;
	while (!list_empty(&g->lqueue)) {
		iter++;
		n = list_entry(g->lqueue.n, struct node, lc);
		list_remove_init(&n->lc);
//		fprintf(stderr, "node %d value=%d\n", n->offset, n->value);
		if (!n->complete) {
			if (gar_read_node(g, NULL, n) < 0) {
				fprintf(stderr, "error reading node\n");
				return -1;
			}
			n->complete = 1;
		}
		if (n == to) {
			printf("route %d\n", iter);
//			to->from = n;
			return 1;
		}
		for (i=0; i < n->narcs; i++) {
			if (n->arcs[i].dest == n)
				continue;
			if (n->arcs[i].islink)
				continue;
			if (can_visit(n, n->arcs[i].dest)) {
				newval = n->value + get_arc_heuristic(to, &n->arcs[i]);
				if (n->arcs[i].dest->value != ~0) {
					if (n->arcs[i].dest->value > newval) {
						n->arcs[i].dest->from = n;
						n->arcs[i].dest->value = newval;
						enqueue_node(g, n->arcs[i].dest);
					} else if (n->arcs[i].dest->value == newval) {
//						fprintf(stderr, "same val %d %d\n", n->class, n->arcs[i].dest->class);
						if (n->class < n->arcs[i].dest->class) {
							n->arcs[i].dest->from = n;
							enqueue_node(g, n->arcs[i].dest);
						}
					}
				} else {
					n->arcs[i].dest->value = newval;
					n->arcs[i].dest->from = n;
					enqueue_node(g, n->arcs[i].dest);
				}
			}
		}
	}
	printf("noroute %d\n", iter);
	return 0;
}

int write_routemap(struct node *goal, char *file)
{
	FILE *fp;
	struct node *pn = goal;

	fp = fopen(file, "w");
	if (!fp)
		return -1;
	while (pn) {
		fprintf(fp, "type=street_route value=\"%d\" label=\"%d\"\n",
			pn->value, pn->offset);
		fprintf(fp, "garmin:0x%x 0x%x\n", pn->c.x, pn->c.y);
		pn = pn->from;
		if (pn)
			fprintf(fp, "garmin:0x%x 0x%x\n",pn->c.x, pn->c.y);
	}
	fclose(fp);
	return 0;

}

int main(int argc, char **argv)
{
	struct gar *gar;
	struct gar_graph *g;
	struct gar_subfile *sub;
	int ofrom = -1, oto = -1;
	char *file = argv[1];
	struct gmap *gm;
	struct timeval tv1, tv2,tv3;
	int i;
	struct node *from, *to, *pn;
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
		ofrom = atoi(argv[4]);
	if (argc > 5)
		oto = atoi(argv[5]);
	gar = load(file);
	fprintf(stderr, "Graph from %d to %d\n", ofrom, oto);
	gm = gar_find_subfiles(gar, NULL, GO_GET_ROUTABLE);
	for (i=0; i < gm->subfiles; i++) {
		sub = gm->subs[i];
		gettimeofday(&tv1, NULL);
//		if (to != -1)
//			g = gar_read_graph(sub, 0, from,  0, to);
//		else
//			g = gar_read_graph(sub, 0, from,  1000, to);
		g = gar_alloc_graph(sub);
		to = gar_get_node(g, oto);
		from = gar_get_node(g, ofrom);
#if 1
		if (route(sub, g, from, to)==1) {
			pn = to;
			while (pn) {
				fprintf(stderr, "P:%d value=%d\n", pn->offset, pn->value);
				pn = pn->from;
			}
			write_routemap(to, "/tmp/route.txt");
		}
#endif
		gettimeofday(&tv2, NULL);
		timersub(&tv2,&tv1,&tv3);
		printf("Read %d in %d.%d sec\n", g->totalnodes, tv3.tv_sec,tv3.tv_usec);
		if (g) {
			char buf[512];
			sprintf(buf,"/tmp/%d-graph.txt", ofrom);
			gar_graph2tfmap(g, buf);
			gar_free_graph(g);
		}
		break;
	}

	gar_free(gar);
	return 0;
}
