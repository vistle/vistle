
# OsgRenderer
OpenSceneGraph remote renderer

<svg width="73.8em" height="6.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="1.8em" width="7.38em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<rect x="0.2em" y="1.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="0.7em" y="0.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="0.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="0.9em" class="text" >input data<tspan> (data_in)</tspan></text>
<text x="0.2em" y="3.6500000000000004em" class="moduleName" >OsgRenderer</text><rect x="0.2em" y="3.8em" width="1.0em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#c8c81eff;" >
<title>image_out</title></rect>
<rect x="0.7em" y="4.8em" width="0.03333333333333333em" height="1.0em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<rect x="0.7em" y="5.8em" width="1.0em" height="0.03333333333333333em" rx="0.0em" ry="0.0em" style="fill:#000000;" />
<text x="1.9em" y="5.8999999999999995em" class="text" >connect to COVER<tspan> (image_out)</tspan></text>
</svg>

## Parameters
|name|description|type|
|-|-|-|
|render_mode|Render on which nodes? (LocalOnly, MasterOnly, AllRanks, LocalShmLeader, AllShmLeaders)|Int|
|objects_per_frame|Max. no. of objects to load between calls to render|Int|
|rhr_connection_method|how local/remote endpoint should be determined (RendezvousOnHub, AutomaticHostname, UserHostname, ViaHub, AutomaticReverse, UserReverse)|Int|
|rhr_base_port|listen port for RHR server|Int|
|rhr_local_address|address where clients should connect to|String|
|rhr_remote_port|port where renderer should connect to|Int|
|rhr_remote_host|address where renderer should connect to|String|
|send_tile_size|edge lengths of tiles used during sending|IntVector|
|color_codec|codec for image data (Raw, PredictRGB, PredictRGBA, Jpeg_YUV411, Jpeg_YUV444)|Int|
|color_compress|compression for RGBA messages (CompressionNone, CompressionLz4, CompressionZstd)|Int|
|depth_codec|Depth codec (DepthRaw, DepthPredict, DepthPredictPlanar, DepthQuant, DepthQuantPlanar, DepthZfp)|Int|
|zfp_mode|Accuracy:, Precision:, Rate:  (ZfpFixedRate, ZfpPrecision, ZfpAccuracy)|Int|
|depth_compress|entropy compression for depth data (CompressionNone, CompressionLz4, CompressionZstd)|Int|
|depth_prec|quantized depth precision (16 bit + 4 bits/pixel, 24 bit + 3 bits/pixel)|Int|
|rhr_dump_images|dump image data to disk|Int|
|continuous_rendering|render even though nothing has changed|Int|
|delay|artificial delay (s)|Float|
|color_rank|different colors on each rank|Int|
|debug|debug level|Int|
|visible_view|no. of visible view (positive values will open window)|Int|
|threading_model|OpenSceneGraph threading model (Single_Threaded, Cull_Draw_Thread_Per_Context, Thread_Per_Context, Draw_Thread_Per_Context, Cull_Thread_Per_Camera_Draw_Thread_Per_Context, Thread_Per_Camera, Automatic_Selection)|Int|
|asynchronicity|number of outstanding frames to tolerate|Int|
