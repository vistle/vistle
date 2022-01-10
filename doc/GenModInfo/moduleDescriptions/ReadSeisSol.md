
# ReadSeisSol
Read cheese seismic data files (xdmf/hdf5)

<svg width="2214.0" height="270" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="30" width="221.39999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>ugrid</title></rect>
<rect x="21.0" y="120" width="1.0" height="120" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="240" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="243.0" class="text" >UnstructuredGrid<tspan> (ugrid)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 0</title></rect>
<rect x="57.0" y="120" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="213.0" class="text" >Scalar port for attribute 0<tspan> (Scalar port 0)</tspan></text>
<rect x="78.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 1</title></rect>
<rect x="93.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="183.0" class="text" >Scalar port for attribute 1<tspan> (Scalar port 1)</tspan></text>
<rect x="114.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 2</title></rect>
<rect x="129.0" y="120" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="129.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="165.0" y="153.0" class="text" >Scalar port for attribute 2<tspan> (Scalar port 2)</tspan></text>
<text x="6.0" y="85.5" class="moduleName" >ReadSeisSol</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Int|
|last_step|last timestep to read (-1: last)|Int|
|step_increment|number of steps to increment|Int|
|first_rank|rank for first partition of first timestep|Int|
|check_convexity|whether to check convexity of grid cells|Int|
|xfile|Xdmf File|String|
|SeisSolMode|Select file format (XDMF, HDF5)|Int|
|ParallelMode|Select ParallelMode (Block or Serial (with and without timestepdistribution)) (BLOCKS, SERIAL)|Int|
|Scalar 0|Select scalar for scalar port.)|String|
|Scalar 1|Select scalar for scalar port.)|String|
|Scalar 2|Select scalar for scalar port.)|String|
|ReuseGrid|Reuses grid from first XdmfGrid specified in Xdmf.|Int|
