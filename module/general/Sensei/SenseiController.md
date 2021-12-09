[headline]:<>
SenseiController - acquire data from SENSEI instrumented simulations
====================================================================
[headline]:<>

This module is used to connect to a simulation via the SENSEI in-situ interface.
Depending on the connected ports and the frequency parameter the module requests data from the simulation.
Once the simulation generates the requested data the module is automatically executed.
blablabla

[inputPorts]:<>
[inputPorts]:<>
[outputPorts]:<>
[outputPorts]:<>
[parameters]:<>
Parameters
----------
|name|type|description|
|-|-|-|
|first_step|Int|first timestep to read|
|last_step|Int|last timestep to read (-1: last)|
|step_increment|Int|number of steps to increment|
|first_rank|Int|rank for first partition of first timestep|
|check_convexity|Int|whether to check convexity of grid cells|
|path|String|path to a .vistle file|
|deleteShm|Int|delete the shm potentially used for communication with sensei|
|frequency|Int|frequency in which data is retrieved from the simulation|
|keep timesteps|Int|keep data of processed timestep during this execution|

[parameters]:<>

simulation specific commands
----------------------------
connected simulations might provide additional commands that are (for now) represented by parameters. Changing one of these parameters triggers the displayed action in the simulation, the actual value of these parameters is meaningless.

Oscillator Example (multi-process vistle)
------------------------------------------
- checkout git@github.com:hpcdgrie/sensei.git and build SENSEI with the ENABLE_VISTLE CMake flag
- start the simulation: *path to SENSEI build*/bin/oscillator -f ../../configs/oscillatorVistle.xml ../../miniapps/oscillators/inputs/sample.osc
- start vistle and load oscillatorExample.vsl

![](OscillatorNet.png)

- click on the run/paused parameter of this module to start/pause the simulation. The rusults should look like the following image:

![](OscillatorResult.png)
