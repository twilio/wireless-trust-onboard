#!/bin/bash

if [ "$(whoami)" != "root" ]; then
	echo "/!\\ You must run this script as root using sudo for example /!\\"
	exit 1
fi

if [ -e /dev/ttyACM0 ]; then
	echo "Please turn off Cinterion board!"
	echo "Waiting..."
	while [ -e /dev/ttyACM0 ]; do
		sleep 1
	done
fi

echo "Please turn on Cinterion board!"
echo "Waiting..."
while [ ! -e /dev/ttyACM0 ]; do
	sleep 1
done

echo "Detected!"

sleepTime=10
echo "Waiting $sleepTime seconds for network interface to be ready..."

log="modem-$(date).log" 
sleep "$sleepTime"
wvdial 1&> "$log" 2>&1 &
disown


i=0
while ! ping -c 1 -W 1 www.google.com; do
    sleep 1
    i=$((i+1))
    if [ "$i" -eq "10" ]; then
        cat "$log"
    fi
done

echo "Modem is ready!"
