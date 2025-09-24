#include <fstream>
#include <sstream>
#include <iomanip>
#include <cfloat>
#include <limits>
#include <random>
#include <map>
#include <vector>
#include <vistle/core/scalar.h>
#include <vistle/core/vector.h>
#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/texture1d.h>
#include <vistle/core/coords.h>
#include <vistle/util/math.h>
#include <vistle/module/resultcache.h>
#include <vistle/alg/objalg.h>

#include "Color.h"

#ifndef COLOR_RANDOM
#include "matplotlib.h"
#include "fake_parula.h"
#include "turbo_colormap.h"
#include "etopo1_modified.h"
#endif

MODULE_MAIN(Color)

//#define USE_OPENMP

using namespace vistle;

#ifndef COLOR_RANDOM
// clang-format off
DEFINE_ENUM_WITH_STRING_CONVERSIONS(
    TransferFunction,
    (COVISE)
    (Plasma)
    (Inferno)
    (Magma)
    (CoolWarmBrewer)
    (CoolWarm)
    (Frosty)
    (Dolomiti)
    (Parula)
    (Viridis)
    (Cividis)
    (Turbo)
    (Blue_Light)
    (Grays)
    (Gray20)
    (ANSYS)
    (Star)
    (ITSM)
    (Rainbow)
    (Earth)
    (Topography)
    (RainbowPale)
    (FromFile)
)
// clang-format on

static const vistle::Float TopographyMin(-8000), TopographyMax(8000);
#endif

static const vistle::Integer MaxSteps(0x4000);

ColorMap::ColorMap(const size_t steps): width(steps)
{
    auto rand = std::minstd_rand0();
    auto dist = std::uniform_int_distribution<unsigned short>(0, 0xff);

    data.resize(steps * 4);
    for (size_t i = 0; i < width; ++i) {
        data[i * 4 + 0] = dist(rand);
        data[i * 4 + 1] = dist(rand);
        data[i * 4 + 2] = dist(rand);
        data[i * 4 + 3] = 0xff;
    }
}

ColorMap::ColorMap(TF &pins, const size_t steps, const size_t w, Scalar center, Scalar compress): width(w)
{
    data.resize(width * 4);

    TF::iterator current = pins.begin();
    TF::iterator next = current;
    if (next != pins.end())
        ++next;

    for (size_t index = 0; index < width; index++) {
        Scalar x = 0.5;
        if (steps >= 1) {
            int step = Scalar(index) / (Scalar(width) / Scalar(steps));
            x = (step + Scalar(.5)) / (vistle::Scalar)(steps);
        }
        if (x > center) {
            auto r = 1. - center;
            x = (x - center) / r * 0.5 + 0.5;
        } else {
            auto r = center;
            x = x / r * 0.5;
        }
        if (x > 0.5) {
            x = pow((x - 0.5) * 2., compress) * 0.5 + 0.5;
        } else {
            x = 0.5 - pow((0.5 - x) * 2., compress) * 0.5;
        }
        while (next != pins.end() && x > next->first) {
            ++next;
            ++current;
        }

        if (next == pins.end()) {
            data[index * 4] = clamp(current->second[0] * 255.99, 0., 255.);
            data[index * 4 + 1] = clamp(current->second[1] * 255.99, 0., 255.);
            data[index * 4 + 2] = clamp(current->second[2] * 255.99, 0., 255.);
            data[index * 4 + 3] = clamp(current->second[3] * 255.99, 0., 255.);
        } else {
            vistle::Scalar a = current->first;
            vistle::Scalar b = next->first;

            auto t = vistle::interpolation_weight<vistle::Scalar>(a, b, x);

            data[index * 4] = clamp(lerp(current->second[0], next->second[0], t) * 255.99, 0., 255.);
            data[index * 4 + 1] = clamp(lerp(current->second[1], next->second[1], t) * 255.99, 0., 255.);
            data[index * 4 + 2] = clamp(lerp(current->second[2], next->second[2], t) * 255.99, 0., 255.);
            data[index * 4 + 3] = clamp(lerp(current->second[3], next->second[3], t) * 255.99, 0., 255.);
        }
    }
}

ColorMap::~ColorMap() = default;

#ifndef COLOR_RANDOM
ColorMap::TF pinsFromArray(const float *data, size_t n)
{
    ColorMap::TF tf;
    for (size_t i = 0; i < n; ++i) {
        Scalar x = i / (n - 1.f);
        tf[x] = Vector4(data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2], 1);
    }

    return tf;
}

ColorMap::TF pinsFromArrayWithCoord(const float *data, size_t n, Scalar scale = 1., Scalar coordRangeMin = 0.f,
                                    Scalar coordRangeMax = 1.f)
{
    ColorMap::TF tf;
    for (size_t i = 0; i < n; ++i) {
        Scalar x = data[i * 4];
        x -= coordRangeMin;
        x /= coordRangeMax - coordRangeMin;
        assert(x >= 0 && x <= 1);
        tf[x] = Vector4(clamp(data[i * 4 + 1] * scale, Scalar(0), Scalar(1)),
                        clamp(data[i * 4 + 2] * scale, Scalar(0), Scalar(1)),
                        clamp(data[i * 4 + 3] * scale, Scalar(0), Scalar(1)), 1);
    }

    return tf;
}
#endif


