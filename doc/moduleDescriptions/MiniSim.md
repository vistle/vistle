
# MiniSim
Small simulation to compare performance with other in situ interfaces



## Output ports
|name|description|
|-|-|
|grid_out|structured grid|
|data_out|oscillators|


## Parameters
|name|description|type|
|-|-|-|
|Input params|path to file with input parameters|String|
|numTimesteps|maximum number of timesteps to execute|Integer|
|numThreads|number of threads used by the sim|Integer|
|numGhost|number of ghost cells in each direction|Integer|
|numParticles|number of particles|Integer|
|numBlocks|number of overall blocks, if zero mpi size is used|Integer|
|verbose|print debug messages|Integer|
|sync|synchronize after each timestep|Integer|
|velocityScale|scale factor to convert function gradient to velocity|Float|
|tEnd|end time|Float|
|dt|time step length|Float|
