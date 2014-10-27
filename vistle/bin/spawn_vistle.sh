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
      exec mpirun -np ${MPISIZE} -hosts ${MPIHOSTS} "$@"
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
