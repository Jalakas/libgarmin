#include <stdio.h>
#include "module.h"

#define MODNAME		"testmod"

static int test_load(void)
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	return M_OK;
}

static int test_reconfigure(void)
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	return M_OK;
}

static int test_unload(void)
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	return M_OK;
}

NAVIT_MODULE(test_load,test_reconfigure,test_unload);

