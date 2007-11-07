pkgdocdir=$(pkgdatadir)/doc
CFLAGS = -shared
AM_CPPFLAGS = -fPIC
if DEBUG
AM_CPPFLAGS += -DDEBUG
endif

