#! /bin/bash

#echo SPAWN "$@"

LOGFILE="$(basename $1)"-$$.log

if [ -n "$4" ]; then
   # include module ID
   LOGFILE="$(basename $1)"-$4-$$.log
fi

echo "spawn_vistle.sh: $@" > "$LOGFILE"
cd /tmp
export MV2_ENABLE_AFFINITY=0
export MPI_UNBUFFERED_STDIO=1
export DYLD_LIBRARY_PATH="$VISTLE_DYLD_LIBRARY_PATH"

export GMON_OUT_PREFIX=readcfx.out

if [ -n "$SLURM_JOB_ID" ]; then
   exec mpiexec -bind-to none "$@"
   #exec srun --overcommit "$@"
   #exec srun --overcommit --cpu_bind=no "$@"
fi

WRAPPER=""
#WRAPPER="valgrind"

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
         MPIHOSTS=$(echo viscluster70 viscluster{71..79} viscluster{51..60} |sed -e 's/ /,/g')
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
      exec mpirun -x LD_LIBRARY_PATH $LAUNCH -np ${MPISIZE} -tag-output -bind-to none $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   else
      exec mpirun -x LD_LIBRARY_PATH $LAUNCH -np ${MPISIZE} -tag-output -bind-to none -H ${MPIHOSTS} $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   fi
else
   if [ -z "$MPIHOSTS" ]; then
      exec mpirun -envall -prepend-rank -np ${MPISIZE} $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   elif [ "$BIND" = "1" ]; then
      echo exec mpirun -envall -prepend-rank -np ${MPISIZE} -hosts ${MPIHOSTS} -bind-to none $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
      exec mpirun -envall -prepend-rank -np ${MPISIZE} -hosts ${MPIHOSTS} -bind-to none $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   else
      exec mpirun -envall -prepend-rank -np ${MPISIZE} -hosts ${MPIHOSTS} $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   fi
fi

echo "default: $@"
exec WRAPPER "$@"
