
# SenseiController
Aquire data from sensei instrumented simulations



<svg width="308.4" height="90" >
<rect x="0" y="0" width="308.4" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<text x="6.0" y="54.0" font-size="1.7999999999999998em">SenseiController</text></svg>


This module is used to connect to a simulation via the SENSEI in-situ interface.
Depending on the connected ports and the frequency parameter the module requests data from the simulation.
Once the simulation generates the requested data the module is automatically executed.


## Parameters
|name|description|type|
|-|-|-|
|path|path to the connection file written by the simulation|String|
|deleteShm|clean the shared memory message queues used to communicate with the simulation|Integer|
|frequency|the pipeline is processed for every nth simulation cycle|Integer|
|keep timesteps|if true timesteps are cached and processed as time series|Integer|


simulation specific commands
----------------------------
connected simulations might provide additional commands that are (for now) represented by parameters. Changing one of these parameters triggers the displayed action in the simulation, the actual value of these parameters is meaningless.

Oscillator Example (multi-process vistle)
------------------------------------------
- checkout git@github.com:hpcdgrie/sensei.git and build SENSEI with the ENABLE_VISTLE CMake flag
- start the simulation: *path to SENSEI build*/bin/oscillator -f ../../configs/oscillatorVistle.xml ../../miniapps/oscillators/inputs/sample.osc
- start vistle and load oscillatorExample.vsl

![](../../../module/general/Sensei/OscillatorNet.png)

- click on the run/paused parameter of this module to start/pause the simulation. The rusults should look like the following image:

![](../../../module/general/Sensei/OscillatorResult.png)
