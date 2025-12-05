[headline]:<>

## Ports

[moduleHtml]:<>

The Calc module performs calculations on input data and input coordinates according to user-defined formulas.
The output grid reuses the topology of the input geometry.
If no `grid_formula` or `normal_formula` is provided, also the output coordinates or output normals are reused from the input geometry, respectively.
The result of the calculation described by `formula` is mapped onto the output grid.

[parameters]:<>

The expressions in the parameters `formula`, `grid_formula`, and `normal_formula` are
parsed with [ExprTK](https://www.partow.net/programming/exprtk/index.html), and so the operations and functions described in `SECTION 08 - BUILT-IN OPERATIONS & FUNCTIONS` of [this document](https://github.com/ArashPartow/exprtk) can be used.

The variables listed in this table are available to be used in the expressions.
Individual components of vector variables can be accessed using `.x`, `.y`, `.z` suffixes as shown,
for 1-dimensional variables `var` and `var.x` are equivalent.

:::{list-table} Available symbols and variables, and their meaning
:header-rows: 1

*  - Symbol / Variable
   - Description

*  - `result`
   - assign the computed value to this variable (done implicitly if no assignment is made)
*  - `outdim`
   - result dimension (typically 1 or 3)
*  - `p`
   - position vector of current point
*  - `x`, `p.x`
   - x coordinate of current point
*  - `y`, `p.y`
   - y coordinate of current point
*  - `z`, `p.z`
   - z coordinate of current point
*  - `d0`, `d`
   - data value at current item (point or cell) received at `data_in0`
*  - `d1`
   - data value at current item (point or cell) received at `data_in1`
*  - `d2`
   - data value at current item (point or cell) received at `data_in2`
*  - `timestep`, `step`
   - timestep number for current block
*  - `time`, `t`
   - real time for current block
*  - `rank`
   - MPI rank number
*  - `block`
   - block ID number of current block
*  - `i`, `idx`, `index`
   - index of current item (point or cell) in current block
:::
