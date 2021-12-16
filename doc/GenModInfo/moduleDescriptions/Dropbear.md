
# Dropbear
Start dropbear ssh server on compute nodes



<svg width="394.79999999999995" height="210" >
<rect x="0" y="0" width="394.79999999999995" height="210" rx="5" ry="5" style="fill:#64c8c8ff;" />
<text x="14.0" y="126.0" font-size="4.2em">Dropbear</text></svg>



## Parameters
|name|description|type|
|-|-|-|
|dropbear_path|pathname to dropbear binary|String|
|dropbear_options|additional dropbear arguments|String|
|dropbear_port|port where dropbear should listen (passed as argument to -p)|Integer|
|hostkey_rsa|path to rsa host key (~/.ssh/vistle_dropbear_rsa)|String|
|hostkey_dss|path to dss host key (~/.ssh/vistle_dropbear_dss)|String|
|hostkey_ecdsa|path to ecdsa host key (~/.ssh/vistle_dropbear_ecdsa)|String|
|exposed_rank|rank to expose on hub (-1: none)|Integer|
|exposed_port|port to which exposed rank's dropbear should be forwarded to on hub (<=0: absolute value used as offset to dropbear port)|Integer|