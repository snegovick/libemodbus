ACLOCAL_AMFLAGS = -I m4

if BUILD_TESTS
EXTRA=tests
else
EXTRA=
endif

SUBDIRS = src $(EXTRA)

TESTS = tests/emodbus

include_HEADERS = src/emodbus_parser.h src/emodbus_parser.c
