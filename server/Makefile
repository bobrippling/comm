svrcomm: server.o ../common/resolv.o ../common/socketwrapper.o ../common/util.o
	$Qecho LD server
	$Q${CC} ${LDFLAGS} -o $@ $^

server.o: server.c ../config.h ../common/resolv.h \
 ../common/socketwrapper.h ../common/util.h

clean: mostlyclean
	$Qrm -f svrcomm

include ../config.mk
