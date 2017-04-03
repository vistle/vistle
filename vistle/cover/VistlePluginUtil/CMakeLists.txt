vistle_find_package(Boost 1.40 REQUIRED COMPONENTS system filesystem)
using(GLEW)

set(CR_IsUsingAVX2 1)
set(CR_IsOculusEnabled 1)
add_definitions(-DCR_IS_USING_AVX2=${CR_IsUsingAVX2})
add_definitions(-DCR_IS_OCULUS_ENABLED=${CR_IsOculusEnabled})

vistle_find_package(glm)

set(pluginutil_SOURCES
   VistleRenderObject.cpp
   MultiChannelDrawer.cpp

   CubemapReprojection/Core/DataStructures/Properties.cpp
   CubemapReprojection/Core/System/SimpleIO.cpp
   CubemapReprojection/Core/System/ThreadPool.cpp
   CubemapReprojection/EngineBuildingBlocks/Graphics/Camera/Camera.cpp
   CubemapReprojection/EngineBuildingBlocks/Graphics/Camera/CameraProjection.cpp
   CubemapReprojection/EngineBuildingBlocks/Graphics/Camera/FreeCamera.cpp
   CubemapReprojection/EngineBuildingBlocks/Graphics/Primitives/Primitive.cpp
   CubemapReprojection/EngineBuildingBlocks/Graphics/Primitives/PrimitiveCreation.cpp
   CubemapReprojection/EngineBuildingBlocks/Input/KeyHandler.cpp
   CubemapReprojection/EngineBuildingBlocks/Input/Keys.cpp
   CubemapReprojection/EngineBuildingBlocks/Input/MouseHandler.cpp
   CubemapReprojection/EngineBuildingBlocks/Input/WindowsKeys.cpp
   CubemapReprojection/EngineBuildingBlocks/Math/AABoundingBox.cpp
   CubemapReprojection/EngineBuildingBlocks/Math/BoundingFrustum.cpp
   CubemapReprojection/EngineBuildingBlocks/EventHandling.cpp
   CubemapReprojection/EngineBuildingBlocks/PathHandler.cpp
   CubemapReprojection/EngineBuildingBlocks/SceneNode.cpp
   CubemapReprojection/EngineBuildingBlocks/SystemTime.cpp
   CubemapReprojection/OpenGLRender/Resources/Buffer.cpp
   CubemapReprojection/OpenGLRender/Resources/FrameBufferObject.cpp
   CubemapReprojection/OpenGLRender/Resources/IndexBuffer.cpp
   CubemapReprojection/OpenGLRender/Resources/Shader.cpp
   CubemapReprojection/OpenGLRender/Resources/Texture.cpp
   CubemapReprojection/OpenGLRender/Resources/Texture2D.cpp
   CubemapReprojection/OpenGLRender/Resources/VertexArrayObject.cpp
   CubemapReprojection/OpenGLRender/Resources/VertexBuffer.cpp
   CubemapReprojection/OpenGLRender/OpenGL.cpp
   CubemapReprojection/CR_Message.cpp
   CubemapReprojection/CubemapReprojector.cpp
   CubemapReprojection/GridGeometry.cpp
   CubemapReprojection/GridReprojector.cpp
   CubemapReprojection/OpenGLHelper.cpp
)

