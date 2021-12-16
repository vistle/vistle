
# SortBlocks
Sort data objects according to meta data

## Input ports
|name|description|
|-|-|
|data_in||


<svg width="475.99999999999994" height="210" >
<rect x="0" y="0" width="475.99999999999994" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">SortBlocks</text></svg>

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