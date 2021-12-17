
# LibSimController
Acquire data from libsim instrumented simulations

<svg width="3084.0" height="150" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="308.4" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<text x="6.0" y="85.5" class="moduleName" >LibSimController</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|path|path to a .sim2 file or directory containing these files|String|
|simulation name|the name of the simulation as used in the filename of the sim2 file |String|
|VTKVariables|sort the variable data on the grid from VTK ordering to Vistles|Integer|
|constant grids|are the grids the same for every timestep?|Integer|
|frequency|frequency in which data is retrieved from the simulation|Integer|
|combine grids|combine all structure grids on a rank to a single unstructured grid|Integer|
|keep timesteps|keep data of processed timestep of this execution|Integer|
