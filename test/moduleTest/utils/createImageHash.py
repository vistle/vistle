from PIL import Image
import imagehash
import argparse
import json

def createImageHash(imagePath):
    hash = imagehash.average_hash(Image.open(imagePath))
    return hash

def addJson(name:str, jsonDir:str, hash):
    with open(jsonDir, "r+") as file:
        data = json.load(file)

    data[name] = str(hash)

    with open(jsonDir, "w") as file:
        json.dump(data, file)

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
        metavar=("jsonDir"),
        default="",
        help="Path to json.",
    )
    args = parser.parse_args()
    print(args)

    if args.name != None:
        name = args.name[0]
    
    if args.sdir != None:
        srcDir = args.sdir[0]

    if args.jdir != None:
        jsonDir = args.jdir[0]

    hash = createImageHash(srcDir)
    addJson(name, jsonDir, hash)

    # for debugging 
    # print(hash)
    # print(type(hash))
    # print(str(hash))
    # print(type(str(hash)))
    # print(json.dumps(str(hash)))
