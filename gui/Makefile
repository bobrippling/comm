GTKINC   = gtk+-2.0 gmodule-2.0
CFLAGS  += `pkg-config --cflags ${GTKINC}`
LDFLAGS += `pkg-config --libs   ${GTKINC}`

GUI_DEPS = gui.o gtkutil.o gladegen.o ../log/log.o \
					 ../cfg/cfg.o ../libcomm/libcomm.a

guicomm: ${GUI_DEPS}
	$Qecho LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

# rule is in ../Makefile
#gladegen.c: gladegen_pre.c \
		glade/comm.glade glade/col.glade \
		gladegen_post.c

clean: mostlyclean
	$Qrm -f guicomm comm.exe gladegen.c

comm.exe: ${GUI_DEPS} ../common/sockprintf.o
	@#${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS} -LC:/MinGW/lib
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS} -lws2_32 -lole32

include ../config.mk

gladegen.o: gladegen.c gladegen.h
gladegen_post.o: gladegen_post.c
gladegen_pre.o: gladegen_pre.c gladegen.h
gtkutil.o: gtkutil.c gtkutil.h
gui.o: gui.c gtkutil.h gladegen.h ../log/log.h ../cfg/cfg.h \
 ../libcomm/comm.h ../config.h
