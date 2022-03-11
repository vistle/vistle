[headline]:<>

[SeisSol](https://www.seissol.org/) is an Open Source Software for the numerical simulation of seismic wave phenomena and earthquake dynamics. This Reader has been developed in the ChEESE project to be able to visualize SeisSol generated earthquakes in vistle. The result of the SeisSol simulation is stored in an HDF5 file which will be read by this Reader via [XDMF](https://xdmf.org/index.php/XDMF_Model_and_Format) files. The XDMF files can be generated with the python script [seissolxdmfwriter](https://pypi.org/project/seissolxdmfwriter/) provided by the SeisSol team.

- **lon**: Longitude of the sea domain in WGS84 coordinates

- **lat**: Latitdude of the sea domain in WGS84 coordinates

- **grid_lat**: Latitdude of the topography in WGS84 coordinates

- **grid_lon**: Longitude of the topography in WGS84 coordinates

- **time**: Timesteps used for the simulation

- **eta**: Wave amplitude per timestep

Additional attributes like scalar data will be mapped onto the sea surface.

## How the reader works

The reader is able to read netCDF files which uses a [PnetCDF](https://parallel-netcdf.github.io/) supported file type. In general the module will fetch the longitude and latitude values for the sea (**lon**, **lat**) and the batheymetry (**grid_lon**, **grid_lat**) from the ncfile which has to be specified in the parameter browser. Based on each longitude-latitude pair the reader creates a 2D grid, varies the wave height (**eta** - orthogonal to longitude and latitude coordinates) per timestep and creates a 3D polygon surface representing the sea surface out of it. If an attribute **bathymetry** is provided with the netCDF file the reader will build a ground surface as well. 

---

## Install Requirements

- PnetCDF: build with --enable-thread-safe flag as described on [PnetCDF-GitHub](https://github.com/Parallel-NetCDF/PnetCDF)

The pnetcdf install directory should be added to your $PATH environment variable in order to be found by CMake, otherwise it won't be build.

[moduleHtml]:<>

[outputPorts]:<>

[parameters]:<>

## Example Usage

![](example.png)
