# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.0

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/tszep_local/projects/External/glew-1.12.0

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tszep_local/projects/External/glew-1.12.0/build/Linux

# Include any dependencies generated for this target.
include CMakeFiles/GLEW_MX_static.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/GLEW_MX_static.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/GLEW_MX_static.dir/flags.make

CMakeFiles/GLEW_MX_static.dir/src/glew.o: CMakeFiles/GLEW_MX_static.dir/flags.make
CMakeFiles/GLEW_MX_static.dir/src/glew.o: ../../src/glew.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/tszep_local/projects/External/glew-1.12.0/build/Linux/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object CMakeFiles/GLEW_MX_static.dir/src/glew.o"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/GLEW_MX_static.dir/src/glew.o   -c /home/tszep_local/projects/External/glew-1.12.0/src/glew.c

CMakeFiles/GLEW_MX_static.dir/src/glew.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/GLEW_MX_static.dir/src/glew.i"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -E /home/tszep_local/projects/External/glew-1.12.0/src/glew.c > CMakeFiles/GLEW_MX_static.dir/src/glew.i

CMakeFiles/GLEW_MX_static.dir/src/glew.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/GLEW_MX_static.dir/src/glew.s"
	/usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -S /home/tszep_local/projects/External/glew-1.12.0/src/glew.c -o CMakeFiles/GLEW_MX_static.dir/src/glew.s

CMakeFiles/GLEW_MX_static.dir/src/glew.o.requires:
.PHONY : CMakeFiles/GLEW_MX_static.dir/src/glew.o.requires

CMakeFiles/GLEW_MX_static.dir/src/glew.o.provides: CMakeFiles/GLEW_MX_static.dir/src/glew.o.requires
	$(MAKE) -f CMakeFiles/GLEW_MX_static.dir/build.make CMakeFiles/GLEW_MX_static.dir/src/glew.o.provides.build
.PHONY : CMakeFiles/GLEW_MX_static.dir/src/glew.o.provides

CMakeFiles/GLEW_MX_static.dir/src/glew.o.provides.build: CMakeFiles/GLEW_MX_static.dir/src/glew.o

# Object files for target GLEW_MX_static
GLEW_MX_static_OBJECTS = \
"CMakeFiles/GLEW_MX_static.dir/src/glew.o"

# External object files for target GLEW_MX_static
GLEW_MX_static_EXTERNAL_OBJECTS =

lib/libGLEWmx.a: CMakeFiles/GLEW_MX_static.dir/src/glew.o
lib/libGLEWmx.a: CMakeFiles/GLEW_MX_static.dir/build.make
lib/libGLEWmx.a: CMakeFiles/GLEW_MX_static.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C static library lib/libGLEWmx.a"
	$(CMAKE_COMMAND) -P CMakeFiles/GLEW_MX_static.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/GLEW_MX_static.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/GLEW_MX_static.dir/build: lib/libGLEWmx.a
.PHONY : CMakeFiles/GLEW_MX_static.dir/build

CMakeFiles/GLEW_MX_static.dir/requires: CMakeFiles/GLEW_MX_static.dir/src/glew.o.requires
.PHONY : CMakeFiles/GLEW_MX_static.dir/requires

CMakeFiles/GLEW_MX_static.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/GLEW_MX_static.dir/cmake_clean.cmake
.PHONY : CMakeFiles/GLEW_MX_static.dir/clean

CMakeFiles/GLEW_MX_static.dir/depend:
	cd /home/tszep_local/projects/External/glew-1.12.0/build/Linux && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tszep_local/projects/External/glew-1.12.0 /home/tszep_local/projects/External/glew-1.12.0 /home/tszep_local/projects/External/glew-1.12.0/build/Linux /home/tszep_local/projects/External/glew-1.12.0/build/Linux /home/tszep_local/projects/External/glew-1.12.0/build/Linux/CMakeFiles/GLEW_MX_static.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/GLEW_MX_static.dir/depend

