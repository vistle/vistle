#include <sstream>
#include <limits>
#include <algorithm>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>

#include <vistle/core/vec.h>
#include <vistle/module/module.h>
#include <vistle/core/scalars.h>
#include <vistle/core/paramvector.h>
#include <vistle/core/message.h>
#include <vistle/core/coords.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/core/standardattributes.h>
#include <vistle/alg/objalg.h>

#ifdef BOUNDINGBOX
#define Extrema BoundingBox
#endif

using namespace vistle;

class Extrema: public vistle::Module {
public:
    Extrema(const std::string &name, int moduleID, mpi::communicator comm);
    ~Extrema();

private:
#ifdef BOUNDINGBOX
    static const int MaxDim = 3;
#else
    static const int MaxDim = ParamVector::MaxDimension;
#endif

    int dim;
    bool handled;
    bool haveGeometry;
    ParamVector min, max, gmin, gmax;
    std::vector<ParamVector> tmin, tmax;
    IntParamVector minIndex, maxIndex, gminIndex, gmaxIndex;
    IntParamVector minBlock, maxBlock, gminBlock, gmaxBlock;

    bool compute() override;
    bool reduce(int timestep) override;

    template<int Dim>
    friend struct Compute;

    bool prepare() override
    {
        haveGeometry = false;

        tmin.clear();
        tmax.clear();
        for (int c = 0; c < MaxDim; ++c) {
            gmin[c] = std::numeric_limits<ParamVector::Scalar>::max();
            gmax[c] = -std::numeric_limits<ParamVector::Scalar>::max();
            gminIndex[c] = InvalidIndex;
            gmaxIndex[c] = InvalidIndex;
        }

        return true;
    }

#ifdef BOUNDINGBOX
    IntParameter *perTimestepParam = nullptr;
    StringParameter *transformationNameParam = nullptr;

    void updateReducePolicy()
    {
        bool transform = isConnected("transform_out");
        bool per_timestep = perTimestepParam->getValue() != 0;
        if (transform)
            setReducePolicy(message::ReducePolicy::PerTimestepZeroFirst);
        else if (per_timestep)
            setReducePolicy(message::ReducePolicy::PerTimestep);
        else
            setReducePolicy(message::ReducePolicy::OverAll);
    }

    bool changeParameter(const Parameter *param) override
    {
        if (!param || param == perTimestepParam) {
            updateReducePolicy();
        }
        return Module::changeParameter(param);
    }

    void connectionAdded(const Port *from, const Port *to) override
    {
        updateReducePolicy();
        Module::connectionAdded(from, to);
    }

    void connectionRemoved(const Port *from, const Port *to) override
    {
        updateReducePolicy();
        Module::connectionRemoved(from, to);
    }

    void expandTimestepExtrema(size_t num)
    {
        while (tmin.size() < num) {
            tmin.emplace_back();
            tmax.emplace_back();
            tmin.back().dim = MaxDim;
            tmax.back().dim = MaxDim;
            for (int c = 0; c < MaxDim; ++c) {
                tmin.back()[c] = std::numeric_limits<ParamVector::Scalar>::max();
                tmax.back()[c] = -std::numeric_limits<ParamVector::Scalar>::max();
            }
        }
    }

    bool boundsValid(int timestep) const
    {
        if (timestep < 0 || timestep >= static_cast<int>(tmin.size())) {
            //std::cerr << "timestep " << timestep << " out of bounds: no data" << std::endl;
            return false;
        }
        for (int c = 0; c < MaxDim; ++c) {
            if (tmin[timestep][c] > tmax[timestep][c]) {
                //std::cerr << "timestep " << timestep << " component " << c << " invalid" << std::endl;
                return false;
            }
        }
        return true;
    }
#endif

    template<int Dim>
    struct Compute {
        Object::const_ptr object;
        Extrema *module;
        Compute(Object::const_ptr obj, Extrema *module): object(obj), module(module) {}
        template<typename S>
        void operator()(S)
        {
            typedef Vec<S, Dim> V;
            typename V::const_ptr in(V::as(object));
            if (!in)
                return;

            module->handled = true;
            module->min.dim = Dim;
            module->max.dim = Dim;
            module->minIndex.dim = Dim;
            module->maxIndex.dim = Dim;
            module->minBlock.dim = Dim;
            module->maxBlock.dim = Dim;

            Index size = in->getSize();
#pragma omp parallel
            for (int c = 0; c < Dim; ++c) {
                S min = module->min[c];
                S max = module->max[c];
                const S *x = in->x(c).data();
                Index imin = InvalidIndex, imax = InvalidIndex;
                for (Index index = 0; index < size; index++) {
                    if (x[index] < min) {
                        min = x[index];
                        imin = index;
                    }
                    if (x[index] > max) {
                        max = x[index];
                        imax = index;
                    }
                }
                if (imin != InvalidIndex) {
                    module->min[c] = min;
                    module->minIndex[c] = imin;
                    module->minBlock[c] = in->getBlock();
                }
                if (imax != InvalidIndex) {
                    module->max[c] = max;
                    module->maxIndex[c] = imax;
                    module->maxBlock[c] = in->getBlock();
                }
            }
        }
    };
};

