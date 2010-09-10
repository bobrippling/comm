GTKINC   = gtk+-2.0 gmodule-2.0
CFLAGS  += `pkg-config --cflags ${GTKINC}`
LDFLAGS += `pkg-config --libs   ${GTKINC}`

guicomm: comm.o gtkutil.o ../libcomm/libcomm.a
	$Qecho LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

clean: mostlyclean
	$Qrm -f comm

wincomm: comm.o gtkutil.o ../common/sockprintf.o \
		../libcomm/libcomm.a
	${CC} ${CFLAGS} -o guicomm $^ ${LDFLAGS} -lws2_32

include ../config.mk
