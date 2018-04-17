#!/bin/bash

if [ "$(whoami)" != "root" ]; then
	echo "/!\\ You must run this script as root using sudo for example /!\\"
	exit 1
fi

killall wvdial