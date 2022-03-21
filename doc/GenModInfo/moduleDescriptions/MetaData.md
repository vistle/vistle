
# MetaData
transform meta data to mappable data

<svg width="56.4em" height="6.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="1.8em" width="5.64em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="1.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<rect x="0.7em" y="0.8em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="0.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="0.9em" class="text" ><tspan> (grid_in)</tspan></text>
<text x="0.2em" y="3.6500000000000004em" class="moduleName" >MetaData</text><rect x="0.2em" y="3.8em" width="1.0em" height="1.0em" rx="0em" ry="0em" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="0.7em" y="4.8em" width="0.03333333333333333em" height="1.0em" rx="0em" ry="0em" style="fill:#000000;" />
<rect x="0.7em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0em" ry="0em" style="fill:#000000;" />
<text x="1.9em" y="5.8999999999999995em" class="text" ><tspan> (data_out)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|attribute|attribute to map to vertices (MpiRank, BlockNumber, TimestepNumber, VertexIndex, ElementIndex, ElementType)|Int|
|range|range to which data shall be clamped|IntVector|
|modulus|wrap around output value|Int|
