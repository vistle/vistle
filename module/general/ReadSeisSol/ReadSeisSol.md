[headline]:<>

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

:::{Note}
If ***path/to/build/bin*** has been added to your environment PATH your good to go to use the tool. Otherwise you can invoke it via ***./path/to/build/bin***.
:::

:::{tip}
In the second case defining an alias for your shell which references this executable is a viable option as well.
:::

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
[moduleHtml]:<>

The first port outputs the raw geometry specified by the **Topology** and **Geometry** sections of the XDMF file as an unstructured grid object. The scalar ports 0-2 will deliver a vistle scalar object which holds one of the attributes specified in the **Attribute** definitions in the XDMF file. These scalar objects itself containing a reference to the geometry object with a cell-based mapping enabled for the attributes.

[parameters]:<>

---

## Usage Examples

### Surfaces

The visualization pipeline in figure 1 shows two **ReadSeisSol** modules which reading different XDMF files. The left pipeline is for the bathymetry of the area while the right one is used to visualize the fault itself. Both pipelines uses the common combination of [DomainSurface](DomainSurface_link.md) and [Color](Color_link.md) to map the scalars (**U** for bathymetry, **u_n** for fault) onto the geometry surfaces. In the end both pipelines will be visualized in [COVER](COVER_link.md). 

```{figure} seissol_example.png
---
align: center
---
Fig 1: Surface pipeline.`
```

The below images shows how the result of an execution could look like.

![](seissol_husavik.png)

![](seissol_husavik_fault1.png)

![](seissol_husavik_fault2.png)

### Unstructured Grids

Using the visualization pipeline like mentioned in [Surfaces](#surfaces) for unstructured data will result in a rendering output in [COVER](COVER_link.md) like the following picture.

![](seissol_unstr.png)

Analysis of the internal grid is often more interesting than the outter boundary. With a pipeline like shown in figure 2 it is possible to only visualize the simulation boundary as a bounding box and add a plane to the scene which represents a slice of the internal unstructured grid with mapped data on it ([CuttingSurface](CuttingSurface_link.md)). Since the outcome of the SeisSol simulation example is cell-based it is needed to convert the scalar data to vertices with the module [CellToVert](CellToVert_link.md).

```{figure} seissol_boundingbox.png
---
align: center
---
Fig 2: Unstructured grid pipeline.
```

The next image shows the result of an execution in COVER.

![](seissol_boundingbox_cover.png)

## Related Modules

### Often Used With

- [Color](Color_link.md)
- [DomainSurface](DomainSurface_link.md)
- [IsoSurface](IsoSurface_link.md)
- [MapDrape](MapDrape_link.md)

## Troubleshooting

### Inverted Normals

Sometimes it can happen that you have datasets with inverted normals which looks like shown in the next image.

![](seissol_inverted_normal.png)

If you are certain that the scalar data seems correct you can fix the geometry with a visualization pipeline like shown in figure 3 which builds a new geometry with correct normals.

```{figure} seissol_inverted_normal_prevention.png
---
align: center
---
Fig 3: Pipeline to correct normals.
```

The next picture shows an outcome of an execution of this example.

![](seissol_inverted_normal_correct.png)

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
