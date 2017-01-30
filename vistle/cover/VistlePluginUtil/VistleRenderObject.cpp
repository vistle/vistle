#include "VistleRenderObject.h"

#include <cover/coVRPluginList.h>

using namespace opencover;

BaseRenderObject::BaseRenderObject() {
}

BaseRenderObject::~BaseRenderObject() {
}

VistleRenderObject::VistleRenderObject(std::shared_ptr<const vistle::RenderObject> ro)
: m_vistleRo(ro)
, m_roGeo(NULL)
, m_roCol(NULL)
, m_roNorm(NULL)
, m_roTex(NULL)
{
}

VistleRenderObject::VistleRenderObject(vistle::Object::const_ptr obj)
: m_obj(obj)
, m_roGeo(NULL)
, m_roCol(NULL)
, m_roNorm(NULL)
, m_roTex(NULL)
{
}

VistleRenderObject::~VistleRenderObject() {

   delete m_roGeo;
   delete m_roCol;
   delete m_roNorm;
   delete m_roTex;
}

std::shared_ptr<const  vistle::RenderObject> VistleRenderObject::renderObject() const {

   return m_vistleRo.lock();
}

const char *VistleRenderObject::getName() const {

   if (m_name.empty()) {
      m_name = "UNDEFINED";
      if (m_obj) {
         m_name = m_obj->getName();
      } else if (auto ro = renderObject()) {
         if (ro->container)
            m_name = ro->container->getName();
         else if (ro->geometry)
            m_name = ro->geometry->getName() + "_container";
      }
   }

   return m_name.c_str();
}

int VistleRenderObject::getCreator() const {

   auto ro = renderObject();
   if (!ro)
      return 0;

   if (ro->geometry)
      return ro->geometry->getCreator();
   else if (ro->container)
      return ro->container->getCreator();

   return 0;
}

void VistleRenderObject::setNode(osg::Node *node) {

   m_node = node;
}

osg::Node *VistleRenderObject::node() const {

   return m_node.get();
}


bool VistleRenderObject::isGeometry() const {

   auto ro = renderObject();
   return ro && ro->container && ro->geometry;
}

opencover::RenderObject *VistleRenderObject::getGeometry() const {

   return m_roGeo;
}

RenderObject *VistleRenderObject::getNormals() const {

   return m_roNorm;
}

RenderObject *VistleRenderObject::getColors() const {

   return m_roCol;
}

RenderObject *VistleRenderObject::getTexture() const {

   return m_roTex;
}

RenderObject *VistleRenderObject::getVertexAttribute() const {

   return NULL;
}

const char *VistleRenderObject::getAttribute(const char *attr) const {

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
: m_moduleName(moduleName)
, m_moduleId(moduleId)
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
   if (!strcmp(attrname, "OBJECTNAME"))
      return getName();

   return NULL;
}

VariantRenderObject::VariantRenderObject(const std::string &variantName, vistle::RenderObject::InitialVariantVisibility visible)
: variant(variantName)
, m_node(new osg::Group)
, m_visible(visible)
{
    std::string nn("RhrClient:Variant:");
    m_node->setName(nn+variant);
    variant_onoff = variant;
    if (visible == vistle::RenderObject::Hidden)
        variant_onoff += "_off";
    else if (visible == vistle::RenderObject::Visible)
        variant_onoff += "_on";
}

const char *VariantRenderObject::getAttribute(const char *key) const {
    if (key) {
        std::string k(key);
        if (k == "VARIANT") {
            return variant_onoff.c_str();
        }
    }
    return nullptr;
}

osg::ref_ptr<osg::Node> VariantRenderObject::node() const {

    return m_node;
}
