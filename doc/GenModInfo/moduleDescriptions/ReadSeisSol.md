
# ReadSeisSol
Read cheese seismic data files (xdmf/hdf5)



<svg width="516.5999999999999" height="210" >
<rect x="0" y="0" width="516.5999999999999" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>ugrid</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 0</title></rect>
<rect x="182.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 1</title></rect>
<rect x="266.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 2</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">ReadSeisSol</text></svg>

## Output ports
|name|description|
|-|-|
|ugrid|UnstructuredGrid|
|Scalar port 0|Scalar port for attribute 0|
|Scalar port 1|Scalar port for attribute 1|
|Scalar port 2|Scalar port for attribute 2|


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|xfile|Xdmf File|String|
|SeisSolMode|Select file format (XDMF, HDF5)|Integer|
|ParallelMode|Select ParallelMode (Block or Serial (with and without timestepdistribution)) (BLOCKS, SERIAL)|Integer|
|Scalar 0|Select scalar for scalar port.)|String|
|Scalar 1|Select scalar for scalar port.)|String|
|Scalar 2|Select scalar for scalar port.)|String|
|ReuseGrid|Reuses grid from first XdmfGrid specified in Xdmf.|Integer|
