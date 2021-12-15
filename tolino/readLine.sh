xResolution="$1"
yResolution="$2"
command="$3"
let yPress=$yResolution/2
margin=50
if [[ "$command" == "prev" ]]; then
    # Press the middle left side of the screen
    xPress=$margin
elif [[ "$command" == "next" ]]; then
    # Press the middle right side of the screen
    let xPress=$xResolution-$margin
fi
input tap $xPress $yPress
