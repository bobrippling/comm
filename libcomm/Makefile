CFLAGS += -std=c99 -fstack-protector

all: libcomm.a term

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $@ $<

libcomm.a: comm.o wrapper.o resolv.o
	@echo AR $@
	@rm -f $@
	@ar rc $@ $^

term: term.c libcomm.a
	@echo LD $@
	@${CC} ${CFLAGS} -o $@ $^

clean:
	@rm -f libcomm.a *.o term

comm.c: comm.h
resolv.c: resolv.h
wrapper.c: wrapper.h

.PHONY: clean
