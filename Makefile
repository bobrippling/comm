MAKEFLAGS = --no-print-directory

.PHONY : clean mostlyclean all \
	server libcomm term gui fifo

all: server libcomm term gui fifo
windows: libcomm comm.exe

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

winzip:
	$Qmkdir Comm\ v2.0
	$Qcp gui/comm.exe Comm\ v2.0
	$Qcp -R gui/dlls/*  Comm\ v2.0
	$Qrm -f Comm\ v2.0.zip
	$Qzip -r Comm\ v2.0.zip Comm\ v2.0
	$Qrm -r Comm\ v2.0

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
