#! /bin/bash

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

exec "$(dirname $($grealpath $0))/vistle_opener.app/Contents/MacOS/vistle_opener" "$@"
