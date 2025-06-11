#! /bin/sh

port=$(vistle_config int system net controlport 31093)
host=localhost

if [ -n "$1" ]; then
    host="$1"
fi

if [ -n "$2" ]; then
    port="$2"
fi

for v in LIBRARY_PATH FRAMEWORK_PATH FALLBACK_LIBRARY_PATH FALLBACK_FRAMEWORK_PATH INSERT_LIBRARIES; do
    eval val="\${VISTLE_DYLD_${v}}"
    if [ -n "${val}" ]; then
        export DYLD_$v="$val"
    fi
done

PY=python3
if which ipython >/dev/null 2>&1; then
    PY=ipython
fi
exec "$PY" -i -c "import vistle; vistle._vistle.sessionConnect(None, \"$host\", $port); from vistle import *;" -- "$@"
