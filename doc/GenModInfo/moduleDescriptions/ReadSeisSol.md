
# ReadSeisSol
Read cheese seismic data files (xdmf/hdf5)

## Purpose

[SeisSol](https://www.seissol.org/) is an Open Source Software for the numerical simulation of seismic wave phenomena and earthquake dynamics. This reader has been developed to be able to visualize SeisSol generated earthquakes in Vistle. The result of the SeisSol simulation is stored in XDMF/HDF5 files which will be read by this reader via [XDMF](https://xdmf.org/index.php/XDMF_Model_and_Format).

## Data Preparation

### XDMF Adjustments

To be able to read the files and utilizing the Vistle distribution it is needed to resample the intial files. This can be done by using one of the cmd tools called ***seissol_resample*** or ***seissol_resample_xdmf***. 

## Build & Usage of Command-line Tools

### seissol_resample_xdmf

**seissol_resample_xdmf** is a cmd tool to resample the XDMF file to your needs. You need to install all python dependecies mentioned in [Build Requirements](#build-requirements) with the exception of **seissolxdmf** to be able to build this tool (with an more recent os version the python dependecies should already be installed). The build process of this tool will be automatically invoked by CMake while compiling Vistle with the necessary dependecies or can be triggerd via the custom commad ***build_seissol_resample_xdmf***. 

Example for the CMake generator Ninja:

```bash
    ninja build_seissol_resample_xdmf
```

::::{Note}
If ***path/to/build/bin*** has been added to your environment PATH your good to go to use the tool. Otherwise you can invoke it via ***./path/to/build/bin***.
:::{tip}
In the second case defining an alias for your shell which references this executable is a viable option as well.
:::
::::

The tool can be called via shell:

```bash
    seissol_resample_xdmf -h
```

Use ***-h/--help*** to get a hint how to use this tool. The following command shows an usage example:

```bash
    seissol_resample_xdmf /path/to/xmdf_file_without_extension --version 3 --Data partition SRs T_s RT DS
```
In the example the tool will read the XDMF file **xdmf_name_without_extension.xdmf**, fetches the ***--Data*** arguments specified from the file and create a new XDMF file called **xdmf_name_without_extension-resampled.xdmf** at your current directory location specified by the shell. This new file can then be read by this reader module.

### seissol_resample

The usage of this tool and the build process are similar to **seissol_resample_xdmf**. To build it it needs an additional dependency called **seissolxdmf**. With **seissol_resample** the XDMF file and the underlying HDF5 files referenced in the XDMF source file will be resampled. Due to the fact that SeisSol files can get very large which are sometimes to large for local computers it is often needed to extract a subset of the data. For this purpose you can use this tool.

Example:

```bash
    seissol_resample /path/to/xmdf_name_without_extension --add2prefix test --version 3 --Data partition SRs DS --idt 1 2 3
```
The example will extract the timesteps [1-3], the data arguments **partition SRs DS** and add the prefix **test** to the resampled HDF5/XDMF files.

## Ports

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

The first port outputs the raw geometry specified by the **Topology** and **Geometry** sections of the XDMF file as an unstructured grid object. The scalar ports 0-2 will deliver a vistle scalar object which holds one of the attributes specified in the **Attribute** definitions in the XDMF file. These scalar objects itself containing a reference to the geometry object with a cell-based mapping enabled for the attributes.


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

---

## Usage Examples

### Surfaces

The visualization pipeline in the following image shows two **ReadSeisSol** modules which reading different XDMF files. The left pipeline is for the bathymetry of the area while the right one is used to visualize the fault itself. Both pipelines uses the common combination of [DomainSurface](DomainSurface_link.md) and [Color](Color_link.md) to map the scalars (**U** for bathymetry, **u_n** for fault) onto the geometry surfaces. In the end both pipelines will be visualized in [COVER](COVER_link.md). 

<center><img src="../../../../module/general/ReadSeisSol/seissol_example.png" alt="seissol example" title="SeisSol Example" height="300"/></center>

The below images shows how the result of an execution could look like.

<img src="../../../../module/general/ReadSeisSol/seissol_husavik.png" alt="seissol husavik" title="SeisSol Result Husavik" height="300"/>
<img src="../../../../module/general/ReadSeisSol/seissol_husavik_fault1.png" alt="seissol husavik fault 1" title="Husavik Fault 1" height="300"/>
<img src="../../../../module/general/ReadSeisSol/seissol_husavik_fault2.png" alt="seissol husavik fault 2" title="Husavik Fault 2" height="300"/>

### Unstructured Grids

Using the visualization pipeline like mentioned in [Surfaces](#surfaces) for unstructured data will result in a rendering output in [COVER](COVER_link.md) like the following picture.

<img src="../../../../module/general/ReadSeisSol/seissol_unstr.png" alt="seissol sulawesi unstr" title="Sulawesi Unstructured" height="300"/>

Analysis of the internal grid is often more interesting than the outter boundary. With a pipeline like the below one it is possible to only visualize the simulation boundary as a bounding box and add a plane to the scene which represents a slice of the internal unstructured grid with mapped data on it ([CuttingSurface](CuttingSurface_link.md)). Since the outcome of the SeisSol simulation example is cell-based it is needed to convert the scalar data to vertices with the module [CellToVert](CellToVert_link.md).

<center><img src="../../../../module/general/ReadSeisSol/seissol_boundingbox.png" alt="seissol sulawesi unstr boundingbox" title="Sulawesi Unstructured Pipeline" height="300"/></center>

The next image shows the result of an execution in COVER.

<img src="../../../../module/general/ReadSeisSol/seissol_boundingbox_cover.png" alt="seissol sulawesi unstr boundingbox cover" title="Sulawesi Unstructured Cutting Surface" height="300"/>

## Related Modules

### Often Used With

- [Color](Color_link.md)
- [DomainSurface](DomainSurface_link.md)
- [IsoSurface](IsoSurface_link.md)
- [MapDrape](MapDrape_link.md)

## Build Requirements

### Reader

- XDMF: version 3; build from source as shared library or installed via packagemanager ([XDMF](https://xdmf.org/index.php/XDMF_Model_and_Format))

:::{tip}
If you build the XDMF library yourself add the install path to PATH.
:::

### Command-line Tools

- **Python**: version 3.6 or later
- **pyinstaller**: pip install pyinstaller (is often preinstalled on linux and mac os)
- **seissolxdmf**: pip install seissolxdmf

## Acknowledgements

- [SeisSol](https://www.seissol.org/)
- [ChEESE](https://cheese-coe.eu/)
