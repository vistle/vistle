
# LibSimController
acquire data from LibSim instrumented simulations

<svg width="102.8em" height="4.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="0.8em" width="10.28em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<text x="0.2em" y="2.6500000000000004em" class="moduleName" >LibSimController</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|path|path to a .sim2 file or directory containing these files|String|
|simulation name|the name of the simulation as used in the filename of the sim2 file |String|
|VTK variables|sort the variable data on the grid from VTK ordering to Vistles|Int|
|constant grids|are the grids the same for every timestep?|Int|
|frequency|frequency in which data is retrieved from the simulation|Int|
|combine grids|combine all structure grids on a rank to a single unstructured grid|Int|
|keep timesteps|keep data of processed timestep of this execution|Int|
