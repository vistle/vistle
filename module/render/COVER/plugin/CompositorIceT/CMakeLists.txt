if(NOT VISTLE_USE_MPI)
    return()
endif()

using(OpenGL)
using(Boost)

vistle_find_package(GLEW)
if(GLEW_FOUND AND OPENGL_FOUND)
    if(VISTLE_CUDA_ENABLED)
        set(USE_CUDA TRUE)
        set(RHR_CUDA_SOURCES ReadBackCuda.cu)
    else()
        set(RHR_CUDA_SOURCES ReadBackCuda.cpp)
    endif()
endif()

include_directories(SYSTEM ${ICET_INCLUDE_DIRS})

set(HEADERS CompositorIceT.h)
set(SOURCES CompositorIceT.cpp ${RHR_CUDA_SOURCES})

cover_add_plugin(CompositorIceT ${HEADER} ${SOURCES})
target_link_libraries(CompositorIceT vistle_rhr VistlePluginUtil ${ICET_CORE_LIBS} ${ICET_MPI_LIBS} MPI::MPI_CXX)

if(USE_CUDA)
    target_link_libraries(CompositorIceT ${CUDA_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
    target_compile_definitions(CompositorIceT PRIVATE HAVE_CUDA)
    target_include_directories(CompositorIceT PRIVATE SYSTEM ${CUDA_INCLUDE_DIRS} ${GLEW_INCLUDES} ${OPENGL_INCLUDES})
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(CompositorIceT PRIVATE "-Wno-pedantic")
    endif()
endif()