Color::Color(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setReducePolicy(message::ReducePolicy::OverAll);

    m_dataIn = createInputPort("data_in", "field to create colormap for");
    setPortOptional(m_dataIn, true);
    m_dataOut = createOutputPort("data_out", "field converted to colors");
    m_colorOut = createOutputPort("color_out", "color map");
    linkPorts(m_dataIn, m_dataOut);

    m_speciesPara = addStringParameter("species", "species attribute of input data", "");

    m_minPara = addFloatParameter("min", "minimum value of range to map", 0.0);
    m_maxPara = addFloatParameter("max", "maximum value of range to map", 0.0);
    m_constrain = addIntParameter("constrain_range", "constrain range for min/max to data", true, Parameter::Boolean);
#ifndef COLOR_RANDOM
    m_center = addFloatParameter("center", "center of colormap range", 0.5);
    setParameterRange(m_center, 0., 1.);
    m_centerAbsolute = addIntParameter("center_absolute", "absolute value for center", false, Parameter::Boolean);
    m_compress = addFloatParameter("range_compression", "compression of range towards center", 0.);
    setParameterRange(m_compress, -1., 1.);
#endif
    m_opacity = addFloatParameter("opacity_factor", "multiplier for opacity", 1.0);
    setParameterRange(m_opacity, 0., 1.);
#ifndef COLOR_RANDOM
    m_mapPara = addIntParameter("map", "transfer function name", CoolWarmBrewer, Parameter::Choice);
    m_rgbFile = addStringParameter("rgb_file", " file containing pin rgb values", "", Parameter::ExistingFilename);
    V_ENUM_SET_CHOICES(m_mapPara, TransferFunction);
    m_stepsPara = addIntParameter("steps", "number of color map steps", 32);
    setParameterRange(m_stepsPara, (Integer)1, MaxSteps);
#endif
    m_blendWithMaterialPara = addIntParameter("blend_with_material", "use alpha for blending with diffuse material",
                                              false, Parameter::Boolean);

    m_autoRangePara = addIntParameter("auto_range", "compute range automatically", m_autoRange, Parameter::Boolean);
    m_preview = addIntParameter("preview", "use preliminary colormap for showing preview when determining bounds", true,
                                Parameter::Boolean);

#ifndef COLOR_RANDOM
    setCurrentParameterGroup("Nested Color Map");
    m_nestPara = addIntParameter("nest", "inset another color map", m_nest, Parameter::Boolean);
    m_autoInsetCenterPara =
        addIntParameter("auto_center", "compute center of nested color map", m_autoInsetCenter, Parameter::Boolean);
    m_insetRelPara =
        addIntParameter("inset_relative", "width and center of inset are relative to range", true, Parameter::Boolean);
    m_insetCenterPara =
        addFloatParameter("inset_center", "where to inset other color map (auto range: 0.5=middle)", 0.5);
    m_insetWidthPara = addFloatParameter("inset_width", "range covered by inset color map (auto range: relative)", 0.1);
    auto inset_map = addIntParameter("inset_map", "transfer function to inset", COVISE, Parameter::Choice);
    V_ENUM_SET_CHOICES(inset_map, TransferFunction);
    auto inset_steps = addIntParameter("inset_steps", "number of color map steps for inset (0: as outer map)", 0);
    setParameterRange(inset_steps, (Integer)0, MaxSteps);
    auto res = addIntParameter("resolution", "number of steps to compute", 1024);
    setParameterRange(res, (Integer)1, MaxSteps);
    setParameterRange(m_insetCenterPara, (Float)0, (Float)1);
    setParameterRange(m_insetWidthPara, (Float)0, (Float)1);
    m_insetOpacity = addFloatParameter("inset_opacity_factor", "multiplier for opacity of inset color", 1.0);
    setParameterRange(m_insetOpacity, 0., 1.);
#endif

    addResultCache(m_cache);

#ifndef COLOR_RANDOM
    ColorMap::TF pins;
    typedef ColorMap::RGBA RGBA;

    pins.insert(std::make_pair(0.0, RGBA(0.0, 0.0, 1.0, 1.0)));
    pins.insert(std::make_pair(0.5, RGBA(1.0, 0.0, 0.0, 1.0)));
    pins.insert(std::make_pair(1.0, RGBA(1.0, 1.0, 0.0, 1.0)));
    transferFunctions[COVISE] = pins;
    pins.clear();

    transferFunctions[Topography] = pinsFromArrayWithCoord(
        &etopo1_modified_data[0][0], sizeof(etopo1_modified_data) / sizeof(etopo1_modified_data[0]), 1.f / 255.f,
        TopographyMin, TopographyMax);

#define TF(a) pinsFromArray(&a[0][0], sizeof(a) / sizeof(a[0]))

    transferFunctions[Plasma] = TF(_plasma_data);
    transferFunctions[Inferno] = TF(_inferno_data);
    transferFunctions[Magma] = TF(_magma_data);
    transferFunctions[Viridis] = TF(_viridis_data);
    transferFunctions[Cividis] = TF(_cividis_data);
    transferFunctions[Parula] = TF(_fake_parula_data);
    transferFunctions[Turbo] = TF(turbo_srgb_floats);

#undef TF

    pins[0.00] = RGBA(0.10, 0.00, 0.90, 1.);
    pins[0.07] = RGBA(0.00, 0.00, 1.00, 1.);
    pins[0.14] = RGBA(0.63, 0.63, 1.00, 1.);
    pins[0.21] = RGBA(0.00, 0.75, 1.00, 1.);
    pins[0.28] = RGBA(0.00, 1.00, 1.00, 1.);
    pins[0.35] = RGBA(0.10, 0.80, 0.70, 1.);
    pins[0.42] = RGBA(0.10, 0.90, 0.00, 1.);
    pins[0.50] = RGBA(0.50, 1.00, 0.63, 1.);
    pins[0.57] = RGBA(0.75, 1.00, 0.25, 1.);
    pins[0.64] = RGBA(1.00, 1.00, 0.00, 1.);
    pins[0.71] = RGBA(1.00, 0.80, 0.10, 1.);
    pins[0.78] = RGBA(1.00, 0.60, 0.30, 1.);
    pins[0.85] = RGBA(1.00, 0.67, 0.95, 1.);
    pins[0.92] = RGBA(1.00, 0.00, 0.50, 1.);
    pins[1.00] = RGBA(1.00, 0.00, 0.00, 1.);
    transferFunctions[Star] = pins;
    pins.clear();

    pins[0.] = RGBA(1, 1, 1, 1);
    pins[1.] = RGBA(0, 0, 1, 1);
    transferFunctions[Blue_Light] = pins;
    pins.clear();

    pins[0.00] = RGBA(0, 0, 1, 1);
    pins[0.25] = RGBA(0, 1, 1, 1);
    pins[0.50] = RGBA(0, 1, 0, 1);
    pins[0.75] = RGBA(1, 1, 0, 1);
    pins[1.00] = RGBA(1, 0, 0, 1);
    transferFunctions[ANSYS] = pins;
    pins.clear();

    pins[0.00] = RGBA(0.231, 0.298, 0.752, 1);
    pins[0.25] = RGBA(0.552, 0.690, 0.996, 1);
    pins[0.50] = RGBA(0.866, 0.866, 0.866, 1);
    pins[0.75] = RGBA(0.956, 0.603, 0.486, 1);
    pins[1.00] = RGBA(0.705, 0.015, 0.149, 1);
    transferFunctions[CoolWarm] = pins;
    pins.clear();

    pins[0.00] = RGBA(1, 133, 113, 255) / 255.;
    pins[0.25] = RGBA(128, 205, 193, 280) / 280.;
    pins[0.50] = RGBA(245, 245, 245, 300) / 300.;
    pins[0.75] = RGBA(223, 194, 125, 280) / 280.;
    pins[1.00] = RGBA(166, 97, 26, 255) / 255.;
    transferFunctions[Frosty] = pins;
    pins.clear();

    pins[0.00] = RGBA(77, 172, 38, 255) / 255.;
    pins[0.25] = RGBA(184, 225, 134, 280) / 280.;
    pins[0.50] = RGBA(247, 247, 247, 300) / 300.;
    pins[0.75] = RGBA(241, 182, 218, 280) / 280.;
    pins[1.00] = RGBA(208, 28, 139, 255) / 255.;
    transferFunctions[Dolomiti] = pins;
    pins.clear();

    pins[0.00] = RGBA(44, 123, 182, 255) / 255.;
    pins[0.25] = RGBA(171, 217, 233, 255) / 255.;
    pins[0.50] = RGBA(255, 255, 191, 255) / 255.;
    pins[0.75] = RGBA(253, 174, 97, 255) / 255.;
    pins[1.00] = RGBA(215, 25, 28, 255) / 255.;
    transferFunctions[CoolWarmBrewer] = pins;
    pins.clear();

    pins[0.00] = RGBA(0.00, 0.00, 0.35, 1);
    pins[0.05] = RGBA(0.00, 0.00, 1.00, 1);
    pins[0.26] = RGBA(0.00, 1.00, 1.00, 1);
    pins[0.50] = RGBA(0.00, 0.00, 1.00, 1);
    pins[0.74] = RGBA(1.00, 1.00, 0.00, 1);
    pins[0.95] = RGBA(1.00, 0.00, 0.00, 1);
    pins[1.00] = RGBA(0.40, 0.00, 0.00, 1);
    transferFunctions[ITSM] = pins;
    pins.clear();

    pins[0.0] = RGBA(0.40, 0.00, 0.40, 1);
    pins[0.2] = RGBA(0.00, 0.00, 1.00, 1);
    pins[0.4] = RGBA(0.00, 1.00, 1.00, 1);
    pins[0.6] = RGBA(0.00, 1.00, 0.00, 1);
    pins[0.8] = RGBA(1.00, 1.00, 0.00, 1);
    pins[1.0] = RGBA(1.00, 0.00, 0.00, 1);
    transferFunctions[Rainbow] = pins;
    pins.clear();

    pins[0.0] = RGBA(1.0, 1.00, 1.00, 1);
    pins[0.2] = RGBA(0.27, 0.87, 1.00, 1);
    pins[0.4] = RGBA(0.31, 0.60, 1.00, 1);
    pins[0.6] = RGBA(0.30, 0.66, 0.28, 1);
    pins[0.8] = RGBA(1.00, 0.80, 0.13, 1);
    pins[1.0] = RGBA(0.78, 0.11, 0.21, 1);
    transferFunctions[RainbowPale] = pins;
    pins.clear();


    pins[0.00] = RGBA(25, 25, 25, 255) / 255.;
    pins[1.00] = RGBA(230, 230, 230, 255) / 255.;
    transferFunctions[Grays] = pins;
    pins.clear();

    pins[0.00] = RGBA(189, 79, 6, 255) / 255.;
    pins[0.50] = RGBA(219, 205, 94, 255) / 255.;
    pins[1.00] = RGBA(5, 31, 232, 255) / 255.;
    transferFunctions[Earth] = pins;
    pins.clear();

    float d = 0.2;
    pins[0.0] = pins[1.0] = RGBA(d, d, d, 1.0);
    transferFunctions[Gray20] = pins;
    pins.clear();
#endif

    updateHaveData();
}

