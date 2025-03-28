include(FindPackageHandleStandardArgs)

# We are likely to find Sphinx near the Python interpreter
find_package(Python COMPONENTS Interpreter)
if(Python_FOUND)
    message("Found Python ${Python_EXECUTABLE} version ${Python_VERSION}")
    get_filename_component(_PYTHON_DIR "${Python_EXECUTABLE}" DIRECTORY)
    set(_PYTHON_PATHS "${_PYTHON_DIR}" "${_PYTHON_DIR}/bin" "${_PYTHON_DIR}/Scripts")
else()
    message(STATUS "Python not found")
endif()

find_program(
    SPHINX_EXECUTABLE
    NAMES sphinx-build sphinx-build.exe
    HINTS ${_PYTHON_PATHS})
mark_as_advanced(SPHINX_EXECUTABLE)

find_package_handle_standard_args(Sphinx DEFAULT_MSG SPHINX_EXECUTABLE)
