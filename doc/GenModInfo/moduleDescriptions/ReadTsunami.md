
# ReadTsunami
Read cheese tsunami files



<svg width="516.5999999999999" height="210" >
<rect x="0" y="0" width="516.5999999999999" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Sea surface</title></rect>
<rect x="98.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Ground surface</title></rect>
<rect x="182.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 0</title></rect>
<rect x="266.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 1</title></rect>
<rect x="350.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 2</title></rect>
<rect x="434.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>Scalar port 3</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">ReadTsunami</text></svg>


This module is designed to read ChEESE tsunami files a new file format for probabilistic tsunami forecasting. Data can be generated with [Tsunami-HYSEA](https://edanya.uma.es/hysea/index.php/models/tsunami-hysea) a numerical model for quake generated tsunami simulation. The raw data output of the simulation is a netCDF file which contains the following mandatory attributes:


- **lon**: Longitude of the sea domain in WGS84 coordinates

- **lat**: Latitdude of the sea domain in WGS84 coordinates

- **grid_lat**: Latitdude of the topography in WGS84 coordinates

- **grid_lon**: Longitude of the topography in WGS84 coordinates

- **time**: Timesteps used for the simulation

- **eta**: Wave amplitude per timestep

Additional attributes like scalar data will be mapped onto the sea surface.

---

## Install Requirements

- PnetCDF: build with --enable-thread-safe flag as described on [PnetCDF-GitHub](https://github.com/Parallel-NetCDF/PnetCDF)

The pnetcdf install directory should be added to your $PATH environment variable in order to be found by CMake, otherwise it won't be build.

![](../../../module/general/ReadTsunami/modpic.png)


## Output ports
|name|description|
|-|-|
|Sea surface|2D Grid Sea (Polygons)|
|Ground surface|2D Sea floor (Polygons)|
|Scalar port 0|Port for scalar number 0|
|Scalar port 1|Port for scalar number 1|
|Scalar port 2|Port for scalar number 2|
|Scalar port 3|Port for scalar number 3|




## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Integer|
|last_step|last timestep to read (-1: last)|Integer|
|step_increment|number of steps to increment|Integer|
|first_rank|rank for first partition of first timestep|Integer|
|check_convexity|whether to check convexity of grid cells|Integer|
|file_dir|NC File directory|String|
|ghost|Show ghostcells.|Integer|
|fill|Replace filterValue.|Integer|
|VerticalScale|Vertical Scale parameter sea|Float|
|blocks latitude|number of blocks in lat-direction|Integer|
|blocks longitude|number of blocks in lon-direction|Integer|
|fillValue|ncFile fillValue offset for eta|Float|
|fillValueNew|set new fillValue offset for eta|Float|
|bathymetry |Select bathymetry stored in netCDF)|String|
|Scalar 0|Select scalar.)|String|
|Scalar 1|Select scalar.)|String|
|Scalar 2|Select scalar.)|String|
|Scalar 3|Select scalar.)|String|


## Example Usage

![](../../../module/general/ReadTsunami/example.png)
