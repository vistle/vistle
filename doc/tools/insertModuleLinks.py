import sys
import os

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
    input_file = open(input_file_path, "r")
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


pictures_source_dir = os.path.join(source_dir, "doc/source/pictures/")
picture_dest_dir = os.path.join(doc_source_dir, "pictures/")
pictures = os.listdir(pictures_source_dir)
# Get the relative path from input file to vistle/module directory
this_directory = os.path.dirname(os.path.realpath(__file__))
relative_module_path = os.path.relpath(doc_source_dir, os.path.dirname(output_file_path))
relative_module_path = relative_module_path.replace("\\", "/")
relative_picture_path = os.path.relpath(picture_dest_dir, os.path.dirname(output_file_path))
relative_picture_path = relative_picture_path.replace("\\", "/")
print("relative_picture_path: " + relative_picture_path)
tsunami = input_file_path.endswith("ReadTsunami.md")
# Search input for occurrences of [moduleName]()
for line in input_file:
    for module in modules:
        module_key = "[" + module + "]()"
        if module_key in line:
            category = categories[modules.index(module)]
            link = f"{relative_module_path}/modules/{category}/{module}.md"
            replacement = f"[{module}]({link})"
            line = line.replace(module_key, replacement)
    for picture in pictures:
        picture_key = f"![]({picture})"
        if(tsunami):
            print("picture_key: " + picture_key)
        if tsunami and picture in line:
            print("at lest picture found: " + line)
        if picture_key in line:
            if(tsunami):
                print("found picture key")
            link = relative_picture_path + "/" + picture
            replacement = f"![]({link})"
            print(f"Replacing {picture_key} with {replacement}")
            line = line.replace(picture_key, replacement)
    output_file.write(line)

# Close the files
input_file.close()
output_file.close()
