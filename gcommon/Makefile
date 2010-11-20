all: glist.o gtray.o gballoon.o test

test: gballoon.o

%.o:%.c
	@echo CC_GTK $@
	$Q${CC} ${CFLAGS} -c -o $@ $<

clean:
	@rm -f *.o

include ../config.mk

CFLAGS  = `pkg-config --cflags ${GTKINC}`
LDFLAGS = `pkg-config --libs ${GTKINC}`
