import os
import time
imageName = os.getenv("VISTLE_DOC_IMAGE_NAME")
sourceDir = os.getenv("VISTLE_DOC_SOURCE_DIR")
binaryDir = os.getenv("VISTLE_DOC_BINARY_DIR")
source(sourceDir + "/" + imageName + ".vsl")
barrier()
compute()
time.sleep(5) #needed until we can wait for cover to finish rendering

sendCoverMessage(coGRSetViewpointFile(sourceDir + "/" + imageName + ".vwp", 0))
sendCoverMessage(coGRShowViewpointMsg(0))
import time
time.sleep(3) #needed until we can wait for cover to finish rendering
snapshotGui(binaryDir + "/" + imageName + "_workflow.png")
snapshotCover(findFirstModule("COVER"), binaryDir + "/" + imageName +  "_result.png")
time.sleep(1) #needed until we can wait for cover to finish rendering
quit()
