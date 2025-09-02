#include "VisObjectSensor.h"

#include <vrml97/vrml/VrmlNamespace.h>
#include <vrml97/vrml/VrmlScene.h>
#include <vrml97/vrml/System.h>

#include <VistlePluginUtil/VistleRenderObject.h>
#include <vistle/module/module.h>

using namespace opencover;

namespace {
const char VrmlNodeName[] = "VisObjectSensor";
VisObjectSensorPlugin *plugin = nullptr;
vistle::Matrix4 VistleToVrml, VrmlToVistle;
} // namespace

VisObjectSensorPlugin::VisObjectSensorPlugin(): opencover::coVRPlugin(VrmlNodeName)
{
    assert(!plugin);
    plugin = this;
    VistleToVrml = vistle::Matrix4::Identity(4, 4);
    vistle::Vector4 col2 = VistleToVrml.col(2);
    VistleToVrml.col(2) = VistleToVrml.col(1);
    VistleToVrml.col(1) = -col2;
    VrmlToVistle = VistleToVrml.inverse();
}

VisObjectSensorPlugin::~VisObjectSensorPlugin()
{
    plugin = nullptr;
}

bool VisObjectSensorPlugin::init()
{
    vrml::VrmlNamespace::addBuiltIn(VrmlNodeVisObjectSensor::defineType<VrmlNodeVisObjectSensor>());
    return true;
}

bool VisObjectSensorPlugin::destroy()
{
    return m_nodes.empty();
}

bool VisObjectSensorPlugin::update()
{
    return false;
}

void VisObjectSensorPlugin::setTimestep(int ts)
{
    m_timestep = ts;
    updateAllNodes();
}

void VisObjectSensorPlugin::addObject(const RenderObject *container, osg::Group *parent, const RenderObject *geometry,
                                      const RenderObject *normals, const RenderObject *colors,
                                      const RenderObject *texture)
{
    if (!container)
        return;
    auto *visObject = dynamic_cast<const VistleRenderObject *>(container);
    if (!visObject)
        return;
    auto ro = visObject->renderObject();
    if (!ro)
        return;
    auto obj = ro->container;
    if (!obj)
        return;
    auto xformName = visObject->getAttribute(vistle::attribute::TransformName);
    if (!xformName)
        return;
    auto xform = obj->getTransform();
    auto ts = vistle::getTimestep(obj);
    if (ts < 0) {
        return;
    }

    auto key = visObject->getName();
    auto it_insert = m_objToData.emplace(key, XformData{xform, ts, xformName, visObject->getName()});
    if (!it_insert.second) {
        // object already exists, update transform and timestep
        auto &data = it_insert.first->second;
        data.transform = xform;
        data.timestep = ts;
        data.transformName = xformName;
    }
    auto it = it_insert.first;
    auto &data = it->second;

    auto &xforms = m_xformToData[xformName];
    if (ts >= xforms.size()) {
        xforms.resize(ts + 1, nullptr);
    }
    xforms[ts] = &data;
}

void VisObjectSensorPlugin::removeObject(const char *objName, bool replaceFlag)

{
    auto it = m_objToData.find(objName);
    if (it == m_objToData.end()) {
        return;
    }

    auto &data = it->second;
    auto xit = m_xformToData.find(data.transformName);
    if (xit != m_xformToData.end()) {
        auto &xforms = xit->second;
        if (xforms.size() > data.timestep) {
            xforms[data.timestep] = nullptr;
        }
        while (!xforms.empty() && xforms.back() == nullptr) {
            xforms.pop_back();
        }
        if (xforms.empty()) {
            m_xformToData.erase(xit);
        }
    }
    m_objToData.erase(it);
}

void VisObjectSensorPlugin::updateAllNodes()
{
    for (auto &node: m_nodes) {
        if (node) {
            node->updateTransform();
        }
    }
}

