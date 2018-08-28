#ifndef VISTLEGEOMETRYGENERATOR_H
#define VISTLEGEOMETRYGENERATOR_H

#include <core/object.h>
#include <renderer/renderobject.h>
#include <mutex>
#include <osg/StateSet>
#include <osg/KdTree>
#include <osg/Texture1D>
#include <osg/Image>
#include <osg/ref_ptr>

namespace osg {
   class MatrixTransform;
}

struct OsgColorMap {
    OsgColorMap();
    float rangeMin = 0.f;
    float rangeMax = 1.f;
    osg::ref_ptr<osg::Texture1D> texture;
    osg::ref_ptr<osg::Image> image;
};

typedef std::map<std::string, OsgColorMap> OsgColorMapMap;

class VistleGeometryGenerator {
   public:
      VistleGeometryGenerator(std::shared_ptr<vistle::RenderObject> ro,
            vistle::Object::const_ptr geo,
            vistle::Object::const_ptr normal,
            vistle::Object::const_ptr tex);

      const std::string &species() const;
      void setColorMaps(const OsgColorMapMap *colormaps);

      osg::MatrixTransform *operator()(osg::ref_ptr<osg::StateSet> state = NULL);

      static bool isSupported(vistle::Object::Type t);

   private:
      std::shared_ptr<vistle::RenderObject> m_ro;
      vistle::Object::const_ptr m_geo;
      vistle::Object::const_ptr m_normal;
      vistle::Object::const_ptr m_tex;

      std::string m_species;

      const OsgColorMapMap *m_colormaps = nullptr;

      static std::mutex s_coverMutex;
      static osg::ref_ptr<osg::KdTreeBuilder> s_kdtree;
};

#endif
