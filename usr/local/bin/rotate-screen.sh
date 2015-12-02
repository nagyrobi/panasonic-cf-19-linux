#!/bin/bash
# This script rotates the screen and touchscreen input 90 degrees each time it is called,
# also enables the virtual keyboard accordingly

# originally by Ruben Barkow: https://gist.github.com/rubo77/daa262e0229f6e398766
# adapted for Panasonic Toughbook CF-19 by nagyrobi

#### configuration
# find your Touchscreen  device with `xinput`
TouchscreenDevice='Fujitsu Component USB Touch Panel'

if [ "$1" = "--help"  ] || [ "$1" = "-h"  ] ; then
echo 'Usage: rotate-screen.sh [OPTION]'
echo
echo 'This script rotates the screen and touchscreen input 90 degrees each time it is called,'
echo 'also disables the touchpad, and enables the virtual keyboard accordingly'
echo
echo Usage:
echo ' -h --help display this help'
echo ' -j (just horizontal) rotates the screen and touchscreen input only 180 degrees'
echo ' -n always rotates the screen back to normal'
exit 0
fi

screenMatrix=$(xinput --list-props "$TouchscreenDevice" | awk '/Coordinate Transformation Matrix/{print $5$6$7$8$9$10$11$12$NF}')

# Matrix for rotation
# ⎡ 1 0 0 ⎤
# ⎜ 0 1 0 ⎥
# ⎣ 0 0 1 ⎦
normal='1 0 0 0 1 0 0 0 1'
normal_float='1.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,0.000000,1.000000'

#⎡ -1  0 1 ⎤
#⎜  0 -1 1 ⎥
#⎣  0  0 1 ⎦
inverted='-1 0 1 0 -1 1 0 0 1'
inverted_float='-1.000000,0.000000,1.000000,0.000000,-1.000000,1.000000,0.000000,0.000000,1.000000'

# 90° to the left
# ⎡ 0 -1 1 ⎤
# ⎜ 1  0 0 ⎥
# ⎣ 0  0 1 ⎦
left='0 -1 1 1 0 0 0 0 1'
left_float='0.000000,-1.000000,1.000000,1.000000,0.000000,0.000000,0.000000,0.000000,1.000000'

# 90° to the right
#⎡  0 1 0 ⎤
#⎜ -1 0 1 ⎥
#⎣  0 0 1 ⎦
right='0 1 0 -1 0 1 0 0 1'

if [ $screenMatrix == $normal_float ] && [ "$1" != "-n" ]
then
  echo "90° to the right"
  xrandr -o right
  xinput set-prop "$TouchscreenDevice" 'Coordinate Transformation Matrix' $right
  sleep 1
  onboard &
elif [ $screenMatrix == $inverted_float ] && [ "$1" != "-j" ] && [ "$1" != "-n" ]
then
  #this is not desired on Panasonic CF-19, code left in if somebody still wants it
  echo "Upside down"
  xrandr -o inverted
  xinput set-prop "$TouchscreenDevice" 'Coordinate Transformation Matrix' $inverted
elif [ $screenMatrix == $left_float ] && [ "$1" != "-j" ] && [ "$1" != "-n" ]
then
  echo "Back to normal"
  xrandr -o normal
  xinput set-prop "$TouchscreenDevice" 'Coordinate Transformation Matrix' $normal
  killall onboard
else
  echo "90° to the left"
  xrandr -o left
  xinput set-prop "$TouchscreenDevice" 'Coordinate Transformation Matrix' $left
fi

