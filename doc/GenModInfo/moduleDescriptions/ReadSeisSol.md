
# ReadSeisSol
Read cheese seismic data files (xdmf/hdf5)

## Introduction

[SeisSol](https://www.seissol.org/) is an Open Source Software for the numerical simulation of seismic wave phenomena and earthquake dynamics. This Reader has been developed in the [ChEESE](https://cheese-coe.eu/https://cheese-coe.eu/) project to be able to visualize SeisSol generated earthquakes in Vistle. The result of the SeisSol simulation is stored in XDMF/HDF5 or XDMF/Binary files which will be read by this Reader in both cases via [XDMF](https://xdmf.org/index.php/XDMF_Model_and_Format).

## Preconditions

### Requirements

#### Reader

- XDMF: version 3; build from source as shared library or installed via packagemanager ([XDMF](https://xdmf.org/index.php/XDMF_Model_and_Format))

If you build the xdmf library yourself add the install path to PATH.

#### Command-line Tools

- **Python**: version 3.6 or later
- **pyinstaller**: pip install pyinstaller (is often preinstalled on linux and mac os)
- **seissolxdmf**: pip install seissolxdmf

### XDMF Adjustments

To be able to read the files and utilizing the vistle distribution it is needed to resample the intial files. This can be done by using one of the cmd tools called ***seissol_resample*** or ***seissol_resample_xdmf***. 

---

## Build & Usage of Command-line Tools

### seissol_resample_xdmf

**seissol_resample_xdmf** is a cmd tool to resample the xdmf file to your needs. You need to install all [python dependecies](#seissol_resample_xdmf) with the exception of **seissolxdmf** to be able to build this tool (with an more recent os version the python dependecies should already be installed). The build process of this tool will automatically invoked by cmake while compiling vistle with the necessary dependecies or can be triggerd via the custom commad ***build_seissol_resample_xdmf***. Example for the cmake generater Ninja:

```bash
    ninja build_seissol_resample_xdmf
```

If ***path/to/build/bin*** has been added to PATH the tool can be called via shell:

```bash
    seissol_resample_xdmf -h
```

Use ***-h/--help*** to get a hint how to use this tool. The following command shows an usage example:

```bash
    seissol_resample_xdmf /data/ChEESE/SeisSol/xmdf_name_without_extension --version 3 --Data partition SRs T_s RT DS
```

In the example the tool will read the xdmf file **xdmf_name_without_extension.xdmf**, fetches the ***--Data*** arguments from the file and create a new xdmf file called **xdmf_name_without_extension-resampled.xdmf** at your current directory location specified by the shell. This file format can then be read by this reader module.

### seissol_resample

The usage of this tool and the build process are similar to **seissol_resample_xdmf**. To build it it needs an additional dependency called **seissolxdmf**. With **seissol_resample** the xdmf file and the underlying HDF5/Binary files referenced in the xdmf source file will be resampled. Due to the fact that SeisSol files can get very large which are sometimes to large for local computers. This tool allows to extract only a subset of the original file.

Example usage:

```bash
    seissol_resample /data/ChEESE/SeisSol/xmdf_name_without_extension --add2prefix test --version 3 --Data partition SRs DS --idt 1 2 3
```

The example will extract only the timesteps [1-3], the data arguments **partition SRs DS** and add the prefix **test** to the resampled HDF5/Binary/XDMF files.

---

## Usage


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

[outputPorts]:<>


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

## Example Usage