set(pluginutil_HEADERS
   export.h
   VistleRenderObject.h
   MultiChannelDrawer.h
   
   CubemapReprojection/Core/DataStructures/Pool.hpp
   CubemapReprojection/Core/DataStructures/Properties.h
   CubemapReprojection/Core/DataStructures/ResourceUnorderedVector.hpp
   CubemapReprojection/Core/DataStructures/SimpleTypeUnorderedVector.hpp
   CubemapReprojection/Core/DataStructures/SimpleTypeVector.hpp
   CubemapReprojection/Core/System/Filesystem.h
   CubemapReprojection/Core/System/Semaphore.hpp
   CubemapReprojection/Core/System/SimpleIO.h
   CubemapReprojection/Core/System/ThreadPool.h
   CubemapReprojection/Core/Comparison.h
   CubemapReprojection/Core/Constants.h
   CubemapReprojection/Core/Debug.h
   CubemapReprojection/Core/Enum.h
   CubemapReprojection/Core/Functional.hpp
   CubemapReprojection/Core/ObjectCache.hpp
   CubemapReprojection/Core/Parse.hpp
   CubemapReprojection/Core/Percentage.hpp
   CubemapReprojection/Core/Platform.h
   CubemapReprojection/Core/SimpleBinarySerialization.hpp
   CubemapReprojection/Core/String.hpp
   CubemapReprojection/Core/StringStreamHelper.h
   CubemapReprojection/Core/Utility.hpp
   CubemapReprojection/Core/Windows.h
   CubemapReprojection/EngineBuildingBlocks/Graphics/Camera/Camera.h
   CubemapReprojection/EngineBuildingBlocks/Graphics/Camera/CameraProjection.h
   CubemapReprojection/EngineBuildingBlocks/Graphics/Camera/CubemapHelper.hpp
   CubemapReprojection/EngineBuildingBlocks/Graphics/Camera/FreeCamera.h
   CubemapReprojection/EngineBuildingBlocks/Graphics/Primitives/Primitive.h
   CubemapReprojection/EngineBuildingBlocks/Graphics/Primitives/PrimitiveCreation.h
   CubemapReprojection/EngineBuildingBlocks/Graphics/Resources/ResourceUtility.h
   CubemapReprojection/EngineBuildingBlocks/Graphics/Graphics.h
   CubemapReprojection/EngineBuildingBlocks/Graphics/ViewFrustumCuller.h
   CubemapReprojection/EngineBuildingBlocks/Input/DefaultInputBinder.h
   CubemapReprojection/EngineBuildingBlocks/Input/KeyHandler.h
   CubemapReprojection/EngineBuildingBlocks/Input/Keys.h
   CubemapReprojection/EngineBuildingBlocks/Input/MouseHandler.h
   CubemapReprojection/EngineBuildingBlocks/Input/WindowsKeys.h
   CubemapReprojection/EngineBuildingBlocks/Math/AABoundingBox.h
   CubemapReprojection/EngineBuildingBlocks/Math/BoundingFrustum.h
   CubemapReprojection/EngineBuildingBlocks/Math/GLM.h
   CubemapReprojection/EngineBuildingBlocks/Math/Intrinsic.h
   CubemapReprojection/EngineBuildingBlocks/Math/Matrix.h
   CubemapReprojection/EngineBuildingBlocks/Math/Plane.h
   CubemapReprojection/EngineBuildingBlocks/Math/Transformations.h
   CubemapReprojection/EngineBuildingBlocks/Math/Vector256.h
   CubemapReprojection/EngineBuildingBlocks/ErrorHandling.h
   CubemapReprojection/EngineBuildingBlocks/EventHandling.h
   CubemapReprojection/EngineBuildingBlocks/PathHandler.h
   CubemapReprojection/EngineBuildingBlocks/SceneNode.h
   CubemapReprojection/EngineBuildingBlocks/SystemTime.h
   CubemapReprojection/OpenGLRender/Resources/Buffer.h
   CubemapReprojection/OpenGLRender/Resources/FrameBufferObject.h
   CubemapReprojection/OpenGLRender/Resources/IndexBuffer.h
   CubemapReprojection/OpenGLRender/Resources/Resource.h
   CubemapReprojection/OpenGLRender/Resources/Shader.h
   CubemapReprojection/OpenGLRender/Resources/Texture.h
   CubemapReprojection/OpenGLRender/Resources/Texture2D.h
   CubemapReprojection/OpenGLRender/Resources/TextureEnums.h
   CubemapReprojection/OpenGLRender/Resources/VertexArrayObject.h
   CubemapReprojection/OpenGLRender/Resources/VertexBuffer.h
   CubemapReprojection/OpenGLRender/OpenGL.h
   CubemapReprojection/CR_Common.h
   CubemapReprojection/CR_Message.h
   CubemapReprojection/CubemapReprojector.h
   CubemapReprojection/GridGeometry.h
   CubemapReprojection/GridReprojector.h
   CubemapReprojection/OpenGLHelper.h
)

include_directories(
        ../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}
        ${COVISE_INCLUDE_DIRS}
        ${COVER_INCLUDE_DIRS}

        CubemapReprojection
		$ENV{EXTERNLIBS}
		$ENV{EXTERNLIBS}/glm
)

if(${CR_IsOculusEnabled})
	list(APPEND pluginutil_SOURCES
		CubemapReprojection/OculusVR/SimpleVR.cpp
		CubemapReprojection/OculusVR/SimpleVR_GL.cpp)
	list(APPEND pluginutil_HEADERS
		CubemapReprojection/OculusVR/GraphicsAPI.h
		CubemapReprojection/OculusVR/SimpleVR.h
		CubemapReprojection/OculusVR/SimpleVR_GL.h
		CubemapReprojection/OculusVR/SimpleVR_HelperImplementor.h)
endif()

file(GLOB_RECURSE pluginutil_SHADERS "CubemapReprojection/Shaders/*.glsl")
file(GLOB_RECURSE pluginutil_CONFIGS "CubemapReprojection/Configurations/*.xml")
source_group("Shaders" FILES ${pluginutil_SHADERS})
source_group("Configurations" FILES ${pluginutil_CONFIGS})

vistle_add_library(VistlePluginUtil ${VISTLE_LIB_TYPE} ${pluginutil_SOURCES} ${pluginutil_HEADERS} ${pluginutil_SHADERS} ${pluginutil_CONFIGS})

target_link_libraries(VistlePluginUtil
        ${Boost_LIBRARIES}
        ${OPENSCENEGRAPH_LIBRARIES}
        ${COVER_LIBRARY}
        vistle_core
)

if(${CR_IsOculusEnabled})
	target_link_libraries(VistlePluginUtil
		$ENV{EXTERNLIBS}/OculusSDK/LibOVR/Lib/Windows/x64/Release/VS2015/LibOVR.lib
		$ENV{EXTERNLIBS}/OculusSDK/LibOVRKernel/Lib/Windows/x64/Release/VS2015/LibOVRKernel.lib)
endif()