#ifndef VISTLE_COVER_PLUGIN_VISOBJECTSENSOR_VISOBJECTSENSOR_H
#define VISTLE_COVER_PLUGIN_VISOBJECTSENSOR_VISOBJECTSENSOR_H

#include <vrml97/vrml/VrmlNode.h>
#include <cover/coVRPlugin.h>
#include <vistle/core/vector.h>

class VrmlNodeVisObjectSensor: public vrml::VrmlNode {
public:
    static void initFields(VrmlNodeVisObjectSensor *node, vrml::VrmlNodeType *t);
    static const char *typeName();

    VrmlNodeVisObjectSensor(vrml::VrmlScene *scene = nullptr);
    VrmlNodeVisObjectSensor(const VrmlNodeVisObjectSensor &n);
    ~VrmlNodeVisObjectSensor() override;

    void render(vrml::Viewer *viewer) override;

    void updateTransform();

private:
    vrml::VrmlSFString d_transformName;
    vrml::VrmlSFVec3f d_translation;
    vrml::VrmlSFRotation d_rotation;
    vrml::VrmlSFVec3f d_scale;
    bool m_translationChanged = false;
    bool m_rotationChanged = false;
    bool m_scaleChanged = false;
};

class VisObjectSensorPlugin: public opencover::coVRPlugin {
    friend class VrmlNodeVisObjectSensor;

public:
    VisObjectSensorPlugin();
    ~VisObjectSensorPlugin() override;
    bool init() override;
    bool destroy() override;
    bool update() override;
    void setTimestep(int ts) override;
    void addObject(const opencover::RenderObject *container, osg::Group *parent,
                   const opencover::RenderObject *geometry, const opencover::RenderObject *normals,
                   const opencover::RenderObject *colors, const opencover::RenderObject *texture) override;
    void removeObject(const char *objName, bool replaceFlag) override;

private:
    void updateAllNodes();
    struct XformData {
        vistle::Matrix4 transform = vistle::Matrix4::Identity(4, 4);
        int timestep = -1;
        std::string transformName;
        std::string objectName;
    };
    std::map<std::string, XformData> m_objToData;
    std::map<std::string, std::vector<XformData *>> m_xformToData;
    std::set<VrmlNodeVisObjectSensor *> m_nodes;
    int m_timestep = -1;
};

#endif
