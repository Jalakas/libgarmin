#include <glib.h>
#include "debug.h"
#include "projection.h"
#include "map.h"
#include "mapset.h"
#include "coord.h"
#include "item.h"
#include "search.h"
#include "attr.h"

struct search_list_level {
	struct mapset *ms;
	struct item *parent;
	struct attr attr;
	int partial;
	struct mapset_search *search;
	GHashTable *hash;
	GList *list,*curr,*last;
	int complete;
};

struct search_list {
	struct mapset *ms;
	int level;
	struct search_list_level levels[4];
	struct search_list_result result;
};

static guint
search_item_hash_hash(gconstpointer key)
{
	const struct item *itm=key;
	gconstpointer hashkey=(gconstpointer)(itm->id_hi^itm->id_lo);
	return g_direct_hash(hashkey);
}

static gboolean
search_item_hash_equal(gconstpointer a, gconstpointer b)
{
	const struct item *itm_a=a;
	const struct item *itm_b=b;
	if (item_is_equal_id(*itm_a, *itm_b))
		return TRUE;
	return FALSE;
}

static enum attr_type search_attrs[] = {
	attr_country_id,
	attr_district_id,
	attr_town_id,
	attr_street_id,
};

struct search_list *
search_list_new(struct mapset *ms)
{
	struct search_list *ret;

	ret=g_new0(struct search_list, 1);
	ret->ms=ms;
	ret->result.attrs = attr_group_alloc_types(sizeof(search_attrs)/sizeof(search_attrs[0]),
				search_attrs);
	if (!ret->result.attrs) {
		g_free(ret);
		return NULL;
	}
	return ret;
}

static void search_list_search_free(struct search_list *sl, int level);
void
search_list_search(struct search_list *this_, struct attr *search_attr, int partial)
{
	int level=-1;
	struct search_list_level *le;
	switch(search_attr->type) {
	case attr_country_all:
	case attr_country_id:
	case attr_country_iso2:
	case attr_country_iso3:
	case attr_country_car:
	case attr_country_name:
		level=0;
		break;
	case attr_district_name:
		level=1;
		break;
	case attr_town_name:
		level=2;
		break;
	case attr_street_name:
		level=3;
		break;
	default:
		break;
	}
	dbg(0,"level=%d\n", level);
	if (level != -1) {
		this_->level=level;
		le=&this_->levels[level];
		le->attr=*search_attr;
		if (search_attr->type != attr_country_id)
			le->attr.u.str=g_strdup(search_attr->u.str);
		search_list_search_free(this_, level);
		le->partial=partial;
		if (level > 0) {
			le=&this_->levels[level-1];
			le->curr=le->list;
		}
		dbg(1,"le=%p partial=%d\n", le, partial);
	}
}

static struct search_list_country *
search_list_country_new(struct item *item)
{
	struct search_list_country *ret=g_new0(struct search_list_country, 1);
	struct attr attr;
	struct coord c;

	ret->item=*item;
	if (item_attr_get(item, attr_country_car, &attr))
		ret->car=g_strdup(attr.u.str);
	if (item_attr_get(item, attr_country_iso2, &attr))
		ret->iso2=g_strdup(attr.u.str);
	if (item_attr_get(item, attr_country_iso3, &attr))
		ret->iso3=g_strdup(attr.u.str);
	if (item_attr_get(item, attr_country_name, &attr))
		ret->name=g_strdup(attr.u.str);
	if (item_attr_get(item, attr_country_id, &attr))
		ret->id=attr.u.num;
	if (item_coord_get(item, &c, 1)) {
		ret->c=g_new(struct pcoord, 1);
		ret->c->x=c.x;
		ret->c->y=c.y;
		ret->c->pro = map_projection(item->map);
	}
	return ret;
}

static void
search_list_country_destroy(struct search_list_country *this_)
{
	g_free(this_->car);
	g_free(this_->iso2);
	g_free(this_->iso3);
	g_free(this_->name);
	if (this_->c)
		g_free(this_->c);
	g_free(this_);
}

