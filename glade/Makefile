CFLAGS   = -g -Wall -Wextra -pedantic -fstack-protector
LDFLAGS  = -g

GTKINC   = gtk+-2.0 gmodule-2.0
CFLAGS  += `pkg-config --cflags ${GTKINC}`
LDFLAGS += `pkg-config --libs   ${GTKINC}`

Q        =

comm: comm.o gtkutil.o ../libcomm/libcomm.a
	$Qecho LD $@
	$Q${CC} ${LDFLAGS} -o $@ $^

.c.o:
	$Qecho CC $@
	$Q${CC} ${CFLAGS} -c -o $@ $<

mostlyclean:
	$Qrm -f *.o

clean: mostlyclean
	$Qrm -f comm
