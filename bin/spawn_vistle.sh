#! /usr/bin/env bash

#if [ -z "${OMP_NUM_THREADS}" ]; then
#   APRUN_FLAGS="-j2 --cc none"
#else
#   APRUN_FLAGS="-j2 --cc numa_node -d$OMP_NUM_THREADS"
#fi
#APRUN_FLAGS="-j2 --cc numa_node" # enable hyper-threading and bind processes to NUMA nodes
#APRUN_FLAGS="-j2" # enable hyper-threading

#APRUN_FLAGS="-j2 -cc 0-11,24-35:12-23,36-47" # enable hyper-threading

# environment variables to copy to all ranks
envvars="PATH"
envvars="$envvars PYTHONHOME PYTHONPATH"
envvars="$envvars COVISE_PATH COVISEDIR ARCHSUFFIX COCONFIG COCONFIG_DEBUG"
envvars="$envvars COVCONFIG COVCONFIG_HOST COVCONFIG_CLUSTER COVCONFIG_DEBUG COVCONFIG_IGNORE_ERRORS"
envvars="$envvars VISTLE_KEY VISTLE_SHM_SIZE VISTLE_SHM_PER_RANK VISTLE_AFFINITY"
envvars="$envvars VISTLE_ROOT VISTLE_BUILDTYPE"
envvars="$envvars LD_LIBRARY_PATH LD_PRELOAD"
envvars="$envvars DYLD_LIBRRARY_PATH VISTLE_DYLD_LIBRARY_PATH DYLD_FRAMEWORK_PATH VISTLE_DYLD_FRAMEWORK_PATH"
envvars="$envvars DYLD_FALLBACK_LIBRRARY_PATH VISTLE_DYLD_FALLBACK_LIBRARY_PATH DYLD_FALLBACK_FRAMEWORK_PATH VISTLE_DYLD_FALLBACK_FRAMEWORK_PATH"

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
      exit 1
      ;;
      ddt-aprun)
      exec ddt --start --noqueue aprun $APRUN_FLAGS "$@"
      exit 1
      ;;
      ddt-connect-aprun)
      exec ddt --connect aprun $APRUN_FLAGS "$@"
      exit 1
      ;;
      must)
      d="/var/tmp/${USER}/must/${4:-0}"
      mkdir -p "$d" && cd "$d"
      if which mustrun >/dev/null 2>&1; then
          t:hybridxec mustrun --must:temp "$d" --must:hybrid -np "${MPISIZE:-1}" "$@" || true
      fi
      PATH="/usr/local/must/bin:$PATH" exec mustrun --must:temp "$d" --must:hybrid -np "${MPISIZE:-1}" "$@"
      exit 1
      ;;
   esac
fi

#echo SPAWN "$@"

LOGPREFIX=
mkdir -p "/var/tmp/${USER}" && LOGPREFIX="/var/tmp/${USER}"
LOGFILE="${LOGPREFIX}/$(basename $1)-$$.log"
if [ -n "$5" ]; then
   # include module ID
   LOGFILE="${LOGPREFIX}/$(basename $1)_$5-$$.log"
fi

function doexec() {
    echo "$@"
    echo "$@" >> $LOGFILE
    exec "$@" >> "$LOGFILE" 2>&1 < /dev/null
}

echo "spawn_vistle.sh: $@" > "$LOGFILE"
case $(uname) in
   Darwin)
      export DYLD_LIBRARY_PATH="$VISTLE_DYLD_LIBRARY_PATH"
      export DYLD_FRAMEWORK_PATH="$VISTLE_DYLD_FRAMEWORK_PATH"
      export DYLD_FALLBACK_LIBRARY_PATH="$VISTLE_DYLD_FALLBACK_LIBRARY_PATH"
      export DYLD_FALLBACK_FRAMEWORK_PATH="$VISTLE_DYLD_FALLBACK_FRAMEWORK_PATH"
      ;;
esac

WRAPPER="valgrind --track-origins=yes --read-var-info=yes --read-inline-info=yes --error-limit=no"
WRAPPER="valgrind --max-threads=1024"
WRAPPER=""
LAUNCH=""
export TSAN_OPTIONS="history_size=7 force_seq_cst_atomics=1 second_deadlock_stack=1"

export MPI_UNBUFFERED_STDIO=1
export MPICH_MAX_THREAD_SAFETY=multiple
export MPICH_VERSION_DISPLAY=1
export KMP_AFFINITY=verbose,none
export MV2_ENABLE_AFFINITY=0 # necessary for MPI_THREAD_MULTIPLE support
export MV2_HOMOGENEOUS_CLUSTER=1 # faster start-up

MPI_IMPL="mpich"
 
