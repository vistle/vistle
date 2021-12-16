
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


<svg width="180" height="90" >
<rect x="0" y="0" width="180" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in0</title></rect>
<title>data_in0</title></rect><rect x="42.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in1</title></rect>
<title>data_in1</title></rect><rect x="78.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in2</title></rect>
<title>data_in2</title></rect><rect x="114.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in3</title></rect>
<title>data_in3</title></rect><rect x="150.0" y="0" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in4</title></rect>
<title>data_in4</title></rect><rect x="6.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="42.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="78.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out2</title></rect>
<rect x="114.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out3</title></rect>
<rect x="150.0" y="60" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out4</title></rect>
<text x="6.0" y="54.0" font-size="1.7999999999999998em">Cache</text></svg>

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
