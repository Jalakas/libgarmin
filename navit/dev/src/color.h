#ifndef NAVIT_COLOR_H
#define NAVIT_COLOR_H

struct color {
	int r,g,b,a;
};
struct color_scheme;
struct color_scheme *cs_lookup(char *name);
struct color *cs_lookup_color(struct color_scheme *cs, char *name);
int cs_resolve_color(char *scheme, char *name, struct color *color);
int cscheme_init(void);

#endif
