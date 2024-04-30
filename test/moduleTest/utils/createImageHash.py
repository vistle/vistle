from PIL import Image
import imagehash
import argparse
import json

def createImageHash(imagePath):
    hash = imagehash.average_hash(Image.open(imagePath))
    return hash

def compareHash(hash1, hash2):
    return hash1 == hash2

def addJson(name:str, jsonDir:str, hash):
    with open(jsonDir, "r+") as file:
        data = json.load(file)

    data[name] = str(hash)

    with open(jsonDir, "w") as file:
        file.write(json.dumps(data, indent=4))
        
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Create imagehash."
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
        default="",
        help="Image path.",
    )
    parser.add_argument(
        "-jdir",
        nargs=1,
        metavar=("jdir"),
        default=None,
        help="Path to json.",
    )
    parser.add_argument(
        "-cmp",
        action=argparse.BooleanOptionalAction,
        metavar=("cmp"),
        default=False,
        help="if true compares with hash in json",
    )

    parser.add_argument(
        "-refHashString",
        nargs=1,
        metavar=("refHashString"),
        default=None,
        help="hash to compare",
    )
    args = parser.parse_args()

    if args.name != None:
        name = args.name[0]
    
    if args.sdir != None:
        srcDir = args.sdir[0]

    if args.cmp != None:
        cmp = args.cmp

    if args.refHashString != None:
        refHashString = args.refHashString[0]

    hash = createImageHash(srcDir)
    if args.jdir != None:
        jdir = args.jdir[0]
        addJson(name, jdir, hash)

    if cmp:
        if not compareHash(hash, imagehash.hex_to_hash(refHashString)):
            raise Exception("Hashes are not the same for " + name + ": " + str(hash) + " != " + refHashString)
