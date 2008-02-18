include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/src -I$(top_srcdir)/src/lib -I$(top_srcdir)/src/modules
lib_LTLIBRARIES = testmod.la
testmod_la_SOURCES = testmod.c
#testmod_la_LDADD =
testmod_la_LDFLAGS =  -module

lib_LTLIBRARIES += tflog.la
tflog_la_SOURCES = tflog.c
tflog_la_LDFLAGS =  -module

lib_LTLIBRARIES += xmlconfig.la
xmlconfig_la_SOURCES = xmlconfig.c
xmlconfig_la_LDFLAGS =  -module

if VEHICLE_GPSD
lib_LTLIBRARIES += gpsd.la
gpsd_la_SOURCES = gpsd.c
gpsd_la_LIBADD = @GPSD_LIBS@
gpsd_la_LDFLAGS =  -module
endif

if HAVEDBUS
lib_LTLIBRARIES += dbusgps.la
AM_CPPFLAGS += @DBUS_CFLAGS@
dbusgps_la_SOURCES = dbusgps.c
dbusgps_la_LIBADD = @DBUS_LIBS@
dbusgps_la_LDFLAGS =  -module
endif
