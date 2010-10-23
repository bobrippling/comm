CC        = gcc
CFLAGS   += -g -Wextra -Wall -pedantic -pipe -std=c99

LD        = gcc
LDFLAGS  += -g

# make lacks ||, i think
ifeq ($(shell uname -o),MSys)
else
  ifeq ($(shell uname -o),Cygwin)
  else
    ifeq ($(shell uname -o),Msys)
    else
      CFLAGS   += -fstack-protector
      LDFLAGS  += -fstack-protector
    endif
  endif
endif

Q         = @

.c.o:
	$Qecho CC $<
	$Q${CC} -c ${CFLAGS} -o $@ $<

mostlyclean:
	$Qrm -f *.o
