CC        = gcc
CFLAGS   += -g -Wextra -Wall -pedantic -pipe -std=c99

LD        = gcc
LDFLAGS  += -g

ifneq ($(shell uname -o),MSys)
CFLAGS   += -fstack-protector
LDFLAGS  += -fstack-protector
endif

Q         = @

.c.o:
	$Qecho CC $<
	$Q${CC} -c ${CFLAGS} -o $@ $<

mostlyclean:
	$Qrm -f *.o
