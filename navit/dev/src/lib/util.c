#include <glib.h>
#include <ctype.h>
#include "util.h"

char *
convert2utf8(const char *charset, char *str)
{
	return g_convert(str, -1,"utf-8",charset,NULL,NULL,NULL);
}

void
freeutf8(char *str)
{
	g_free(str);
}

void
strtoupper(char *dest, const char *src)
{
	while (*src)
		*dest++=toupper(*src++);
	*dest='\0';
}

void
strtolower(char *dest, const char *src)
{
	while (*src)
		*dest++=tolower(*src++);
	*dest='\0';
}


static void
hash_callback(gpointer key, gpointer value, gpointer user_data)
{
	GList **l=user_data;
	*l=g_list_prepend(*l, value);
}

GList *
g_hash_to_list(GHashTable *h)
{
	GList *ret=NULL;
	g_hash_table_foreach(h, hash_callback, &ret);

	return ret;
}
