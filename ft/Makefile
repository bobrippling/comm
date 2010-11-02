GTKINC     = gtk+-2.0 gmodule-2.0

CFLAGS    += -Wall -Wextra -pedantic -std=c99

ifeq ($(shell uname -o),Msys)
LDFLAGS += -lws2_32
endif

CFLAGS_G  += ${CFLAGS}  `pkg-config --cflags ${GTKINC}`
LDFLAGS_G  = ${LDFLAGS} `pkg-config --libs ${GTKINC}`

all: tft gft

tft: tft.o libft/ft.o
	@echo LD $@
	$Q${CC} -o $@ $^ ${LDFLAGS}

gft: gft.o libft/ft.o gladegen.o
	@echo LD $@
	$Q${CC} ${LDFLAGS_G} -o $@ $^

gft.o: gft.c
	@echo CC $@
	$Q${CC} ${CFLAGS_G} -c -o $@ $<

gladegen.c: glade/gft.glade

clean:
	rm -f `find . -iname \*.o` tft gft gladegen.c

.PHONY: all clean

include ../config.mk

gft.o: gft.c libft/ft.h gladegen.h
tft.o: tft.c libft/ft.h
libft/ft.o: libft/ft.c libft/ft.h
