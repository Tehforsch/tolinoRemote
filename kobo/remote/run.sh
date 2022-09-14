#!/bin/sh
# In order to keep the TCP connection from the ESP32 open
# we need to keep stdin open, since otherwise the netcat
# version of the Kobo will send a "FIN ACK" signal, which
# implies that it doesn't want to send anymore. The WifiClient
# of the ESP32 will then close the connection (as opposed to
# keeping the connection open only for sending to the Kobo)
# This is different from how the OpenBSD version netcat behaves.
# 
# To do so, we need to pipe some process into netcat, but
# this process also needs to die as soon as the netcat process dies.
# Now, unfortunately we don't have process substitution here (bash
# is not available) and we don't understand the shell well enough
# to do this properly, so the "solution" is to not pipe directly into
# netcat, but into a script which runs netcat and will kill all sleep
# commands once netcat finishes. This is ugly as hell, but seems to work.
while true; do
    sleep infinity | /remote/runNetCatAndKillSleep.sh | xargs -n1 sh /remote/readLine.sh 1024 758
done
