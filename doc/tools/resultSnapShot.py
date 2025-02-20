import os
import time
from coGRMsg import *
imageName = os.getenv("VISTLE_DOC_IMAGE_NAME")
sourceDir = os.getenv("VISTLE_DOC_SOURCE_DIR")
targetDir = os.getenv("VISTLE_DOC_TARGET_DIR")
filename = sourceDir + "/" + imageName + ".vsl"
source(sourceDir + "/" + imageName + ".vsl")
barrier()
setLoadedFile(filename) #neded to reset the execution counter to preevnt save dialog
setStatus("Workflow loaded: "+filename)
compute()
time.sleep(5) #needed until we can wait for cover to finish rendering
sendCoverMessage(coGRSetViewpointFile(sourceDir + "/" + imageName + ".vwp", 0))
sendCoverMessage(coGRShowViewpointMsg(0))

time.sleep(3) #needed until we can wait for cover to finish rendering
snapshotGui(targetDir + "/" + imageName + "_workflow.png")
snapshotCover(findFirstModule("COVER"), targetDir + "/" + imageName +  "_result.png")
print("Snapshot taken in " + targetDir + "/" + imageName + "_result.png")
time.sleep(1) #needed until we can wait for cover to finish rendering
vistle.quit()
