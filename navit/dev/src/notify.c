#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "notify.h"
#include "debug.h"
#include "item.h"
#include "attr.h"

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

int attr_query_get_id(char *name)
{
	struct attr_query *aq;
	list_for_entry(aq, &lattrq, l) {
		if (!strcmp(aq->name, name))
			return aq->id;
	}
	return -1;
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

int attr_query_byid(unsigned int id, enum attr_type type, struct attr *attr)
{
	return 0;
}

int attr_query_byname(char *name, enum attr_type type, struct attr *attr)
{
	return 0;
}
