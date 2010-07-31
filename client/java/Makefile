VERBOSE = @

.PHONY: clean run

Main.class:

%.class : %.java
	${VERBOSE}echo javac $<
	${VERBOSE}javac -d bin $<

clean:
	find bin -type f -delete

run: Main.class
	sh -c 'cd bin; java Main localhost'

Comm.class: Comm.java
Main.class: Main.java
