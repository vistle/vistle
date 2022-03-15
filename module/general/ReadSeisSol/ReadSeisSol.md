[headline]:<>

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

[moduleHtml]:<>

[outputPorts]:<>

[parameters]:<>

## Example Usage
