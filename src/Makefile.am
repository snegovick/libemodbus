lib_LTLIBRARIES = libemodbus.la
libemodbus_la_SOURCES = emodbus_parser.c emodbus_parser.h
libemodbus_la_LDFLAGS = ${LDFLAGS} ${CCONTAINER_LIBDIR} ${CCONTAINER_LIBS} -Wl,--export-dynamic -rdynamic -version-info 0:1:0 
libemodbus_la_CFLAGS = ${CFLAGS} ${CCONTAINER_CFLAGS} -D_GNU_SOURCE=1 -D_REENTRANT -I./ -Wall
