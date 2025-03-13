import sys
import os
import re

#expects 4 arguments: source directory, documentation directory, input file, output file
source_dir = sys.argv[1]
doc_source_dir = sys.argv[2]
input_file_path = sys.argv[3]
output_file_path = sys.argv[4]
#and the environment variables ALL_VISTLE_MODULES and ALL_VISTLE_MODULES_CATEGORY
modules = os.environ.get('ALL_VISTLE_MODULES', '').split(" ")
categories = os.environ.get('ALL_VISTLE_MODULES_CATEGORY', '').split(" ")

# Only handle .md files
if not input_file_path.endswith(".md"):
    sys.exit(1)

# Open the input file for reading
try:
    input_file = open(input_file_path, "r", encoding="utf-8")
except IOError as e:
    print(f"Error opening input file: {e}")
    sys.exit(1)

# Create output directory if it does not exist
output_dir = os.path.dirname(output_file_path)
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

# Open the output file for writing
try:
    output_file = open(output_file_path, "w")
except IOError as e:
    print(f"Error opening output file: {e}")
    sys.exit(1)


# pictures_source_dir = os.path.join(source_dir, "doc/source/pictures/")
# picture_dest_dir = os.path.join(doc_source_dir, "pictures/")
# pictures = os.listdir(pictures_source_dir)
# Get the relative path from input file to vistle/module directory
# relative_picture_path = os.path.relpath(picture_dest_dir, os.path.dirname(output_file_path))
# relative_picture_path = relative_picture_path.replace("\\", "/")
this_directory = os.path.dirname(os.path.realpath(__file__))
relative_module_path = os.path.relpath(doc_source_dir, os.path.dirname(output_file_path))
relative_module_path = relative_module_path.replace("\\", "/")


# Search input for occurrences of [moduleName]()
linenumber = 0
for line in input_file:
    linenumber += 1
    for module in modules:
        module_key = "[" + module + "]()"
        warnRegex = r"\[\W+" + re.escape(module) + r"\W+\]\(\)"
        #regex search in line
        if re.search(warnRegex, line):
            assert False, f"Error: {module} in {input_file_path} line {linenumber} uses illegal modifier" 

        if module_key in line:
            category = categories[modules.index(module)]
            link = f"{relative_module_path}/module/{category}/{module}/{module}.md"
            replacement = f"[{module}]({link})"
            line = line.replace(module_key, replacement)
    
    output_file.write(line)

# Close the files
input_file.close()
output_file.close()
