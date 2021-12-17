
# IsoSurface
Extract surfaces with constant data level

<svg width="2040.0" height="240" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="90" width="204.0" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="21.0" y="30" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="30" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="33.0" class="text" ><tspan> (data_in)</tspan></text>
<rect x="42.0" y="90" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>mapdata_in</title></rect>
<rect x="57.0" y="60" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="60" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="63.0" class="text" ><tspan> (mapdata_in)</tspan></text>
<rect x="6.0" y="150" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="21.0" y="180" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="210" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="213.0" class="text" ><tspan> (data_out)</tspan></text>
<text x="6.0" y="145.5" class="moduleName" >IsoSurface</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|isovalue|isovalue|Float|
|isopoint|isopoint|Vector|
|point_or_value|point or value interaction (PointPerTimestep, Value, PointInFirstStep)|Integer|
|processortype|processortype (Host, Device)|Integer|
|compute_normals|compute normals (structured grids only)|Integer|
