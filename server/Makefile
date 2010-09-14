LDFLAGS += -lcrypt

svrcomm: server.o ../common/resolv.o ../common/socketwrapper.o \
		../common/util.o md5.o
	$Qecho LD server
	$Q${CC} ${LDFLAGS} -o $@ $^

clean: mostlyclean
	$Qrm -f svrcomm

include ../config.mk

md5.o: md5.c md5.h
server.o: server.c ../config.h ../common/resolv.h \
 ../common/socketwrapper.h ../common/util.h md5.h