Color::~Color()
{}

namespace {
template<class V>
void updateMinMax(typename V::const_ptr &v, Scalar &min, Scalar &max)
{
    auto mmv = v->getMinMax();
    if (mmv.first[0] <= mmv.second[0]) {
        std::tuple<Scalar, Scalar> mm{mmv.first[0], mmv.second[0]};
        if (min > std::get<0>(mm))
            min = std::get<0>(mm);
        if (max < std::get<1>(mm))
            max = std::get<1>(mm);
    }
}
} // namespace

void Color::getMinMax(vistle::DataBase::const_ptr object, vistle::Scalar &min, vistle::Scalar &max)
{
    if (Vec<Byte>::const_ptr scal = Vec<Byte>::as(object)) {
        updateMinMax<Vec<Byte>>(scal, min, max);
    } else if (Vec<Index>::const_ptr scal = Vec<Index>::as(object)) {
        updateMinMax<Vec<Index>>(scal, min, max);
    } else if (Vec<Scalar>::const_ptr scal = Vec<Scalar>::as(object)) {
        updateMinMax<Vec<Scalar>>(scal, min, max);
    } else if (Vec<Scalar, 3>::const_ptr vec = Vec<Scalar, 3>::as(object)) {
        const vistle::Scalar *x = vec->x().data();
        const vistle::Scalar *y = vec->y().data();
        const vistle::Scalar *z = vec->z().data();
#ifdef USE_OPENMP
#pragma omp parallel
#endif
        {
            Scalar tmin = std::numeric_limits<Scalar>::max();
            Scalar tmax = -std::numeric_limits<Scalar>::max();
#ifdef USE_OPENMP
#pragma omp for
#endif
            const ssize_t numElements = object->getSize();
            for (ssize_t index = 0; index < numElements; index++) {
                Scalar v = Vector3(x[index], y[index], z[index]).norm();
                if (v < tmin)
                    tmin = v;
                if (v > tmax)
                    tmax = v;
            }
#ifdef USE_OPENMP
#pragma omp critical
#endif
            {
                if (tmin < min)
                    min = tmin;
                if (tmax > max)
                    max = tmax;
            }
        }
    }
}