static struct search_list_district *
search_list_district_new(struct item *item)
{
	struct search_list_district *ret=g_new0(struct search_list_district, 1);
	struct attr attr;
	struct coord c;

	ret->item=*item;
	if (item_attr_get(item, attr_district_name, &attr))
		ret->name=g_strdup(attr.u.str);
	if (item_attr_get(item, attr_district_id, &attr))
		ret->id=attr.u.num;
	if (item_attr_get(item, attr_country_id, &attr))
		ret->cid=attr.u.num;
	if (item_attr_get(item, attr_country_name, &attr))
		ret->country=g_strdup(attr.u.str);
	if (item_coord_get(item, &c, 1)) {
		ret->c=g_new(struct pcoord, 1);
		ret->c->x=c.x;
		ret->c->y=c.y;
		ret->c->pro = map_projection(item->map);
	}

	return ret;
}

static void
search_list_district_destroy(struct search_list_district *this_)
{
	g_free(this_->name);
	g_free(this_->country);
	if (this_->c)
		g_free(this_->c);
	g_free(this_);
}

static struct search_list_town *
search_list_town_new(struct item *item)
{
	struct search_list_town *ret=g_new0(struct search_list_town, 1);
	struct attr attr;
	struct coord c;
	
	ret->itemt=*item;
	if (item_attr_get(item, attr_town_streets_item, &attr)) {
		dbg(1,"town_assoc 0x%x 0x%x\n", attr.u.item->id_hi, attr.u.item->id_lo);
		ret->item=*attr.u.item;
	}
	else
		ret->item=*item;
	if (item_attr_get(item, attr_town_name, &attr))
		ret->name=map_convert_string(item->map,attr.u.str);
	if (item_attr_get(item, attr_town_postal, &attr))
		ret->postal=map_convert_string(item->map,attr.u.str);
	if (item_attr_get(item, attr_country_name, &attr))
		ret->country=map_convert_string(item->map,attr.u.str);
	if (item_attr_get(item, attr_district_name, &attr))
		ret->district=map_convert_string(item->map,attr.u.str);
	if (item_coord_get(item, &c, 1)) {
		ret->c=g_new(struct pcoord, 1);
		ret->c->x=c.x;
		ret->c->y=c.y;
		ret->c->pro = map_projection(item->map);
	}
	if (item_attr_get(item, attr_town_id, &attr))
		ret->id=attr.u.num;
	if (item_attr_get(item, attr_country_id, &attr))
		ret->cid=attr.u.num;
	if (item_attr_get(item, attr_district_id, &attr))
		ret->rid=attr.u.num;
	return ret;
}

static void
search_list_town_destroy(struct search_list_town *this_)
{
	map_convert_free(this_->name);
	map_convert_free(this_->postal);
	if (this_->c)
		g_free(this_->c);
	g_free(this_);
}

static struct search_list_street *
search_list_street_new(struct item *item)
{
	struct search_list_street *ret=g_new0(struct search_list_street, 1);
	struct attr attr;
	struct coord c;
	
	ret->item=*item;
	if (item_attr_get(item, attr_street_name, &attr))
		ret->name=map_convert_string(item->map, attr.u.str);
	if (item_coord_get(item, &c, 1)) {
		ret->c=g_new(struct pcoord, 1);
		ret->c->x=c.x;
		ret->c->y=c.y;
		ret->c->pro = map_projection(item->map);
	}
	return ret;
}

static void
search_list_street_destroy(struct search_list_street *this_)
{
	map_convert_free(this_->name);
	if (this_->c)
		g_free(this_->c);
	g_free(this_);
}


static void
search_list_result_destroy(int level, void *p)
{
	switch (level) {
	case 0:
		search_list_country_destroy(p);
		break;
	case 1:
		search_list_district_destroy(p);
		break;
	case 2:
		search_list_town_destroy(p);
		break;
	case 3:
		search_list_street_destroy(p);
		break;
	}
}

static void
search_list_search_free(struct search_list *sl, int level)
{
	struct search_list_level *le=&sl->levels[level];
	GList *next,*curr;
	if (le->search) {
		mapset_search_destroy(le->search);
		le->search=NULL;
	}
	if (le->search) {
		g_hash_table_destroy(le->hash);
	}
	curr=le->list;
	while (curr) {
		search_list_result_destroy(level, curr->data);
		next=g_list_next(curr);
		curr=next;
	}
	g_list_free(le->list);
	le->list=NULL;
	le->curr=NULL;
	le->last=NULL;
	le->complete=0;
}

