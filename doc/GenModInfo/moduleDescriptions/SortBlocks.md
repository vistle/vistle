
# SortBlocks
sort data objects according to meta data

<svg width="68.0em" height="7.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="1.8em" width="6.8em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="1.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="0.7em" y="0.8em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="0.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="0.9em" class="text" ><tspan> (data_in)</tspan></text>
<text x="0.2em" y="3.6500000000000004em" class="moduleName" >SortBlocks</text><rect x="0.2em" y="3.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="0.7em" y="4.8em" width="0.03333333333333333em" height="2.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="6.8999999999999995em" class="text" ><tspan> (data_out0)</tspan></text>
<rect x="1.4em" y="3.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="1.9em" y="4.8em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="1.9em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="5.8999999999999995em" class="text" ><tspan> (data_out1)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|criterion|Selection criterion (Rank, BlockNumber, Timestep)|Int|
|first_min|Minimum number of MPI rank, block, timestep to output to first output (data_out0)|Int|
|first_max|Maximum number of MPI rank, block, timestep to output to first output (data_out0)|Int|
|modulus|Check min/max after computing modulus (-1: disable)|Int|
|invert|Invert roles of 1st and 2nd output|Int|
