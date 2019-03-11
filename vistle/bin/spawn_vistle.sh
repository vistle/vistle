#! /bin/bash

#if [ -z "${OMP_NUM_THREADS}" ]; then
#   APRUN_FLAGS="-j2 --cc none"
#else
#   APRUN_FLAGS="-j2 --cc numa_node -d$OMP_NUM_THREADS"
#fi
#APRUN_FLAGS="-j2 --cc numa_node" # enable hyper-threading and bind processes to NUMA nodes
#APRUN_FLAGS="-j2" # enable hyper-threading

#APRUN_FLAGS="-j2 -cc 0-11,24-35:12-23,36-47" # enable hyper-threading



if [ -n "$VISTLE_LAUNCH" ]; then
   case $VISTLE_LAUNCH in
      *aprun*)
         NUMNODES=$(wc -l $PBS_NODEFILE | awk '{print $1}')
         APRUN_FLAGS="-b -j 2 -d 24 -cc numa_node -N 2 -n $NUMNODES" # enable hyper-threading
         ;;
   esac

   case $VISTLE_LAUNCH in
      manual)
      echo '**********************************************************'
      echo "log in to $(hostname) and launch "$@" in parallel (mpirun, aprun, ...)"
      echo '**********************************************************'
      sleep 999999999
      exit 0
      ;;
      aprun)
      exec aprun $APRUN_FLAGS "$@"
      exit 0
      ;;
      ddt-aprun)
      exec ddt --start --noqueue aprun $APRUN_FLAGS "$@"
      exit 0
      ;;
      ddt-connect-aprun)
      exec ddt --connect aprun $APRUN_FLAGS "$@"
      exit 0
      ;;
   esac
fi

#echo SPAWN "$@"

LOGFILE="$(basename $1)"-$$.log

if [ -n "$4" ]; then
   # include module ID
   LOGFILE="$(basename $1)"-$4-$$.log
fi

echo "spawn_vistle.sh: $@" > "$LOGFILE"
export MV2_ENABLE_AFFINITY=0 # necessary for MPI_THREAD_MULTIPLE support
case $(uname) in
   Darwin)
      libpath=DYLD_LIBRARY_PATH
      export DYLD_LIBRARY_PATH="$VISTLE_DYLD_LIBRARY_PATH"
      ;;
   *)
      libpath=LD_LIBRARY_PATH
      ;;
esac

WRAPPER=""
LAUNCH=""
if [ -n "$SINGULARITY_CONTAINER" ]; then
    if [ -f /.singularity.d/libs/libGLX_nvidia.so.0 ]; then
        WRAPPER="singularity exec --nv ${SINGULARITY_CONTAINER}"
    else
        WRAPPER="singularity exec ${SINGULARITY_CONTAINER}"
    fi
fi
#WRAPPER="$WRAPPER valgrind"
export TSAN_OPTIONS="history_size=7 force_seq_cst_atomics=1 second_deadlock_stack=1"

export MPI_UNBUFFERED_STDIO=1
export MPICH_MAX_THREAD_SAFETY=multiple
export MPICH_VERSION_DISPLAY=1
export KMP_AFFINITY=verbose,none

if [ -n "$PBS_ENVIRONMENT" ]; then
    echo "PBS: mpiexec $@"
    mpiexec -genvall hostname
    mpiexec -genvall printenv
    exec mpirun -genvall ${PREPENDRANK} $LAUNCH $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
    exec mpiexec -genvall \
        "$@"

        #-genv KMP_AFFINITY \
        #-genv KMP_AFFINITY=verbose,none \
        #-genv I_MPI_DEBUG=5 \

fi

if [ -n "$SLURM_JOB_ID" ]; then
   exec mpiexec -bind-to none $WRAPPER "$@"
   #exec srun --overcommit "$@"
   #exec srun --overcommit --cpu_bind=no "$@"
fi

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

if [ "$MPISIZE" != "1" ]; then
   PREPENDRANK=-prepend-rank
   TAGOUTPUT=-tag-output
fi

if [ "$OPENMPI" = "1" ]; then
   if [ -z "$MPIHOSTS" ]; then
      exec mpirun -x ${libpath} $LAUNCH -np ${MPISIZE} $TAGOUTPUT -bind-to none $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   else
      exec mpirun -x ${libpath} $LAUNCH -np ${MPISIZE} $TAGOUTPUT -bind-to none -H ${MPIHOSTS} $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   fi
else
   if [ -z "$MPIHOSTS" ]; then
      exec mpirun -envall ${PREPENDRANK} -np ${MPISIZE} $LAUNCH $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   elif [ "$BIND" = "1" ]; then
      echo exec mpirun -envall -prepend-rank -np ${MPISIZE} -hosts ${MPIHOSTS} -bind-to none $LAUNCH $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
      exec mpirun -envall ${PREPENDRANK} -np ${MPISIZE} -hosts ${MPIHOSTS} -bind-to none $LAUNCH $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   else
      exec mpirun -envall ${PREPENDRANK} -np ${MPISIZE} -hosts ${MPIHOSTS} $LAUNCH $WRAPPER "$@" >> "$LOGFILE" 2>&1 < /dev/null
   fi
fi

echo "default: $@"
exec WRAPPER "$@"
