#!/bin/sh

while true; do
    if ping -c 1 10.42.42.1 2> /dev/null; then
        break
    fi
done
(while true; do
    if ping -c 1 10.42.42.1 2> /dev/null; then
       echo . 1>&2
    fi
    sleep 3
 done) 2> /dev/watchdog
