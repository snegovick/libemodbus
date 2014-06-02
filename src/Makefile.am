lib_LTLIBRARIES = libemodbus.la
libccontainer_la_SOURCES = parser.c parser.h
libccontainer_la_LDFLAGS = -Wl,--export-dynamic -rdynamic -version-info 0:1:0
libccontainer_la_CFLAGS = -D_GNU_SOURCE=1 -D_REENTRANT -I./ -Wall
