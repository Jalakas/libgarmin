struct gar_objdraworder {
	int objtype;
	int maxprio;
	unsigned char order[256];
};

struct gobject;

int gar_init_draworder(struct gar_objdraworder *o, int objtype);
int gar_free_draworder(struct gar_objdraworder *o);
int gar_get_draw_prio(struct gar_objdraworder *d, unsigned char objid);
int gar_del_draw_prio(struct gar_objdraworder *d, unsigned char objid, 
		unsigned char prio);
int gar_add_draw_prio(struct gar_objdraworder *d, unsigned char objid, 
			unsigned char prio);
int gar_set_default_poly_order(struct gar_objdraworder *od);
struct gobject *gar_order_objects(struct gobject *objs, struct gar_objdraworder *od, 
				int head);




