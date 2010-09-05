CFLAGS   = -g -Wall -Wextra -pedantic
LDFLAGS  = -g

GTKINC   = gtk+-2.0 gmodule-2.0
CFLAGS  += `pkg-config --cflags ${GTKINC}`
LDFLAGS += `pkg-config --libs   ${GTKINC}`

comm: comm.o util.o ../../libcomm/libcomm.a
	@echo LD $@
	@${CC} ${LDFLAGS} -o $@ $^

.c.o:
	@echo CC $@
	@${CC} ${CFLAGS} -c -o $@ $<

mostlyclean:
	@rm -f *.o

clean: mostlyclean
	@rm -f comm
