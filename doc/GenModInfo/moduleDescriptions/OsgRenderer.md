
# OsgRenderer
Openscenegraph remote renderer

## Input ports
|name|description|
|-|-|
|data_in|input data|


<svg width="516.5999999999999" height="210" >
<rect x="0" y="0" width="516.5999999999999" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="14.0" y="0" width="70" height="70" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in</title></rect>
<title>data_in</title></rect><rect x="14.0" y="140" width="70" height="70" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>image_out</title></rect>
<text x="14.0" y="126.0" font-size="4.2em">OsgRenderer</text></svg>

## Output ports
|name|description|
|-|-|
|image_out|connect to COVER|


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
