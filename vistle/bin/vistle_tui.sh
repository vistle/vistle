#! /bin/bash

port=31093
host=localhost

if [ -n "$1" ]; then
    host="$1"
fi

if [ -n "$2" ]; then
    port="$2"
fi

exec ipython -i -c "import vistle; vistle._vistle.sessionConnect(None, \"$host\", $port); from vistle import *;" -- "$@"
