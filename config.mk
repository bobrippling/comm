INCDIR   = ../libcomm

CC       = gcc
CFLAGS  += -g -Wextra -Wall -pedantic -pipe -std=c99 -fstack-protector -I${INCDIR}

LD       = gcc
LDFLAGS += -g -fstack-protector

Q        = @

.c.o:
	$Qecho CC $<
	$Q${CC} -c ${CFLAGS} -o $@ $<

mostlyclean:
	$Qrm -f *.o
