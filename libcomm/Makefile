libcomm.a: comm.o ../common/socketwrapper.o
	$Qecho AR $@
	$Qrm -f $@
	$Qar rc $@ $^

clean: mostlyclean
	$Qrm -f libcomm.a


.PHONY: clean mostlyclean

include ../config.mk

comm.o: comm.c comm.h ../common/socketwrapper.h ../config.h
