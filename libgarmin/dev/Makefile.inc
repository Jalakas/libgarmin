pkgdocdir=$(pkgdatadir)/doc

AM_CPPFLAGS =
if DEBUG
AM_CPPFLAGS += -DDEBUG
endif

