#ifndef VISTLEGEOMETRYGENERATOR_H
#define VISTLEGEOMETRYGENERATOR_H

#include <vistle/core/object.h>
#include <vistle/renderer/renderobject.h>
#include <vistle/module/resultcache.h>
#include <mutex>
#include <memory>
#include <osg/StateSet>
#include <osg/KdTree>
#include <osg/Texture1D>
#include <osg/Image>
#include <osg/Uniform>
#include <osg/ref_ptr>

#ifdef COVER_PLUGIN
#include <cover/coVRShader.h>
#endif

namespace osg {
class Geode;
}

struct OsgColorMap {
    OsgColorMap();
    void setName(const std::string &species);
    void setRange(float min, float max);
    void setBlendWithMaterial(bool enable);
#ifdef COVER_PLUGIN
    std::shared_ptr<opencover::coVRShader> shader;
    std::shared_ptr<opencover::coVRShader> shaderUnlit;
    std::shared_ptr<opencover::coVRShader> shaderHeightMap;
    std::shared_ptr<opencover::coVRShader> shaderSpheres;
    std::vector<std::shared_ptr<opencover::coVRShader>> allShaders;
#endif
    bool blendWithMaterial = false;
    float rangeMin = 0.f, rangeMax = 1.f;
    osg::ref_ptr<osg::Texture1D> texture;
    osg::ref_ptr<osg::Image> image;
};

typedef std::map<std::string, OsgColorMap> OsgColorMapMap;

struct GeometryCache {
    GeometryCache(): mutex(std::make_shared<std::mutex>()) {}
    void lock()
    {
        assert(mutex);
        mutex->lock();
    }
    void unlock()
    {
        assert(mutex);
        mutex->unlock();
    }

    std::shared_ptr<std::mutex> mutex; // to protect against concurrent access to osg's reference counts?
    std::vector<osg::ref_ptr<osg::Vec3Array>> vertices;
    std::vector<osg::ref_ptr<osg::Vec3Array>> normals;
    std::vector<osg::ref_ptr<osg::PrimitiveSet>> primitives;
};

class VistleGeometryGenerator {
public:
    VistleGeometryGenerator(std::shared_ptr<vistle::RenderObject> ro, vistle::Object::const_ptr geo,
                            vistle::Object::const_ptr normal, vistle::Object::const_ptr mapped);

    const std::string &species() const;
    void setColorMaps(const OsgColorMapMap *colormaps);
    void setGeometryCache(vistle::ResultCache<GeometryCache> &cache);

    osg::Geode *operator()(osg::ref_ptr<osg::StateSet> state = NULL);

    static bool isSupported(vistle::Object::Type t);

    static void lock();
    static void unlock();

private:
    std::shared_ptr<vistle::RenderObject> m_ro;
    vistle::Object::const_ptr m_geo;
    vistle::Object::const_ptr m_normal;
    vistle::Object::const_ptr m_mapped;

    std::string m_species;

    const OsgColorMapMap *m_colormaps = nullptr;
    vistle::ResultCache<GeometryCache> *m_cache = nullptr;

    static std::mutex s_coverMutex;
};

#endif
