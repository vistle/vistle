import sys
import os
import re

def read_module_categories(file_path):
    """
    Read moduledescriptions.txt and return a mapping of module names to categories.
    """
    category = {}

    try:
        with open(file_path, 'r') as file:
            for line in file:
                # Split the line into parts
                parts = line.split()
                if len(parts) >= 2:
                    mod = parts[0]
                    cat = parts[1]
                    # Add the key-value pair to the dictionary
                    category[mod] = cat
    except FileNotFoundError:
        print(f"Error: File '{file_path}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

    return category

#expects 4 arguments: module descriptions, source directory, documentation directory, input file, output file
module_descriptions = sys.argv[1]
source_dir = sys.argv[2]
doc_source_dir = sys.argv[3]
input_file_path = sys.argv[4]
output_file_path = sys.argv[5]

category = read_module_categories(module_descriptions)
modules = list(category.keys())
categories = list(category.values())
unique_categories = list(set(categories))

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


# pictures_source_dir = os.path.join(source_dir, "doc/pictures/")
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
            category = category.lower()
            link = f"{relative_module_path}/module/{category}/{module}/{module}.md"
            replacement = f"[{module}]({link})"
            link = f"project:#mod-{module}"
            replacement = f"[]({link})"
            line = line.replace(module_key, replacement)
    
    for category in unique_categories:
        category_key = "[" + category + "]()"
        warnRegex = r"\[\W+" + re.escape(category) + r"\W+\]\(\)"
        #regex search in line
        if re.search(warnRegex, line):
            assert False, f"Error: {category} in {input_file_path} line {linenumber} uses illegal modifier" 

        if category_key in line:
            link = f"{relative_module_path}/module/{category.lower()}/index"
            link = f"project:#cat-{category}"
            replacement = f"[{category}]({link})"
            replacement = f"[]({link})"
            line = line.replace(category_key, replacement)

    output_file.write(line)

# Close the files
input_file.close()
output_file.close()
