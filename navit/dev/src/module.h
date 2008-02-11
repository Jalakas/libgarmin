
// module started ok
#define M_OK		1
// module started ok and wants to be unloaded
// for example, do a setup and go away
#define M_OK_UNLOAD	2
// can not start
#define M_FAILED	3

struct navit_module {
	char *module_name;
	int (*module_load)(void);
	int (*module_reconfigure)(void);
	int (*module_unload)(void);
};


void navit_modules_init(void);
int navit_request_module(const char *name);

#define NAVIT_MODULE(l,r,u)			\
struct navit_module navit_module = {		\
	.module_name = MODNAME,			\
	.module_load = l,			\
	.module_reconfigure = r,		\
	.module_unload = u,			\
}
