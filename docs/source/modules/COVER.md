
# COVER
VR renderer for immersive environments

<svg width="39.0em" height="5.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="1.8em" width="3.9em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="1.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="0.7em" y="0.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="0.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="0.9em" class="text" >input data<tspan> (data_in)</tspan></text>
<text x="0.2em" y="3.6500000000000004em" class="moduleName" >COVER</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|render_mode|Render on which nodes? (LocalOnly, MasterOnly, AllRanks, LocalShmLeader, AllShmLeaders)|Int|
|objects_per_frame|Max. no. of objects to load between calls to render|Int|
|optimize_indices|optimize geometry indices for better GPU vertex cache utilization|Int|
|indexed_geometry|build indexed geometry, if useful|Int|
|num_primitives|number of primitives to process before splitting into multiple geodes|Int|
