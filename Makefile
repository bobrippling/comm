CFLAGS = -g -W -Wall -Wcast-align -Wcast-qual -Wshadow -Wnested-externs \
		-Waggregate-return -Wbad-function-cast -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wstrict-prototypes -Wmissing-prototypes -Winline -Wredundant-decls -Wextra \
		-pedantic -pipe

CC							= gcc
CXX							= g++
LDFLAGS					=

SVR_OBJS				= server.o util.o
VERBOSE					= @


commsvr : ${SVR_OBJS} settings.h
	${VERBOSE}echo LD $@
	${VERBOSE}${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${SVR_OBJS}

%.o: %.cpp
	${VERBOSE}echo CXX $<
	${VERBOSE}${CXX} ${CXXFLAGS} -c -o $@ $<

%.o: %.c
	${VERBOSE}echo CC $<
	${VERBOSE}${CC} ${CFLAGS} -c -o $@ $<


.PHONY : clean new mostlyclean all love lint

clean: mostlyclean
	${VERBOSE}rm -f commsvr

mostlyclean:
	${VERBOSE}rm -f *.o

# TODO: deps
