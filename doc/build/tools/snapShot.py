import os
import time
from coGRMsg import *
imageName = os.getenv("VISTLE_DOC_IMAGE_NAME")
sourceDir = os.getenv("VISTLE_DOC_SOURCE_DIR")
targetDir = os.getenv("VISTLE_DOC_TARGET_DIR")
ars = os.getenv("VISTLE_DOC_ARGS")
workflow = "workflow" in ars
result = "result" in ars

print("Snapshot: " + imageName + " sourceDir: " + sourceDir + " targetDir: " + targetDir + " args: " + ars)

os.makedirs(targetDir, exist_ok=True)

filename = sourceDir + "/" + imageName + ".vsl"
source(sourceDir + "/" + imageName + ".vsl")
barrier()
setLoadedFile(filename) #neded to reset the execution counter to prevent save dialog
setStatus("Workflow loaded: "+filename)
if result:
    compute()
    time.sleep(5) #needed until we can wait for cover to finish rendering
    sendCoverMessage(coGRSetViewpointFile(sourceDir + "/" + imageName + ".vwp", 0))
    sendCoverMessage(coGRShowViewpointMsg(0))

    time.sleep(3) #needed until we can wait for cover to finish rendering
    snapshotCover(findFirstModule("COVER"), targetDir + "/" + imageName +  "_result.png")
    print("Visualization snapshot taken in " + targetDir + "/" + imageName + "_result.png")

if workflow:
    time.sleep(3) #wait until modules are started
    snapshotGui(targetDir + "/" + imageName + "_workflow.png")
    print("Workflow snapshot taken in " + targetDir + "/" + imageName + "_workflow.png")

time.sleep(1) #needed until we can wait for cover to finish rendering
vistle.quit()
