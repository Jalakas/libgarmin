#ifndef NAVIT_types_H
#define NAVIT_types_H

#include <ctype.h>

char *convert2utf8(const char *charset, char *str);
void freeutf8(char *str);

void strtoupper(char *dest, const char *src);
void strtolower(char *dest, const char *src);
GList * g_hash_to_list(GHashTable *h);

#endif

