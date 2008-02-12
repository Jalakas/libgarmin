#include <config.h>
#include <gps.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include "debug.h"
#include "module.h"
#include "coord.h"
#include "notify.h"
#include "gpssource.h"

#define IOFLAGS (G_IO_IN | G_IO_ERR | G_IO_HUP)

static list_head(lgpsd)

struct gpsd_priv {
	list_t l;
	struct gps_source *src;
	GIOChannel *iochan;
	guint watch;
	struct gps_data_t *gps;
	struct gps_data data;
	int good_speed;
	int backoff;
	char *url;
};

static struct gpsd_priv *polled_priv;

static gboolean gpsd_io(GIOChannel * iochan, GIOCondition condition, gpointer t);

static void
gpsd_callback(struct gps_data_t *data, char *buf, size_t len,
		      int level)
{
	struct gpsd_priv * priv = polled_priv;

	if (priv->gps) {
		priv->data.fix = data.fix.mode;

		// If data->fix.speed is NAN, then the drawing gets jumpy. 
		if (isnan(data->fix.speed)) {
			priv->good_speed = 0;
			return;
		}
		priv->good_speed++;

		if (data->set & SPEED_SET) {
			priv->data.speed = data->fix.speed * MPS_TO_KPH;
			data->set &= ~SPEED_SET;
		}
		if (data->set & TRACK_SET) {
			if (priv->good_speed > 2)
				priv->data.direction = data->fix.track;
			else
				priv->data.direction = NAN;
			data->set &= ~TRACK_SET;
		}
		if (data->set & ALTITUDE_SET) {
			priv->data.height = data->fix.altitude;
			data->set &= ~ALTITUDE_SET;
		}
		if (data->set & SATELLITE_SET) {
			priv->data.sats_used = data->satellites_used;
			priv->data.sats = data->satellites;
			data->set &= ~SATELLITE_SET;
		}
		if (data->set & STATUS_SET) {
			priv->data.status = data->status;
			data->set &= ~STATUS_SET;
		}
		if (data->set & PDOP_SET) {
			dbg(0, "pdop : %g\n", data->pdop);
			data->set &= ~PDOP_SET;
		}
		if (data->set & LATLON_SET) {
			priv->data.geo.lat = data->fix.latitude;
			priv->data.geo.lng = data->fix.longitude;
			dbg(1,"lat=%f lng=%f\n", priv->geo.lat, priv->geo.lng);
			data->set &= ~LATLON_SET;
			gps_source_data(priv->src, &priv->data);
		}
	} else {
		priv->data.fix = GPS_NONE;
		gps_source_data(priv->src, &priv->data);
	}
}

static int
gpsd_open(struct gpsd_priv *priv)
{
	char *source = strdup(priv->url);
	char *colon = index(source + 7, ':');
	if (colon) {
		*colon = '\0';
		priv->gps = gps_open(source + 7, colon + 1);
	} else
		priv->gps = gps_open(source + 7, NULL);
	free(source);
	if (!priv->gps)
		return 0;
	gps_query(priv->gps, "w+x\n");
	gps_set_raw_hook(priv->gps, gpsd_callback);
	priv->iochan = g_io_channel_unix_new(priv->gps->gps_fd);
	priv->watch = g_io_add_watch(priv->iochan, IOFLAGS, gpsd_io, priv);
	return 1;
}

static void
gpsd_close(struct gpsd_priv *priv)
{
	GError *error = NULL;

	if (priv->watch) {
		g_source_remove(priv->watch);
		priv->watch = 0;
	}
	if (priv->iochan) {
		g_io_channel_shutdown(priv->iochan, 0, &error);
		priv->iochan = NULL;
	}
	if (priv->gps) {
		gps_close(priv->gps);
		priv->gps = NULL;
	}
}

static gboolean
gpsd_io(GIOChannel * iochan, GIOCondition condition, gpointer t)
{
	struct gpsd_priv *priv = t;

	debug(10, "enter condition=%d\n", condition);
	if (condition == G_IO_IN) {
		if (priv->gps) {
			polled_priv = priv;
			if (gps_poll(priv->gps) !=0) {
				goto err;
			}
		}
		return TRUE;
	}
err:
	gpsd_close(priv);
	gpsd_start_reconnect(priv);
	return FALSE;
}

static void
gpsd_destroy(struct vehicle_priv *priv)
{
	gpsd_close(priv);
	if (priv->source)
		g_free(priv->source);
	g_free(priv);
}

static gboolean
gpsd_reconnect(gpointer data)
{
	struct gpsd_priv *priv = data;
	if (priv->gps)
		return FALSE;
	if (gpsd_open(priv))
		return FALSE;
	polled_priv = priv;
	gpsd_callback(NULL, NULL, 0, 0);
	return TRUE;
}

static void
gpsd_start_reconnect(struct gpsd_priv *priv)
{
	priv->backoff = 1;
	g_timeout_add_seconds(priv->backoff, gpsd_reconnect, priv); 
}

static struct vehicle_priv *
vehicle_gpsd_new_gpsd(struct vehicle_methods
			*meth, struct callback_list
			*cbl, struct attr **attrs)
{
	struct vehicle_priv *ret;
	struct attr *source;

	ENTER();
	source = attr_search(attrs, NULL, attr_source);
	ret = g_new0(struct vehicle_priv, 1);
	ret->source = g_strdup(source->u.str);
	ret->cbl = cbl;
	*meth = vehicle_gpsd_methods;
	if (vehicle_gpsd_open(ret))
		return ret;
	gpsd_start_reconnect(priv);
	vehicle_gpsd_destroy(ret);
	return NULL;
}

static 
int gpsd_create(char *name, int id, char *url)
{
	struct gps_source *src;
	struct gpsd_priv *priv;

	priv = calloc(1, sizeof(*priv));
	if (!priv)
		return -1;
	priv->url = strdup(url);
	if (!priv->url) {
		free(priv);
		return NULL;
	}
	src = gps_source_alloc(name);
	if (!src) {
		free(priv);
		return -1;
	}
	src->id = id;
	priv->src = src;
	list_append(&priv->l, &lgpsd);
	gps_register_source(src);
	gpsd_reconnect(priv);
	return 1;
}


#define MODNAME		"gpsd"

static int load_config(int reload)
{
	
}

static int gpsd_load(void)
{
	ENTER();
	load_config(0);
	return M_OK;
}

static int gpsd_reconfigure(void)
{
	ENTER();
	load_config(1);
	return M_OK;
}

static int gpsd_unload(void)
{
	ENTER();
	return M_OK;
}

NAVIT_MODULE(gpsd_load,gpsd_reconfigure,gpsd_unload);

