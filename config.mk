CC        = gcc
CFLAGS   += -g -Wextra -Wall -pedantic -pipe -std=c99 -fstack-protector

LD        = gcc
LDFLAGS  += -g

Q         = @

.c.o:
	$Qecho CC $<
	$Q${CC} -c ${CFLAGS} -o $@ $<

mostlyclean:
	$Qrm -f *.o
