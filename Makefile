.PHONY : clean mostlyclean all \
	server libcomm term gui fifo \
	ft gcommon

MAKE_PRE  = "--- make "
MAKE_POST = " ---"

all: server libcomm term gui fifo ft
windows: libcomm comm.exe ft

info:
	@echo OS: $(shell uname -o)
	@echo CC: ${CC}
	@echo CFLAGS:  ${CFLAGS}
	@echo LD: ${LD}
	@echo LDFLAGS: ${LDFLAGS}

server:
	@echo ${MAKE_PRE}$@${MAKE_POST}
	$Qmake ${MAKEFLAGS} -C $@

libcomm:
	@echo ${MAKE_PRE}$@${MAKE_POST}
	$Qmake ${MAKEFLAGS} -C $@

term: libcomm
	@echo ${MAKE_PRE}$@${MAKE_POST}
	$Qmake ${MAKEFLAGS} -C $@

gui: libcomm gcommon
	@echo ${MAKE_PRE}$@${MAKE_POST}
	$Qmake ${MAKEFLAGS} -C $@

comm.exe: libcomm
	@echo ${MAKE_PRE}$@${MAKE_POST}
	$Qmake ${MAKEFLAGS} -C gui $@

ft: gcommon
	@echo ${MAKE_PRE}$@${MAKE_POST}
	$Qmake ${MAKEFLAGS} -C $@

gcommon:
	@echo ${MAKE_PRE}$@${MAKE_POST}
	$Qmake ${MAKEFLAGS} -C $@

fifo: libcomm
	@echo ${MAKE_PRE}$@${MAKE_POST}
	$Qmake ${MAKEFLAGS} -C $@

clean:
	$Qmake ${MAKEFLAGS} -C server  clean
	$Qmake ${MAKEFLAGS} -C libcomm clean
	$Qmake ${MAKEFLAGS} -C term    clean
	$Qmake ${MAKEFLAGS} -C gui     clean
	$Qmake ${MAKEFLAGS} -C fifo    clean
	$Qmake ${MAKEFLAGS} -C ft      clean
	$Qfind . -iname \*.o|xargs rm -f

include config.mk
