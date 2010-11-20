all: glist.o gtray.o

%.o:%.c
	@echo CC_GTK $@
	$Q${CC} ${CFLAGS} -c -o $@ $<

include ../config.mk

CFLAGS  += `pkg-config --cflags ${GTKINC}`
LDFLAGS += `pkg-config --libs ${GTKINC}`
