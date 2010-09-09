GTKINC   = gtk+-2.0 gmodule-2.0
CFLAGS  += `pkg-config --cflags ${GTKINC}`
LDFLAGS += `pkg-config --libs   ${GTKINC}`

guicomm: comm.o gtkutil.o ../libcomm/libcomm.a
	$Qecho LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

clean: mostlyclean
	$Qrm -f comm

wincomm: comm.o gtkutil.o \
		../common/sockprintf.o ../libcomm/libcomm.a
	@# the implimit {comm,gtkutil}.o make includes
	@# the updated cflags/ldflags, hence why it compiles
	${CC} ${CFLAGS} -o guicomm $^ ${LDFLAGS} \
		-lws2_32 c:/Windows/system32/ws2_32.dll
	@# winsock2.h: -lws2_32
	@# winsock.h:  -lwsock32
	@# addrinfo:   c:/MinGW/lib/libws2_32.a

include ../config.mk