void Color::binData(vistle::DataBase::const_ptr object, std::vector<unsigned long> &binsVec)
{
    const int numBins = binsVec.size();

    const ssize_t numElements = object->getSize();
    const Scalar w = m_max - m_min;
    unsigned long *bins = binsVec.data();

    if (Vec<Byte>::const_ptr scal = Vec<Byte>::as(object)) {
        const vistle::Byte *x = scal->x().data();
        for (ssize_t index = 0; index < numElements; index++) {
            const int bin = clamp<int>((x[index] - m_min) / w * numBins, 0, numBins - 1);
            ++bins[bin];
        }
    } else if (Vec<Index>::const_ptr scal = Vec<Index>::as(object)) {
        const vistle::Index *x = scal->x().data();
        for (ssize_t index = 0; index < numElements; index++) {
            const int bin = clamp<int>((x[index] - m_min) / w * numBins, 0, numBins - 1);
            ++bins[bin];
        }
    } else if (Vec<Scalar>::const_ptr scal = Vec<Scalar>::as(object)) {
        const vistle::Scalar *x = scal->x().data();
        for (ssize_t index = 0; index < numElements; index++) {
            const int bin = clamp<int>((x[index] - m_min) / w * numBins, 0, numBins - 1);
            ++bins[bin];
        }
    } else if (Vec<Scalar, 3>::const_ptr vec = Vec<Scalar, 3>::as(object)) {
        const vistle::Scalar *x = vec->x().data();
        const vistle::Scalar *y = vec->y().data();
        const vistle::Scalar *z = vec->z().data();
        for (ssize_t index = 0; index < numElements; index++) {
            const Scalar v = Vector3(x[index], y[index], z[index]).norm();
            const int bin = clamp<int>((v - m_min) / w * numBins, 0, numBins - 1);
            ++bins[bin];
        }
    }
}


