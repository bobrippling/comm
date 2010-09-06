#!/bin/sh

if [ $# -ne 1 ]
then
	echo "Usage: $0 file-to-read"
	exit 1
fi

in="$1"

regex='\(yo\|h\(i\|ello\)\|greetings\|sup\)'

tail -f "$in" |\
	while read line
	do
		if echo $line|grep -qi $regex
		then
			echo 'greets from bot' > in
		fi
	done
