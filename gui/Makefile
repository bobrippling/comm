GTKINC   = gtk+-2.0 gmodule-2.0
CFLAGS  += `pkg-config --cflags ${GTKINC}`
LDFLAGS += `pkg-config --libs   ${GTKINC}`

GUI_DEPS = gui.o gtkutil.o gladegen.o ../log/log.o \
					 ../cfg/cfg.o ../libcomm/libcomm.a

guicomm: ${GUI_DEPS}
	@echo LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

comm.exe: ${GUI_DEPS} ../common/sockprintf.o comm.res
	@echo LD $@
	@${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS} -lws2_32 -lole32

clean: mostlyclean
	$Qrm -f guicomm comm.exe gladegen.c

gladegen.c: glade/comm.glade glade/col.glade

include ../config.mk

gladegen.o: gladegen.c gladegen.h
gladegen_post.o: gladegen_post.c
gladegen_pre.o: gladegen_pre.c gladegen.h
gtkutil.o: gtkutil.c gtkutil.h
gui.o: gui.c gtkutil.h gladegen.h ../log/log.h ../cfg/cfg.h \
 ../libcomm/comm.h ../config.h
