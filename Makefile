MAKEFLAGS = --no-print-directory

.PHONY : clean mostlyclean all \
	server libcomm term gui fifo

all: server libcomm term gui fifo
windows: libcomm wingui

server:
	$Qmake ${MAKEFLAGS} -C $@

libcomm:
	$Qmake ${MAKEFLAGS} -C $@

term: libcomm
	$Qmake ${MAKEFLAGS} -C $@

gui: libcomm
	$Qmake ${MAKEFLAGS} -C $@

wingui: libcomm
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
