MAKEFLAGS = --no-print-directory

.PHONY : clean mostlyclean all \
	server libcomm term gui fifo common

all: server libcomm term gui fifo
windows: libcomm comm.exe

server:
	$Qmake ${MAKEFLAGS} -C $@

common:
	$Qmake ${MAKEFLAGS} -C $@

libcomm: common
	$Qmake ${MAKEFLAGS} -C $@

term: libcomm
	$Qmake ${MAKEFLAGS} -C $@

gui: libcomm
	$Qmake ${MAKEFLAGS} -C $@

comm.exe: libcomm
	$Qmake ${MAKEFLAGS} -C gui $@

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
