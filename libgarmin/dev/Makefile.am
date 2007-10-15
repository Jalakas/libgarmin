AUTOMAKE_OPTIONS = dist-bzip2

include $(top_srcdir)/Makefile.inc

SUBDIRS=src utils data
pkgdoc_DATA = README COPYING TODO
EXTRA_DIST = README COPYING TODO autosh.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgarmin.pc

clean-local:
	rm -rf *~ *.bak core
