
# MiniSim
small simulation to compare performance with other in situ interfaces

<svg width="50.599999999999994em" height="6.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="0.8em" width="5.06em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<text x="0.2em" y="2.6500000000000004em" class="moduleName" >MiniSim</text><rect x="0.2em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="0.7em" y="3.8em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="5.8999999999999995em" class="text" >structured grid<tspan> (grid_out)</tspan></text>
<rect x="1.4em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="1.9em" y="3.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="4.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="4.8999999999999995em" class="text" >oscillators<tspan> (data_out)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|input_params|path to file with input parameters|String|
|num_timesteps|maximum number of timesteps to execute|Int|
|numThreads|number of threads used by the sim|Int|
|numGhost|number of ghost cells in each direction|Int|
|numParticles|number of particles|Int|
|numBlocks|number of overall blocks, if zero mpi size is used|Int|
|verbose|print debug messages|Int|
|sync|synchronize after each timestep|Int|
|velocityScale|scale factor to convert function gradient to velocity|Float|
|tEnd|end time|Float|
|dt|time step length|Float|
