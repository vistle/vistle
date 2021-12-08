SenseiController
================
acquire data from SENSEI instrumented simulations

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
