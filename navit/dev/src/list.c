/*
    Copyright (C) 2007  Alexander Atanasov <aatanasov@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
    
    This file is originaly based on linux kernel lists.
    
*/


#include "list.h"
void list_init(list_t *l)
{
	l->n = l;
	l->p = l;
}

static inline void list_ins(list_t *f, list_t *l, list_t *p, list_t *n)
{
	l->n = n;
	f->p = p;
	p->n = f;
	n->p = l;
}

void list_del(list_t *p,list_t *n)
{
	n->p = p;
	p->n = n;
}

void list_prepend(list_t *e,list_t *l)
{
	list_ins(e, e, l, l->n);
}

void list_append(list_t *e,list_t *l)
{
	list_ins(e, e, l->p, l);
}

void list_remove(list_t *e)
{
	list_del(e->p, e->n);
}

void list_remove_init(list_t *e)
{
	list_del(e->p,e->n);
	list_init(e);
}

void list_prepend_list(list_t *l, list_t *lhead)
{
	if (!list_empty(l)) {
		list_ins(l->n, l->p, lhead, lhead->n);
	}
}

void list_prepend_list_init(list_t *l, list_t *lhead)
{
	if (!list_empty(l)) {
		list_ins(l->n, l->p, lhead, lhead->n);
		list_init(l);
	}
}

void list_append_list(list_t *l, list_t *lhead)
{
	if (!list_empty(l)) {
		list_ins(l->n, l->p, lhead->p, lhead);
	}
}

void list_append_list_init(list_t *l, list_t *lhead)
{
	if (!list_empty(l)) {
		list_ins(l->n, l->p, lhead->p, lhead);
		list_init(l);
	}
}
