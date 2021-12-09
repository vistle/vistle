[headline]:<>
SortBlocks - sort data objects according to meta data
=====================================================
[headline]:<>
[inputPorts]:<>
Input ports
-----------
|name|description|
|-|-|
|data_in||


[inputPorts]:<>
[outputPorts]:<>
Output ports
------------
|name|description|
|-|-|
|data_out0||
|data_out1||


[outputPorts]:<>
[parameters]:<>
Parameters
----------
|name|type|description|
|-|-|-|
|criterion|Int|Selection criterion|
|first_min|Int|Minimum number of MPI rank, block, timestep to output to first output (data_out0)|
|first_max|Int|Maximum number of MPI rank, block, timestep to output to first output (data_out0)|
|modulus|Int|Check min/max after computing modulus (-1: disable)|
|invert|Int|Invert roles of 1st and 2nd output|

[parameters]:<>
