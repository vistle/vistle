#ifndef COLOR_H
#define COLOR_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include <vistle/core/texture1d.h>

#include <deque>

#ifdef COLOR_RANDOM
#define Color ColorRandom
#define ColorMap ColorMapRandom
#endif

class ColorMap {
public:
    typedef vistle::Vector4 RGBA;
    typedef std::map<vistle::Scalar, RGBA> TF;

    ColorMap(const size_t steps);
    ColorMap(TF &pins, const size_t steps, const size_t width, vistle::Scalar center = 0.5f,
             vistle::Scalar compress = 1.f);
    ~ColorMap();

    std::vector<unsigned char> data;
    size_t width = 0;
};

class Color: public vistle::Module {
public:
    Color(const std::string &name, int moduleID, mpi::communicator comm);
    ~Color();

private:
    vistle::Texture1D::ptr addTexture(vistle::DataBase::const_ptr object, const vistle::Scalar min,
                                      const vistle::Scalar max, const ColorMap &cmap);

    void getMinMax(vistle::DataBase::const_ptr object, vistle::Scalar &min, vistle::Scalar &max);
    void binData(vistle::DataBase::const_ptr object, std::vector<unsigned long> &binsVec);
    void computeMap();
    void sendColorMap();

    bool changeParameter(const vistle::Parameter *p) override;
    bool prepare() override;
    bool compute() override;
    bool reduce(int timestep) override;
    void connectionAdded(const vistle::Port *from, const vistle::Port *to) override;

    void process(const vistle::DataBase::const_ptr data);

#ifndef COLOR_RANDOM
    std::map<int, ColorMap::TF> transferFunctions;
#endif

    std::shared_ptr<ColorMap> m_colors;

    bool m_autoRange = true, m_autoInsetCenter = true, m_nest = false;
    vistle::StringParameter *m_speciesPara = nullptr;
    vistle::IntParameter *m_autoRangePara, *m_autoInsetCenterPara, *m_nestPara;
    vistle::FloatParameter *m_minPara = nullptr, *m_maxPara = nullptr;
    vistle::IntParameter *m_constrain = nullptr;
    vistle::FloatParameter *m_center = nullptr;
    vistle::IntParameter *m_centerAbsolute = nullptr;
    vistle::FloatParameter *m_compress = nullptr;
    vistle::IntParameter *m_insetRelPara = nullptr;
    vistle::FloatParameter *m_insetCenterPara = nullptr, *m_insetWidthPara = nullptr;
    vistle::IntParameter *m_blendWithMaterialPara = nullptr;
    vistle::FloatParameter *m_opacity = nullptr, *m_insetOpacity = nullptr;
    vistle::IntParameter *m_mapPara = nullptr;
    vistle::IntParameter *m_stepsPara = nullptr;
    std::deque<vistle::DataBase::const_ptr> m_inputQueue;

    bool m_dataRangeValid = false;
    vistle::Scalar m_dataMin, m_dataMax;
    vistle::Scalar m_min, m_max;
    bool m_reverse = false;

    std::string m_species;
    bool m_colorMapSent = false;
    vistle::Port *m_dataIn = nullptr;
    vistle::Port *m_dataOut = nullptr, *m_colorOut = nullptr;
    vistle::ResultCache<vistle::Object::ptr> m_cache;
};

#endif
