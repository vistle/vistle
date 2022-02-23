
# ReadMPAS
Read mpas files

<svg width="1800" height="270" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="180" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="21.0" y="120" width="1.0" height="120" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="240" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="243.0" class="text" >grid<tspan> (grid_out)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_0</title></rect>
<rect x="57.0" y="120" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="213.0" class="text" >scalar data<tspan> (data_out_0)</tspan></text>
<rect x="78.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_1</title></rect>
<rect x="93.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="183.0" class="text" >scalar data<tspan> (data_out_1)</tspan></text>
<rect x="114.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out_2</title></rect>
<rect x="129.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="129.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="165.0" y="153.0" class="text" >scalar data<tspan> (data_out_2)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >ReadMPAS</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Int|
|last_step|last timestep to read (-1: last)|Int|
|step_increment|number of steps to increment|Int|
|first_rank|rank for first partition of first timestep|Int|
|check_convexity|whether to check convexity of grid cells|Int|
|grid_file|File containing the grid|String|
|data_file|File containing data|String|
|zGrid_file_dir|File containing the vertical coordinates (elevation of cell from mean sea level|String|
|part_file|File containing the grid partitions|String|
|numParts|Number of Partitions|Int|
|numLevels|Number of vertical levels to read|Int|
|cellsOnCell|List of neighboring cells (for ghosts))|String|
|var_dim|Dimension of variables (2D, 3D, other)|String|
|Variable_0|Variable_0 ((NONE))|String|
|Variable_1|Variable_1 ((NONE))|String|
|Variable_2|Variable_2 ((NONE))|String|
