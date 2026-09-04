SRCS=$(wildcard *.cpp)

TARGETS=$(patsubst %.cpp,%,$(SRCS))

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

# inside a built source tree, the tree's own headers and library (the
# examples track the head of development, which an installed sword may
# not match); anywhere else, the installed sword through pkg-config
SWORD_LA = ../../lib/libsword.la
ifneq ($(wildcard $(SWORD_LA)),)
SWORD_CFLAGS = -I../../include
# libsword.la names the libraries a static libsword needs (icu, clucene, ...)
SWORD_LIBS = -L../../lib/.libs -lsword $(shell sed -n "s/^dependency_libs='\(.*\)'/\1/p" $(SWORD_LA)) -Wl,-rpath,$(abspath ../../lib/.libs)
else
SWORD_CFLAGS = $(shell pkg-config --cflags sword)
SWORD_LIBS = $(shell pkg-config --libs sword)
endif

.cpp:
	g++ -O0 -g -Wall -Werror $(SWORD_CFLAGS) $< -o $@ $(SWORD_LIBS)
.c:
	gcc -O0 -g -Wall -Werror $(SWORD_CFLAGS) $< -o $@ $(SWORD_LIBS) -lstdc++


