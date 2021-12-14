#!/bin/sh
while true; do
    /mnt/onboard/remote/runRemote.sh
    sleep 1
    busybox sleep 1
done
