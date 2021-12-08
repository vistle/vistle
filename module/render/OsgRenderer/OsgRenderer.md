OsgRenderer
===========
OpenSceneGraph remote renderer

Input ports
-----------
|name|description|
|-|-|
|data_in|input data|

Output ports
------------
|name|description|
|-|-|
|image_out|connect to COVER|

Parameters
----------
|name|type|description|
|-|-|-|
|render_mode|Int|Render on which nodes?|
|objects_per_frame|Int|Max. no. of objects to load between calls to render|
|rhr_connection_method|Int|how local/remote endpoint should be determined|
|rhr_base_port|Int|listen port for RHR server|
|rhr_local_address|String|address where clients should connect to|
|rhr_remote_port|Int|port where renderer should connect to|
|rhr_remote_host|String|address where renderer should connect to|
|send_tile_size|IntVector|edge lengths of tiles used during sending|
|color_codec|Int|codec for image data|
|color_compress|Int|compression for RGBA messages|
|depth_codec|Int|Depth codec|
|zfp_mode|Int|Accuracy:, Precision:, Rate: |
|depth_compress|Int|entropy compression for depth data|
|depth_prec|Int|quantized depth precision|
|rhr_dump_images|Int|dump image data to disk|
|continuous_rendering|Int|render even though nothing has changed|
|delay|Float|artificial delay (s)|
|color_rank|Int|different colors on each rank|
|debug|Int|debug level|
|visible_view|Int|no. of visible view (positive values will open window)|
|threading_model|Int|OpenSceneGraph threading model|
|asynchronicity|Int|number of outstanding frames to tolerate|
