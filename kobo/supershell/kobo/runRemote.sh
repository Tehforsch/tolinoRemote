#!/bin/sh
folder=/mnt/onboard/remote
nc -l -p 6000 | xargs -n1 $folder/readLine.sh 2>&1 | nc 192.168.178.20 6001
