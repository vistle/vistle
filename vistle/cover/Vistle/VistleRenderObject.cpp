#include "VistleRenderObject.h"

using namespace opencover;

BaseRenderObject::BaseRenderObject() {
}

BaseRenderObject::~BaseRenderObject() {
}

VistleRenderObject::VistleRenderObject(boost::shared_ptr<const vistle::RenderObject> ro)
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

boost::shared_ptr<const  vistle::RenderObject> VistleRenderObject::renderObject() const {

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

   static std::string val;
   val.clear();
   if (m_obj) {
      val = m_obj->getAttribute(attr).c_str();
   } else if (auto ro = renderObject()) {
      if (ro->geometry)
         val = ro->geometry->getAttribute(attr).c_str();
   }
   return val.c_str();
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
