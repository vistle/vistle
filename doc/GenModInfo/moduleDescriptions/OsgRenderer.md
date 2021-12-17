
# OsgRenderer
Openscenegraph remote renderer

<svg width="2214.0" height="210" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="60" width="221.39999999999998" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<rect x="21.0" y="30" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="30" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="33.0" class="text" >input data<tspan> (data_in)</tspan></text>
<rect x="6.0" y="120" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>image_out</title></rect>
<rect x="21.0" y="150" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="180" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="183.0" class="text" >connect to COVER<tspan> (image_out)</tspan></text>
<text x="6.0" y="115.5" class="moduleName" >OsgRenderer</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|render_mode|Render on which nodes? (LocalOnly, MasterOnly, AllRanks, LocalShmLeader, AllShmLeaders)|Integer|
|objects_per_frame|Max. no. of objects to load between calls to render|Integer|
|rhr_connection_method|how local/remote endpoint should be determined (ViaVistle, AutomaticHostname, UserHostname, ViaHub, AutomaticReverse, UserReverse)|Integer|
|rhr_base_port|listen port for RHR server|Integer|
|rhr_local_address|address where clients should connect to|String|
|rhr_remote_port|port where renderer should connect to|Integer|
|rhr_remote_host|address where renderer should connect to|String|
|send_tile_size|edge lengths of tiles used during sending|IntVector|
|color_codec|codec for image data (Raw, PredictRGB, PredictRGBA, Jpeg_YUV411, Jpeg_YUV444)|Integer|
|color_compress|compression for RGBA messages (CompressionNone, CompressionLz4, CompressionZstd, CompressionSnappy)|Integer|
|depth_codec|Depth codec (DepthRaw, DepthPredict, DepthPredictPlanar, DepthQuant, DepthQuantPlanar, DepthZfp)|Integer|
|zfp_mode|Accuracy:, Precision:, Rate:  (ZfpFixedRate, ZfpPrecision, ZfpAccuracy)|Integer|
|depth_compress|entropy compression for depth data (CompressionNone, CompressionLz4, CompressionZstd, CompressionSnappy)|Integer|
|depth_prec|quantized depth precision (16 bit + 4 bits/pixel, 24 bit + 3 bits/pixel)|Integer|
|rhr_dump_images|dump image data to disk|Integer|
|continuous_rendering|render even though nothing has changed|Integer|
|delay|artificial delay (s)|Float|
|color_rank|different colors on each rank|Integer|
|debug|debug level|Integer|
|visible_view|no. of visible view (positive values will open window)|Integer|
|threading_model|OpenSceneGraph threading model (Single_Threaded, Cull_Draw_Thread_Per_Context, Thread_Per_Context, Draw_Thread_Per_Context, Cull_Thread_Per_Camera_Draw_Thread_Per_Context, Thread_Per_Camera, Automatic_Selection)|Integer|
|asynchronicity|number of outstanding frames to tolerate|Integer|
