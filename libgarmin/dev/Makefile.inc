pkgdocdir=$(pkgdatadir)/doc
CFLAGS += -shared
AM_CPPFLAGS = -fPIC
if DEBUG
CFLAGS += -g
AM_CPPFLAGS += -DDEBUG
endif

