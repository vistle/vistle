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

if [ -n "$SLURM_JOB_ID" ]; then
   exec mpiexec -bind-to none "$@"
   #exec srun --overcommit "$@"
   #exec srun --overcommit --cpu_bind=no "$@"
fi

VALGRIND=""

OPENMPI=0
if mpirun -version | grep open-mpi\.org > /dev/null; then
   OPENMPI=1
   echo "OpenMPI spawn: $@"
   LAUNCH="--launch-agent $(which orted)"
fi

BIND=0
case $(hostname) in
   viscluster70)
      BIND=1
      if [ -z "$MPIHOSTS" ]; then
         MPIHOSTS=$(echo viscluster70 viscluster{51..60} viscluster{71..79}|sed -e 's/ /,/g')
      fi
      ;;
   viscluster*)
      BIND=1
      if [ -z "$MPIHOSTS" ]; then
         MPIHOSTS=$(echo viscluster{50..60} viscluster{71..79} viscluster70|sed -e 's/ /,/g')
      fi
      ;;
   vishexa|vishexb)
      BIND=1
      if [ -z "$MPIHOSTS" ]; then
         MPIHOSTS="vishexa,vishexb"
      fi
      ;;
   *)
      if [ "$MPISIZE" = "" ]; then
         MPISIZE=1
      fi
      ;;
esac

if [ "$MPISIZE" = "" ]; then
   MPISIZE=$(echo ${MPIHOSTS} | sed -e 's/,/ /g' | wc -w)
fi

if [ "$OPENMPI" = "1" ]; then
   if [ -z "$MPIHOSTS" ]; then
      exec mpirun -x LD_LIBRARY_PATH $LAUNCH -np ${MPISIZE} $VALGRIND "$@"
   else
      exec mpirun -x LD_LIBRARY_PATH $LAUNCH -np ${MPISIZE} -H ${MPIHOSTS} $VALGRIND "$@"
   fi
else
   if [ -z "$MPIHOSTS" ]; then
      exec mpirun -envall -prepend-rank -np ${MPISIZE} $VALGRIND "$@" >> "$LOGFILE" 2>&1 < /dev/null
   elif [ "$BIND" = "1" ]; then
      exec mpirun -envall -prepend-rank -np ${MPISIZE} -hosts ${MPIHOSTS} -bind-to none $VALGRIND "$@" >> "$LOGFILE" 2>&1 < /dev/null
   else
      exec mpirun -envall -prepend-rank -np ${MPISIZE} -hosts ${MPIHOSTS} $VALGRIND "$@" >> "$LOGFILE" 2>&1 < /dev/null
   fi
fi

echo "default: $@"
exec VALGRIND "$@"
