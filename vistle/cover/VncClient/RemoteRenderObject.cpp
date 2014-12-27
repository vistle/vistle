/**\file
 * \brief class RemoteRenderObject
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013 HLRS
 *
 * \copyright GPL2+
 */

#include "RemoteRenderObject.h"

using namespace opencover;

RemoteRenderObject::RemoteRenderObject(const char *name)
: m_refcount(0)
, m_name(name ? name : "")
, m_geo(NULL)
, m_col(NULL)
, m_norm(NULL)
, m_tex(NULL)
{
}

RemoteRenderObject::~RemoteRenderObject() {

   if (m_geo) m_geo->unref();
   if (m_norm) m_norm->unref();
   if (m_col) m_col->unref();
   if (m_tex) m_tex->unref();
}

void RemoteRenderObject::ref() {

   ++m_refcount;
}

void RemoteRenderObject::unref() {

   --m_refcount;
   if (m_refcount == 0)
      delete this;
}

const char *RemoteRenderObject::getName() const {

   return m_name.c_str();
}


bool RemoteRenderObject::isGeometry() const {

   return m_geo;
}

void RemoteRenderObject::setGeometry(RemoteRenderObject *obj) {

   if (m_geo) m_geo->unref();
   m_geo = obj;
   m_geo->ref();
}

RenderObject *RemoteRenderObject::getGeometry() const {

   return m_geo;
}

void RemoteRenderObject::setNormals(RemoteRenderObject *obj) {

   if (m_norm) m_norm->unref();
   m_norm = obj;
   m_norm->ref();
}

RenderObject *RemoteRenderObject::getNormals() const {

   return m_norm;
}

void RemoteRenderObject::setColors(RemoteRenderObject *obj) {

   if (m_col) m_col->unref();
   m_col = obj;
   m_col->ref();
}

RenderObject *RemoteRenderObject::getColors() const {

   return m_col;
}

void RemoteRenderObject::setTexture(RemoteRenderObject *obj) {

   if (m_tex) m_tex->unref();
   m_tex = obj;
   m_tex->ref();
}

RenderObject *RemoteRenderObject::getTexture() const {

   return m_tex;
}

RenderObject *RemoteRenderObject::getVertexAttribute() const {

   return NULL;
}

size_t RemoteRenderObject::getNumAttributes() const {

   return m_attributes.size();
}

const char *RemoteRenderObject::getAttributeName(size_t idx) const {

   return NULL;
}

const char *RemoteRenderObject::getAttributeValue(size_t idx) const {

   return NULL;
}

const char *RemoteRenderObject::getAttribute(const char *attr) const {

   AttributeMap::const_iterator it = m_attributes.find(attr);
   if (it == m_attributes.end())
      return NULL;

   //std::cerr << "attr: " << attr << " -> " << it->second.c_str() << std::endl;
   return it->second.c_str();
}

void RemoteRenderObject::addAttribute(const std::string &name, const std::string &value) {

   m_attributes[name] = value;
   //std::cerr << "attr: " << name << " -> " << value << "|" << getAttribute(name.c_str()) << std::endl;
}
