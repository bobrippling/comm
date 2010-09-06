MAKEFLAGS = --no-print-directory

.PHONY : clean mostlyclean all \
	server libcomm term glade fifo

all: server libcomm term glade fifo

server:
	$Qmake ${MAKEFLAGS} -C $@

libcomm:
	$Qmake ${MAKEFLAGS} -C $@

term: libcomm
	$Qmake ${MAKEFLAGS} -C $@

glade: libcomm
	$Qmake ${MAKEFLAGS} -C $@

fifo: libcomm
	$Qmake ${MAKEFLAGS} -C $@

clean:
	$Qrm -f server/svrcomm glade/guicomm \
		term/termcomm fifo/fifocomm libcomm/libcomm.a
	$Qfind . -iname \*.o|xargs rm -f

include config.mk
