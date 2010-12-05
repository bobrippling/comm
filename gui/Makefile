GTKINC   = gtk+-2.0 gmodule-2.0
CFLAGS  += `pkg-config --cflags ${GTKINC}`
LDFLAGS += `pkg-config --libs   ${GTKINC}`

GUI_DEPS = gui.o gtkutil.o ../log/log.o \
					 ../cfg/cfg.o ../libcomm/libcomm.a \
					 ../gcommon/glist.o priv.o

guicomm: ${GUI_DEPS}
	@echo LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

comm.exe: ${GUI_DEPS} ../common/sockprintf.o comm.res
	@echo LD $@
	$Q${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS} -lws2_32 -lole32

clean: mostlyclean
	$Qrm -f guicomm comm.exe gladegen.h

gladegen.h: glade/col.glade glade/comm.glade glade/priv.glade

include ../config.mk

gtkutil.o: gtkutil.c gtkutil.h
gui.o: gui.c gtkutil.h ../log/log.h ../cfg/cfg.h ../libcomm/comm.h \
 ../gcommon/glist.h gladegen.h priv.h ../config.h
priv.o: priv.c priv.h
