SenseiModule
=====================================================
This module is used to connect to a simulation via the SENSEI in-situ interface.
Data from the simulation can only be processed during execution of this module.
Parameters
----------

-frequency:
	the pipeline is processed for every nth simulation cycle.
-combine grids:
	if true, (structured-)grids on each rank are combined to a single unstructured grid. The variable fields will be retrieved accordingly.
-keep timesteps:
	if true, every processed cycle/timestep will be kept for rendering, if false only the current timestep is shown.
-simulation specific commands:
	if we are connected, the imulation sets commands that will be send to the simulation when the status of the according parameter is changed (value does not matter).
	
Communication details
---------------------
-command shm-message-queue:
	exchange information between SENSEI's Vistle-analysis-adapter and the module
-recvFromSim shm-message-queue:
	we receive vistle messages from the simulation and pass them to the manager as if the came from this module. Mainly used to add vistle objects to the pipeline.
-SyncShmIDs:
	The shared memory IDs for vistle objects have to be synchrinized between the moule and the Engine. This is done with a shm array that holds the necessary values. 
	The module can only create objects outside of execute(before prepare and after prepareReduce). The Engine must only create module during execution of the module.
	In preparation of a simulation crash the Engine must send the new shm counter directly afterwards to reduce the risk of getting out of sync with the module. 
