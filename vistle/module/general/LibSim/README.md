LibSimController
=====================================================
This module is used to connect to a simulation instrumented with VisIt's LibSim in-situ interface.
Data from the simulation can only be processed during execution of this module.
Parameters
----------
-path: 
	Setting this to the .sim2 file produced by the simulation will connect the module with the simulation.
	Set to a directory that contains .sim2 files to try to connect to the newest. If "simulation name" parameter is set the newset ."simulation name".sim2 file will be used.
-simulation name:
	if "path" parameter points to a directory, the module uses this name to find the newest ."simulation name".sim2 file of this simulation.  
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
	
Communication details
---------------------
-connection socket (only rank 0):
	The information from the .sim2 file is used to send a tcp message to the simulations listening socket. 
	This will let the simulation link to our fake libsimV2runntime library (Engine).This message also contains information about this module,
	which is used to connect the simulation to the shared memory segment and the controll socket of the module. After the message the socket is closed.
-controll socket (only rank 0).
	The simulation connects to this socket after starting the Engine. Messages received from this socket are mpi broadcasted to all slaves and are handled parallely.
	Messages exchanged over this connectins include:
		SetPorts, AddCommands, Commands, ConnectionClosed
-recvFromSim shm-message-queue:
	we receive vistle messages from the simulation and pass them to the manager as if the came from this module. Mainly used to add vistle objects to the pipeline.
-SyncShmIDs:
	The shared memory IDs for vistle objects have to be synchrinized between the moule and the Engine. This is done with a shm array that holds the necessary values. 
	The module can only create objects outside of execue(before prepare and after prepareReduce). The Engine must only create module during execution of the module.
	In preparation of a simulation crash the Engine must send the new shm counter directly afterwards to reduce the risk of getting out of sync with the module. 