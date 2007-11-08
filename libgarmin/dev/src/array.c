#include <malloc.h>
#include "array.h"

#define ARR_DEF_SIZE 4096
/* Array index starts from 1 */
int ga_init(struct garray *ga, unsigned int base, unsigned int isize)
{
	unsigned int s = isize;
	ga->lastidx = 0;
	if (!s)
		s = ARR_DEF_SIZE;
	ga->ar = calloc(s, sizeof(void *));
	if (!ga->ar)
		return -1;
	ga->base = base;
	ga->elements = s;
	ga->resize = s;
	return s;
}

int ga_append(struct garray *ga, void *el)
{
	if (ga->lastidx == ga->elements) {
		void *rptr;
		rptr = realloc(ga->ar, sizeof(void *) * (ga->elements + ga->resize));
		if (!rptr)
			return -1;
		ga->elements += ga->resize;
		ga->ar = rptr;
	}
	ga->ar[ga->lastidx] = el;
	ga->lastidx ++;
	return 0;
}

int ga_trim(struct garray *ga)
{
	void *rptr;
	int s;
	s = sizeof(void *) * ga->lastidx;
	if (!s)
		s = 1;
	rptr = realloc(ga->ar, s);
	if (!rptr)
		return -1;
	ga->elements = ga->lastidx;
	ga->ar = rptr;
	return 0;
}

void *ga_get_abs(struct garray *ga, unsigned int idx)
{
	if (idx >= ga->base + ga->lastidx)
		return NULL;
	if (idx < ga->base)
		return NULL;
	return ga->ar[idx-ga->base];
}

void *ga_get(struct garray *ga, unsigned int idx)
{
	if (idx >= ga->lastidx)
		return NULL;
	return ga->ar[idx];
}

void ga_clear(struct garray *ga, unsigned int idx)
{
	ga->ar[idx] = NULL;
	if (idx == ga->lastidx - 1)
		ga->lastidx --;
}

void ga_empty(struct garray *ga)
{
	int i;
	for (i=0; i < ga->lastidx; i++)
		ga->ar[i] = NULL;
	ga->lastidx = 0;
	ga_trim(ga);
}

int ga_get_count(struct garray *ga)
{
	return ga->lastidx;
}

int ga_get_base(struct garray *ga)
{
	return ga->base;
}

void ga_set_base(struct garray *ga, unsigned int base)
{
	ga->base = base;
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
	struct garray g;
	int i;
	char aaa[] = "alabala";
	char *d;
	ga_init(&g, 10);
	for (i=1; i < 16; i++) {
		ga_append(&g, aaa);
	}
	ga_trim(&g);
	for (i=1; i < 15; i++) {
		d = ga_get(&g, i);
		printf("%s\n", d);
	}
	printf("%d\n", g.elements);
}
#endif
