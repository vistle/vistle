
# MiniSim
Small simulation to compare performance with other in situ interfaces



<svg width="151.79999999999998" height="90" >
<rect x="0" y="0" width="151.79999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">MiniSim</text></svg>

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
