#! /bin/bash

export MPISIZE=4
export VISTLE_HOSTNAME=$(tail -n 1 /etc/hosts | cut -f1)

/root/bin/make_ssh_known_hosts.sh
#/root/bin/make_hostlist.sh

exec /usr/sbin/sshd -D -e
exec /usr/bin/vistle -b