bool Color::changeParameter(const Parameter *p)
{
    bool newMap = false;

#ifndef COLOR_RANDOM
    if (p == m_mapPara) {
        if (m_mapPara->getValue() == Topography) {
            setParameter<Integer>(m_autoRangePara, false);
            setParameter<Integer>(m_constrain, false);
            setParameter<Integer>(m_centerAbsolute, true);
            setParameterRange<Float>(m_minPara, std::numeric_limits<Scalar>::lowest(),
                                     std::numeric_limits<Scalar>::max());
            setParameterRange<Float>(m_maxPara, std::numeric_limits<Scalar>::lowest(),
                                     std::numeric_limits<Scalar>::max());
            setParameterRange<Float>(m_center, std::numeric_limits<Scalar>::lowest(),
                                     std::numeric_limits<Scalar>::max());
            setParameter<Float>(m_center, 0.);
            setParameter<Float>(m_minPara, TopographyMin);
            setParameter<Float>(m_maxPara, TopographyMax);
            auto steps = m_stepsPara->getValue();
            if (steps % 2 == 0) {
                setParameter<Integer>(m_stepsPara, steps < MaxSteps ? steps + 1 : steps - 1);
            }
            // make sure that Topography color map applies to appropriate elevation
            std::vector<const vistle::Parameter *> paramVec{m_constrain, m_autoRangePara, m_centerAbsolute, m_minPara,
                                                            m_maxPara,   m_center,        m_stepsPara};
            std::map<std::string, const vistle::Parameter *> params;
            for (auto &p: paramVec) {
                params.emplace(p->getName(), p);
            }
            changeParameters(params);
        }
    } else if (p == m_rgbFile && !m_rgbFile->getValue().empty()) {
        auto filename = m_rgbFile->getValue();
        std::fstream file(m_rgbFile->getValue());
        std::map<vistle::Scalar, ColorMap::RGBA> pins;
        std::vector<ColorMap::RGBA> rgbValues;
        bool normalized = true;
        while (file.good() && !file.eof()) {
            std::string line;
            std::getline(file, line);
            std::stringstream str(line);
            std::vector<Scalar> rgba{std::istream_iterator<Scalar>(str), std::istream_iterator<Scalar>()};
            auto numValues = rgba.size();
            if (numValues < 3) {
                std::cerr << "ignoring " << line << ": found only " << numValues << " entries" << std::endl;
                continue;
            } else if (numValues == 3) {
                rgba.push_back(-1.f);
            } else {
                if (numValues > 4) {
                    std::cerr << "only using first four of " << numValues << " entries from " << line << std::endl;
                }
            }
            for (auto val: rgba) {
                if (val > 1.f)
                    normalized = false;
            }
            rgbValues.push_back(ColorMap::RGBA(rgba.data()));
        }

        if (!normalized) {
            for_each(rgbValues.begin(), rgbValues.end(), [](ColorMap::RGBA &elem) {
                elem[0] /= 255;
                elem[1] /= 255;
                elem[2] /= 255;
                elem[3] /= 255;
            });
        }
        for_each(rgbValues.begin(), rgbValues.end(), [](ColorMap::RGBA &elem) {
            if (elem[3] < 0.f)
                elem[3] = 1.f;
        });
        auto linecount = rgbValues.size();
        for (unsigned i = 0; i < linecount; ++i) {
            float idx = linecount > 1 ? (float)i / (float)(linecount - 1) : 0;
            pins[idx] = rgbValues[i];
        }

        transferFunctions[FromFile] = pins;
        if (m_mapPara->getValue() == FromFile) {
            newMap = true;
        }
    } else
#endif
        if (p == m_constrain) {
        if (m_constrain->getValue()) {
            if (m_dataRangeValid) {
                auto diff = m_dataMax - m_dataMin;
                setParameterRange<Float>(m_minPara, m_dataMin - diff * 0.5, m_dataMax);
                setParameterRange<Float>(m_maxPara, m_dataMin, m_dataMax + diff * 0.5);
            }
        } else {
            setParameterRange<Float>(m_minPara, std::numeric_limits<Scalar>::lowest(),
                                     std::numeric_limits<Scalar>::max());
            setParameterRange<Float>(m_maxPara, std::numeric_limits<Scalar>::lowest(),
                                     std::numeric_limits<Scalar>::max());
        }
    } else if (p == m_autoRangePara) {
        m_autoRange = m_autoRangePara->getValue();
        newMap = true;
    } else if (p == m_autoInsetCenterPara) {
        m_autoInsetCenter = m_autoInsetCenterPara->getValue();
        newMap = true;
    } else if (p == m_nestPara) {
        m_nest = m_nestPara->getValue();
        newMap = true;
    } else if (p == m_minPara) {
        m_min = m_minPara->getValue();
        newMap = true;
    } else if (p == m_maxPara) {
        m_max = m_maxPara->getValue();
        newMap = true;
    } else if (p == m_insetRelPara) {
        if (m_insetRelPara->getValue()) {
            setParameterRange(m_insetCenterPara, (Float)0, (Float)1);
            setParameterRange(m_insetWidthPara, (Float)0, (Float)1);
        } else {
            setParameterRange(m_insetCenterPara, std::numeric_limits<Float>::lowest(),
                              std::numeric_limits<Float>::max());
            setParameterRange(m_insetWidthPara, std::numeric_limits<Float>::lowest(),
                              std::numeric_limits<Float>::max());
        }
    } else if (p == m_blendWithMaterialPara) {
        newMap = true;
    } else if (p == m_centerAbsolute) {
        if (m_centerAbsolute->getValue()) {
            setParameterRange(m_center, std::numeric_limits<Float>::lowest(), std::numeric_limits<Float>::max());
        } else {
            setParameterRange(m_center, 0., 1.);
        }
    }

    newMap = true;

    if (newMap) {
        computeMap();
    }

    return Module::changeParameter(p);
}

