MAKEFLAGS = --no-print-directory

.PHONY : clean mostlyclean all \
	server libcomm term gui fifo ft

all: server libcomm term gui fifo ft
windows: libcomm comm.exe ft

info:
	@echo OS: $(shell uname -o)
	@echo CC: ${CC}
	@echo CFLAGS:  ${CFLAGS}
	@echo LD: ${LD}
	@echo LDFLAGS: ${LDFLAGS}

server:
	$Qmake ${MAKEFLAGS} -C $@

libcomm:
	$Qmake ${MAKEFLAGS} -C $@

term: libcomm
	$Qmake ${MAKEFLAGS} -C $@

gui: libcomm
	$Qmake ${MAKEFLAGS} -C $@

comm.exe: libcomm
	$Qmake ${MAKEFLAGS} -C gui $@

ft:
	$Qmake ${MAKEFLAGS} -C $@

fifo: libcomm
	$Qmake ${MAKEFLAGS} -C $@

clean:
	$Qmake ${MAKEFLAGS} -C server  clean
	$Qmake ${MAKEFLAGS} -C libcomm clean
	$Qmake ${MAKEFLAGS} -C term    clean
	$Qmake ${MAKEFLAGS} -C gui     clean
	$Qmake ${MAKEFLAGS} -C fifo    clean
	$Qfind . -iname \*.o|xargs rm -f

include config.mk
