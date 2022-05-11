#!/bin/sh
resolutionX=1024
resolutionY=758
while true; do
    nc -l -p 6002 | xargs -n1 sh /remote/readLine.sh "$resolutionX" "$resolutionY"
    sleep 1
done
