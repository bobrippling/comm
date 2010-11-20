ifeq ($(shell uname -o),Msys)
GFT_DEPS +=  gft.res
LDFLAGS  += -lws2_32
endif

gft: gft.o ../libft/ft.o gcfg.o gtransfers.o \
		../util/gqueue.o ../../gcommon/glist.o \
		gtray.o ../../gcommon/gtray.o \
		${GFT_DEPS}
	@echo LD $@
	$Q${CC} -o $@ $^ ${LDFLAGS_G}

gladegen.h: glade/gft.glade

g%.o: g%.c
	@echo CC_GTK $@
	$Q${CC} ${CFLAGS_G} -c -o $@ $<

clean:
	@rm -f gft *.o

include ../../config.mk

CFLAGS_G   += `pkg-config --cflags ${GTKINC}`
LDFLAGS_G  += `pkg-config --libs ${GTKINC}`

.PHONY: clean

gcfg.o: gcfg.c gcfg.h ../../config.h
gft.o: gft.c ../../config.h gladegen.h ../util/gqueue.h \
 ../../gcommon/glist.h gtray.h ../libft/ft.h gcfg.h gtransfers.h
gtransfers.o: gtransfers.c gtransfers.h
gtray.o: gtray.c ../../gcommon/gtray.h
