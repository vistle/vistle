# ReadTsunami

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

## Ports

![ReadTsunami](modpic.png)

| Name            | Description                                          | Value         |
|-----------------|------------------------------------------------------|---------------|
| SeaSurface      | Polygon surface which represents the sea.            | Polygons      |
| GroundSurface   | Polygon ground which represents the sea ground.      | Polygons      |
| Scalar Port\<X\>| Scalar values which holds a reference to sea surface.| Vec\<Scalar\> |

## Parameters

TODO: Add here automatic describtions.

## Example Usage

![Example](example.png)
