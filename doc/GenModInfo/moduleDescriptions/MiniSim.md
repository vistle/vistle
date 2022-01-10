
# MiniSim
Small simulation to compare performance with other in situ interfaces

<svg width="1517.9999999999998" height="210" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="151.79999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="21.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="183.0" class="text" >structured grid<tspan> (grid_out)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="57.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="153.0" class="text" >oscillators<tspan> (data_out)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >MiniSim</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|Input params|path to file with input parameters|String|
|numTimesteps|maximum number of timesteps to execute|Int|
|numThreads|number of threads used by the sim|Int|
|numGhost|number of ghost cells in each direction|Int|
|numParticles|number of particles|Int|
|numBlocks|number of overall blocks, if zero mpi size is used|Int|
|verbose|print debug messages|Int|
|sync|synchronize after each timestep|Int|
|velocityScale|scale factor to convert function gradient to velocity|Float|
|tEnd|end time|Float|
|dt|time step length|Float|
