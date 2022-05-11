#!/bin/sh
ip="192.168.0.123"
nc -l -p 6303 | xargs -n1 /mnt/onboard/remote/supershell/readLine.sh 2>&1 | nc $ip 6304
