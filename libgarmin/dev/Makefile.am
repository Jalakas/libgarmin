AUTOMAKE_OPTIONS = dist-bzip2

include $(top_srcdir)/Makefile.inc

SUBDIRS=src data
if !TARGET_WIN32CE
SUBDIRS+=utils
endif
DIST_SUBDIRS=src data utils
pkgdoc_DATA = README COPYING TODO
EXTRA_DIST = README COPYING TODO autosh.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgarmin.pc
noinst_HEADERS=version.h

CLEANFILES=version.h
BUILT_SOURCES=version.h

clean-local:
	rm -rf *~ *.bak core

##
## on every build, record the working copy revision string
##
.svnversion:
	@if test -d .svn; then 						\
	svnversion > .svnversion;					\
	fi;

version.h: .svnversion
	@echo "Generate version.h";					\
	echo -n "#define LIBVERSION \"$(PACKAGE_STRING)" > version.h;	\
	if test -d .svn; then 						\
		echo -n "-r`svnversion -n`" >> version.h;		\
	fi;								\
	echo "\"" >> version.h

#    echo -n 'const char* svn_version(void) { const char* SVN_Version = "' \
#                                      > svn_version.c
#    svnversion -n .                   >> svn_version.c
#    echo '"; return SVN_Version; }'   >> svn_version.c

