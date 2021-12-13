
# LibSimController
Acquire data from LibSim instrumented simulations





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
