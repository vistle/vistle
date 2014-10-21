#! /bin/bash

echo SPAWN "$@"

case $(hostname) in
   viscluster*)
      if [ -z "$MPIHOSTS" ]; then
         MPIHOSTS=$(echo viscluster{50..60} viscluster{71..75}|sed -e 's/ /,/g')
      fi
      if [ "$MPISIZE" = "" ]; then
         MPISIZE=16
      fi
      #echo LD_LIBRARY_PATH=$LD_LIBRARY_PATH
      export MV2_ENABLE_AFFINITY=0
      exec mpirun -np ${MPISIZE} -hosts ${MPIHOSTS} -bind-to none -envlist LD_LIBRARY_PATH "$@"
      ;;
   *)
      #echo mpirun "$@"
      #exec mpirun -np 1 "$@"
      #echo EXEC: "$@"
      #exec mpirun -np 2 "$@"
      #exec xterm -e gdb --args "$@"
      if [ "$MPISIZE" = "" ]; then
         MPISIZE=1
      fi
      if [ -z "$MPIHOSTS" ]; then
         echo mpirun -np ${MPISIZE} "$@"
         exec mpirun -np ${MPISIZE} "$@"
      else
         echo mpirun -np ${MPISIZE} -hosts ${MPIHOSTS} "$@"
         exec mpirun -np ${MPISIZE} -hosts ${MPIHOSTS} "$@"
      fi
      ;;
esac

exec "$@"
