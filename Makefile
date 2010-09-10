MAKEFLAGS = --no-print-directory

.PHONY : clean mostlyclean all \
	server libcomm term gui fifo

all: server libcomm term gui fifo
windows: libcomm wincomm

server:
	$Qmake ${MAKEFLAGS} -C $@

libcomm:
	$Qmake ${MAKEFLAGS} -C $@

term: libcomm
	$Qmake ${MAKEFLAGS} -C $@

gui: libcomm
	$Qmake ${MAKEFLAGS} -C $@

wincomm: libcomm
	$Qmake ${MAKEFLAGS} -C gui $@

fifo: libcomm
	$Qmake ${MAKEFLAGS} -C $@

clean:
	$Qrm -f server/svrcomm gui/guicomm \
		term/termcomm fifo/fifocomm libcomm/libcomm.a
	$Qfind . -iname \*.o|xargs rm -f

include config.mk
