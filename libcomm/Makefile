libcomm.a: comm.o wrapper.o resolv.o
	$Qecho AR $@
	$Qrm -f $@
	$Qar rc $@ $^

clean:
	$Qrm -f libcomm.a

comm.c: comm.h
resolv.c: resolv.h
wrapper.c: wrapper.h

.PHONY: clean mostlyclean

include ../config.mk
