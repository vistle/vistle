#ifndef VISTLEGEOMETRYGENERATOR_H
#define VISTLEGEOMETRYGENERATOR_H

#include <core/object.h>
#include <mutex>

namespace osg {
   class Node;
};

class VistleGeometryGenerator {
   public:
      VistleGeometryGenerator(vistle::Object::const_ptr geo,
            vistle::Object::const_ptr color,
            vistle::Object::const_ptr normal,
            vistle::Object::const_ptr tex);

      osg::Node *operator()();

   private:
      vistle::Object::const_ptr m_geo;
      vistle::Object::const_ptr m_color;
      vistle::Object::const_ptr m_normal;
      vistle::Object::const_ptr m_tex;

      static std::mutex s_coverMutex;
};

#endif
