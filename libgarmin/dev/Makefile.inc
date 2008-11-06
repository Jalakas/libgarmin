pkgdocdir=$(pkgdatadir)/doc
AM_CPPFLAGS =
if !TARGET_WIN32CE
CFLAGS += -shared
AM_CPPFLAGS += -fPIC
endif
if DEBUG
CFLAGS += -g
AM_CPPFLAGS += -DDEBUG
endif

