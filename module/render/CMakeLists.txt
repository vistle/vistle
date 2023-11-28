# modules producing images from geometry
set(VISTLE_MODULE_CATEGORY "Render")

vistle_find_package(
    OpenSceneGraph
    3.2.0
    COMPONENTS
    osgViewer
    osgGA
    osgDB
    osgUtil
    osgText
    osgParticle
    osgSim
    osgFX)

add_subdirectory(COVER)
add_subdirectory(OsgRenderer)
add_subdirectory(BlenderRenderer)
add_subdirectory(DisCOVERay)
add_subdirectory(ANARemote)
