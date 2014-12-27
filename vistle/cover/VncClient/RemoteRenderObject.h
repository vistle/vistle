/**\file
 * \brief class RemoteRenderObject
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013 HLRS
 *
 * \copyright GPL2+
 */

#ifndef REMOTE_RENDER_OBJECT_H
#define REMOTE_RENDER_OBJECT_H

#include <kernel/RenderObject.h>

//! proxy RenderObject for RenderObjects residing on server
class RemoteRenderObject: public opencover::RenderObject {

   public:
   RemoteRenderObject(const char *name); // container object
   ~RemoteRenderObject();

   void ref();
   void unref();

   const char *getName() const;

   bool isGeometry() const;
   RenderObject *getGeometry() const;
   RenderObject *getColors() const;
   RenderObject *getNormals() const;
   RenderObject *getTexture() const;
   RenderObject *getVertexAttribute() const;

   void setGeometry(RemoteRenderObject *);
   void setColors(RemoteRenderObject *);
   void setNormals(RemoteRenderObject *);
   void setTexture(RemoteRenderObject *);

   const char *getAttribute(const char *) const;
   size_t getNumAttributes() const;
   const char *getAttributeName(size_t idx) const;
   const char *getAttributeValue(size_t idx) const;

   void addAttribute(const std::string &name, const std::string &value);

   //XXX: hacks for Volume plugin and Tracer
   bool isSet() const { return false; }
   size_t getNumElements() const { return 0; }
   RenderObject *getElement(size_t idx) const { return NULL; }

   bool isUniformGrid() const { return false; }
   void getSize(int &nx, int &ny, int &nz) const { nx = ny = nz = 0; }
   void getMinMax(float &xmin, float &xmax,
         float &ymin, float &ymax,
         float &zmin, float &zmax) const { xmin=xmax=ymin=ymax=zmin=zmax=0.; }

   bool isVectors() const { return false; }
   const unsigned char *getByte(opencover::Field::Id idx) const { return NULL; }
   const int *getInt(opencover::Field::Id idx) const { return NULL; }
   const float *getFloat(opencover::Field::Id idx) const { return NULL; }

   bool isUnstructuredGrid() const { return false; }

   protected:

   int m_refcount;
   std::string m_name;

   RemoteRenderObject *m_geo;
   RemoteRenderObject *m_col;
   RemoteRenderObject *m_norm;
   RemoteRenderObject *m_tex;

   typedef std::map<std::string, std::string> AttributeMap;
   AttributeMap m_attributes;
};
#endif
