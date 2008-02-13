
#define NOTIFY_GPS		1
#define NOTIFY_ACTIVEGPS	2
#define NOTIFY_ROUTE		3
#define NOTIFY_NAVIT_SET	4
#define NOTIFY_NAVIT_GET	5
#define NOTIFY_NAVIT_ATTRS	6

typedef int (*notify_fn)(unsigned int what, void *priv, void *data);

int listen_for(int what, notify_fn callback, void *priv);
void notify(int what, void *data);
