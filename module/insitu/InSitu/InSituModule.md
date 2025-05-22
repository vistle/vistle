[headline]:<>

## Purpose
This module is used to connect to a simulation via the Catalyst II or SENSEI in-situ interface.

## Ports

Output ports are created dynamically, based on information from the coupling definition and simulation.
[moduleHtml]:<>

[parameters]:<>

Set the `path` parameter to a connection file written by the simulation. The default value should be sufficient.

The value of `frequency` determines how often the pipeline is processed. If set to 1, the pipeline is processed every cycle. Setting it to a value of `n` will skip `n-1` cycles and process the pipeline every `n`th cycle.

The `keep_timesteps` flag controls whether the data from all timesteps is retained for rendering or if only the current/newest timestep can be displayed.

FIXME
-simulation specific commands:
	if we are connected, the simulation sets commands that will be send to the simulation when the status of the according parameter is changed (value does not matter).
	
Communication details
---------------------
-command shm-message-queue:
	exchange information between SENSEI's Vistle-analysis-adapter and the module
-recvFromSim shm-message-queue:
	we receive vistle messages from the simulation and pass them to the manager as if the came from this module. Mainly used to add vistle objects to the pipeline.
