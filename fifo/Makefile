fifocomm: fifo.o ../common/util.o ../libcomm/libcomm.a
	$Qecho LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

clean:
	$Qrm -f fifocomm

include ../config.mk

fifo.o: fifo.c ../config.h ../libcomm/comm.h ../common/util.h
