
# ReadSeisSol
Read ChEESE seismic data files (XDMF/HDF5)



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
