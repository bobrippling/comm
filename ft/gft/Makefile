ifeq ($(shell uname -o),Msys)
GFT_DEPS    +=  gft.res
LDFLAGS_G   += ${LDFLAGS} -lws2_32 -lcomctl32
endif

gft: gft.o ../libft/ft.o gcfg.o gtransfers.o \
		../util/gqueue.o gtray.o \
		../../common/sleep.o \
		../../gcommon/glist.o \
		../../gcommon/gtray.o \
		../../gcommon/gballoon.o \
		${GFT_DEPS}
	@echo LD $@
	$Q${CC} -o $@ $^ ${LDFLAGS_G}

gladegen.h: glade/gft.glade

g%.o: g%.c
	@echo CC_GTK $@
	$Q${CC} ${CFLAGS_G} -c -o $@ $<

clean:
	@rm -f gft *.o gft.res gladegen.h

include ../../config.mk

CFLAGS_G   = ${CFLAGS}  `pkg-config --cflags ${GTKINC}`
LDFLAGS_G += ${LDFLAGS} `pkg-config --libs ${GTKINC}`

.PHONY: clean

gcfg.o: gcfg.c gcfg.h gft.h ../../config.h
gft.o: gft.c ../../config.h gladegen.h ../util/gqueue.h \
 ../../gcommon/glist.h gtray.h ../libft/ft.h ../../common/gnudefs.h \
 gcfg.h gtransfers.h gft.h
gtransfers.o: gtransfers.c gtransfers.h
gtray.o: gtray.c ../../gcommon/gtray.h gtray.h gft.h
