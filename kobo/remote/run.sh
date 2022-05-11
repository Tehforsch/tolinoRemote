#!/bin/sh
resolutionX=1024
resolutionY=758
while true; do
    nc -l -p 6302 | xargs -n1 sh /remote/readLine.sh "$resolutionX" "$resolutionY"
done
