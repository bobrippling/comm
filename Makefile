CFLAGS = -g -W -Wall -Wcast-align -Wcast-qual -Wshadow -Wnested-externs \
		-Waggregate-return -Wbad-function-cast -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wstrict-prototypes -Wmissing-prototypes -Winline -Wredundant-decls -Wextra \
		-pedantic -pipe

CC							= gcc
LDFLAGS					=

BIN							= bin/Linux/comm
OBJD						= obj/Linux

OBJS						= ${OBJD}/main.o ${OBJD}/comm.o ${OBJD}/log.o \
									${OBJD}/protocol.o ${OBJD}/socket.o \
									${OBJD}/ui/line.o ${OBJD}/strings.o \
									${OBJD}/ui/ui.o


${BIN} : ${OBJS} config.h
	@echo LD $@
	@${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJS}

${OBJD}/%.o: %.c
	@echo CC $<
	@${CC} ${CFLAGS} -c -o $@ $<


.PHONY : clean new mostlyclean all love lint

clean: mostlyclean
	@rm -f ${BINARY}

mostlyclean:
	@rm -f *.o

${OBJD}/comm.o: comm.c protocol.h comm.h socket.h ui/line.h
${OBJD}/line.o: ui/line.c strings.h ui/line.h
${OBJD}/log.o: log.c log.h
${OBJD}/main.o: main.c log.h comm.h
${OBJD}/protocol.o: protocol.c log.h protocol.h
${OBJD}/socket.o: socket.c socket.h
${OBJD}/strings.o: strings.c strings.h
${OBJD}/ui.o: ui/term.h
${OBJD}/window.o: ui/window.c
