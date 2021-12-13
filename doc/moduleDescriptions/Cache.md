
# Cache
Cache input objects

## Input ports
|name|description|
|-|-|
|data_in0|input data 0|
|data_in1|input data 1|
|data_in2|input data 2|
|data_in3|input data 3|
|data_in4|input data 4|


## Output ports
|name|description|
|-|-|
|data_out0|output data 0|
|data_out1|output data 1|
|data_out2|output data 2|
|data_out3|output data 3|
|data_out4|output data 4|


## Parameters
|name|description|type|
|-|-|-|
|mode|operation mode (Memory, From_Disk, To_Disk, Automatic)|Integer|
|file|filename where cache should be created|String|
|step|step width when reading from disk|Integer|
|start|start step|Integer|
|stop|stop step|Integer|
|field_compression|compression mode for data fields (Uncompressed, ZfpFixedRate, ZfpAccuracy, ZfpPrecision)|Integer|
|zfp_rate|ZFP fixed compression rate|Float|
|zfp_precision|ZFP fixed precision|Integer|
|zfp_accuracy|ZFP compression error tolerance|Float|
|archive_compression|compression mode for archives (CompressionNone, CompressionLz4, CompressionZstd, CompressionSnappy)|Integer|
|archive_compression_speed|speed parameter of compression algorithm|Integer|
|reorder|reorder timesteps|Integer|
|renumber|renumber timesteps consecutively|Integer|
