vistle_category("Render" "rendere pixel images from geometry data")

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

add_subdirectory(OsgRenderer)
add_subdirectory(BlenderRenderer)
add_subdirectory(DisCOVERay)
add_subdirectory(ANARemote)
# last for static module registration
add_subdirectory(COVER)
