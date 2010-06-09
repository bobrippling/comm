CFLAGS = -g -W -Wall -Wcast-align -Wcast-qual -Wshadow -Wnested-externs \
		-Waggregate-return -Wbad-function-cast -Wpointer-arith -Wcast-align -Wwrite-strings \
		-Wstrict-prototypes -Wmissing-prototypes -Winline -Wredundant-decls -Wextra \
		-pedantic -pipe

LDFLAGS         =
CC              = gcc

BINARY          = comm
FILES           = main comm log protocol socket line strings ui/term
SRCS            = $(addsuffix .c, $(basename ${FILES}))
OBJS            = $(addsuffix .o, $(basename ${FILES}))

TAR             = ${BINARY}.tar.bz2


# DIRS := $(shell find . -type d)
# FILES := $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp) $(wildcard $(dir)/*.c))
# OBJS := $(addsuffix .o, $(basename $(FILES)))


# Macros
# ------
# tim.o : tim.c timmy.c
#
# $@ - target name (tim.o)
# $? - list of dependants more recent than $@
#      aka tim.c ? (age(tim.c) < arg($@))
#      same for timmy.c
# $^ - all dependancies (duplicates filtered out)
# $+ -  "       "
# $< - First dependant (i think)
# http://www.cprogramming.com/tutorial/makefiles_continued.html


# Main Binary ------------------------

$(BINARY) : $(OBJS)
	@echo CC -o $@
	@${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^

%.o: %.c
	@echo CC $<
	@${CC} ${CFLAGS} -c -o $@ $<


#${OBJS}: config.h # make them depend on it, but don't do any compiling or anything


.PHONY : clean new mostlyclean all love lint

lint:
	splint *.c

new: clean ${BINARY}

clean: mostlyclean
	@rm -f ${BINARY}

mostlyclean:
	@rm -f *.o

all: clean ${BINARY} clean

love:
	@echo "...baby"

srctar: clean
	@rm -f ${TAR}
	@tar -C .. -cjf ../${TAR} ${BINARY}
	@mv ../${TAR} .
