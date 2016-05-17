#! /bin/bash

echo -n "10.2.*.* " > /etc/ssh/ssh_known_hosts
cat /etc/ssh/ssh_host_rsa_key.pub |cut -f1,2 -d' ' >> /etc/ssh/ssh_known_hosts
