#include <string.h>
#include <stdio.h>

struct tf_log {
	int id;
	FILE *fp;
};

//static char *header = "type=track\n";

//sprintf(buffer,"%f %f type=trackpoint\n", pos_attr.u.coord_geo->lng, pos_attr.u.coord_geo->lat);



static int tflog_load(void)
{
	ENTER();
	return M_OK;
}

static int tflog_reconfigure(void)
{
	ENTER();
	return M_OK;
}

static int tflog_unload(void)
{
	ENTER();
	return M_OK;
}

NAVIT_MODULE(tflog, test_load,test_reconfigure,test_unload);
