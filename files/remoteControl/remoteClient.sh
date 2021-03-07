#!/system/bin/sh

resolutionX=$(wm size | busybox tr -s " " | busybox cut -d " " -f 3 | busybox cut -d "x" -f 1)
resolutionY=$(wm size | busybox tr -s " " | busybox cut -d " " -f 3 | busybox cut -d "x" -f 2)
while [[ 1 ]]; do
    /system/usr/remoteControl/netcat -d -l 5000 | busybox xargs -n1 sh /system/usr/remoteControl/readLine.sh "$resolutionX" "$resolutionY"
    sleep 1
done