using namespace vistle;

Extrema::Extrema(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setReducePolicy(message::ReducePolicy::OverAll);

#ifdef BOUNDINGBOX
    Port *gin = createInputPort("grid_in", "input data", Port::MULTI);
    Port *gout = createOutputPort("grid_out", "bounding box", Port::MULTI);
    linkPorts(gin, gout);

    addIntParameter("per_block", "create bounding box for each block individually", false, Parameter::Boolean);
    perTimestepParam = addIntParameter("per_timestep", "create bounding box for each timestep individually", false,
                                       Parameter::Boolean);
    transformationNameParam =
        addStringParameter("transformation_name", "tag for derived transformation", "BoundingBox");
    Port *dout = createOutputPort("transform_out", "empty data object carrying transformation");
    linkPorts(gin, dout);
#else
    Port *din = createInputPort("data_in", "input data", Port::MULTI);
    Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
    linkPorts(din, dout);
#endif

    addVectorParameter("min", "output parameter: minimum",
                       ParamVector(std::numeric_limits<ParamVector::Scalar>::max(),
                                   std::numeric_limits<ParamVector::Scalar>::max(),
                                   std::numeric_limits<ParamVector::Scalar>::max()));
    addVectorParameter("max", "output parameter: maximum",
                       ParamVector(-std::numeric_limits<ParamVector::Scalar>::max(),
                                   -std::numeric_limits<ParamVector::Scalar>::max(),
                                   -std::numeric_limits<ParamVector::Scalar>::max()));
    addIntVectorParameter("min_block", "output parameter: block numbers containing minimum (per component)",
                          IntParamVector(-1, -1, -1));
    addIntVectorParameter("max_block", "output parameter: block numbers containing maximum (per component)",
                          IntParamVector(-1, -1, -1));
    addIntVectorParameter("min_index", "output parameter: indices of minimum (per component)",
                          IntParamVector(-1, -1, -1));
    addIntVectorParameter("max_index", "output parameter: indices of maximum (per component)",
                          IntParamVector(-1, -1, -1));

    for (const auto &p: {"min", "max", "min_block", "max_block", "min_index", "max_index"}) {
        setParameterReadOnly(p, true);
    }
}

Extrema::~Extrema()
{}

#ifdef BOUNDINGBOX
namespace {
Lines::ptr makeBox(Vector3 min, Vector3 max)
{
    Lines::ptr box(new Lines(4, 16, 8));
    Scalar *x[3];
    for (int i = 0; i < 3; ++i) {
        x[i] = box->x(i).data();
    }
    auto corners = box->cl().data();
    auto elements = box->el().data();
    for (int i = 0; i <= 4; ++i) { // include sentinel
        elements[i] = 4 * i;
    }
    corners[0] = 0;
    corners[1] = 1;
    corners[2] = 3;
    corners[3] = 2;

    corners[4] = 1;
    corners[5] = 5;
    corners[6] = 7;
    corners[7] = 3;

    corners[8] = 5;
    corners[9] = 4;
    corners[10] = 6;
    corners[11] = 7;

    corners[12] = 4;
    corners[13] = 0;
    corners[14] = 2;
    corners[15] = 6;

    for (int c = 0; c < 3; ++c) {
        int p = 1 << c;
        for (int i = 0; i < 8; ++i) {
            if (i & p)
                x[c][i] = max[c];
            else
                x[c][i] = min[c];
        }
    }

    return box;
}
} // namespace
#endif


