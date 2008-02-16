#include <stdlib.h>
#include "list.h"
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

