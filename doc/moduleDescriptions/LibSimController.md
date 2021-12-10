
# LibSimController
Acquire data from LibSim instrumented simulations



## Parameters
|name|type|description|
|-|-|-|
|path|String|path to a .sim2 file or directory containing these files|
|simulation name|String|the name of the simulation as used in the filename of the sim2 file |
|VTKVariables|Int|sort the variable data on the grid from VTK ordering to Vistles|
|constant grids|Int|are the grids the same for every timestep?|
|frequency|Int|frequency in which data is retrieved from the simulation|
|combine grids|Int|combine all structure grids on a rank to a single unstructured grid|
|keep timesteps|Int|keep data of processed timestep of this execution|