bool Extrema::compute()
{
    //std::cerr << "Extrema: compute: generation=" << m_generation << std::endl;

    dim = -1;

#ifdef BOUNDINGBOX
    auto obj = expect<Object>("grid_in");
    auto split = splitContainerObject(obj);
    auto data = split.mapped;
    obj = split.geometry;
#else
    Object::const_ptr obj = expect<DataBase>("data_in");
#endif
    if (!obj)
        return true;

    for (int c = 0; c < MaxDim; ++c) {
        min[c] = std::numeric_limits<ParamVector::Scalar>::max();
        max[c] = -std::numeric_limits<ParamVector::Scalar>::max();
    }

    handled = false;

#ifdef BOUNDINGBOX
    if (auto coord = Coords::as(obj)) {
        const Index num = coord->getNumCoords();
        const Scalar *x = coord->x().data(), *y = coord->y().data(), *z = coord->z().data();
        auto t = obj->getTransform();
        if (t.isIdentity()) {
            for (Index i = 0; i < num; ++i) {
                Vector3 p(x[i], y[i], z[i]);
                for (int c = 0; c < 3; ++c) {
                    if (p[c] < min[c]) {
                        min[c] = p[c];
                        minIndex[c] = i;
                        minBlock[c] = obj->getBlock();
                    }
                    if (p[c] > max[c]) {
                        max[c] = p[c];
                        maxIndex[c] = i;
                        maxBlock[c] = obj->getBlock();
                    }
                }
            }
        } else {
            for (Index i = 0; i < num; ++i) {
                auto p = transformPoint(t, Vector3(x[i], y[i], z[i]));
                for (int c = 0; c < 3; ++c) {
                    if (p[c] < min[c]) {
                        min[c] = p[c];
                        minIndex[c] = i;
                        minBlock[c] = obj->getBlock();
                    }
                    if (p[c] > max[c]) {
                        max[c] = p[c];
                        maxIndex[c] = i;
                        maxBlock[c] = obj->getBlock();
                    }
                }
            }
        }
        min.dim = 3;
        max.dim = 3;
        minIndex.dim = 3;
        maxIndex.dim = 3;
        minBlock.dim = 3;
        maxBlock.dim = 3;
        handled = true;
    } else
#endif
        if (auto str = StructuredGridBase::as(obj)) {
        auto mm = str->getBounds();
        auto t = obj->getTransform();
        Vector3 corner[8];
        corner[0] = mm.first;
        corner[1] = mm.second;
        for (int i = 2; i < 8; ++i) {
            corner[i] = Vector3(corner[i % 2][0], corner[(i / 2) % 2][1], corner[(i / 4) % 2][2]);
        }
        for (int i = 0; i < 8; ++i) {
            corner[i] = transformPoint(t, corner[i]);
        }
        for (int i = 0; i < 8; ++i) {
            for (int c = 0; c < 3; ++c) {
                if (corner[i][c] < min[c]) {
                    min[c] = corner[i][c];
                    minIndex[c] = InvalidIndex;
                    minBlock[c] = obj->getBlock();
                }
                if (corner[i][c] > max[c]) {
                    max[c] = corner[i][c];
                    maxIndex[c] = InvalidIndex;
                    maxBlock[c] = obj->getBlock();
                }
            }
        }
        min.dim = 3;
        max.dim = 3;
        minIndex.dim = 3;
        maxIndex.dim = 3;
        minBlock.dim = 3;
        maxBlock.dim = 3;
        handled = true;
    } else {
        boost::mpl::for_each<Scalars>(Compute<1>(obj, this));
        boost::mpl::for_each<Scalars>(Compute<2>(obj, this));
        boost::mpl::for_each<Scalars>(Compute<3>(obj, this));
    }

    if (!handled) {
        std::string error("could not handle input");
        std::cerr << "Extrema: " << error << std::endl;
        throw(vistle::exception(error));
    }

    if (dim == -1) {
        dim = min.dim;
        gmin.dim = dim;
        gmax.dim = dim;
        gminIndex.dim = dim;
        gmaxIndex.dim = dim;
        gminBlock.dim = dim;
        gmaxBlock.dim = dim;
    } else if (dim != min.dim) {
        std::string error("input dimensions not equal");
        std::cerr << "Extrema: " << error << std::endl;
        throw(vistle::exception(error));
    }

#ifdef BOUNDINGBOX
    bool perBlock = getIntParameter("per_block");
    if (perBlock) {
        Lines::ptr box = makeBox(min, max);
        box->copyAttributes(obj);
        box->setTransform(Matrix4::Identity(4, 4));
        updateMeta(box);
        addObject("grid_out", box);
    }

    int ts = split.timestep;
    if (ts >= 0) {
        expandTimestepExtrema(ts + 1);
        for (int c = 0; c < MaxDim; ++c) {
            if (tmin[ts][c] > min[c]) {
                tmin[ts][c] = min[c];
            }
            if (tmax[ts][c] < max[c]) {
                tmax[ts][c] = max[c];
            }
        }
    }
#else
    Object::ptr out = obj->clone();
    out->addAttribute("min", min.str());
    out->addAttribute("max", max.str());
    out->addAttribute("minIndex", minIndex.str());
    out->addAttribute("maxIndex", maxIndex.str());
    //std::cerr << "Extrema: min " << min << ", max " << max << std::endl;

    updateMeta(out);
    addObject("data_out", out);
#endif

    for (int c = 0; c < MaxDim; ++c) {
        if (gmin[c] > min[c]) {
            gmin[c] = min[c];
            gminIndex[c] = minIndex[c];
            gminBlock[c] = minBlock[c];
        }
        if (gmax[c] < max[c]) {
            gmax[c] = max[c];
            gmaxIndex[c] = maxIndex[c];
            gmaxBlock[c] = maxBlock[c];
        }
    }

    if (Coords::as(obj) || StructuredGridBase::as(obj)) {
        haveGeometry = true;
    }

    return true;
}

