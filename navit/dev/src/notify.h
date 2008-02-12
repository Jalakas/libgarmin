
#define NOTIFY_GPS		1
#define NOTIFY_ACTIVEGPS	2
#define NOTIFY_ROUTE		3

typedef int (*notify_fn)(unsigned int what, void *data);

int listen_for(int what, notify_fn callback);
void notify(int what, void *data);
