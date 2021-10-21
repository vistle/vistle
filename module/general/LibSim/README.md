LibSimController
=====================================================
This module is used to connect to a simulation instrumented with VisIt's LibSim in-situ interface.
Data from the simulation can only be processed during execution of this module.
Parameters
----------
-path (muli-procees only): 
	Setting this to the .sim2 file produced by the simulation will connect the module with the simulation.
	Set to a directory that contains .sim2 files to try to connect to the newest. If "simulation name" parameter is set the newset ."simulation name".sim2 file will be used.
-simulation name (muli-procees only):
	if "path" parameter points to a directory, the module uses this name to find the newest timestamp."simulation name".sim2 file of this simulation.  
-VTKVariable:
	if this option is set, the variable fields will be reordered to make VTK style data fit on Vistle's grid representation.
-constant grids:
	if true, the grids will only be retrieved from the simulation once and are then kept for the following iterations/timesteps
-frequency:
	the pipeline is processed for every nth simulation cycle.
-combine grids:
	if true, (structured-)grids on each rank are combined to a single unstructured grid. The variable fields will be retrieved accordingly.
-keep timesteps:
	if true, every processed cycle/timestep will be kept for rendering, if false only the current timestep is shown.
-simulation specific commands:
	if we are connected, the imulation sets commands that will be send to the simulation when the status of the according parameter is changed (value does not matter).
	
