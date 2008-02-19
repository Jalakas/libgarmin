#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "debug.h"
#include "item.h"
#include "attr.h"
#include "notify.h"

static list_head(llisteners);

struct listener {
	list_t l;
	unsigned int group;
	int mask;
	notify_fn callback;
	void *priv;
};

static struct listener *
alloc_listener(unsigned int group, int mask, notify_fn callback, void *priv)
{
	struct listener *l;
	l = calloc(1, sizeof(*l));
	if (!l)
		return NULL;
	list_init(&l->l);
	l->group = group;
	l->mask = mask;
	l->callback = callback;
	l->priv = priv;
	return l;
}

int listen_for(unsigned int group, int mask, notify_fn callback, void *priv)
{
	struct listener *l;
	l = alloc_listener(group, mask, callback, priv);
	if (l) {
		list_append(&l->l, &llisteners);
		return 0;
	}
	return -1;
}

void notify(unsigned int group, int mask, void *data)
{
	struct listener *l;

	list_for_entry(l, &llisteners, l) {
		if (l->group == group)
			l->callback(group, mask, l->priv, data);
	}
}

static int aq_id;
static list_head(lattrq);

struct attr_query {
	list_t l;
	char *name;
	int id;
	unsigned int group;
	void *data;
	query_fn attr_get;
};

static int attr_query_get_id(char *name)
{
	struct attr_query *aq;
	list_for_entry(aq, &lattrq, l) {
		if (!strcmp(aq->name, name))
			return aq->id;
	}
	return -1;
}

static struct attr_query *attr_query_find_name(char *name)
{
	struct attr_query *aq;
	list_for_entry(aq, &lattrq, l) {
		if (!strcmp(aq->name, name))
			return aq;
	}
	return NULL;
}

static struct attr_query *attr_query_find_group(unsigned int group)
{
	struct attr_query *aq;
	list_for_entry(aq, &lattrq, l) {
		if (aq->group == group)
			return aq;
	}
	return NULL;
}

static struct attr_query *attr_query_find_id(int id)
{
	struct attr_query *aq;
	list_for_entry(aq, &lattrq, l) {
		if (aq->id == id)
			return aq;
	}
	return NULL;
}

int attr_query_register(unsigned int group, char *name, query_fn query, void *data)
{
	int id;
	struct attr_query *aq;

	id = attr_query_get_id(name);
	if (id != -1) {
		debug(1, "ERROR: %s already registered\n");
		return -1;
	}
	aq = calloc(1, sizeof(*aq));
	if (!aq)
		return -1;
	aq->name = strdup(name);
	aq->group = group;
	aq->id = ++aq_id;
	aq->attr_get = query;
	aq->data  = data;
	list_append(&aq->l, &lattrq);
	return aq_id;
}

static int attr_query_do(struct attr_query *aq, enum attr_type type, struct attr *attr)
{
	return aq->attr_get(aq->data, type, attr);
}

int attr_query_byid(unsigned int id, enum attr_type type, struct attr *attr)
{
	struct attr_query *aq;
	aq = attr_query_find_id(id);
	if (aq)
		return attr_query_do(aq, type, attr);
	return 0;
}

int attr_query_byname(char *name, enum attr_type type, struct attr *attr)
{
	struct attr_query *aq;
	aq = attr_query_find_name(name);
	if (aq)
		return attr_query_do(aq, type, attr);
	return 0;
}

int attr_query_bygroup(unsigned int group, enum attr_type type, struct attr *attr)
{
	struct attr_query *aq;
	aq = attr_query_find_group(group);
	if (aq)
		return attr_query_do(aq, type, attr);
	return 0;
}
