#ifndef NAVIT_SEARCH_H
#define NAVIT_SEARCH_H

#ifdef __cplusplus
extern "C" {
#endif
struct search_list_country {
	struct item item;
	char *car;
	char *iso2;
	char *iso3;
	char *name;
	struct pcoord *c;
	unsigned int id;
};

struct search_list_district {
	struct item item;
	char *name;
	char *country;
	struct pcoord *c;
	unsigned int cid;
	unsigned int id;
};

struct search_list_town {
	struct item item;
	struct item itemt;
	struct pcoord *c;
	char *postal;
	char *name;
	char *country;
	char *district;
	unsigned int cid;
	unsigned int rid;
	unsigned int id;
};

struct search_list_street {
	struct item item;
	struct pcoord *c;
	char *name;
	char *postal;
	char *city;
	char *country;
	char *district;
	unsigned int cid;
	unsigned int rid;
	unsigned int tid;
	unsigned int id;
};

struct search_list_result {
	struct pcoord *c;
	struct search_list_country *country;
	struct search_list_district *district;
	struct search_list_town *town;
	struct search_list_street *street;
	struct attr_group *attrs;
};

/* prototypes */
struct attr;
struct mapset;
struct search_list;
struct search_list_result;
struct search_list *search_list_new(struct mapset *ms);
void search_list_search(struct search_list *this_, struct attr *search_attr, int partial);
struct search_list_result *search_list_get_result(struct search_list *this_);
void search_list_destroy(struct search_list *this_);
void search_list_select(struct search_list *sl, void *p);

/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif

