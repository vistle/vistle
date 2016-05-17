#! /bin/bash

ipaddr=$(tail -n 1 /etc/hosts | cut -f1)

mpirun -localhost $ipaddr  -f /root/.mpi-hostlist "$@"
