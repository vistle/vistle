#! /bin/bash

LLDB=""
if [ "$1" = "-g" ]; then
   LLDB="lldb --"
   shift
fi

for v in LIBRARY_PATH FRAMEWORK_PATH FALLBACK_LIBRARY_PATH FALLBACK_FRAMEWORK_PATH INSERT_LIBRARIES; do
    eval val="\${VISTLE_DYLD_${v}}"
    if [ -n "${val}" ]; then
        export DYLD_$v="$val"
    fi
done

grealpath=echo
if which grealpath >/dev/null 2>&1; then
    grealpath=grealpath
fi

exec $LLDB "$(dirname $($grealpath $0))/Vistle.app/Contents/MacOS/Vistle" "$@"