void
search_list_select(struct search_list *sl, void *p)
{
	struct search_list_result *res = p;
	struct search_list_country *country;
	struct search_list_district *district;
	struct search_list_town *town;
	struct search_list_street *street;

	attr_group_reset(sl->result.attrs);
	switch(sl->level) {
		case 0:
			country = res->country;
			if (country) {
				printf("Cid:%d\n", country->id);
				attr_group_set_intvalue(sl->result.attrs, attr_country_id,
					 country->id);
			}
			break;
		case 1:
			district = res->district;
			if (district) {
				attr_group_set_intvalue(sl->result.attrs, attr_country_id,
					 district->cid);
				attr_group_set_intvalue(sl->result.attrs, attr_district_id,
					 district->id);
			}
			break;
		case 2:
			town = res->town;
			if (town) {
				attr_group_set_intvalue(sl->result.attrs, attr_country_id,
					 town->cid);
				attr_group_set_intvalue(sl->result.attrs, attr_district_id,
					 town->rid);
				attr_group_set_intvalue(sl->result.attrs, attr_town_id,
					 town->id);
			}
			break;
		case 3:
			street = p;
			break;
	}
}

static int
search_add_result(struct search_list_level *le, void *p)
{
	if (! g_hash_table_lookup(le->hash, p)) {
		g_hash_table_insert(le->hash, p, (void *)1);
		le->list=g_list_append(le->list, p);
		return 1;
	}
	return 0;
}

struct search_list_result *
search_list_get_result(struct search_list *this_)
{
	struct search_list_level *le,*leu;
	struct item *item;
	int level=this_->level;

	dbg(1,"enter\n");
	le=&this_->levels[level];
	dbg(1,"le=%p\n", le);
	while(! le->complete) {
		dbg(1,"le->search=%p\n", le->search);
		if (! le->search) {
			dbg(1,"partial=%d\n", le->partial);
			if (! level) 
				le->parent=NULL;
			else {
				leu=&this_->levels[level-1];
				if (leu->curr) {
					leu->last=leu->curr;
					le->parent=leu->curr->data;
					leu->curr=g_list_next(leu->curr);
				} else
					le->parent = NULL;
			}
			le->search=mapset_search_new(this_->ms, this_->result.attrs, le->parent, &le->attr, le->partial);
			le->hash=g_hash_table_new_full(search_item_hash_hash, search_item_hash_equal, g_free, NULL);
		}
		dbg(1,"le->search=%p\n", le->search);
		item=mapset_search_get_item(le->search);
		dbg(1,"item=%p\n", item);
		if (item) {
			void *p=NULL;
			this_->result.country=NULL;
			this_->result.district=NULL;
			this_->result.town=NULL;
			this_->result.street=NULL;
			this_->result.c=NULL;
			switch (level) {
			case 0:
				p=search_list_country_new(item);
				this_->result.country=p;
				break;
			case 1:
				p = search_list_district_new(item);
//				this_->result.country=this_->levels[0].last->data;
				this_->result.district = p;
				break;
			case 2:
				p=search_list_town_new(item);
//				this_->result.country=this_->levels[0].last->data;
//				this_->result.district=this_->levels[1].last->data;
				this_->result.town=p;
				this_->result.c=this_->result.town->c;
				break;
			case 3:
				p=search_list_street_new(item);
//				this_->result.country=this_->levels[0].last->data;
//				this_->result.town=this_->levels[1].last->data;
				this_->result.street=p;
				this_->result.c=this_->result.street->c;
				break;
				
			}
			if (p) {
				if (search_add_result(le, p)) 
					return &this_->result;
				else 
					search_list_result_destroy(level, p);
			}
		} else 
			le->complete=1;
	}
	return NULL;
}

void
search_list_destroy(struct search_list *this_)
{
	if (this_->result.attrs) {
		attr_group_free(this_->result.attrs);
	}
	g_free(this_);
}
