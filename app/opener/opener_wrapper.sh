#! /bin/bash

export DYLD_LIBRARY_PATH="$VISTLE_DYLD_LIBRARY_PATH"
export DYLD_FRAMEWORK_PATH="$VISTLE_DYLD_FRAMEWORK_PATH"
export DYLD_FALLBACK_LIBRARY_PATH="$VISTLE_FALLBACK_DYLD_LIBRARY_PATH"
export DYLD_FALLBACK_FRAMEWORK_PATH="$VISTLE_FALLBACK_DYLD_FRAMEWORK_PATH"
grealpath=echo
if which grealpath >/dev/null 2>&1; then
    grealpath=grealpath
fi

exec "$(dirname $($grealpath $0))/vistle_opener.app/Contents/MacOS/vistle_opener" "$@"
