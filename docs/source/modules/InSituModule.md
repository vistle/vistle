
# InSituModule
acquire data from SENSEI or Catalyst II instrumented simulations

<svg width="79.6em" height="4.6em" >
<style>.text { font: normal 1.0em sans-serif;}tspan{ font: italic 1.0em sans-serif;}.moduleName{ font: bold 1.0em sans-serif;}</style>
<rect x="0em" y="0.8em" width="7.959999999999999em" height="3.0em" rx="0.1em" ry="0.1em" style="fill:#64c8c8ff;" />
<text x="0.2em" y="2.6500000000000004em" class="moduleName" >InSituModule</text></svg>

## Parameters
|name|description|type|
|-|-|-|
|path|path to the connection file written by the simulation|String|
|frequency|the pipeline is processed for every nth simulation cycle|Int|
|keep_timesteps|if true timesteps are cached and processed as time series|Int|
