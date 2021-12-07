SenseiModule
=====================================================
This module is used to connect to a simulation via the SENSEI in-situ interface.

Parameters
----------
-path
	path to a connection file written by the simulation (default value should be sufficent)
-frequency:
	the pipeline is processed for every nth simulation cycle.
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
