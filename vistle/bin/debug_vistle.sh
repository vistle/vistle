#! /bin/bash

D="$(dirname ${0})"
"$D/terminal_vistle.sh" -x gdb -x "$D/run.gdb" --args "$@" &
