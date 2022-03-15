#! /bin/sh

port=31093
host=localhost

if [ -n "$1" ]; then
    host="$1"
fi

if [ -n "$2" ]; then
    port="$2"
fi

if [ -n "$VISTLE_DYLD_LIBRARY_PATH" ]; then
    export DYLD_LIBRARY_PATH="$VISTLE_DYLD_LIBRARY_PATH"
fi
exec ipython -i -c "import vistle; vistle._vistle.sessionConnect(None, \"$host\", $port); from vistle import *;" -- "$@"
