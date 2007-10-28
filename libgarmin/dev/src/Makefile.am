include $(top_srcdir)/Makefile.inc

AM_CPPFLAGS += -DDATAPREFIX=\"@datarootdir@\" 

lib_LIBRARIES = libgarmin.a

libgarmin_a_SOURCES = GarminTypedef.h  garmin_fat.c  garmin_rgn.h \
		bsp.c  garmin_fat.h  garmin_subdiv.c libgarmin_priv.h \
		bsp.h garmin_lbl.c  garmin_subdiv.h \
		garmin_lbl.h  garmin_typ.c list.c \
		garmin_obj.c  geoutils.c list.h \
		garmin.c garmin_rgn.c  geoutils.h \
		garmin_order.c garmin_order.h	\
		garmin_net.c garmin_net.h	\
		garmin_tdb.c garmin_tdb.h

include_HEADERS = libgarmin.h

clean-local:
	rm -rf *~ *.bak core
