
# ColorRandom
map index data to random colors

<svg width="73.8em" height="7.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="1.8em" width="7.38em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="1.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="0.7em" y="0.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="0.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="0.9em" class="text" >field to create colormap for<tspan> (data_in)</tspan></text>
<text x="0.2em" y="3.6500000000000004em" class="moduleName" >ColorRandom</text><rect x="0.2em" y="3.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>data_out</title></rect>
<rect x="0.7em" y="4.8em" width="0.03333333333333333em" height="2.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="6.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="6.8999999999999995em" class="text" >field converted to colors<tspan> (data_out)</tspan></text>
<rect x="1.4em" y="3.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>color_out</title></rect>
<rect x="1.9em" y="4.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="1.9em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="3.0999999999999996em" y="5.8999999999999995em" class="text" >color map<tspan> (color_out)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|species|species attribute of input data|String|
|min|minimum value of range to map|Float|
|max|maximum value of range to map|Float|
|constrain_range|constrain range for min/max to data|Int|
|opacity_factor|multiplier for opacity|Float|
|blend_with_material|use alpha for blending with diffuse material|Int|
|auto_range|compute range automatically|Int|
|preview|use preliminary colormap for showing preview when determining bounds|Int|
