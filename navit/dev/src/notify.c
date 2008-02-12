#include <stdlib.h>
#include "list.h"
#include "notify.h"

static list_head(llisteners);

struct listener {
	list_t l;
	unsigned int what;
	notify_fn callback;
};

static struct listener *
alloc_listener(int what, notify_fn callback)
{
	struct listener *l;
	l = calloc(1, sizeof(*l));
	if (!l)
		return NULL;
	list_init(&l->l);
	l->what = what;
	l->callback = callback;
	return l;
}

int listen_for(int what, notify_fn callback)
{
	struct listener *l;
	l = alloc_listener(what, callback);
	if (l) {
		list_append(&l->l, &llisteners);
		return 0;
	}
	return -1;
}

void notify(int what, void *data)
{
	struct listener *l;

	list_for_entry(l, &llisteners, l) {
		if (l->what == what)
			l->callback(what, data);
	}
}

