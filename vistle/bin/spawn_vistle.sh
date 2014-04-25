#! /bin/bash

#echo mpirun "$@"
exec mpirun -np 1 "$@"
