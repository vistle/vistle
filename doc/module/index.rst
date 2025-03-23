Module Reference
================

* :doc:`Read - get data from files <read/index>`
* :doc:`In-situ - get data from simulations <insitu/index>`
* :doc:`Filter - manipulate data <filter/index>`
* :doc:`Map - generate geometric shapes <map/index>`
* :doc:`Geometry - modify geometric objects <geometry/index>`
* :doc:`General - manipulate data at any pipeline stage <general/index>`
* :doc:`Info - meta data <info/index>`
* :doc:`Univiz - vector fields <univiz/index>`
* :doc:`Develop - tools for developers <test/index>`

.. toctree::
   :maxdepth: 1
   :hidden:

    Read <read/index.rst>
    In-Situ <insitu/index.rst>
    Filter <filter/index.rst>
    Map <map/index.rst>
    Geometry <geometry/index.rst>
    Render <geometry/index.rst>
    General <general/index.rst>
    Univiz <univiz/index.rst>
    Develop <test/index.rst>


Module Categories
-----------------

The modules in Vistle are categorized according to the role they can take in a visualization workflow.
The linear succession proposed by the visualization pipeline is an idealized view, so that more categories have been created than just the pipeline steps.

:doc:`Read <read/index>`
------------------------

.. toctree::
   :maxdepth: 2

    Read <read/index.rst>

Read modules read in data from files.
They are the most common modules to start a pipeline strand in a workflow.
Most often, they provide data that has not yet undergone the mapping stage.
Together with the simulation modules, these are the sources of the data flow network describing the workflow.

### Simulation

Simulation modules provide a direct link to a simulation that is still on-going. They enable acquiring data from running simulations for in situ processing, bypassing file I/O.
Interfaces to several in situ visualization frameworks are provided. Details can be found in the [Library Documentation](../../lib/libsim_link.md).

### Filter
Filter modules transform abstract data into abstract data, so that another filter step can be applied.
Data processed by filters does not have a geometric representation.

### Map
Map modules ingest abstract data and generate geometric shapes from it.
This geometry output can serve as input for a render module.

### Geometry

Geometry modules perform transformations on geometry data and yield geometry output.

### Render

Render modules take geometry data and generate pixel images.
They either display these immediately to the user or forward them to another render module for compositing.
These are the most common sinks in the data flow graph.

### General
General modules are able to process data at any stage in the pipeline.
Generally, they provide output for the same pipeline stage.

### Information
Information modules provide information about their input.
They do not need to generate output that can serve as input for further pipeline steps,
i.e. often these modules are sinks in the workflow graph.

Information about the data at a certain port can be printed using dedicated modules. These list, among other, the number of vertices, blocks or the type of data structure. Output is printed to the Vistle Console.

### UniViz

The UniViz flow visualization modules build on a common framework
enabling them to be used within several visualization systems.
They are categorized together solely based on their provenance.
They have been implemented by Filip Sadlo for [AVS](https://www.avs.com/avs-express/), [COVISE](https://www.hlrs.de/covise) and [ParaView](https://www.paraview.org),
and have been ported to Vistle later.

### Test
Test modules support testing and development of Vistle and modules.
A workflow for processing an actual data set should not benefit from any of these modules.
This category also comprises modules that are still being worked on.
