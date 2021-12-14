#!/bin/sh
# The kobo is rotated - The origin (0, 0) is at the top right corner, x goes down, y goes to the left.
xResolution="$1"
yResolution="$2"
command="$3"
xPress=$xResolution/2
margin=100
if [[ "$command" == "back" ]]; then
    # Press the middle left side of the screen
    # This translates to a medium x value and high y value
    yPress=$yResolution-$margin
elif [[ "$command" == "next" ]]; then
    # Press the middle right side of the screen
    # This translates to a medium x value and low y value
    yPress=$margin
fi
/remote/koboTouch $xPress $yPress
