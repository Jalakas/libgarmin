include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/src -I$(top_srcdir)/src/lib -I$(top_srcdir)/src/modules
lib_LTLIBRARIES = testmod.la
testmod_la_SOURCES = testmod.c
#testmod_la_LDADD =
testmod_la_LDFLAGS =  -module

if VEHICLE_GPSD
lib_LTLIBRARIES += gpsd.la
gpsd_la_SOURCES = gpsd.c
gpsd_la_LIBADD = @GPSD_LIBS@
gpsd_la_LDFLAGS =  -module
endif
