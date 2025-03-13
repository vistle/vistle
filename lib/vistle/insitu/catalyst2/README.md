Catalyst II adapter for Vistle
=====================================

usage:

In the simulation code send conduit node with vistle conection details in catalyst_initialize.
An example node can be found in vistleCatalystConfig.yaml.
With the "catalyst/mpi_comm" node the mpi communicator can be passed to Vistle.

For a multi process Vistle:
Export MPISIZE and MPIHOSTS/MPIHOSTFILE in an extra shell according to the simulation,
start Vistle and the [InSituModule]() module. The module should automatically connect and create output ports for the data fields the simulation provides.
