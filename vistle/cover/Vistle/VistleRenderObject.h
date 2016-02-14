#ifndef VISTLERENDEROBJECT_H
#define VISTLERENDEROBJECT_H

#include <cover/RenderObject.h>

#include <core/polygons.h>
#include <core/normals.h>

#include <osg/ref_ptr>
#include <osg/Node>
#include <renderer/renderobject.h>
#include <boost/weak_ptr.hpp>

namespace opencover {
class coInteractor;
}

class BaseRenderObject: public opencover::RenderObject {

   public:
   BaseRenderObject();
   ~BaseRenderObject();

   //XXX: hacks for Volume plugin and Tracer
   bool isSet() const override { return false; }
   size_t getNumElements() const override { return 0; }
   RenderObject *getElement(size_t idx) const override { return NULL; }

   bool isUniformGrid() const override { return false; }
   void getSize(int &nx, int &ny, int &nz) const override { nx = ny = nz = 0; }
   void getMinMax(float &xmin, float &xmax,
         float &ymin, float &ymax,
         float &zmin, float &zmax) const override { xmin=xmax=ymin=ymax=zmin=zmax=0.; }
   float getMin(int channel) const override { return 0.; }
   float getMax(int channel) const override { return 0.; }

   bool isVectors() const override { return false; }
   const unsigned char *getByte(opencover::Field::Id idx) const override { return NULL; }
   const int *getInt(opencover::Field::Id idx) const override { return NULL; }
   const float *getFloat(opencover::Field::Id idx) const override { return NULL; }

   bool isUnstructuredGrid() const override { return false; }

};


class VistleRenderObject: public BaseRenderObject {

   public:
   VistleRenderObject(boost::shared_ptr<const vistle::RenderObject> ro);
   VistleRenderObject(vistle::Object::const_ptr obj);
   ~VistleRenderObject();

   void setNode(osg::Node *node);
   osg::Node *node() const;
   boost::shared_ptr<const vistle::RenderObject> renderObject() const;
   int getCreator() const;

   const char *getName() const override;
   bool isGeometry() const override;
   RenderObject *getGeometry() const override;
   RenderObject *getColors() const override;
   RenderObject *getNormals() const override;
   RenderObject *getTexture() const override;
   RenderObject *getVertexAttribute() const override;

   const char *getAttribute(const char *) const override;
   size_t getNumAttributes() const override;
   const char *getAttributeName(size_t idx) const override;
   const char *getAttributeValue(size_t idx) const override;

   protected:

   boost::weak_ptr<const vistle::RenderObject> m_vistleRo;
   vistle::Object::const_ptr m_obj;
   osg::ref_ptr<osg::Node> m_node;

   mutable std::string m_name;
   mutable opencover::RenderObject *m_roGeo;
   mutable opencover::RenderObject *m_roCol;
   mutable opencover::RenderObject *m_roNorm;
   mutable opencover::RenderObject *m_roTex;
};

// pseudo RenderObject for handling module parameters
class ModuleRenderObject: public BaseRenderObject {

 public:
   ModuleRenderObject(const std::string &moduleName, int moduleId);
   ~ModuleRenderObject();

   const char *getName() const;

   bool isGeometry() const;
   RenderObject *getGeometry() const;
   RenderObject *getColors() const;
   RenderObject *getNormals() const;
   RenderObject *getTexture() const;
   RenderObject *getVertexAttribute() const;

   const char *getAttribute(const char *) const;
   size_t getNumAttributes() const;
   const char *getAttributeName(size_t idx) const;
   const char *getAttributeValue(size_t idx) const;

 private:
   std::string m_moduleName;
   int m_moduleId;
   std::string m_name;
};
#endif
