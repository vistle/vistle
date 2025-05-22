[headline]:<>

## Purpose
This module is used to connect to a simulation instrumented with VisIt's LibSim in-situ interface.
Data from the simulation can only be processed during execution of this module.

## Ports

Output ports are created dynamically, based on information from the coupling definition and simulation.
[moduleHtml]:<>

[parameters]:<>

In multi-process mode, the path to the `.sim2` file provided by the simulation to connect to is required.
Set the `path` parameter to a directory containing `.sim2` files and the newest such file will be used.
If the `simulation_name` parameter is set, the newest *simulation_name*`.sim2` file will be used.

If the `vtk_variables` flag is set, the variable fields will be reordered to make VTK style data fit on Vistle's grid representation.

Setting the `constant_grids` flag will retrieve the grids from the simulation only once and keep them for the following iterations/timesteps.

The value of `frequency` determines how often the pipeline is processed. If set to 1, the pipeline is processed every cycle. Setting it to a value of `n` will skip `n-1` cycles and process the pipeline every `n`th cycle.

The `combine_grids` flag will combine the (structured) grids from all MPI ranks into a single unstructured grid. The variable fields will be retrieved accordingly.

The `keep_timesteps` flag controls whether the data from all timesteps is retained for rendering or if only the current/newest timestep can be displayed.

If the coupling module is connected, `simulation_specific_commands`
FIXME

-simulation specific commands:
	if we are connected, the simulation sets commands that will be send to the simulation when the status of the according parameter is changed (value does not matter).
