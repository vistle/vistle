#ifndef VISTLEGEOMETRYGENERATOR_H
#define VISTLEGEOMETRYGENERATOR_H

#include <core/object.h>
#include <renderer/renderobject.h>
#include <mutex>
#include <osg/StateSet>
#include <osg/ref_ptr>

namespace osg {
   class MatrixTransform;
};

class VistleGeometryGenerator {
   public:
      VistleGeometryGenerator(std::shared_ptr<vistle::RenderObject> ro,
            vistle::Object::const_ptr geo,
            vistle::Object::const_ptr color,
            vistle::Object::const_ptr normal,
            vistle::Object::const_ptr tex);

      osg::MatrixTransform *operator()(osg::ref_ptr<osg::StateSet> state = NULL);

      static bool isSupported(vistle::Object::Type t);

   private:
      std::shared_ptr<vistle::RenderObject> m_ro;
      vistle::Object::const_ptr m_geo;
      vistle::Object::const_ptr m_color;
      vistle::Object::const_ptr m_normal;
      vistle::Object::const_ptr m_tex;

      static std::mutex s_coverMutex;
};

#endif
