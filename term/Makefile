comm: term.o ../libcomm/libcomm.a
	$Qecho LD $@
	$Q${CC} ${CFLAGS} -o $@ $^

clean: mostlyclean
	$Qrm -f term

.PHONY: clean mostlyclean

include ../config.mk
