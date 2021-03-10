FIND_PATH(HDF5_INCLUDE_DIR hdf5.h hdf5) 
FIND_PATH(HDF5_C++_INCLUDE_DIR H5Cpp.h H5Cpp h5cpp)

FIND_LIBRARY(HDF5_LIBRARY NAMES hdf5) 
FIND_LIBRARY(HDF5_C++_LIBRARY NAMES hdf5_cpp hdf5_c++) 

IF(MSVC)
  # VisualStudio needs a debug version
  # FIND_LIBRARY(NETCDF_C++_LIBRARY_DEBUG NAMES ${NETCDF_C++_DBG_NAMES}
  #   PATHS
  #   $ENV{NETCDF_HOME}
  #   $ENV{EXTERNLIBS}/NETCDF
  #   $ENV{EXTERNLIBS}/netcdf
  #   ~/Library/Frameworks
  #   /Library/Frameworks
  #   /usr/local
  #   /usr
  #   /sw
  #   /opt/local
  #   /opt/csw
  #   /opt
  #   PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  #   DOC "NETCDF_C++ - Library (Debug)"
  # )
  # FIND_LIBRARY(NETCDF_LIBRARY_DEBUG NAMES ${NETCDF_DBG_NAMES}
  #   PATHS
  #   $ENV{NETCDF_HOME}
  #   $ENV{EXTERNLIBS}/NETCDF
  #   $ENV{EXTERNLIBS}/netcdf
  #   ~/Library/Frameworks
  #   /Library/Frameworks
  #   /usr/local
  #   /usr
  #   /sw
  #   /opt/local
  #   /opt/csw
  #   /opt
  #   PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  #   DOC "NETCDF - Library (Debug)"
  # )
  
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(HDF5 DEFAULT_MSG HDF5_C++_LIBRARY HDF5_INCLUDE_DIR HDF5_C++_INCLUDE_DIR)
  if (HDF5_FOUND)
      SET(HDF5_LIBRARIES optimized ${HDF5_LIBRARY} optimized ${HDF5_C++_LIBRARY})
      SET(HDF5_C++_LIBRARIES optimized ${HDF_C++_LIBRARY})
  endif()

  MARK_AS_ADVANCED(HDF5_C++_LIBRARY HDF5_LIBRARY HDF5_INCLUDE_DIR HDF5_C++_INCLUDE_DIR)
  
ELSE(MSVC)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(HDF5 DEFAULT_MSG HDF5_LIBRARY HDF5_C++_LIBRARY HDF5_INCLUDE_DIR HDF5_C++_INCLUDE_DIR)
    if (HDF5_FOUND)
        SET(HDF5_LIBRARIES ${HDF5_LIBRARY} ${HDF5_C++_LIBRARY})
  endif()
ENDIF(MSVC)

if (HDF5_FOUND)
    SET(HDF5_INCLUDES ${HDF5_INCLUDE_DIR} ${HDF5_C++_INCLUDE_DIR})
endif()
