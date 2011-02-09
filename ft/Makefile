TARGETS = tft gft tftd

ifeq ($(shell uname -o),GNU/Linux)
#TARGETS += tftd
endif

all: ${TARGETS}

tft: tft.o libft/ft.o
	@echo LD $@
	$Q${CC} -o $@ $^ ${LDFLAGS}

gft:
	@make ${MAKEFLAGS} -C $@

tftd: tftd.o libft/ft.o
	@echo LD $@
	$Q${CC} -o $@ $^ ${LDFLAGS}

gft_clean:
	@make ${MAKEFLAGS} -C gft clean

clean: gft_clean
	@rm -f *.o tft tftd

.PHONY: all clean gft_clean gft

include ../config.mk

ifeq ($(shell uname -o),Msys)
LDFLAGS  += -lws2_32
endif

tft.o: tft.c libft/ft.h
tftd.o: tftd.c libft/ft.h
