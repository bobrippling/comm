include config.mk

CFLAGS         += -std=c99 -fstack-protector

SVR_OBJS        = server/server.o util.o socket_util.o
CLIENT_OBJS     = client/init.o   util.o socket_util.o client/common.o client/comm.o
TCOMM_OBJS      = ${CLIENT_OBJS} client/ui/term.o
FIFO_OBJS       = ${CLIENT_OBJS} client/ui/fifo.o
VERBOSE         = @

.PHONY : clean mostlyclean all libcomm


all: svrcomm tcomm fifocomm libcomm

svrcomm : ${SVR_OBJS}
	${VERBOSE}echo LD $@
	${VERBOSE}${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^

tcomm : ${TCOMM_OBJS}
	${VERBOSE}echo LD $@
	${VERBOSE}${CXX} ${CXXFLAGS} ${LDFLAGS} -o $@ $^

fifocomm : ${FIFO_OBJS}
	${VERBOSE}echo LD $@
	${VERBOSE}${CXX} ${CXXFLAGS} ${LDFLAGS} -o $@ $^

libcomm :
	@make -C libcomm

%.o: %.cpp
	${VERBOSE}echo CXX $<
	${VERBOSE}${CXX} ${CXXFLAGS} -c -o $@ $<

%.o: %.c
	${VERBOSE}echo CC $<
	${VERBOSE}${CC} ${CFLAGS} -c -o $@ $<


clean: mostlyclean
	${VERBOSE}rm -f svrcomm tcomm fifocomm

mostlyclean:
	${VERBOSE}find . -iname \*.o|xargs rm -f

client/init.o: client/init.c client/../config.h client/comm.h
client/ui/fifo.o: client/ui/fifo.c client/ui/../../config.h client/ui/../comm.h \
 client/ui/ui.h client/ui/../common.h
client/ui/term.o: client/ui/term.c client/ui/../comm.h client/ui/../../config.h
client/ui/line.o: client/ui/line.c client/ui/line.h
client/common.o: client/common.c client/../socket_util.h client/ui/ui.h \
 client/common.h
client/comm.o: client/comm.c client/common.h client/ui/ui.h client/comm.h \
 client/../config.h
server/server.o: server/server.c server/../config.h server/../util.h \
 server/../socket_util.h
socket_util.o: socket_util.c socket_util.h
util.o: util.c util.h
