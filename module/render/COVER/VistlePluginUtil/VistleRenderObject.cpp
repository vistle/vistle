#include "VistleRenderObject.h"

#include <cover/coVRPluginList.h>

#include <vistle/core/placeholder.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/message.h>

using namespace opencover;

BaseRenderObject::BaseRenderObject()
{}

BaseRenderObject::~BaseRenderObject()
{}

VistleRenderObject::VistleRenderObject(std::shared_ptr<const vistle::RenderObject> ro)
: m_vistleRo(ro)
, m_roGeo(ro->geometry ? new VistleRenderObject(ro->geometry) : nullptr)
, m_roData(ro->mapdata ? new VistleRenderObject(ro->mapdata) : nullptr)
, m_roNorm(ro->normals ? new VistleRenderObject(ro->normals) : nullptr)
{}

VistleRenderObject::VistleRenderObject(vistle::Object::const_ptr obj)
: m_obj(obj), m_roGeo(NULL), m_roData(NULL), m_roNorm(NULL)
{
    osg::Matrix osgMat;
    vistle::Matrix4 vistleMat = obj->getTransform();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            osgMat(i, j) = vistleMat.col(i)[j];
        }
    }
    transform = osgMat;
}

VistleRenderObject::~VistleRenderObject()
{
    delete m_roGeo;
    delete m_roData;
    delete m_roNorm;
}

std::shared_ptr<const vistle::RenderObject> VistleRenderObject::renderObject() const
{
    return m_vistleRo.lock();
}

const char *VistleRenderObject::getName() const
{
    if (m_name.empty()) {
        m_name = "UNDEFINED";
        if (m_obj) {
            if (auto ph = vistle::PlaceHolder::as(m_obj))
                m_name = ph->originalName();
            else
                m_name = m_obj->getName();
        } else if (auto ro = renderObject()) {
            if (ro->container) {
                if (auto ph = vistle::PlaceHolder::as(ro->container))
                    m_name = ph->originalName();
                else
                    m_name = ro->container->getName();
            } else if (ro->geometry) {
                if (auto ph = vistle::PlaceHolder::as(ro->geometry))
                    m_name = ph->originalName();
                else
                    m_name = ro->geometry->getName();
                m_name += "_container";
            }
        }
    }

    return m_name.c_str();
}

bool VistleRenderObject::isPlaceHolder() const
{
    if (vistle::PlaceHolder::as(m_obj))
        return true;
    return false;
}

bool VistleRenderObject::isUniformGrid() const
{
    if (vistle::UniformGrid::as(m_obj))
        return true;
    return false;
}

int VistleRenderObject::getCreator() const
{
    int creator = vistle::message::Id::Invalid;
    auto ro = renderObject();
    if (!ro)
        return creator;

    if (ro->container) {
        creator = ro->container->getCreator();
        if (creator == vistle::message::Id::Invalid)
            std::cerr << "VistleRenderObject::getCreator: invalid id on container" << std::endl;
    }
    if (creator == vistle::message::Id::Invalid && ro->geometry) {
        creator = ro->geometry->getCreator();
        if (creator == vistle::message::Id::Invalid)
            std::cerr << "VistleRenderObject::getCreator: invalid id on geometry" << std::endl;
    }

    return creator;
}

void VistleRenderObject::setNode(osg::Node *node)
{
    m_node = node;
}

osg::Node *VistleRenderObject::node() const
{
    return m_node.get();
}


bool VistleRenderObject::isGeometry() const
{
    auto ro = renderObject();
    return ro && ro->container && ro->geometry;
}

opencover::RenderObject *VistleRenderObject::getGeometry() const
{
    return m_roGeo;
}

RenderObject *VistleRenderObject::getNormals() const
{
    return m_roNorm;
}

RenderObject *VistleRenderObject::getColors() const
{
    return m_roData;
}

RenderObject *VistleRenderObject::getTexture() const
{
    return m_roData;
}

RenderObject *VistleRenderObject::getVertexAttribute() const
{
    return NULL;
}

void VistleRenderObject::getSize(int &nx, int &ny, int &nz) const
{
    nx = ny = nz = 0;

    if (!m_obj)
        return;

    auto str = m_obj->getInterface<vistle::StructuredGridBase>();
    if (!str)
        return;

    nx = str->getNumDivisions(0);
    ny = str->getNumDivisions(1);
    nz = str->getNumDivisions(2);
}

void VistleRenderObject::getMinMax(float &xmin, float &xmax, float &ymin, float &ymax, float &zmin, float &zmax) const
{
    xmin = 1;
    xmax = -1;
    ymin = 1;
    ymax = -1;
    zmin = 1;
    zmax = -1;

    if (!m_obj)
        return;

    auto uni = vistle::UniformGrid::as(m_obj);
    if (!uni)
        return;

    auto bounds = uni->getBounds();
    xmin = bounds.first[0];
    ymin = bounds.first[1];
    zmin = bounds.first[2];
    xmax = bounds.second[0];
    ymax = bounds.second[1];
    zmax = bounds.second[2];
}

