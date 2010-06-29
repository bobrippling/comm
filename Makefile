CFLAGS = -g -W -Wall -Wcast-align -Wcast-qual -Wshadow -Wnested-externs \
		-Waggregate-return -Wbad-function-cast -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wstrict-prototypes -Wmissing-prototypes -Winline -Wredundant-decls -Wextra \
		-pedantic -pipe

CC							= gcc
CXX							= g++
LDFLAGS					=

SVR_OBJS				= server/server.o util.o socket_util.o
TCOMM_OBJS			= client/init.o   util.o socket_util.o client/ui/term.o
VERBOSE					= @

.PHONY : clean mostlyclean all


all: commsvr tcomm

commsvr : ${SVR_OBJS}
	${VERBOSE}echo LD $@
	${VERBOSE}${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^

tcomm : ${TCOMM_OBJS}
	${VERBOSE}echo LD $@
	${VERBOSE}${CXX} ${CXXFLAGS} ${LDFLAGS} -o $@ $^

%.o: %.cpp
	${VERBOSE}echo CXX $<
	${VERBOSE}${CXX} ${CXXFLAGS} -c -o $@ $<

%.o: %.c
	${VERBOSE}echo CC $<
	${VERBOSE}${CC} ${CFLAGS} -c -o $@ $<


clean: mostlyclean
	${VERBOSE}rm -f commsvr tcomm

mostlyclean:
	${VERBOSE}find . -iname \*.o|xargs rm -f


server.o: server/server.c settings.h util.h \
 socket_util.h server/server.h
