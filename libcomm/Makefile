CFLAGS += -std=c99

all: libcomm.a test

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $@ $<

libcomm.a: comm.o common.o socket_util.o
	@echo AR $@
	@rm -f $@
	@ar rc $@ $^

test: test.c libcomm.a
	@echo CC $@
	@${CC} ${CFLAGS} -o $@ $^

clean:
	@rm -f libcomm.a *.o

.PHONY: clean
