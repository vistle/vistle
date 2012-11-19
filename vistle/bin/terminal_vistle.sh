#! /bin/bash

if [ "$1" = "-x" ]; then
   shift
   EXIT='; exit'
else
   EXIT=''
fi

WD="$(pwd)"
exec osascript \
   -e 'tell application "Terminal"' \
   -e activate \
   -e "do script with command \"cd $WD; $* $EXIT\"" \
   -e "end tell"
