#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "debug.h"
#include "item.h"
#include "coord.h"
#include "transform.h"
#include "color.h"
#include "attr.h"
#include "map.h"

struct attr_name {
	enum attr_type attr;
	char *name;
};


static struct attr_name attr_names[]={
#define ATTR2(x,y) ATTR(y)
#define ATTR(x) { attr_##x, #x },
#include "attr_def.h"
#undef ATTR2
#undef ATTR
};

enum attr_type
attr_from_name(const char *name)
{
	int i;

	for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
		if (! strcmp(attr_names[i].name, name))
			return attr_names[i].attr;
	}
	return attr_none;
}

char *
attr_to_name(enum attr_type attr)
{
	int i;

	for (i=0 ; i < sizeof(attr_names)/sizeof(struct attr_name) ; i++) {
		if (attr_names[i].attr == attr)
			return attr_names[i].name;
	}
	return NULL; 
}

struct attr *
attr_new_from_text(const char *name, const char *value)
{
	enum attr_type attr;
	struct attr *ret;
	struct coord_geo *g;
	struct coord c;

	ret=g_new0(struct attr, 1);
	dbg(1,"enter name='%s' value='%s'\n", name, value);
	attr=attr_from_name(name);
	ret->type=attr;
	switch (attr) {
	case attr_item_type:
		ret->u.item_type=item_from_name(value);
		break;
	default:
		if (attr >= attr_type_string_begin && attr <= attr_type_string_end) {
			ret->u.str=(char *)value;
			break;
		}
		if (attr >= attr_type_int_begin && attr <= attr_type_int_end) {
			ret->u.num=atoi(value);
			break;
		}
		if (attr >= attr_type_color_begin && attr <= attr_type_color_end) {
			struct color *color=g_new0(struct color, 1);
			ret->u.color=color;
			parse_color(value, color);
			break;
		}
		if (attr >= attr_type_coord_geo_start && attr <= attr_type_coord_geo_end) {
			g=g_new(struct coord_geo, 1);
			ret->u.coord_geo=g;
			coord_parse(value, projection_mg, &c);
			transform_to_geo(projection_mg, &c, g);
			break;
		}
		dbg(1,"default\n");
		g_free(ret);
		ret=NULL;
	}
	if (ret)
		ret->flags |= (ATTR_ALLOCATED|ATTR_ALLOCDATA);
	return ret;
}

char *
attr_to_text(struct attr *attr, int pretty)
{
	char *ret;
	enum attr_type type=attr->type;

	if (type >= attr_type_item_begin && type <= attr_type_item_end) {
		struct item *item=attr->u.item;
		if (! item)
			return g_strdup("(nil)");
		// FIXME: Can not use %p, map id
		return g_strdup_printf("type=0x%x id=0x%x,0x%x map=%p (%s:%s) ", item->type, item->id_hi, item->id_lo, item->map, item->map ? map_get_type(item->map) : "", item->map ? map_get_filename(item->map) : "");
	}
	if (type >= attr_type_string_begin && type <= attr_type_string_end) {
#if 0
		FIXME: all strings in utf8
		if (map) {
			char *mstr=map_convert_string(map, attr->u.str);
			ret=g_strdup(mstr);
		} else
#endif
		ret=g_strdup(attr->u.str);
		return ret;
	}
	if (type >= attr_type_int_begin && type <= attr_type_int_end) 
		return g_strdup_printf("%d", attr->u.num);

	if (type >= attr_type_color_begin && type <= attr_type_color_end) 
		return g_strdup_printf("#%02x%02x%02x%02x", 
					attr->u.color->r&0xff,
					attr->u.color->g&0xff,
					attr->u.color->b&0xff,
					attr->u.color->a&0xff);
	if (type >= attr_type_coord_geo_start && type <= attr_type_coord_geo_end) {
		struct coord c;
		transform_from_geo(projection_mg, attr->u.coord_geo, &c);
		return g_strdup_printf("mg:0x%x 0x%x",
					c.x, c.y);

	}
	return g_strdup("(no text)");
}

struct attr *
attr_search(struct attr **attrs, struct attr *last, enum attr_type attr)
{
	dbg(1, "enter attrs=%p\n", attrs);
	while (*attrs) {
		dbg(1,"*attrs=%p\n", *attrs);
		if ((*attrs)->type == attr) {
			return *attrs;
		}
		attrs++;
	}
	return NULL;
}

