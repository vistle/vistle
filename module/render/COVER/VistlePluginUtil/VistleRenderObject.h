#ifndef VISTLE_COVER_VISTLEPLUGINUTIL_VISTLERENDEROBJECT_H
#define VISTLE_COVER_VISTLEPLUGINUTIL_VISTLERENDEROBJECT_H

#include "export.h"

#include <cover/RenderObject.h>

#include <vistle/core/polygons.h>
#include <vistle/core/normals.h>

#include <osg/ref_ptr>
#include <osg/Node>
#include <vistle/renderer/renderobject.h>
#include <memory>

class V_PLUGINUTILEXPORT BaseRenderObject: public opencover::RenderObject {
public:
    BaseRenderObject();
    ~BaseRenderObject();

    //XXX: hacks for Volume plugin and Tracer
    bool isSet() const override { return false; }
    size_t getNumElements() const override { return 0; }
    RenderObject *getElement(size_t idx) const override { return NULL; }
    RenderObject *getColorMap(int idx) const override { return NULL; }

    bool isUniformGrid() const override { return false; }
    void getSize(int &nx, int &ny, int &nz) const override { nx = ny = nz = 0; }
    void getMinMax(float &xmin, float &xmax, float &ymin, float &ymax, float &zmin, float &zmax) const override
    {
        xmin = xmax = ymin = ymax = zmin = zmax = 0.;
    }
    float getMin(int channel) const override { return 0.; }
    float getMax(int channel) const override { return 0.; }

    bool isVectors() const override { return false; }
    const unsigned char *getByte(opencover::Field::Id idx) const override { return NULL; }
    const int *getInt(opencover::Field::Id idx) const override { return NULL; }
    const float *getFloat(opencover::Field::Id idx) const override { return NULL; }

    bool isUnstructuredGrid() const override { return false; }

    bool fromCovise() const override { return false; }
};

// RenderObject created for each vistle::Object
class V_PLUGINUTILEXPORT VistleRenderObject: public BaseRenderObject {
public:
    VistleRenderObject(std::shared_ptr<const vistle::RenderObject> ro);
    VistleRenderObject(vistle::Object::const_ptr obj);
    ~VistleRenderObject() override;

    void setNode(osg::Node *node);
    osg::Node *node() const;
    std::shared_ptr<const vistle::RenderObject> renderObject() const;
    int getCreator() const;

    const char *getName() const override;
    bool isPlaceHolder() const;
    bool isUniformGrid() const override;
    bool isGeometry() const override;
    RenderObject *getGeometry() const override;
    RenderObject *getColors() const override;
    RenderObject *getNormals() const override;
    RenderObject *getTexture() const override;
    RenderObject *getVertexAttribute() const override;

    void getSize(int &nx, int &ny, int &nz) const override;
    void getMinMax(float &xmin, float &xmax, float &ymin, float &ymax, float &zmin, float &zmax) const override;
    float getMin(int channel) const override;
    float getMax(int channel) const override;
    const float *getFloat(opencover::Field::Id idx) const override;

    const char *getAttribute(const char *) const override;

protected:
    std::weak_ptr<const vistle::RenderObject> m_vistleRo;
    vistle::Object::const_ptr m_obj;
    osg::ref_ptr<osg::Node> m_node;

    mutable std::string m_name;
    mutable opencover::RenderObject *m_roGeo;
    mutable opencover::RenderObject *m_roData;
    mutable opencover::RenderObject *m_roNorm;
};

// pseudo RenderObject for handling module parameters, created for each Vistle module
class V_PLUGINUTILEXPORT ModuleRenderObject: public BaseRenderObject {
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

    void addAttribute(const std::string &key, const std::string &value);
    void removeAttribute(const std::string &key);

private:
    std::string m_moduleName;
    int m_moduleId;
    std::string m_name;

    std::map<std::string, std::string> m_attributes;
};

// pseudo RenderObject for handling Variants, as configured by the Variant module
class V_PLUGINUTILEXPORT VariantRenderObject: public BaseRenderObject {
public:
    VariantRenderObject(const std::string &variantName,
                        vistle::RenderObject::InitialVariantVisibility visible = vistle::RenderObject::DontChange);

    const char *getName() const override { return ""; }
    bool isGeometry() const override { return false; }
    RenderObject *getGeometry() const override { return nullptr; }
    RenderObject *getColors() const override { return nullptr; }
    RenderObject *getNormals() const override { return nullptr; }
    RenderObject *getTexture() const override { return nullptr; }
    RenderObject *getVertexAttribute() const override { return nullptr; }

    const char *getAttribute(const char *key) const override;

    osg::ref_ptr<osg::Node> node() const;
    void setInitialVisibility(vistle::RenderObject::InitialVariantVisibility visible);

private:
    std::string variant;
    osg::ref_ptr<osg::Node> m_node; //< dummy osg node
    vistle::RenderObject::InitialVariantVisibility m_visible;
};
#endif
