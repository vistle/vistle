
# Cache
Cache input objects

<svg width="2100" height="450" >
<style>.text { font: normal 24.0px sans-serif;}tspan{ font: italic 24.0px sans-serif;}.moduleName{ font: italic 30px sans-serif;}</style>
<rect x="0" y="180" width="210" height="90" rx="5" ry="5" style="fill:#64c8c8ff;" />
<rect x="6.0" y="180" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in0</title></rect>
<rect x="21.0" y="30" width="1.0" height="150" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="30" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="33.0" class="text" >input data 0<tspan> (data_in0)</tspan></text>
<rect x="42.0" y="180" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in1</title></rect>
<rect x="57.0" y="60" width="1.0" height="120" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="60" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="63.0" class="text" >input data 1<tspan> (data_in1)</tspan></text>
<rect x="78.0" y="180" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in2</title></rect>
<rect x="93.0" y="90" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="90" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="93.0" class="text" >input data 2<tspan> (data_in2)</tspan></text>
<rect x="114.0" y="180" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in3</title></rect>
<rect x="129.0" y="120" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="129.0" y="120" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="165.0" y="123.0" class="text" >input data 3<tspan> (data_in3)</tspan></text>
<rect x="150.0" y="180" width="30" height="30" rx="0" ry="0" style="fill:#c81e1eff;" >
<title>data_in4</title></rect>
<rect x="165.0" y="150" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="165.0" y="150" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="201.0" y="153.0" class="text" >input data 4<tspan> (data_in4)</tspan></text>
<rect x="6.0" y="240" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out0</title></rect>
<rect x="21.0" y="270" width="1.0" height="150" rx="0" ry="0" style="fill:#000000;" />
<rect x="21.0" y="420" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="57.0" y="423.0" class="text" >output data 0<tspan> (data_out0)</tspan></text>
<rect x="42.0" y="240" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out1</title></rect>
<rect x="57.0" y="270" width="1.0" height="120" rx="0" ry="0" style="fill:#000000;" />
<rect x="57.0" y="390" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="93.0" y="393.0" class="text" >output data 1<tspan> (data_out1)</tspan></text>
<rect x="78.0" y="240" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out2</title></rect>
<rect x="93.0" y="270" width="1.0" height="90" rx="0" ry="0" style="fill:#000000;" />
<rect x="93.0" y="360" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="129.0" y="363.0" class="text" >output data 2<tspan> (data_out2)</tspan></text>
<rect x="114.0" y="240" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out3</title></rect>
<rect x="129.0" y="270" width="1.0" height="60" rx="0" ry="0" style="fill:#000000;" />
<rect x="129.0" y="330" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="165.0" y="333.0" class="text" >output data 3<tspan> (data_out3)</tspan></text>
<rect x="150.0" y="240" width="30" height="30" rx="0" ry="0" style="fill:#c8c81eff;" >
<title>data_out4</title></rect>
<rect x="165.0" y="270" width="1.0" height="30" rx="0" ry="0" style="fill:#000000;" />
<rect x="165.0" y="300" width="30" height="1.0" rx="0" ry="0" style="fill:#000000;" />
<text x="201.0" y="303.0" class="text" >output data 4<tspan> (data_out4)</tspan></text>
<text x="6.0" y="235.5" class="moduleName" >Cache</text></svg>

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
