#!/bin/bash
# This script toggles the appearance of the 'onboard' on-screen keyboard utility.
# Please make sure the filename of this script does not contain the 'onboard' string.
# For Panasonic Toughbook CF-19, to use with tablet buttons, provided by the modified panasonic-hbtn driver

if pgrep "onboard" > /dev/null 2>&1
then
    echo "onboard running, killing"
    killall onboard
else
#    echo "onboard not running, starting"
    onboard &
fi
