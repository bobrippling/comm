CFLAGS += -std=c99

all: libcomm.a term

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $@ $<

libcomm.a: comm.o wrapper.o resolv.o
	@echo AR $@
	@rm -f $@
	@ar rc $@ $^

term: term.c libcomm.a
	@echo CC $@
	@${CC} ${CFLAGS} -o $@ $^

clean:
	@rm -f libcomm.a *.o term

.PHONY: clean
