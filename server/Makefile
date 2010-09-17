LDFLAGS += -lcrypt

svrcomm: server.o cfg.o restrict.o ../common/socketwrapper.o \
		../common/util.o md5.o
	$Qecho LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

clean: mostlyclean
	$Qrm -f svrcomm

include ../config.mk

cfg.o: cfg.c cfg.h restrict.h ../config.h
md5.o: md5.c md5.h
restrict.o: restrict.c restrict.h
server.o: server.c ../config.h ../common/socketwrapper.h ../common/util.h \
 md5.h cfg.h restrict.h
