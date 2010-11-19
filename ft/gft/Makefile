GTKINC     = gtk+-2.0 gmodule-2.0

ifeq ($(shell uname -o),Msys)
GFT_DEPS +=  gft.res
endif

CFLAGS_G  += ${CFLAGS}  `pkg-config --cflags ${GTKINC}`
LDFLAGS_G  = ${LDFLAGS} `pkg-config --libs ${GTKINC}`

gft: gft.o ../libft/ft.o gcfg.o gtransfers.o \
		../util/gqueue.o ../../common/glist.o \
		gtray.o ../../common/gtray.o ${GFT_DEPS}
	@echo LD $@
	$Q${CC} -o $@ $^ ${LDFLAGS_G}

gladegen.h: glade/gft.glade

g%.o: g%.c
	@echo CC_GTK $@
	$Q${CC} ${CFLAGS_G} -c -o $@ $<

clean:
	@rm -f gft *.o

include ../../config.mk

.PHONY: clean

gcfg.o: gcfg.c gcfg.h ../../config.h
gft.o: gft.c ../../config.h gladegen.h ../util/gqueue.h \
 ../../common/glist.h gtray.h ../libft/ft.h gcfg.h gtransfers.h
gtransfers.o: gtransfers.c gtransfers.h
gtray.o: gtray.c ../../common/gtray.h
