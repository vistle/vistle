# Getting Up and Running

- get (Dropbear)[https://matt.ucc.asn.au/dropbear/dropbear.html] SSH tools from (GitHub)[https://github.com/mkj/dropbear.git]
- build
    - `autoconf`
    - `autoheader`
    - `./configure --disable-lastlog --disable-utmp --disable-utmpx --disable-wtmp --disable-wtmpx --disable-syslog --disable-loginfunc --disable-pututline --disable-pututxline --disable-pam`
- move the `dropbear` binary somewhere into your `$PATH`
- generate keys
    `./dropbearkey -t rsa -f ~/.ssh/vistle_dropbear_rsa`
    `./dropbearkey -t dss -f ~/.ssh/vistle_dropbear_dss`
    `./dropbearkey -t ecdsa -f ~/.ssh/vistle_dropbear_ecdsa`
- launch the Dropbear, keep default settings
- execute the Dropbear
- `ssh -p 31222 somempinode`
- `killall dropbear` on all MPI nodes
