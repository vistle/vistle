#! /usr/bin/env python3
import os
import sys
import re

def fatal(message):
    print(f"{sys.argv[0]}: {message}", file=sys.stderr)
    sys.exit(1)

# check if `file` should be updated by this script
def requires_guard(file):
    # include these unconditionally
    include = [
        "lib/vistle/insitu/libsim/libsimInterface/VisItDataTypes.h",
        "lib/vistle/insitu/libsim/libsimInterface/export.h",
    ]

    # but exclude everything starting with these...
    exclude = [
        # should be included several times
        "lib/vistle/util/boost_interprocess_config.h",
        # shared with COVISE
        "module/read/ReadFoam/foamtoolbox.h",
        "module/render/ANARemote/projection.h",
        # submodules
        "lib/vistle/config/covconfig/",
        "module/univiz/",
        "module/read/ReadCovise/file/",
        "module/general/Calc/exprtk/",
        "module/geometry/DelaunayTriangulator/tetgen/",
        "module/read/ReadCsv/fast-cpp-csv-parser/",
        "module/read/ReadIewMatlabCsvExports/rapidcsv/",
        # not our code
        "lib/vistle/insitu/libsim/libsimInterface/",
        "lib/vistle/insitu/libsim/VisItControlInterfaceRuntime.h",
        "lib/vistle/util/valgrind.h",
        "app/gui/propertybrowser/",
        "app/gui/remotefilebrowser/",
        "app/gui/qconsole/qconsole.h",
        "app/gui/vistleconsole.h",
        "module/read/ReadIagTecplot/tecplotfile.h",
        "module/read/ReadIagTecplot/topo.h",
        "module/read/ReadIagTecplot/sources.h",
        "module/read/ReadIagTecplot/mesh.h",
    ]

    for i in include:
        if file.startswith(i):
            return True

    if not file.endswith(".h"):
        return False

    for e in exclude:
        if file.startswith(e):
            return False

    return True

# generate a name for the header guard
def guard_name(file):
    if os.name == "nt": # windows   
        file = file.replace("\\", "/")

    prefix = None
    if file.startswith("lib/vistle/"):
        prefix = "lib/vistle/"
    if file.startswith("app/"):
        prefix = "app/"
    if file.startswith("module/"):
        prefix = re.sub(r'^(module/[^/]+/).*$', r'\1', file)
    if not prefix:
        return None

    file = file[len(prefix):] 
    return "VISTLE_" + file.replace("/", "_").replace(".", "_").upper()

# compress sequences of white space
def compress_whitespace(s):
    return re.sub(r'\s+', ' ', s).strip()

# check and update one file for consistent header guaurd
def check_one(file):
    if not requires_guard(file):
        return True

    guard = guard_name(file)
    if not guard:
        return True

    if not os.path.isfile(file):
        fatal(f"{file} must be a file")
    if not os.access(file, os.R_OK):
        fatal(f"{file} must be readable")

    with open(file, 'r') as f:
        lines = f.readlines()

    first = compress_whitespace(lines[0]) if len(lines) > 0 else ""
    second = compress_whitespace(lines[1]) if len(lines) > 1 else ""
    third = compress_whitespace(lines[2]) if len(lines) > 2 else ""
    last = compress_whitespace(lines[-1]) if len(lines) > 0 else ""

    if not first.startswith("#ifndef"):
        print(f"{file}: first line is not include guard: {first}")
        return False
    if not second.startswith("#define"):
        print(f"{file}: second line does not define include guard: {second}")
        return False
    if not last.startswith("#endif"):
        print(f"{file}: last line does not end include guard: {last}")
        return False

    require_update = False
    actualguard = first.split()[1]
    if guard != actualguard:
        print(f"{file}: actual guard {actualguard}, expected {guard}")
        require_update = True

    if first != f"#ifndef {actualguard}":
        print(f"{file}: first line is {first} instead of #ifndef {actualguard}")
        require_update = True

    if second != f"#define {actualguard}":
        print(f"{file}: second line is {second} instead of #define {actualguard}")
        require_update = True

    insert_empty = False
    if third != "":
        print(f"{file}: third line is not empty: {third}")
        require_update = True
        insert_empty = True

    if require_update:
        update_one(file, insert_empty, guard)

    return require_update

# update the C++ header `file` with include guard `guard`, inserting an additional empty line after if `insert_empty`
def update_one(file, insert_empty, guard):
    with open(file, 'r') as f:
        lines = f.readlines()

    with open(file, 'w') as f:
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n")
        if insert_empty != 0:
            f.write("\n")
        f.writelines(lines[2:-1])
        f.write("#endif\n")


if __name__ == "__main__":
    if not os.path.isfile("LICENSE.txt"):
        fatal("script must be run from Vistle source root directory")

    if len(sys.argv) > 1:
        for f in sys.argv[1:]:
            check_one(f)
    else:
        for d in ["lib/vistle", "app", "module"]:
            for root, _, files in os.walk(d):
                for file in files:
                    if file.endswith(".h"):
                        check_one(os.path.join(root, file))
