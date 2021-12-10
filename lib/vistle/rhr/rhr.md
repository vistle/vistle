# Remote Hybrid Rendering (RHR)

Remote Hybrid Rendering (RHR) can composite 2.5D imagery rendered remotely
with locally rendered data. This is useful to
- avoid transfer of large dynamic data sets,
- decouple interaction from the slow rendering of large data.

The data structures for the RHR protocol are defined in <rhr/rfbext.h>.

Its implementation consists of two parts, a server-side (remote) plug-in (VncServer) and a
client-side (local) plug-in (RhrClient) for COVER, the VR renderer of the
visualization system COVISE [http://www.hlrs.de/covise]
and Vistle [https://vistle.io].
For sort-last parallel rendering, VncServer can be combined with
CompositorIceT.

It is also compatible with Vistle's ray caster render module DisCOVERay.

Depth images can be optionally compressed with a lossy algorithm similar to Direct3D texture compression
(cf. \ref DepthQuantize), also on the GPU before read-back.

This work was funded by the EU within the project CRESTA [http://cresta-project.eu]
and by the [Ministry of Science, Research and the Arts of Baden-Württemberg](https://mwk.baden-wuerttemberg.de)
within the projects bwVisu [http://bwvisu.de] and bwVisu2.
