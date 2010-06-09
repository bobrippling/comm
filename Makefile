CFLAGS = -g -W -Wall -Wcast-align -Wcast-qual -Wshadow -Wnested-externs \
		-Waggregate-return -Wbad-function-cast -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wstrict-prototypes -Wmissing-prototypes -Winline -Wredundant-decls -Wextra \
		-pedantic -pipe

CC              = gcc
LDFLAGS         =

BIN             = comm
OBJS            = main.o comm.o log.o protocol.o socket.o line.o strings.o ui/term.o


${BIN} : ${OBJS} config.h
	@echo LD $@
	@${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJS}

%.o: %.c
	@echo CC $<
	@${CC} ${CFLAGS} -c -o $@ $<


.PHONY : clean new mostlyclean all love lint

clean: mostlyclean
	@rm -f ${BINARY}

mostlyclean:
	@rm -f *.o

comm.o: comm.c protocol.h comm.h socket.h line.h
line.o: line.c strings.h line.h
log.o: log.c log.h
main.o: main.c log.h comm.h
protocol.o: protocol.c log.h protocol.h
socket.o: socket.c socket.h
strings.o: strings.c strings.h
term.o: ui/term.c ui/term.h
window.o: ui/window.c
