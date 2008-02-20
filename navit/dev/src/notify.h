#include "item.h"
#include "attr.h"

#define NOTIFY_GPS		1
#define NOTIFY_NAVIT		2
#define NOTIFY_ROUTE		3

#define GPS_ACTIVE		(1<<0)
#define GPS_DATA		(1<<1)
#define GPS_NMEA		(1<<2)

#define NAVIT_GET		(1<<0)
#define NAVIT_SET		(1<<1)
#define NAVIT_ATTRS		(1<<2)

#define ROUTE_SETPOS		(1<<0)
#define ROUTE_SETDEST		(1<<1)

typedef int (*notify_fn)(unsigned int group, int mask, void *priv, void *data);

int listen_for(unsigned int group, int mask, notify_fn callback, void *priv);
void notify(unsigned int group, int mask, void *data);

typedef int (*query_fn)(void *data, enum attr_type type, struct attr *attr);
int attr_query_register(unsigned int group, char *name, query_fn query, void *data);
int attr_query_byid(unsigned int id, enum attr_type type, struct attr *attr);
int attr_query_byname(char *name, enum attr_type type, struct attr *attr);
int attr_query_bygroup(unsigned int group, enum attr_type type, struct attr *attr);

