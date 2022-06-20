
# ReadTsunami
read ChEESE Tsunami files

## Purpose

This module is designed to read [TsunamiHySEA](https://edanya.uma.es/hysea/index.php/models/tsunami-hysea) files. [TsunamiHySEA](https://edanya.uma.es/hysea/index.php/models/tsunami-hysea) is a numerical model for quake generated tsunami simulation. The raw data output of the simulation is a netCDF file which contains the following mandatory attributes:

- **lon**: Longitude array which specifies one dimension of the sea domain

- **lat**: Latitdude array which specifies one dimension of the sea domain

- **grid_lat**: Latitdude array of the topography

- **grid_lon**: Longitude array of the topography

- **time**: Timesteps used for the simulation

- **eta**: Wave amplitude per timestep

## How the reader works

The reader is able to read netCDF files which uses a [PnetCDF](https://parallel-netcdf.github.io/) supported file type. In general the module will fetch the longitude and latitude values for the sea (**lon**, **lat**) and the batheymetry (**grid_lon**, **grid_lat**) from the netCDF file. Based on each longitude-latitude pair the reader creates a 2D grid, varies the wave height (**eta** - orthogonal to longitude and latitude coordinates) per timestep and creates a 3D surface representing the sea surface out of it. If an attribute **bathymetry** is provided with the netCDF file the reader will build a ground surface as well. Additional attributes like scalar data will be mapped onto the sea surface.

---

## Ports

<svg width="80.0em" height="10.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="0.8em" width="8.0em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<text x="0.2em" y="2.6500000000000004em" class="moduleName" >ReadTsunami</text><rect x="0.2em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>Sea surface</title></rect>
<rect x="0.7em" y="3.8em" width="0.03333333333333333em" height="6.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="9.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="9.9em" class="text" >Grid Sea (Heightmap/LayerGrid)<tspan> (Sea surface)</tspan></text>
<rect x="1.4em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>Ground surface</title></rect>
<rect x="1.9em" y="3.8em" width="0.03333333333333333em" height="5.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="1.9em" y="8.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="8.9em" class="text" >Sea floor (Heightmap/LayerGrid)<tspan> (Ground surface)</tspan></text>
<rect x="2.5999999999999996em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>Scalar port 0</title></rect>
<rect x="3.0999999999999996em" y="3.8em" width="0.03333333333333333em" height="4.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="3.0999999999999996em" y="7.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="4.3em" y="7.8999999999999995em" class="text" >Port for scalar number 0<tspan> (Scalar port 0)</tspan></text>
<rect x="3.8em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>Scalar port 1</title></rect>
<rect x="4.3em" y="3.8em" width="0.03333333333333333em" height="3.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="4.3em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="5.5em" y="6.8999999999999995em" class="text" >Port for scalar number 1<tspan> (Scalar port 1)</tspan></text>
<rect x="5.0em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>Scalar port 2</title></rect>
<rect x="5.5em" y="3.8em" width="0.03333333333333333em" height="2.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="5.5em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="6.7em" y="5.8999999999999995em" class="text" >Port for scalar number 2<tspan> (Scalar port 2)</tspan></text>
<rect x="6.2em" y="2.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>Scalar port 3</title></rect>
<rect x="6.7em" y="3.8em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="6.7em" y="4.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="7.9em" y="4.8999999999999995em" class="text" >Port for scalar number 3<tspan> (Scalar port 3)</tspan></text>
</svg>

The first two ports output the geometry for the sea surface and bathymetry as an LayerGrid/Heightmap object. The other ports are providing a vistle scalar object which representing the corresponding in the parameter browser selected attributes.


## Parameters
|name|description|type|
|-|-|-|
|first_step|first timestep to read|Int|
|last_step|last timestep to read (-1: last)|Int|
|step_increment|number of steps to increment|Int|
|first_rank|rank for first partition of first timestep|Int|
|check_convexity|whether to check convexity of grid cells|Int|
|file_dir|NC File directory|String|
|ghost|Show ghostcells.|Int|
|fill|Replace filterValue.|Int|
|VerticalScale|Vertical Scale parameter sea|Float|
|blocks latitude|number of blocks in lat-direction|Int|
|blocks longitude|number of blocks in lon-direction|Int|
|fillValue|ncFile fillValue offset for eta|Float|
|fillValueNew|set new fillValue offset for eta|Float|
|bathymetry |Select bathymetry stored in netCDF)|String|
|Scalar 0|Select scalar.)|String|
|Scalar 1|Select scalar.)|String|
|Scalar 2|Select scalar.)|String|
|Scalar 3|Select scalar.)|String|

### Important to note

This module distributes the data into domain blocks per spawned mpi process. With the parameters **blocks latitude** and **blocks longitude** an user needs to specify the distribution of the simulation domain. The parameter **fill** enables the option to replace in the netCDF file used fillValues for the attribute **eta** with a new fillValue to reduce the height dimension of the sea surface. If **fill** is enabled an user need to specify the current fillValue with the parameter **fillValue** (default fillValue: -9999) along with the replacement with **fillValueNew**. A netCDF file read by this reader which does not contain a bathymetry attribute named with a string containing atleast the substring *"bathy"* won't get noticed as bathymetry data and therefore not providing output data for the second port. In this case the parameter browser will show **None** for the parameter **bathymetry**. Otherwise there will be a selection of bathymetry options available.

## Example Usage

### Simple cases

In the simplest and most cases a visualization of the simulation result can be done like shown in figure 1. 

```{figure} ../../../module/general/ReadTsunami/simple.png
---
align: center
---
Fig 1: Surface pipeline.
```
In this example the second port will be used to visualize the topography. For the third port the additional scalar data **max_height** is selected which will directly mapped onto the sea surface. The following picture shows the result of executing this visualization pipeline.

![](../../../module/general/ReadTsunami/simpleResult.png)

## Related Modules

### Often Used With

- [Color](Color_link.md)
- [MapDrape](MapDrape_link.md)
- [IndexManifolds](IndexManifolds_link.md)
- [Variant](Variant_link.md)

## Build Requirements

- PnetCDF: build with --enable-thread-safe flag as described on [PnetCDF-GitHub](https://github.com/Parallel-NetCDF/PnetCDF) or installed via packagemanager

:::{note}
The pnetcdf install directory should be added to your $PATH environment variable in order to be found by CMake, otherwise it won't be build.
:::

## Acknowledgements

- [TsunamiHySEA](https://github.com/edanya-uma/TsunamiHySEA)
- [ChEESE](https://cheese-coe.eu/)
