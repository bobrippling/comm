fifocomm: fifo.o ../common/util.o ../libcomm/libcomm.a
	$Qecho LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

fifo.o: fifo.c ../config.h ../libcomm/comm.h ../common/util.h

clean:
	$Qrm -f fifocomm

include ../config.mk
