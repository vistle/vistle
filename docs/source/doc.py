import os
from collections import namedtuple

Markdown = namedtuple("Markdown", "root filename")

markdown_files = []

docpy_path = os.path.dirname(os.path.realpath(__file__))
vistle_root_path = docpy_path.split("docs")[0]

# search_markdown_dirs = "module lib/vistle app"
search_markdown_dirs = "module"
application_path = docpy_path + "/dev/application"
modules_path = application_path + "/modules"
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
        rel_dir_from_module_dir = os.path.relpath(root, modules_path)
        createLinkToMarkdownFile(modules_path, rel_dir_from_module_dir, filename)

def addLinkToRSTFile(rst_path, md_link_filename):
    with open(rst_path, 'r') as readOnlyFile:
        if md_link_filename in readOnlyFile.read():
            return

    if os.path.exists(rst_path):
        writeableFile = open(rst_path, 'a')
    else:
        writeableFile = open(rst_path, 'w')
        writeableFile.write(rst_index_header_str.format(name = "Module"))
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
    addLinkToRSTFile(modules_path + "/index.rst", new_link_file_path.split('/')[-1])

def searchFileInPath(path, output, predicate_func):
    for root, _, files in os.walk(path):
        for file in files:
            if predicate_func(file):
                output.append(Markdown(root = root, filename = file))

if __name__ == "__main__":
    func = lambda file : file.endswith(md_extension_str)
    for dir in search_markdown_dirs.split(' '):
        rel_dir_path = vistle_root_path + dir
        searchFileInPath(rel_dir_path, markdown_files, func)

    createDocHierachy(markdown_files, application_path)
