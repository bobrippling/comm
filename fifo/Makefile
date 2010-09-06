fifocomm: fifo.o ../common/util.o ../libcomm/libcomm.a
	$Qecho LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

fifo.o: fifo.c ../config.h ../common/util.h

include ../config.mk
