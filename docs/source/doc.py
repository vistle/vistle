import os
import argparse
from collections import namedtuple
from clear import deleteFilesInDir

Markdown = namedtuple("Markdown", "root filename")

BASE_DIR = os.path.dirname(os.path.realpath(__file__))
INDENT = "   {}\n"
MD_EXTENSION = ".md"
LINK_STR = "_link"
MYST_INCLUDE = """```{include} %s
:relative-images:
```
""" # other python format would replace {include}
RST_INDEX_HEADER = "{name}\n{underline}\n\n.. toctree::\n   :maxdepth: 1\n\n"


def strInFile(openReadOnlyFile, s) -> bool:
    if s in openReadOnlyFile.read():
        return True


def check(predicate_func, val) -> bool:
    return True if predicate_func(val) else False


def isFileEmpty(path) -> bool:
    return check(os.path.getsize, path)


def isFile(path) -> bool:
    return check(os.path.isfile, path)


def pathExists(path) -> bool:
    return check(os.path.exists, path)


def createLinks(markdown_list, root_link_output_path):
    if not pathExists(root_link_output_path):
        print("Directory {} does not exist.".format(root_link_output_path))
        return
    return [createLinkToMarkdownFile(root_link_output_path,
                                     os.path.relpath(root, root_link_output_path),
                                     filename)
            for root, filename in markdown_list]


def addLinkToRSTFile(rst_path, md_link_filename):
    with open(rst_path, 'a+') as arf:
        if not strInFile(arf, md_link_filename):
            arf.write(INDENT.format(md_link_filename))


def createRSTHeaderNameFromRootPath(path, file_path = False):
    md_name = os.path.basename(path)
    if file_path:
        md_name = path.split('/')[-2]
    name_len = "{:=^" + str(len(md_name)) + "}"
    underline = name_len.format("")
    return RST_INDEX_HEADER.format(name = md_name.capitalize(), underline = underline)


def createValidLinkFilePath(md_linkdir, md_root) -> str:
    prevname = new_link = ""
    for dirname in reversed(md_root.split('/')):
        new_link_filename = dirname + prevname + LINK_STR + MD_EXTENSION
        tmp_link = md_linkdir + '/' + new_link_filename
        if not pathExists(tmp_link) or not isFileEmpty(tmp_link):
            new_link = tmp_link
            break
        prevname = '_' + dirname
    return new_link


def createLinkToMarkdownFile(md_linkdir, md_root, md_filename) -> str:
    md_origin_path = md_root + '/' + md_filename
    new_link_file_path = md_linkdir + '/' + md_filename.split('.')[0] + LINK_STR + MD_EXTENSION
    if md_filename == "README.md":
        new_link_file_path = createValidLinkFilePath(md_linkdir, md_root)
    if new_link_file_path == "":
        print("Please rename your README.md in " + md_root + " to 'Modulename'.md.")
        return
    # write include string
    with open(new_link_file_path, 'w') as f:
        f.write(MYST_INCLUDE % md_origin_path)
    return new_link_file_path.split('/')[-1]


def searchFilesInDirs(root_path, dirs, predicate_func):
    return [Markdown(root=root, filename=file) for path in dirs
            for root, _, files in os.walk(root_path + '/' + path)
            for file in files
            if predicate_func(file)]


def createIndexFile(name):
    with open(name, 'w') as wf:
        wf.write(createRSTHeaderNameFromRootPath(name, True))


def createIndexFileIfNotExisting(link_path) -> str:
    if pathExists(link_path):
        if not isFile(link_path):
            link_path = link_path + "/index.rst"
            createIndexFile(link_path)
            print("Don't forget to add " + link_path + " to your main .rst file of your readthedocs environment.")
    return link_path


def run(root_path, search_dir_list, link_docs_output_relpath):
    endswith = lambda file : file.endswith(MD_EXTENSION)
    markdown_files = searchFilesInDirs(root_path, search_dir_list, endswith)
    root_link_output_path = BASE_DIR + '/' + link_docs_output_relpath
    file_link_output_path = createIndexFileIfNotExisting(root_link_output_path)
    deleteFilesInDir(root_link_output_path, pattern="*.md")
    index_link_list = createLinks(markdown_files, root_link_output_path)
    [addLinkToRSTFile(file_link_output_path, link) for link in sorted(index_link_list)]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Search for Markdown files in your project and link them in readthedocs. Place this script in your docs folder and execute it. Default place for executing is <path_to_project>/docs/source.")
    parser.add_argument("-l", nargs=1, metavar=("link"), default=BASE_DIR,
                        help="relative path from doc.py path to output directory where links will be created")
    parser.add_argument("-r", nargs=1, metavar=("root"), default=BASE_DIR.split("docs")[0],
                        help="root path to git project (default: path to this script as reference and set root to path_before_docs_folder)")
    parser.add_argument("-d", nargs="+", metavar=("dirs"), default=[],
                        help="list of relative paths of directories in root this script will searching in")
    args = parser.parse_args()

    root = BASE_DIR.split("docs")[0]
    if args.r != None:
        root = args.r[0]

    if args.l != None:
        link = args.l[0]

    if args.d != None:
        dirs = args.d
    else:
        print("No search dirs specified!")

    run(root, dirs, link)
