
// module started ok
#define M_OK		1
// module started ok and wants to be unloaded
// for example, do a setup and go away
#define M_OK_UNLOAD	2
// can not start
#define M_FAILED	3

struct navit_module {
	int (*module_load)(void);
	int (*module_reconfigure)(void);
	int (*module_unload)(void);
};


void navit_modules_init(void);
int navit_request_module(const char *name);
