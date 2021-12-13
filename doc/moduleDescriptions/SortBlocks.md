
# SortBlocks
Sort data objects according to meta data

## Input ports
|name|description|
|-|-|
|data_in||


## Output ports
|name|description|
|-|-|
|data_out0||
|data_out1||


## Parameters
|name|description|type|
|-|-|-|
|criterion|Selection criterion (Rank, BlockNumber, Timestep)|Integer|
|first_min|Minimum number of MPI rank, block, timestep to output to first output (data_out0)|Integer|
|first_max|Maximum number of MPI rank, block, timestep to output to first output (data_out0)|Integer|
|modulus|Check min/max after computing modulus (-1: disable)|Integer|
|invert|Invert roles of 1st and 2nd output|Integer|
