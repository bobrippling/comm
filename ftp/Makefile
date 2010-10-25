GTKINC     = gtk+-2.0 gmodule-2.0

CFLAGS    += -Wall -Wextra -pedantic -std=c99

ifeq ($(shell uname -o),Msys)
LDFLAGS += -lws2_32
endif

CFLAGS_G  += ${CFLAGS}  `pkg-config --cflags ${GTKINC}`
LDFLAGS_G  = ${LDFLAGS} `pkg-config --libs ${GTKINC}`

all: tft gft

tft: tft.o libft/ft.o
	${CC} -o $@ $^ ${LDFLAGS}

gft: gft.o libft/ft.o
	${CC} ${LDFLAGS_G} -o $@ $^

gft.o: gft.c
	${CC} ${CFLAGS_G} -o $@ $<

clean:
	rm -f `find . -iname \*.o` tft

.PHONY: all clean

tft.o: tft.c libft/ft.h
libft/ft.o: libft/ft.c libft/ft.h
