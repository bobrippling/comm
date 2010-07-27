#!/bin/sh

usage(){
	if [ -n "$1" ]
	then echo $1 >&2
	fi
	echo "Usage: $0 [-p port] [-n name] host" >&2
	exit 1
}

host=
port=2848
name="Tim"

while [ -n "$1" ]
do
	if [ "$1" = "-n" ]
	then
		shift
		if [ -z "$1" ]
		then usage "need name"
		fi
		name="$1"
	elif [ "$1" = "-p" ]
	then
		shift
		if [ -z "$1" ]
		then usage "need port"
		fi
		port="$1"
	elif [ -z "$host" ]
	then
		host="$1"
	else
		usage "already have host: $host"
	fi
	shift
done

if [ -z "$host" ]
then usage "need host"
elif [ -z "$name" ]
then usage "need name"
fi

# Doesn't check for newlines in $name, oh well

(echo "NAME $name"; sed -u "s/^/MESSAGE $name: /")|netcat -c $host $port
