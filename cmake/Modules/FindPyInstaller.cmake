# Find PyInstaller (PI)
# Will find the path to Makespec.py and Build.py

# python Makespec.py [opts] <scriptname> [<scriptname> ...]
# python Build.py specfile
find_path(
    PyInstaller_PATH Makespec.py
    # TODO an env var should be easier to maintain
    $ENV{HOME}/tools/pyinstaller $ENV{HOMEDRIVE}/Python26/tools/pyinstaller /tmp/pyinstaller
    DOC "Path to the pyinstaller directory (where to find Makespec.py)")

# $ python Makespec.py hello.py
# -> wrote /home/mmalaterre/Projects/pyinstaller/hello/hello.spec
set(PI_MAKESPEC ${PyInstaller_PATH}/Makespec.py)

set(PI_BUILD_SPEC ${PyInstaller_PATH}/Build.py)