if mpirun -version 2>&1| grep -q open-mpi\.org; then
   MPI_IMPL="ompi"
   echo "OpenMPI spawn: $@"
   #LAUNCH="--launch-agent $(which orted)"
   export OMPI_MCA_btl_openib_allow_ib=1 # Open MPI 4.0.1 seems to require this
   #export OMPI_MCA_btl_openib_if_include="mlx4_0:1"
   export OMPI_MCA_mpi_abort_delay=1
   #export OMPI_MCA_mpi_yield_when_idle=1
elif mpirun -version 2>&1| grep -q MPT; then
   MPI_IMPL="mpt"
   echo "HPE MPT spawn: $@"
fi

if [ -n "$PBS_ENVIRONMENT" ]; then
   if [ "$MPI_IMPL" = "mpich" ]; then
    
      echo "PBS: mpiexec $@"
      mpiexec -genvall hostname
      mpiexec -genvall printenv
      doexec mpirun -genvall ${PREPENDRANK} $LAUNCH $WRAPPER "$@"
      exec mpiexec -genvall \
         "$@"

         #-genv KMP_AFFINITY \
         #-genv KMP_AFFINITY=verbose,none \
         #-genv I_MPI_DEBUG=5 \
   else
       doexec mpirun  ${PREPENDRANK} $LAUNCH $WRAPPER "$@"
   fi
elif [ -n "$SLURM_JOB_ID" ]; then
   exec mpiexec --bind-to none $WRAPPER "$@"
   #exec srun --overcommit "$@"
   #exec srun --overcommit --cpu_bind=no "$@"
fi

BINDTO=none
case $(hostname) in
   viscluster70)
      if [ -z "$MPIHOSTS" ]; then
         MPIHOSTS=$(echo viscluster70 viscluster{71..79}|sed -e 's/ /,/g')
      fi
      ;;
   viscluster50)
      if [ -z "$MPIHOSTS" ]; then
         MPIHOSTS=$(echo viscluster50 viscluster{51..60}|sed -e 's/ /,/g')
      fi
      ;;
   viscluster80)
      if [ -z "$MPIHOSTS" ]; then
         MPIHOSTS="viscluster80"
      fi
      ;;
   viscluster*)
      if [ -z "$MPIHOSTS" ]; then
         MPIHOSTS=$(echo viscluster{50..60} viscluster{70..80}|sed -e 's/ /,/g')
      fi
      ;;
   vishexa|vishexb)
      if [ -z "$MPIHOSTS" ]; then
         MPIHOSTS="vishexa,vishexb"
      fi
      ;;
   keller|keller.*|buttercup|buttercup.*)
      if [ -z "$MPIHOSTS" ]; then
          if [ -z "$MPISIZE" ]; then
              MPISIZE=8
          fi
         slots=$(((${MPISIZE:=2}+1)/2))
         if [ "$slots" -ge "2" ]; then
             BINDTO=numa
         fi
         MPIHOSTS="keller:$slots,buttercup:$slots"
         #WRAPPER="-mca pml ucx -x UCX_LOG_LEVEL=diag"
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

case "$MPI_IMPL" in
    ompi)
        case "$BINDTO" in
            none|numa)
                BIND="--bind-to $BINDTO"
                ;;
            *)
                BIND="--bind-to none"
                ;;
        esac

        ENVS=""
        for v in $envvars; do
            eval test -z \${${v}+x} || ENVS="$ENVS -x $v"
        done

        if [ -n "$MPIHOSTFILE" ]; then
            doexec mpirun $ENVS $LAUNCH -np ${MPISIZE} $TAGOUTPUT $BIND --hostfile ${MPIHOSTFILE} $WRAPPER "$@"
        elif [ -n "$MPIHOSTS" ]; then
            doexec mpirun $ENVS $LAUNCH -np ${MPISIZE} $TAGOUTPUT $BIND -H ${MPIHOSTS} $WRAPPER "$@"
        else
            doexec mpirun $ENVS $LAUNCH -np ${MPISIZE} $TAGOUTPUT $BIND $WRAPPER "$@"
        fi
        ;;
      mpt)
          doexec mpirun -np ${MPISIZE} $LAUNCH $WRAPPER "$@"
      ;;
    *)
        if [ -n "$MPIHOSTFILE" ]; then	
            doexec mpirun -envall ${PREPENDRANK} -np ${MPISIZE} -f ${MPIHOSTFILE} $LAUNCH $WRAPPER "$@"
        elif [ -n "$MPIHOSTS" ]; then
            doexec mpirun -envall ${PREPENDRANK} -np ${MPISIZE} -hosts ${MPIHOSTS} $LAUNCH $WRAPPER "$@"
        else
            doexec mpirun -envall ${PREPENDRANK} -np ${MPISIZE} $LAUNCH $WRAPPER "$@"
        fi
        ;;
esac

echo "default: $@"
exec $WRAPPER "$@"
