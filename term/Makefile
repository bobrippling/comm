termcomm: term.o ../libcomm/libcomm.a
	$Qecho LD $@
	$Q${CC} ${CFLAGS} -o $@ $^

clean: mostlyclean
	$Qrm -f termcomm

.PHONY: clean mostlyclean

include ../config.mk

term.o: term.c ../config.h ../libcomm/comm.h
