#!/bin/sh

me="Kickus The Kicker"
pass=

if [ $# -ne 2 ]
then
	echo "Usage: $0 pass name-to-kick"
	exit 1
fi

pass="$1"
name="$2"

echo "signal to quit"

ncat localhost 2848 <<!
NAME $me
MESSAGE $me: Goodday $name!
SU $pass
KICK $name
!
