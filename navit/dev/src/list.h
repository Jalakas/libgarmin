#ifndef	__LIST_H
#define	__LIST_H


#define list_t struct struct_list

list_t {
	list_t	*n, *p;
};

#define	list_head(h) list_t h = { &h, &h }

void list_init(list_t *l);
void list_del(list_t *p,list_t *n);
void list_prepend(list_t *e,list_t *l);
void list_append(list_t *e,list_t *l);
#define list_add(e,l) list_append(e,l);
void list_remove(list_t *e);
void list_remove_init(list_t *e);
void list_prepend_list(list_t *l, list_t *lhead);
void list_prepend_list_init(list_t *l, list_t *lhead);
void list_append_list(list_t *l, list_t *lhead);
void list_append_list_init(list_t *l, list_t *lhead);


static __inline__ int list_empty(list_t *l) {return l->n == l;}
#define	list_for(_p,_l)	for (_p = (_l)->n; _p != (_l); _p = _p->n)
#define	list_for_backward(_p,_l) for (_p = (_l)->p; _p != (_l); _p = _p->p)
#define	list_entry(_p,_t,_m) ((_t *)((char *)(&_p->n)-(unsigned long)(&((_t *)0)->_m)))

#define	list_for_entry(_e,_l,_m)				\
	for (_e = list_entry((_l)->n, typeof(*_e), _m);		\
		&_e->_m != (_l);				\
		_e = list_entry(_e->_m.n, typeof(*_e), _m))

#define	list_for_entry_backward(_e,_l,_m)			\
	for (_e = list_entry((_l)->p, typeof(*_e), _m);		\
		&_e->_m != (_l);				\
		_e = list_entry(_e->_m.p, typeof(*_e), _m))

#define	list_for_first_entry(e,l,m)				\
	for (e = list_entry((l)->n, typeof(*e), m);		\
		&e->m != (l);					\
		e = list_entry((l)->n, typeof(*e), m))

#define	list_for_entry_safe(_e,_n,_l,_m)			\
	for (_e = list_entry((_l)->n, typeof(*_e), _m),		\
		_n = list_entry(_e->_m.n, typeof(*_e), _m);	\
		&_e->_m != (_l);				\
		_e = _n, _n = list_entry(_e->_m.n, typeof(*_e), _m))

#endif
