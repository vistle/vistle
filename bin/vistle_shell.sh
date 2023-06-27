#! /bin/sh

port=31093
host=localhost

if [ -n "$1" ]; then
    host="$1"
fi

if [ -n "$2" ]; then
    port="$2"
fi

for v in DYLD_LIBRARY_PATH DYLD_FRAMEWORK_PATH DYLD_FALLBACK_LIBRARY_PATH DYLD_FALLBACK_FRAMEWORK_PATH DYLD_INSERT_LIBRARIES; do
    eval val="\${VISTLE_${v}}"
    if [ -n "${val}" ]; then
        export $v="$val"
    fi
done
exec ipython -i -c "import vistle; vistle._vistle.sessionConnect(None, \"$host\", $port); from vistle import *;" -- "$@"
