#!/bin/sh

regex='\(yo\|h\(i\|ello\)\|greetings\|sup\)'

tail -f out |\
	while read line
	do
		if echo $line|grep -qi $regex
		then
			echo 'greets from bot' > in
		fi
	done
