#!/bin/sh

host="$@"
(printf "NAME Tim\0MESSAGE hi\0MESSAGE there\0"; sleep 1)|netcat -c $host 2848|tr '\0' '\n'
