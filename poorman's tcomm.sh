#!/bin/sh

usage(){
	if [ -n "$1" ]
	then echo $1 >&2
	fi
	echo "Usage: $0 [-p port] host" >&2
	exit 1
}

host=
port=2848

while [ -n "$1" ]
do
	if [ "$1" = "-p" ]
	then
		shift
		if [ -z "$1" ]
		then
			usage "need port"
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
fi

netcat -c $host $port
