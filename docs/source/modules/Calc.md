
# Calc
compute with coordinates and data

<svg width="50.0em" height="8.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="3.8em" width="5.0em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="3.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in0</title></rect>
<rect x="0.7em" y="0.7999999999999998em" width="0.03333333333333333em" height="3.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="0.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="0.8999999999999998em" class="text" >input data<tspan> (data_in0)</tspan></text>
<rect x="1.4em" y="3.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in1</title></rect>
<rect x="1.9em" y="1.7999999999999998em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="1.7999999999999998em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="1.9em" class="text" >input data<tspan> (data_in1)</tspan></text>
<rect x="2.5999999999999996em" y="3.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in2</title></rect>
<rect x="3.0999999999999996em" y="2.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="3.0999999999999996em" y="2.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="4.3em" y="2.9em" class="text" >input data<tspan> (data_in2)</tspan></text>
<text x="0.2em" y="5.65em" class="moduleName" >Calc</text><rect x="0.2em" y="5.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="0.7em" y="6.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="7.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="7.8999999999999995em" class="text" >output data<tspan> (data_out)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|formula|formula for computing data with exprtk (use result := {x,y,z} for vector output)|String|
|grid_formula|formula for computing grid coordinates|String|
|normal_formula|formula for computing grid normals|String|
|output_type|type of output (AsInput, AsInputGrid, Vec1, Vec3)|Int|
|species|species of output data|String|
