
# Chainmail
transform 2D data(triangle and quads) to rings

<svg width="62.199999999999996em" height="8.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="2.8em" width="6.22em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>quads_in</title></rect>
<rect x="0.7em" y="0.7999999999999998em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="0.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="0.8999999999999998em" class="text" >Unstructured grid with quads<tspan> (quads_in)</tspan></text>
<rect x="1.4em" y="2.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="1.9em" y="1.7999999999999998em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="1.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="1.9em" class="text" >mapped data<tspan> (data_in)</tspan></text>
<text x="0.2em" y="4.65em" class="moduleName" >Chainmail</text><rect x="0.2em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>circles_out</title></rect>
<rect x="0.7em" y="5.8em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="7.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="7.8999999999999995em" class="text" >circles<tspan> (circles_out)</tspan></text>
<rect x="1.4em" y="4.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="1.9em" y="5.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="6.8999999999999995em" class="text" >tubes or spheres with mapped data<tspan> (data_out)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|geo_mode|geometry generation mode (Circle, Spline)|Int|
|radius|radius of the rings (diameter = radius / 20)|Float|
|number of torus segments|number of quads used to aproximate the torus in it's plane|Int|
|number of diameter segments|number of quads used to aproximate the torus around its axis|Int|
