pkgdocdir=$(pkgdatadir)/doc
CFLAGS = -fPIC
AM_CPPFLAGS = -fPIC
if DEBUG
AM_CPPFLAGS += -DDEBUG
endif

