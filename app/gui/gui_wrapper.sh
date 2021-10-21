#! /bin/bash

GDB=""
if [ "$1" = "-g" ]; then
   GDB="gdb --args"
   shift
fi

export DYLD_LIBRARY_PATH="$VISTLE_DYLD_LIBRARY_PATH"
grealpath=echo
if which grealpath >/dev/null 2>&1; then
    grealpath=grealpath
fi

exec $GDB "$(dirname $($grealpath $0))/Vistle.app/Contents/MacOS/Vistle" "$@"
