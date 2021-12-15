#!/bin/sh
ip="192.168.178.20"
nc -l -p 6003 | xargs -n1 /mnt/onboard/remote/supershell/readLine.sh 2>&1 | nc $ip 6004
