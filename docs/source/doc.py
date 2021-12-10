import os
import argparse
from collections import namedtuple

Markdown = namedtuple("Markdown", "root filename")

index_link_list = []

# default
docpy_path = os.path.dirname(os.path.realpath(__file__))
link_output_path = docpy_path + "/modules"
md_extension_str = ".md"
link_str = "_link"
myst_include_str = """```{include} %s
:relative-images:
```
"""
rst_index_header_str = """{name}
=================================

.. toctree::
   maxdepth: 1

"""

def createDocHierachy(markdown_list, start_dir):
    if (not os.path.exists(start_dir)):
        print("Directory {} does not exist.".format(start_dir))
        return
    for root, filename in markdown_list:
        rel_dir_from_module_dir = os.path.relpath(root, link_output_path)
        createLinkToMarkdownFile(link_output_path, rel_dir_from_module_dir, filename)


def addLinkToRSTFile(rst_path, md_link_filename):
    with open(rst_path, 'r') as readOnlyFile:
        if md_link_filename in readOnlyFile.read():
            return

    if os.path.exists(rst_path):
        writeableFile = open(rst_path, 'a')
    else:
        writeableFile = open(rst_path, 'w')
        writeableFile.write(rst_index_header_str.format(name = "Module Guide"))
    writeableFile.write("   " + md_link_filename + '\n')
    writeableFile.close()


def createValidLinkFilePath(md_linkdir, md_root) -> str:
    prevname = new_link = ""
    for dirname in reversed(md_root.split('/')):
        new_link_filename = dirname + prevname + link_str + md_extension_str
        tmp_link = md_linkdir + '/' + new_link_filename
        if not os.path.exists(tmp_link) or not os.path.getsize(tmp_link) == 0:
            new_link = tmp_link
            break
        prevname = '_' + dirname
    return new_link


def createLinkToMarkdownFile(md_linkdir, md_root, md_filename):
    md_origin_path = md_root + '/' + md_filename
    new_link_file_path = createValidLinkFilePath(md_linkdir, md_root)
    if new_link_file_path == "":
        return
    with open(new_link_file_path, 'w') as f:
        f.write(myst_include_str % md_origin_path)
    index_link_list.append(new_link_file_path.split('/')[-1])


def searchFileInPath(path, output_list, predicate_func):
    for root, _, files in os.walk(path):
        for file in files:
            if predicate_func(file):
                output_list.append(Markdown(root = root, filename = file))


def run(root_path, search_dir_list, link_docs_output_relpath):
    markdown_files = []
    endswith = lambda file : file.endswith(md_extension_str)
    for dir in search_dir_list:
        rel_dir_path = root_path + '/' + dir
        searchFileInPath(rel_dir_path, markdown_files, endswith)

    link_output_path = docpy_path + '/' + link_docs_output_relpath
    createDocHierachy(markdown_files, link_output_path)
    [addLinkToRSTFile(link_output_path + "/index.rst", link) for link in sorted(index_link_list)]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Search for Markdown files in your project and link them in readthedocs. Place this script in your docs folder and execute it. Default place for executing is <path_to_project>/docs/source.")
    parser.add_argument("-l", nargs=1, metavar=("link"), default=docpy_path,
                        help="relative path from doc.py path to output directory where links will be created (needs index.rst in path)")
    parser.add_argument("-r", nargs=1, metavar=("root"), default=docpy_path.split("docs")[0],
                        help="root path to git project (default: path to this script as reference and set root to path_before_docs_folder)")
    parser.add_argument("-d", nargs="+", metavar=("dirs"), default=[],
                        help="list of relative paths of directories in root this script will searching in")
    args = parser.parse_args()

    if args.r != None:
        root = args.r[0]

    if args.l != None:
        link = args.l[0]

    if args.d != None:
        dirs = args.d
    else:
        print("No search dirs specified!")

    run(root, dirs, link)
