[headline]:<>

## Introduction

[SeisSol](https://www.seissol.org/) is an Open Source Software for the numerical simulation of seismic wave phenomena and earthquake dynamics. This Reader has been developed in the [ChEESE](https://cheese-coe.eu/https://cheese-coe.eu/) project to be able to visualize SeisSol generated earthquakes in Vistle. The result of the SeisSol simulation can be stored in XDMF/HDF5 or XDMF/Binary files which will be read by this Reader via [XDMF](https://xdmf.org/index.php/XDMF_Model_and_Format). The XDMF files can be generated or readjusted with the python scripts [seissolxdmfwriter](https://pypi.org/project/seissolxdmfwriter/) and [seissolxdmf](https://pypi.org/project/seissolxdmf/) provided by the SeisSol team.

## Preconditions

### Install Requirements

- [XDMF](https://xdmf.org/index.php/XDMF_Model_and_Format) 3: build from source or installed via packagemanager

### XDMF-File Adjustments

To be able to read the files and utilizing the vistle distribution it is needed to resample the intial XDMF files. For this purpose

## How the reader works

The reader is able to read netCDF files which uses a [PnetCDF](https://parallel-netcdf.github.io/) supported file type. In general the module will fetch the longitude and latitude values for the sea (**lon**, **lat**) and the batheymetry (**grid_lon**, **grid_lat**) from the ncfile which has to be specified in the parameter browser. Based on each longitude-latitude pair the reader creates a 2D grid, varies the wave height (**eta** - orthogonal to longitude and latitude coordinates) per timestep and creates a 3D polygon surface representing the sea surface out of it. If an attribute **bathymetry** is provided with the netCDF file the reader will build a ground surface as well. 

---

[moduleHtml]:<>

[outputPorts]:<>

[parameters]:<>

## Example Usage

![](example.png)