vistle::Texture1D::ptr Color::addTexture(vistle::DataBase::const_ptr object, const vistle::Scalar min,
                                         const vistle::Scalar max, const ColorMap &cmap)
{
    const Scalar invRange = 1.f / (max - min);

    vistle::Texture1D::ptr tex(new vistle::Texture1D(cmap.width, min, max));
    unsigned char *pix = tex->pixels().data();
    for (size_t index = 0; index < cmap.width * 4; index++)
        pix[index] = cmap.data[index];

    const ssize_t numElem = object->getSize();
    tex->coords().resize(numElem);
    auto tc = tex->coords().data();

    if (Vec<Scalar>::const_ptr f = Vec<Scalar>::as(object)) {
        const vistle::Scalar *x = f->x().data();

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for (ssize_t index = 0; index < numElem; index++)
            tc[index] = (x[index] - min) * invRange;
    } else if (Vec<Index>::const_ptr f = Vec<Index>::as(object)) {
        const vistle::Index *x = f->x().data();

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for (ssize_t index = 0; index < numElem; index++)
            tc[index] = (x[index] - min) * invRange;
    } else if (Vec<Byte>::const_ptr f = Vec<Byte>::as(object)) {
        const vistle::Byte *x = f->x().data();

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for (ssize_t index = 0; index < numElem; index++)
            tc[index] = (x[index] - min) * invRange;
    } else if (Vec<Scalar, 3>::const_ptr f = Vec<Scalar, 3>::as(object)) {
        const vistle::Scalar *x = f->x().data();
        const vistle::Scalar *y = f->y().data();
        const vistle::Scalar *z = f->z().data();

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for (ssize_t index = 0; index < numElem; index++) {
            const Scalar v = Vector3(x[index], y[index], z[index]).norm();
            tc[index] = (v - min) * invRange;
        }
    } else {
        std::cerr << "Color: cannot handle input of type " << object->getType() << std::endl;

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
        for (ssize_t index = 0; index < numElem; index++) {
            tc[index] = (index % 2) ? 0. : 1.;
        }
    }

    return tex;
}

void Color::computeMap()
{
    Scalar op = m_opacity->getValue();
#ifdef COLOR_RANDOM
    auto steps = std::min(Float(MaxSteps), 1 + std::abs(m_maxPara->getValue() - m_minPara->getValue()));
    m_colors.reset(new ColorMap(steps));
    for (size_t i = 0; i < m_colors->width; ++i) {
        m_colors->data[i * 4 + 3] *= op;
    }
#else
    auto pins = transferFunctions[getIntParameter("map")];
    if (pins.empty()) {
        pins = transferFunctions[COVISE];
    }
    Scalar inset_op = m_insetOpacity->getValue();
    int steps = getIntParameter("steps");
    int resolution = steps;
    bool relative = getIntParameter("inset_relative");
    if (m_nest) {
        resolution = getIntParameter("resolution");
        double width = getFloatParameter("inset_width");
        int inset_steps = getIntParameter("inset_steps");
        if (inset_steps <= 0)
            inset_steps = steps;
        if (relative && width > 0.) {
            int res2 = inset_steps / width;
            if (resolution < res2)
                resolution = std::min(res2, int(MaxSteps));
        }
    }
    //std::cerr << "computing color map with " << steps << " steps and a resolution of " << resolution << std::endl;
    double relcenter = m_center->getValue();
    if (m_centerAbsolute->getValue()) {
        if (m_min >= relcenter) {
            relcenter = 0.;
        } else if (m_max <= relcenter) {
            relcenter = 1.;
        } else {
            relcenter = (relcenter - m_min) / (m_max - m_min);
        }
    }
    m_colors.reset(new ColorMap(pins, steps, resolution, relcenter, std::pow(10., m_compress->getValue())));
    for (size_t i = 0; i < m_colors->width; ++i) {
        m_colors->data[i * 4 + 3] *= op;
    }

    if (m_nest) {
        auto inset_pins = transferFunctions[getIntParameter("inset_map")];
        if (inset_pins.empty()) {
            inset_pins = transferFunctions[COVISE];
        }
        int inset_steps = getIntParameter("inset_steps");
        if (inset_steps <= 0)
            inset_steps = steps;
        double width = getFloatParameter("inset_width");
        double center = getFloatParameter("inset_center");
        if (!relative) {
            width /= m_max - m_min;
            center = (center - m_min) / (m_max - m_min);
        }
        int insetStart = clamp(center - 0.5 * width, 0., 1.) * (resolution - 1);
        int insetEnd = clamp(center + 0.5 * width, 0., 1.) * (resolution - 1);
        int insetRes = insetEnd - insetStart + 1;
        assert(insetEnd >= insetStart);
        assert(insetStart >= 0);
        assert(unsigned(insetEnd) < m_colors->width);
        ColorMap inset(inset_pins, inset_steps, insetRes);
        for (size_t i = 0; i < inset.width; ++i) {
            inset.data[i * 4 + 3] *= inset_op;
        }
        for (int i = insetStart; i <= insetEnd; ++i) {
            for (int c = 0; c < 4; ++c)
                m_colors->data[i * 4 + c] = inset.data[(i - insetStart) * 4 + c];
        }
    }
#endif

    if (m_reverse) {
        for (size_t i = 0; i < m_colors->width; ++i) {
            for (int c = 0; c < 4; ++c) {
                std::swap(m_colors->data[(m_colors->width - 1 - i) * 4 + c], m_colors->data[i * 4 + c]);
            }
        }
    }

    m_colorMapSent = false;
    sendColorMap();
}

