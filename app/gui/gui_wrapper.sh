#! /bin/bash

LLDB=""
if [ "$1" = "-g" ]; then
   LLDB="lldb --"
   shift
fi

export DYLD_LIBRARY_PATH="$VISTLE_DYLD_LIBRARY_PATH"
export DYLD_FRAMEWORK_PATH="$VISTLE_DYLD_FRAMEWORK_PATH"
export DYLD_FALLBACK_LIBRARY_PATH="$VISTLE_FALLBACK_DYLD_LIBRARY_PATH"
export DYLD_FALLBACK_FRAMEWORK_PATH="$VISTLE_FALLBACK_DYLD_FRAMEWORK_PATH"
grealpath=echo
if which grealpath >/dev/null 2>&1; then
    grealpath=grealpath
fi

exec $LLDB "$(dirname $($grealpath $0))/Vistle.app/Contents/MacOS/Vistle" "$@"
