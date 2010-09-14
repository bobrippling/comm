#!/bin/sh

set -e

for d in common term libcomm fifo server gui
do
	cd $d
	gcc -MM *.c >> Makefile
	cd - > /dev/null
done
