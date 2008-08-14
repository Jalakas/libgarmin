#ifndef NAVIT_MAPSET_H
#define NAVIT_MAPSET_H

/* prototypes */
struct attr;
struct item;
struct map;
struct mapset;
struct mapset_handle;
struct mapset_search;
struct attr_group;
struct route;
struct mapset *mapset_new(void);
void mapset_add(struct mapset *ms, struct map *m);
void mapset_destroy(struct mapset *ms);
struct mapset_handle *mapset_open(struct mapset *ms);
struct map *mapset_next(struct mapset_handle *msh, int active);
void mapset_close(struct mapset_handle *msh);
struct mapset_search *mapset_search_new(struct mapset *ms, struct attr_group *ag, struct item *item, struct attr *search_attr, int partial);
struct item *mapset_search_get_item(struct mapset_search *this);
void mapset_search_destroy(struct mapset_search *this);
int mapset_can_route(struct mapset *ms);
void *mapset_route(struct mapset *ms, struct route *r);
void mapset_free_route(struct mapset *ms, void *maproute);
#endif

