#TODO: get rid of env variables
# adjustments to make: Documentation.cmake and VistleUtils.cmake
import os
import argparse
import time
import coGRMsg

def createGUISnapshot(imageName, sourceDir):
    snapshotGui(sourceDir + "/" + imageName + "_workflow.png")

def createCOVERSnapshot(imageName, outputDir):
    snapshotCover(findFirstModule("COVER"), outputDir + "/" + imageName +  "_result.png")

def initVistle(imageName, sourceDir):
    source(sourceDir + "/" + imageName + ".vsl")
    barrier()
    compute()
    time.sleep(5) #needed until we can wait for cover to finish rendering
    
    sendCoverMessage(coGRMsg.coGRSetViewpointFile(sourceDir + "/" + imageName + ".vwp", 0))
    sendCoverMessage(coGRMsg.coGRShowViewpointMsg(0))
    time.sleep(3) #needed until we can wait for cover to finish rendering

if __name__ == '__main__':
    default_src_dir = os.path.dirname(os.path.realpath(__file__))
    parser = argparse.ArgumentParser(
        description="Create vistle snapshot."
    )
    parser.add_argument(
        "-name",
        nargs=1,
        metavar=("name"),
        default="module",
        help="Vsl name.",
    )
    parser.add_argument(
        "-sdir",
        nargs=1,
        metavar=("srcDir"),
        default=default_src_dir,
        help="Src dir containing vsl network file.",
    )
    parser.add_argument(
        "-o",
        nargs=1,
        metavar=("outputDir"),
        default=default_src_dir,
        help="Output dir for images.",
    )
    parser.add_argument(
        "-gui",
        action=argparse.BooleanOptionalAction,
        metavar=("gui"),
        help="Create gui snapshot.",
    )
    args = parser.parse_args(os.getenv("SNAPSHOT_ARGS").split())

    if args.name != None:
        name = args.name[0]
    if args.o != None:
        outputDir = args.o[0]
    
    if args.sdir != None:
        srcDir = args.sdir[0]
    else:
        print("No search dirs specified!")

    initVistle(name, srcDir)
    createCOVERSnapshot(name, outputDir)
    if args.gui:
        createGUISnapshot(name, srcDir)

    time.sleep(1) #needed until we can wait for cover to finish rendering
    quit()