bool Color::prepare()
{
    m_colorMapSent = false;
    if (m_haveData) {
        m_species.clear();

        m_dataMin = std::numeric_limits<Scalar>::max();
        m_dataMax = -std::numeric_limits<Scalar>::max();
        m_dataRangeValid = false;

        if (!m_autoRange) {
            m_min = m_minPara->getValue();
            m_max = m_maxPara->getValue();
            if (m_min == m_max)
                m_max = m_min + 1.;
        }
    }
    m_reverse = m_min > m_max;
    if (m_reverse)
        std::swap(m_min, m_max);
    m_inputQueue.clear();

    computeMap();

    if (m_haveData) {
        bool preview = getIntParameter("preview");
        if (m_autoRange || (m_nest && m_autoInsetCenter)) {
            if (preview)
                startIteration();
        }
    }

    return true;
}

bool Color::compute()
{
    if (!m_haveData) {
        sendColorMap();
        return true;
    }

    Object::const_ptr obj = expect<Object>(m_dataIn);
    auto split = splitContainerObject(obj);
    DataBase::const_ptr data = split.mapped;

    if (obj && !data) {
        // only grid, no mapped data
        if (m_dataOut->isConnected()) {
            Object::ptr nobj;
            if (auto entry = m_cache.getOrLock(obj->getName(), nobj)) {
                nobj = obj->clone();
                updateMeta(nobj);
                m_cache.storeAndUnlock(entry, nobj);
            }
            addObject(m_dataOut, nobj);
        }
        return true;
    }

    assert(data);

    getMinMax(data, m_dataMin, m_dataMax);
    bool preview = getIntParameter("preview");
    if (m_autoRange) {
        m_inputQueue.push_back(data);
        if (preview)
            process(data);
    } else if (m_nest && m_autoInsetCenter) {
        m_inputQueue.push_back(data);
        if (preview)
            process(data);
    } else {
        process(data);
    }

    return true;
}

