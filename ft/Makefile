CFLAGS    += -Wall -Wextra -pedantic -std=c99

ifeq ($(shell uname -o),Msys)
LDFLAGS  += -lws2_32
endif

all: tft gft

tft: tft.o libft/ft.o
	@echo LD $@
	$Q${CC} -o $@ $^ ${LDFLAGS}

gft:
	@make ${MAKEFLAGS} -C $@

gft_clean:
	@make ${MAKEFLAGS} -C gft clean

clean: gft_clean
	@rm -f `find . -iname \*.o` tft gft/gladegen.c

.PHONY: all clean gft_clean gft

include ../config.mk

tft.o: tft.c libft/ft.h
