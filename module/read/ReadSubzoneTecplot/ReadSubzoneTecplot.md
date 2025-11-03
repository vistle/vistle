[headline]:<>

## Purpose

The **ReadSubzoneTecplot** module reads Tecplot 360 Sub-Zone Load-on-demand (`*.szplt`) data from a directory. Only structured (ordered) zones are supported. All .szplt files found in the input directory are considered according to the selected time-step range and step increment. 

## Ports
[moduleHtml]:<>


- `grid_out` — structured grid (all subzones)
- `field_out_0` .. `field_out_4` — data fields chosen by the user (up to five). 

The first output port (`grid_out`) provides the structured geometry. The remaining ports provide solution fields selected via the module parameters.

[parameters]:<>

## Behavior

- If `static_geometry` is enabled, the grid is loaded once from the specified reference time step (`static_ref_timestep`) and reused for all output timesteps.
- If `static_geometry` is disabled, the geometry is reloaded for each requested timestep.
- Data fields are attached to the corresponding `field_out_*` ports only if the selected field exists in the file/zone; missing fields are skipped.

## Related Modules

### Similar Modules

[ReadIagTecplot]()