bool Color::reduce(int timestep)
{
    assert(timestep == -1);
    if (!m_haveData) {
        return true;
    }

    bool preview = getIntParameter("preview");

    m_dataMin = boost::mpi::all_reduce(comm(), m_dataMin, boost::mpi::minimum<Scalar>());
    m_dataMax = boost::mpi::all_reduce(comm(), m_dataMax, boost::mpi::maximum<Scalar>());
    m_dataRangeValid = true;
    if (m_constrain->getValue()) {
        auto diff = m_dataMax - m_dataMin;
        setParameterRange<Float>(m_minPara, m_dataMin - diff * 0.5, m_dataMax);
        setParameterRange<Float>(m_maxPara, m_dataMin, m_dataMax + diff * 0.5);
    } else {
        setParameterRange<Float>(m_minPara, std::numeric_limits<Scalar>::lowest(), std::numeric_limits<Scalar>::max());
        setParameterRange<Float>(m_maxPara, std::numeric_limits<Scalar>::lowest(), std::numeric_limits<Scalar>::max());
    }

    if (m_autoRange) {
        setParameter<Float>(m_minPara, m_dataMin);
        setParameter<Float>(m_maxPara, m_dataMax);
    }

    m_min = m_minPara->getValue();
    m_max = m_maxPara->getValue();
    if (m_min == m_max)
        m_max = m_min + 1.;
    m_reverse = m_min > m_max;
    if (m_reverse)
        std::swap(m_min, m_max);

    if (m_nest && m_autoInsetCenter) {
        std::vector<unsigned long> bins(getIntParameter("resolution"));
        for (auto data: m_inputQueue) {
            binData(data, bins);
        }
        for (size_t i = 0; i < bins.size(); ++i) {
            bins[i] = boost::mpi::all_reduce(comm(), bins[i], std::plus<unsigned long>());
        }

        bool relative = getIntParameter("inset_relative");
        double width = getFloatParameter("inset_width");
        if (!relative) {
            width /= m_max - m_min;
        }
        size_t insetRes = width * bins.size();
        insetRes = std::min(insetRes, bins.size());
        unsigned long numEnt = 0;
        for (size_t i = 0; i < insetRes; ++i) {
            numEnt += bins[i];
        }
        unsigned long maxEnt = numEnt;
        size_t maxIdx = 0;
        for (size_t i = 0; i < bins.size() - insetRes; ++i) {
            numEnt -= bins[i];
            numEnt += bins[i + insetRes];
            if (numEnt >= maxEnt) {
                maxIdx = i + 1;
                maxEnt = numEnt;
            }
        }
        if (bins.size() > 1) {
            if (relative) {
                setFloatParameter("inset_center", (maxIdx + 0.5 * insetRes) / (bins.size() - 1));
            } else {
                setFloatParameter("inset_center",
                                  (maxIdx + 0.5 * insetRes) / (bins.size() - 1) * (m_max - m_min) + m_min);
            }
        }
    }

    if (m_autoRange || (m_nest && m_autoInsetCenter)) {
        computeMap();
        if (preview)
            startIteration();
    }

    while (!m_inputQueue.empty()) {
        if (cancelRequested())
            break;

        auto data = m_inputQueue.front();
        m_inputQueue.pop_front();
        process(data);
    }

    return true;
}

void Color::updateHaveData()
{
    setParameterReadOnly(m_speciesPara, m_haveData);
    setParameterReadOnly(m_autoRangePara, !m_haveData);
    setParameterReadOnly(m_autoInsetCenterPara, !m_haveData);
    setParameterReadOnly(m_constrain, !m_haveData);
    setParameterReadOnly(m_preview, !m_haveData);
}

void Color::connectionAdded(const Port *from, const Port *to)
{
    if (from == m_colorOut) {
        m_colorMapSent = false;
        sendColorMap();
    }

    if (to == m_dataIn) {
        m_haveData = true;
        updateHaveData();
    }
}

void Color::connectionRemoved(const Port *from, const Port *to)
{
    if (to == m_dataIn) {
        m_haveData = false;
        updateHaveData();
    }
}

void Color::process(const DataBase::const_ptr data)
{
    m_species = data->getAttribute(attribute::Species);
    sendColorMap();

    if (m_dataOut->isConnected()) {
        Object::ptr nobj;
        if (auto entry = m_cache.getOrLock(data->getName(), nobj)) {
            auto out(addTexture(data, m_min, m_max, *m_colors));
            out->setGrid(data->grid());
            out->setMeta(data->meta());
            out->copyAttributes(data);
            nobj = out;
            updateMeta(out);
            m_cache.storeAndUnlock(entry, nobj);
        }

        addObject(m_dataOut, nobj);
    }
}

void Color::sendColorMap()
{
    if (m_colorMapSent)
        return;

    if (m_haveData) {
        setParameter(m_speciesPara, m_species);
    } else {
        m_species = m_speciesPara->getValue();
    }
    if (!m_species.empty())
        setItemInfo(m_species);

    if (m_colorOut->isConnected() && !m_species.empty()) {
#ifdef COLOR_RANDOM
        vistle::Texture1D::ptr tex(new vistle::Texture1D(m_colors->width, m_min - 0.5, m_max + 0.5));
#else
        vistle::Texture1D::ptr tex(new vistle::Texture1D(m_colors->width, m_min, m_max));
#endif
        unsigned char *pix = tex->pixels().data();
        for (size_t index = 0; index < m_colors->width * 4; index++)
            pix[index] = m_colors->data[index];
        tex->addAttribute(attribute::Species, m_species);
        if (m_blendWithMaterialPara->getValue())
            tex->addAttribute(attribute::BlendWithMaterial, "true");
        updateMeta(tex);
        addObject(m_colorOut, tex);

        std::stringstream buffer;
        buffer << tex->getName() << '\n'
               << m_species << '\n'
#ifdef COLOR_RANDOM
               << (m_reverse ? m_max : m_min) - 0.5 << '\n'
               << (m_reverse ? m_min : m_max) + 0.5 << '\n'
#else
               << (m_reverse ? m_max : m_min) << '\n'
               << (m_reverse ? m_min : m_max) << '\n'
#endif
               << m_colors->width << '\n'
               << '0';

        buffer.precision(4);
        buffer << std::fixed;
        for (size_t index = 0; index < m_colors->width * 4; index++)
            buffer << "\n" << int(pix[index]) / 255.f;

#ifdef ColorMap
#undef ColorMap
#endif
        tex->addAttribute(attribute::ColorMap, buffer.str());
        tex->addAttribute(attribute::Plugin, "ColorBars");

        m_colorMapSent = true;
    }
}
