Catalyst II implementation for Vistle
-------------------------------------

usage:

send conduit node with vistle conection details in catalyst_initialize
an example node can be found in vistleCatalystConfig.yaml
under "catalyst/mpi_comm" the mpi communicator can be passed to Vistle.

For a multi process Vistle:
export MPISIZE and MPIHOSTS/MPIHOSTFILE according to the simulation 
start Vistle and the InSituReader module
