#ifndef VISTLERENDEROBJECT_H
#define VISTLERENDEROBJECT_H

#include <kernel/RenderObject.h>

#include <core/polygons.h>
#include <core/normals.h>

#include <osg/ref_ptr>
#include <osg/Node>


class VistleRenderObject: public opencover::RenderObject {

   public:
   VistleRenderObject(); // container object
   VistleRenderObject(const vistle::Object::const_ptr &geo);
   ~VistleRenderObject();

   const char *getName() const;
   void setNode(osg::Node *node);
   osg::Node *node() const;

   bool isGeometry() const;
   RenderObject *getGeometry() const;
   RenderObject *getColors() const;
   RenderObject *getNormals() const;
   RenderObject *getTexture() const;
   RenderObject *getVertexAttribute() const;

   void setGeometry(vistle::Object::const_ptr obj);
   void setColors(vistle::Object::const_ptr obj);
   void setNormals(vistle::Object::const_ptr obj);
   void setTexture(vistle::Object::const_ptr obj);

   const char *getAttribute(const char *) const;

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

   mutable std::string m_name;

      vistle::Object::const_ptr m_obj;

      vistle::Object::const_ptr m_geo;
      vistle::Object::const_ptr m_norm;
      vistle::Object::const_ptr m_col;
      vistle::Object::const_ptr m_tex;

      osg::ref_ptr<osg::Node> m_node;

      mutable opencover::RenderObject *m_roGeo;
      mutable opencover::RenderObject *m_roCol;
      mutable opencover::RenderObject *m_roNorm;
      mutable opencover::RenderObject *m_roTex;
};
#endif
