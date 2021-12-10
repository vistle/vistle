
# Dropbear
Start Dropbear SSH server on compute nodes



## Parameters
|name|type|description|
|-|-|-|
|dropbear_path|String|pathname to dropbear binary|
|dropbear_options|String|additional dropbear arguments|
|dropbear_port|Int|port where dropbear should listen (passed as argument to -p)|
|hostkey_rsa|String|path to rsa host key (~/.ssh/vistle_dropbear_rsa)|
|hostkey_dss|String|path to dss host key (~/.ssh/vistle_dropbear_dss)|
|hostkey_ecdsa|String|path to ecdsa host key (~/.ssh/vistle_dropbear_ecdsa)|
|exposed_rank|Int|rank to expose on hub (-1: none)|
|exposed_port|Int|port to which exposed rank's dropbear should be forwarded to on hub (<=0: absolute value used as offset to dropbear port)|
