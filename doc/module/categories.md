# Module Categories

The modules in Vistle are categorized according to the role they can take in a visualization workflow.
The linear succession proposed by the visualizatino pipeline is an idealized view, so that more categories have been created than just the pipeline steps presented above.

### Read

Read modules read in data from files.
They are the most common modules to start a pipeline strand in a workflow.
Most often, they provide data that has not yet undergone the mapping stage.
Together with the simulation modules, these are the sources of the data flow network describing the workflow.

*Examples*: [ReadCovise](), [ReadEnsight](), [ReadFoam](), [ReadModel](),   [ReadNek5000](),  [ReadTsunami](), [ReadVtk](), [ReadWrfChem]()

### Simulation

Simulation modules provide a direct link to a simulation that is still on-going. They enable acquiring data from running simulations for in situ processing, bypassing file I/O.
Interfaces to several in situ visualization frameworks are provided. Details can be found in the [Library Documentation](../../lib/libsim_link.md).

*Examples*: [LibSim](../../lib/libsim_link.md)

### Filter
Filter modules transform abstract data into abstract data, so that another filter step can be applied.
Data processed by filters does not have a geometric representation.

*Examples*: [IndexManifolds](), [CellToVert]() (transform mapping), [ScalarToVec](), [MapDrape]() (transform coordinates)


### Map
Map modules ingest abstract data and generate geometric shapes from it.
This geometry output can serve as input for a render module.

*Examples*: [Color](), [IsoSurface](), [DomainSurface]()

### Geometry

Geometry modules perform transformations on geometry data and yield geometry output.

*Examples*: [CutGeometry](), [ToTriangles]() (transform mesh type)

### Render

Render modules take geometry data and generate pixel images.
They either display these immediately to the user or forward them to another render module for compositing.
These are the most common sinks in the data flow graph.

*Examples*: [COVER](), [DisCOVERay]()

### General
General modules are able to process data at any stage in the pipeline.
Generally, they provide output for the same pipeline stage.

*Examples*: [AddAttribute](), [MetaData](), [Variant](),  [Transform]() (affine transformation for geometries)

### Information
Information modules provide information about their input.
They do not need to generate output that can serve as input for further pipeline steps,
i.e. often these modules are sinks in the workflow graph.

Information about the data at a certain port can be printed using dedicated modules. These list, among other, the number of vertices, blocks or the type of data structure. Output is printed to the Vistle Console.

*Examples*: ObjectStatistics, PrintMetaData


### UniViz

The UniViz flow visualization modules build on a common framework
enabling them to be used within several visualization systems.
They are categorized together solely based on their provenance.
They have been implemented by Filip Sadlo for [AVS](https://www.avs.com/avs-express/), [COVISE](https://www.hlrs.de/covise) and [ParaView](https://www.paraview.org),
and have been ported to Vistle later.

*Examples*: [VortexCores](), [VortexCriteria]()

### Test
Test modules support testing and development of Vistle and modules.
A workflow for processing an actual data set should not benefit from any of these modules.
This category also comprises modules that are still being worked on.

*Examples*: [Gendat]()
