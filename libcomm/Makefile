libcomm.a: comm.o ../common/resolv.o ../common/socketwrapper.o
	$Qecho AR $@
	$Qrm -f $@
	$Qar rc $@ $^

comm.o: comm.c comm.h ../common/resolv.h ../common/socketwrapper.h \
	../config.h

comm.c: comm.h

clean: mostlyclean
	$Qrm -f libcomm.a


.PHONY: clean mostlyclean

include ../config.mk
