import os
import argparse
from collections import namedtuple

Markdown = namedtuple("Markdown", "root filename")

# default
docpy_path = os.path.dirname(os.path.realpath(__file__))
indent = "   {}\n"
md_extension_str = ".md"
link_str = "_link"
myst_include_str = """```{include} %s
:relative-images:
```
""" # other python format would replace {include}
rst_index_header_str = "{name}\n{underline}\n\n.. toctree::\n   :maxdepth: 1\n\n"


def createLinks(markdown_list, root_link_output_path):
    if not os.path.exists(root_link_output_path):
        print("Directory {} does not exist.".format(root_link_output_path))
        return
    return [createLinkToMarkdownFile(root_link_output_path,
                                     os.path.relpath(root, root_link_output_path),
                                     filename)
            for root, filename in markdown_list]


def strInFile(openReadOnlyFile, s) -> bool:
    if s in openReadOnlyFile.read():
        return True


def addLinkToRSTFile(rst_path, md_link_filename):
    with open(rst_path, 'a+') as arf:
        if not strInFile(arf, md_link_filename):
            arf.write(indent.format(md_link_filename))


def createRSTHeaderNameFromRootPath(path, file_path = False):
    md_name = os.path.basename(path)
    if file_path:
        md_name = path.split('/')[-2]
    name_len = "{:=^" + str(len(md_name)) + "}"
    underline = name_len.format("")
    return rst_index_header_str.format(name = md_name, underline = underline)


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


def createLinkToMarkdownFile(md_linkdir, md_root, md_filename) -> str:
    md_origin_path = md_root + '/' + md_filename
    if md_filename == "README.md":
        new_link_file_path = createValidLinkFilePath(md_linkdir, md_root)
    else:
        new_link_file_path = md_linkdir + '/' + md_filename.split('.')[0] + link_str + md_extension_str
    if new_link_file_path == "":
        return
    with open(new_link_file_path, 'w') as f:
        f.write(myst_include_str % md_origin_path)
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
    if os.path.exists(link_path):
        if not os.path.isfile(link_path):
            rst_file_path = link_path + "/index.rst"
            createIndexFile(rst_file_path)
            link_path = rst_file_path
            print("Don't forget to add " + rst_file_path + " to your main .rst file of your readthedocs environment.")
    return link_path


def run(root_path, search_dir_list, link_docs_output_relpath):
    endswith = lambda file : file.endswith(md_extension_str)
    markdown_files = searchFilesInDirs(root_path, search_dir_list, endswith)
    root_link_output_path = docpy_path + '/' + link_docs_output_relpath
    link_output_path = createIndexFileIfNotExisting(root_link_output_path)
    index_link_list = createLinks(markdown_files, root_link_output_path)
    [addLinkToRSTFile(link_output_path, link) for link in sorted(index_link_list)]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Search for Markdown files in your project and link them in readthedocs. Place this script in your docs folder and execute it. Default place for executing is <path_to_project>/docs/source.")
    parser.add_argument("-l", nargs=1, metavar=("link"), default=docpy_path,
                        help="relative path from doc.py path to output directory where links will be created")
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
