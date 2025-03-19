# Workflows
A workflow is a graph of Vistle modules which represent processing steps and implements a visualization pipeline. The necessary stages when creating a workflow strongly depend on the individual application. However, the following pipeline can be used as a reference:

![](workflow_pipeline.png)

## Visualization Pipeline Stages

### Acquisition
First, data needs to be imported, which is typically retrieved by reading from files (allthough dynamic connection to in-situ simulations is supported as well). Reader modules are provided for data import from files, for a number of different file formats (e.g. OpenFOAM, WRF, Nek5000). Moreover, they often allow to refine data to be read by setting limits in timesteps or selecting certain variables only.

### Filter
Features of interest can be extracted from the data set through filter modules. These include, among others, cropping geometries, selecting specific grid layers or the surface of a geometry.

### Map
Mapping modules allow to visualize data values. This is achieved by mapping data values to colors or computing iso surfaces.

### Render & Display
Often, the two steps of rendering pixel images and presenting them on screen are mashed together in a single program.
Only for workflows involving remote rendering, these steps are split up into two modules, e.g. DisCOVERay and COVER.
