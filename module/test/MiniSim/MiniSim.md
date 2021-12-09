[headline]:<>
MiniSim - small simulation to compare performance with other in situ interfaces
===============================================================================
[headline]:<>
[inputPorts]:<>
[inputPorts]:<>
[outputPorts]:<>
Output ports
------------
|name|description|
|-|-|
|grid_out|structured grid|
|data_out|oscillators|


[outputPorts]:<>
[parameters]:<>
Parameters
----------
|name|type|description|
|-|-|-|
|Input params|String|path to file with input parameters|
|numTimesteps|Int|maximum number of timesteps to execute|
|numThreads|Int|number of threads used by the sim|
|numGhost|Int|number of ghost cells in each direction|
|numParticles|Int|number of particles|
|numBlocks|Int|number of overall blocks, if zero mpi size is used|
|verbose|Int|print debug messages|
|sync|Int|synchronize after each timestep|
|velocityScale|Float|scale factor to convert function gradient to velocity|
|tEnd|Float|end time|
|dt|Float|time step length|

[parameters]:<>
