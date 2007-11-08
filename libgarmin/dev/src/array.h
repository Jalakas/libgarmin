#ifndef _GAR_ARRAY_H__
struct garray {
	unsigned int base;
	unsigned int elements;
	unsigned int resize;
	unsigned int lastidx;
	void **ar;
};

int ga_init(struct garray *ga, unsigned int base, unsigned int isize);
void ga_free(struct garray *ga);
int ga_append(struct garray *ga, void *el);
int ga_trim(struct garray *ga);
// Get zero based index
void *ga_get(struct garray *ga, unsigned int idx);
// Get absolute index
void *ga_get_abs(struct garray *ga, unsigned int idx);
void ga_clear(struct garray *ga, unsigned int idx);
int ga_get_count(struct garray *ga);
void ga_set_base(struct garray *ga, unsigned int base);
int ga_get_base(struct garray *ga);
void ga_empty(struct garray *ga);
#define _GAR_ARRAY_H__
#endif
