# needs python 3.8
from typing import TypedDict
from PIL import Image
import imagehash
import argparse
import json


class Option(TypedDict):
    name: str
    metavar: str
    help: str
    nargs: str | int
    action: argparse.Action
    default: str = None


Options = [
    Option(name="-name", metavar="name", help="Vsl network filename.", nargs=1),
    Option(name="-sdir", metavar="srcDir", help="Image path.", nargs=1),
    Option(name="-jdir", metavar="jsondir", help="Path to json.", nargs=1),
    Option(
        name="-cmp",
        metavar="compare",
        default=False,
        help="Compare with hash in json file <vistle_root>/test/moduleTest/utils/refImageHash.json.",
        action=argparse.BooleanOptionalAction,
    ),
    Option(
        name="-refHashString",
        metavar="referenceHashString",
        help="Reference hash as string literal to compare with new created imagehashes. Add -cmp additionally",
        nargs="?",
    ),
]


def createImageHash(imagePath):
    print("Creating imagehash for: " + imagePath)
    hash = imagehash.average_hash(Image.open(imagePath))
    print(hash)
    return hash

def compareHash(hash1, hash2) -> bool:
    return hash1 == hash2

def addModuleImagehashToJson(pathToModuleRoot: str, jsonDir: str, hash) -> None:
    with open(jsonDir, "r+") as file:
        data = json.load(file)

    data[pathToModuleRoot] = str(hash)

    with open(jsonDir, "w") as file:
        file.write(json.dumps(data, indent=4))


def initParser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Create imagehash.")
    for option in Options:
        name = option.pop("name")
        parser.add_argument(name, **option)
    return parser


def getArgs():
    parser = initParser()
    return parser.parse_args()


if __name__ == "__main__":
    args = getArgs()

    if args.name != None:
        name = args.name[0]

    if args.sdir != None:
        srcDir = args.sdir[0]

    if args.cmp != None:
        cmp = args.cmp

    if args.refHashString != None:
        refHashString = args.refHashString
        cmp = True

    hash = createImageHash(srcDir)
    if args.jdir != None:
        jdir = args.jdir[0]
        addModuleImagehashToJson(name, jdir, hash)

    if cmp:
        if not compareHash(hash, imagehash.hex_to_hash(refHashString)):
            raise Exception(
                "Hashes are not the same for "
                + name
                + ": "
                + str(hash)
                + " != "
                + refHashString
            )
