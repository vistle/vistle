set(HEADERS worklet/GenerateSeeds.h StreamlineVtkm.h)
set(SOURCES worklet/GenerateSeeds.cpp StreamlineVtkm.cpp)

add_vtkm_module(StreamlineVtkm "create streamlines for a vector field" ${HEADERS} DEVICE_SOURCES ${SOURCES})
