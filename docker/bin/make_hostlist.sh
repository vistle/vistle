#! /bin/bash

if [ -z "$MPISIZE" ]; then
	MPISIZE=1
fi

workers_needed=$(($MPISIZE - 1))

get_workers() {
	getent hosts vistle-workers | cut -f1 -d' '
}

num_workers() {
	get_workers | wc -l
}

have_all_workers() {
	local wanted="$1"
	if [ "${wanted}" -le "$(num_workers)" ]; then
		return 0
	else
		return 1
	fi
}

while ! have_all_workers $workers_needed; do
	sleep 1
done

tail -n 1 /etc/hosts | cut -f1 > /root/.mpi-hostlist
get_workers|head -n $workers_needed >> /root/.mpi-hostlist