float VistleRenderObject::getMin(int channel) const
{
    if (!m_obj)
        return 0.;

    if (auto scal = vistle::Vec<float>::as(m_obj)) {
        if (channel == Field::X || channel == Field::Channel0 || channel == Field::Red) {
            auto mm = scal->getMinMax();
            return mm.first[0];
        }
    }

    return 0.;
}

float VistleRenderObject::getMax(int channel) const
{
    if (!m_obj)
        return 0.;

    if (auto scal = vistle::Vec<float>::as(m_obj)) {
        if (channel == Field::X || channel == Field::Channel0 || channel == Field::Red) {
            auto mm = scal->getMinMax();
            return mm.second[0];
        }
    }

    return 0.;
}

const float *VistleRenderObject::getFloat(Field::Id idx) const
{
    if (!m_obj)
        return nullptr;

    if (auto scal = vistle::Vec<float>::as(m_obj)) {
        if (idx == Field::X || idx == Field::Channel0 || idx == Field::Red) {
            return scal->x().data();
        }
    }

    return nullptr;
}

const char *VistleRenderObject::getAttribute(const char *attr) const
{
    bool hasAttribute = false;
    static std::string val;
    val.clear();
    if (m_obj) {
        if (m_obj->hasAttribute(attr)) {
            val = m_obj->getAttribute(attr);
            hasAttribute = true;
        } else {
            if (auto data = vistle::DataBase::as(m_obj)) {
                if (data->grid() && data->grid()->hasAttribute(attr)) {
                    val = data->grid()->getAttribute(attr);
                    hasAttribute = true;
                }
            }
        }
    } else if (auto ro = renderObject()) {
        if (ro->container && ro->container->hasAttribute(attr)) {
            val = ro->container->getAttribute(attr);
            hasAttribute = true;
        }
        if (!hasAttribute && ro->geometry && ro->geometry->hasAttribute(attr)) {
            val = ro->geometry->getAttribute(attr);
            hasAttribute = true;
        }
    }
    return hasAttribute ? val.c_str() : nullptr;
}


ModuleRenderObject::ModuleRenderObject(const std::string &moduleName, int moduleId)
: m_moduleName(moduleName), m_moduleId(moduleId)
{
    std::stringstream str;
    str << m_moduleName << "_" << m_moduleId;
    m_name = str.str();
}

ModuleRenderObject::~ModuleRenderObject()
{
    coVRPluginList::instance()->removeObject(getName(), false);
}

const char *ModuleRenderObject::getName() const
{
    return m_name.c_str();
}

bool ModuleRenderObject::isGeometry() const
{
    return false;
}

RenderObject *ModuleRenderObject::getGeometry() const
{
    return NULL;
}

RenderObject *ModuleRenderObject::getColors() const
{
    return NULL;
}

RenderObject *ModuleRenderObject::getNormals() const
{
    return NULL;
}

RenderObject *ModuleRenderObject::getTexture() const
{
    return NULL;
}

RenderObject *ModuleRenderObject::getVertexAttribute() const
{
    return NULL;
}

const char *ModuleRenderObject::getAttribute(const char *attrname) const
{
    auto it = m_attributes.find(attrname);
    if (it != m_attributes.end()) {
        return it->second.c_str();
    }

    if (!strcmp(attrname, "OBJECTNAME"))
        return getName();

    return NULL;
}

void ModuleRenderObject::addAttribute(const std::string &key, const std::string &value)
{
    m_attributes[key] = value;
}

void ModuleRenderObject::removeAttribute(const std::string &key)
{
    m_attributes.erase(key);
}

VariantRenderObject::VariantRenderObject(const std::string &variantName,
                                         vistle::RenderObject::InitialVariantVisibility visible)
: variant(variantName), m_node(new osg::Group), m_visible(visible)
{
    std::string nn("RhrClient:Variant:");
    m_node->setName(nn + variant);
}

void VariantRenderObject::setInitialVisibility(vistle::RenderObject::InitialVariantVisibility visible)
{
    m_visible = visible;
}

const char *VariantRenderObject::getAttribute(const char *key) const
{
    if (key) {
        std::string k(key);
        if (k == "VARIANT") {
            return variant.c_str();
        }
        if (k == "VARIANT_VISIBLE") {
            if (m_visible == vistle::RenderObject::Hidden)
                return "off";
            if (m_visible == vistle::RenderObject::Visible)
                return "on";
        }
    }
    return nullptr;
}

osg::ref_ptr<osg::Node> VariantRenderObject::node() const
{
    return m_node;
}
