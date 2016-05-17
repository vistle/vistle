#! /bin/bash

#echo SPAWN "$@"

LOGFILE="$(basename $1)"-$$.log

if [ -n "$4" ]; then
   # include module ID
   LOGFILE="$(basename $1)"-$4-$$.log
fi

echo "spawn_vistle.sh: $@" > "$LOGFILE"
export MV2_ENABLE_AFFINITY=0
export MPI_UNBUFFERED_STDIO=1
export DYLD_LIBRARY_PATH="$VISTLE_DYLD_LIBRARY_PATH"

if [ "$MPISIZE" = "" ]; then
   MPISIZE=$(wc -l /root/.mpi-hostlist)
fi

/root/bin/mpirun.sh "$@"
