VERBOSE = @

.PHONY: clean run all

all: Main.class

%.class : %.java bin
	${VERBOSE}echo javac $<
	${VERBOSE}javac -d bin $<

bin:
	mkdir -p bin

clean:
	find bin -delete

run: all
	sh -c 'cd bin; java Main localhost'

Comm.class: Comm.java
Main.class: Main.java Comm.class
