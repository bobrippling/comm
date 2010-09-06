server: server.o ../util.o ../socket_util.o
	$Qecho LD server
	$Q${CC} ${LDFLAGS} -o $@ $^

server.o: server.c ../config.h ../util.h ../socket_util.h

include ../config.mk