int
attr_data_size(struct attr *attr)
{
	if (attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end) {
		return strlen(attr->u.str)+1;
	}
	if (attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) {
		return sizeof(attr->u.num);
	}
	return 0;
}

void *
attr_data_get(struct attr *attr)
{
	if (attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end) {
		return attr->u.str;
	}
	if (attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) {
		return &attr->u.num;
	}
	return NULL;
}

void
attr_data_set(struct attr *attr, void *data)
{
	if (attr->type >= attr_type_string_begin && attr->type <= attr_type_string_end) {
		attr->u.str=data;
	}
	if (attr->type >= attr_type_int_begin && attr->type <= attr_type_int_end) {
		attr->u.num=*((int *)data);
	}
}

static void
attr_free_data(struct attr *attr)
{
	if (attr->type == attr_position_coord_geo)
		g_free(attr->u.coord_geo);
	if (attr->type >= attr_type_color_begin && attr->type <= attr_type_color_end) 
		g_free(attr->u.color);
}

void
attr_free(struct attr *attr)
{
	if (attr->flags&ATTR_ALLOCDATA)
		attr_free_data(attr);
	if (attr->flags&ATTR_ALLOCATED)
		g_free(attr);
}

void
attr_group_free(struct attr_group *ag)
{
	int i;

	for (i=0; i < ag->count; i++) {
		if (ag->present & (1<<i))
			attr_free_data(&ag->attrs[i]);
	}
	free(ag);
}

void
attr_group_reset(struct attr_group *ag)
{
	int i;

	for (i=0; i < ag->count; i++) {
		if (ag->present & (1<<i))
			attr_free_data(&ag->attrs[i]);
	}
	ag->present = 0;
}

struct attr_group *
attr_group_alloc(unsigned int count)
{
	struct attr_group *ag;
	if (count > 31) {
		dbg(0,"request for more than 31 attributes in group, please split \n");
		return NULL;
	}
	ag = calloc(1, sizeof(*ag) + count * sizeof(struct attr));
	if (!ag)
		return NULL;
	ag->count = count;
	return ag;
}

static int
attr_group_present(struct attr_group *ag, int idx)
{
	return ag->present & (1<<idx);
}

static void
attr_group_set_present(struct attr_group *ag, int idx)
{
	ag->present |= (1<<idx);
}

static void
attr_group_clear_present(struct attr_group *ag, int idx)
{
	ag->present &= ~(1<<idx);
}

struct attr_group *
attr_group_alloc_types(unsigned int count, enum attr_type *types)
{
	struct attr_group *ag;
	int i;
	ag = attr_group_alloc(count);
	if (!ag)
		return NULL;
	for (i = 0; i < count; i++) {
		ag->attrs[i].type = *types++;
	}
	return ag;
}

struct attr *
attr_group_get(struct attr_group *ag, int idx)
{
	if (attr_group_present(ag, idx))
		return &ag->attrs[idx];
	return NULL;
}

struct attr *
attr_group_get_attr(struct attr_group *ag, int idx)
{
	if (idx >= ag->count)
		return NULL;
	return &ag->attrs[idx];
}

struct attr *
attr_group_gettype(struct attr_group *ag, enum attr_type type)
{
	int i;
	for (i=0; i < ag->count; i++) {
		if (ag->attrs[i].type == type) {
			if (attr_group_present(ag, i))
				return &ag->attrs[i];
		}
	}
	return NULL;
}

int
attr_group_get_data(void *priv, struct item *it, struct attr_group *ag)
{
	int i;
	int rc = 0;
	for (i=0; i < ag->count; i++) {
		if (item_attr_get(it, ag->attrs[i].type , &ag->attrs[i])) {
			attr_group_set_present(ag, i);
			rc++;
		}
	}
	return rc;
}

int
attr_group_set_intvalue(struct attr_group *ag, enum attr_type type, int val)
{
	int i;
	for (i=0; i < ag->count; i++) {
		if (ag->attrs[i].type == type) {
			ag->attrs[i].u.num = val;
			attr_group_set_present(ag, i);
			return 1;
		}
	}
	return 0;
}

int
attr_group_clear(struct attr_group *ag, enum attr_type type)
{
	int i;
	for (i=0; i < ag->count; i++) {
		if (ag->attrs[i].type == type) {
			if (attr_group_present(ag, i))
				attr_free_data(&ag->attrs[i]);
			attr_group_clear_present(ag, i);
			return 1;
		}
	}
	return 0;
}