bool Extrema::reduce(int timestep)
{
    //std::cerr << "reduction for timestep " << timestep << std::endl;

    if (timestep >= 0) {
#ifdef BOUNDINGBOX
        expandTimestepExtrema(timestep + 1);
        for (int i = 0; i < MaxDim; ++i) {
            tmin[timestep][i] =
                boost::mpi::all_reduce(comm(), tmin[timestep][i], boost::mpi::minimum<ParamVector::Scalar>());
            tmax[timestep][i] =
                boost::mpi::all_reduce(comm(), tmax[timestep][i], boost::mpi::maximum<ParamVector::Scalar>());
        }
#endif
    } else {
        for (int i = 0; i < MaxDim; ++i) {
            gmin[i] = boost::mpi::all_reduce(comm(), gmin[i], boost::mpi::minimum<ParamVector::Scalar>());
            gmax[i] = boost::mpi::all_reduce(comm(), gmax[i], boost::mpi::maximum<ParamVector::Scalar>());
        }

        setVectorParameter("min", gmin);
        setVectorParameter("max", gmax);
        setIntVectorParameter("min_block", gminBlock);
        setIntVectorParameter("max_block", gmaxBlock);
        setIntVectorParameter("min_index", gminIndex);
        setIntVectorParameter("max_index", gmaxIndex);
    }

#ifdef BOUNDINGBOX
    bool perTimestep = perTimestepParam->getValue() != 0;
    bool perBlock = getIntParameter("per_block");
    if (!perBlock && haveGeometry && rank() == 0) {
        if (perTimestep) {
            if (timestep >= 0 && boundsValid(timestep)) {
                Lines::ptr box = makeBox(tmin[timestep], tmax[timestep]);
                updateMeta(box);
                box->setTimestep(timestep);
                addObject("grid_out", box);
            }
        } else {
            Lines::ptr box = makeBox(gmin, gmax);
            updateMeta(box);
            addObject("grid_out", box);
        }
    }

    if (isConnected("transform_out") && timestep >= 0 && rank() == 0) {
        auto dummy = std::make_shared<Points>(0);
        if (timestep > 0 && boundsValid(timestep)) {
            Vector3 center0 =
                (Vector3(tmin[0][0], tmin[0][1], tmin[0][2]) + Vector3(tmax[0][0], tmax[0][1], tmax[0][2])) / 2.0;
            Vector3 center = (Vector3(tmin[timestep][0], tmin[timestep][1], tmin[timestep][2]) +
                              Vector3(tmax[timestep][0], tmax[timestep][1], tmax[timestep][2])) /
                             2.0;
            Vector3 translate = center - center0;
            Matrix4 translateMat(Matrix4::Identity());
            translateMat.col(3) << translate, 1;
            dummy->setTransform(translateMat);
        }
        updateMeta(dummy);
        dummy->setTimestep(timestep);
        dummy->addAttribute(attribute::Plugin, "VisObjectSensor");
        auto name = transformationNameParam->getValue();
        dummy->addAttribute(attribute::TransformName, name);
        addObject("transform_out", dummy);
    }
#endif

    return Module::reduce(timestep);
}

#ifdef BOUNDINGBOX
MODULE_MAIN(BoundingBox)
#else
MODULE_MAIN(Extrema)
#endif
