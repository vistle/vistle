#include "VistleRenderObject.h"

using namespace opencover;
using namespace vistle;

BaseRenderObject::BaseRenderObject() {
}

BaseRenderObject::~BaseRenderObject() {
}

VistleRenderObject::VistleRenderObject(const vistle::Object::const_ptr &geo)
: m_obj(geo)
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

const char *VistleRenderObject::getName() const {

   if (m_name.empty()) {
      if (m_obj)
         m_name = m_obj->getName();
      else if (m_geo)
         m_name = m_geo->getName() + "_container";
      else
         m_name = "UNDEFINED";
   }

   return m_name.c_str();
}

void VistleRenderObject::setNode(osg::Node *node) {

   m_node = node;
}

osg::Node *VistleRenderObject::node() const {

   return m_node.get();
}


bool VistleRenderObject::isGeometry() const {

   return m_obj && m_geo;
}

void VistleRenderObject::setGeometry(Object::const_ptr obj) {

   m_geo = obj;
   if (obj)
      m_roGeo = new VistleRenderObject(obj);
}

RenderObject *VistleRenderObject::getGeometry() const {

   return m_roGeo;
}

void VistleRenderObject::setNormals(Object::const_ptr obj) {

   m_norm = obj;
   if (obj)
      m_roNorm = new VistleRenderObject(obj);
}

RenderObject *VistleRenderObject::getNormals() const {

   return m_roNorm;
}

void VistleRenderObject::setColors(Object::const_ptr obj) {

   m_col = obj;
   if (obj)
      m_roCol = new VistleRenderObject(obj);
}

RenderObject *VistleRenderObject::getColors() const {

   return m_roCol;
}

void VistleRenderObject::setTexture(Object::const_ptr obj) {

   m_tex = obj;
   if (obj)
      m_roTex = new VistleRenderObject(obj);
}

RenderObject *VistleRenderObject::getTexture() const {

   return m_roTex;
}

RenderObject *VistleRenderObject::getVertexAttribute() const {

   return NULL;
}

size_t VistleRenderObject::getNumAttributes() const {

   return 0;
}

const char *VistleRenderObject::getAttributeName(size_t idx) const {

   return NULL;
}

const char *VistleRenderObject::getAttributeValue(size_t idx) const {

   return NULL;
}

const char *VistleRenderObject::getAttribute(const char *attr) const {

   static std::string val;
   val = m_geo->getAttribute(attr).c_str();
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

size_t ModuleRenderObject::getNumAttributes() const
{
   return 1;
}

const char *ModuleRenderObject::getAttributeName(size_t idx) const
{
   if (idx == 0)
      return "OBJECTNAME";

   return NULL;
}

const char *ModuleRenderObject::getAttributeValue(size_t idx) const
{
   if (idx == 0)
      return getName();

   return NULL;
}
