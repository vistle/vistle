
# ShowGrid
show outlines of grid cells

<svg width="56.4em" height="6.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="1.8em" width="5.64em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="1.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>grid_in</title></rect>
<rect x="0.7em" y="0.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="0.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="0.9em" class="text" >grid or data mapped to grid<tspan> (grid_in)</tspan></text>
<text x="0.2em" y="3.6500000000000004em" class="moduleName" >ShowGrid</text><rect x="0.2em" y="3.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>grid_out</title></rect>
<rect x="0.7em" y="4.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="5.8999999999999995em" class="text" >edges of grid cells<tspan> (grid_out)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|normalcells|Show normal (non ghost) cells|Int|
|ghostcells|Show ghost cells|Int|
|convex|Show convex cells|Int|
|nonconvex|Show non-convex cells|Int|
|tetrahedron|Show tetrahedron|Int|
|pyramid|Show pyramid|Int|
|prism|Show prism|Int|
|hexahedron|Show hexahedron|Int|
|polyhedron|Show polyhedron|Int|
|polygon|Show polygon|Int|
|quad|Show quad|Int|
|triangle|Show triangle|Int|
|bar|Show bar|Int|
|min_cell_index|show cells starting from this index|Int|
|max_cell_index|show cells up to this index|Int|