void VrmlNodeVisObjectSensor::initFields(VrmlNodeVisObjectSensor *node, vrml::VrmlNodeType *t)
{
    vrml::VrmlNode::initFields(node, t);
    // clang-format off
    initFieldsHelper(node, t,
                     field("transformName", node->d_transformName, [node](auto f) {
                         node->updateTransform();
                     }),
                     exposedField("translation", node->d_translation),
                     exposedField("rotation", node->d_rotation),
                     exposedField("scale", node->d_scale));
    // clang-format on

    if (t) {
        t->addEventOut("translation_changed", vrml::VrmlField::SFVEC3F);
        t->addEventOut("rotation_changed", vrml::VrmlField::SFROTATION);
        t->addEventOut("scale_changed", vrml::VrmlField::SFVEC3F);
    }
}
const char *VrmlNodeVisObjectSensor::typeName()
{
    return VrmlNodeName;
}

VrmlNodeVisObjectSensor::VrmlNodeVisObjectSensor(vrml::VrmlScene *scene)
: VrmlNode(scene, VrmlNodeName), d_transformName(""), d_scale(1, 1, 1)
{
    plugin->m_nodes.insert(this);
    updateTransform();
    setModified();
}

VrmlNodeVisObjectSensor::VrmlNodeVisObjectSensor(const VrmlNodeVisObjectSensor &n)
: VrmlNode(n)
, d_transformName(n.d_transformName)
, d_translation(n.d_translation)
, d_rotation(n.d_rotation)
, d_scale(n.d_scale)
{
    plugin->m_nodes.insert(this);
    updateTransform();
    setModified();
}

VrmlNodeVisObjectSensor::~VrmlNodeVisObjectSensor()
{
    plugin->m_nodes.erase(this);
}

void VrmlNodeVisObjectSensor::render(vrml::Viewer *viewer)
{
    VrmlNode::render(viewer);

    auto timeNow = vrml::System::the->time();
    if (m_translationChanged) {
        eventOut(timeNow, "translation_changed", d_translation);
        m_translationChanged = false;
    }
    if (m_rotationChanged) {
        eventOut(timeNow, "rotation_changed", d_rotation);
        m_rotationChanged = false;
    }
    if (m_scaleChanged) {
        eventOut(timeNow, "scale_changed", d_scale);
        m_scaleChanged = false;
    }

    clearModified();
}

void VrmlNodeVisObjectSensor::updateTransform()
{
    //static const vistle::Matrix4 ToVrml = vistle::Matrix4::Rotation(0, 0, 1, M_PI / 2);
    static const vistle::Matrix4 ToVrml = vistle::Matrix4::Identity(4, 4);

    auto resetTransform = [this]() {
        if (d_translation.x() != 0 || d_translation.y() != 0 || d_translation.z() != 0)
            m_translationChanged = true;
        d_translation.set(0, 0, 0);
        if (d_rotation.x() != 0 || d_rotation.y() != 0 || d_rotation.z() != 1 || d_rotation.r() != 0)
            m_rotationChanged = true;
        d_rotation.set(0, 0, 1, 0);
        if (d_scale.x() != 1 || d_scale.y() != 1 || d_scale.z() != 1)
            m_scaleChanged = true;
        d_scale.set(1, 1, 1);
        if (m_translationChanged || m_rotationChanged || m_scaleChanged) {
            setModified();
        }
    };

    auto ts = plugin->m_timestep;
    if (ts < 0) {
        resetTransform();
        return;
    }

    auto xformName = d_transformName.get();
    auto it = plugin->m_xformToData.find(xformName);
    if (it == plugin->m_xformToData.end()) {
        resetTransform();
        return;
    }

    auto &xforms = it->second;
    if (ts >= xforms.size() || xforms[ts] == nullptr) {
        // keep the last transform
        return;
    }

    auto *data = xforms[ts];
    vistle::Matrix4 transform = data->transform;
    transform = VistleToVrml * transform; // convert from vistle to vrml coordinate system
    m_translationChanged = true;
    d_translation.set(transform(0, 3), transform(1, 3), transform(2, 3));

    if (m_translationChanged || m_rotationChanged || m_scaleChanged) {
        setModified();
    }
}

COVERPLUGIN(VisObjectSensorPlugin)
