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
#include <vistle/core/structuredgridbase.h>
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
    static const int MaxDim = ParamVector::MaxDimension;

    int dim;
    bool handled;
    bool haveGeometry;
    ParamVector min, max, gmin, gmax;
    IntParamVector minIndex, maxIndex, gminIndex, gmaxIndex;
    IntParamVector minBlock, maxBlock, gminBlock, gmaxBlock;

    virtual bool compute();
    virtual bool reduce(int timestep);

    template<int Dim>
    friend struct Compute;

    bool prepare()
    {
        haveGeometry = false;

        for (int c = 0; c < MaxDim; ++c) {
            gmin[c] = std::numeric_limits<ParamVector::Scalar>::max();
            gmax[c] = -std::numeric_limits<ParamVector::Scalar>::max();
            gminIndex[c] = InvalidIndex;
            gmaxIndex[c] = InvalidIndex;
        }

        return true;
    }

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
                const S *x = &in->x(c)[0];
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
    gin->link(gout);

    addIntParameter("per_block", "create bounding box for each block", false, Parameter::Boolean);
#else
    Port *din = createInputPort("data_in", "input data", Port::MULTI);
    Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
    din->link(dout);
#endif

    addVectorParameter("min", "output parameter: minimum",
                       ParamVector(std::numeric_limits<ParamVector::Scalar>::max(),
                                   std::numeric_limits<ParamVector::Scalar>::max(),
                                   std::numeric_limits<ParamVector::Scalar>::max()));
    addVectorParameter("max", "output parameter: maximum",
                       ParamVector(-std::numeric_limits<ParamVector::Scalar>::max(),
                                   -std::numeric_limits<ParamVector::Scalar>::max(),
                                   -std::numeric_limits<ParamVector::Scalar>::max()));
    addIntVectorParameter("min_block", "output parameter: block numbers containing minimum",
                          IntParamVector(-1, -1, -1));
    addIntVectorParameter("max_block", "output parameter: block numbers containing maximum",
                          IntParamVector(-1, -1, -1));
    addIntVectorParameter("min_index", "output parameter: indices of minimum", IntParamVector(-1, -1, -1));
    addIntVectorParameter("max_index", "output parameter: indices of maximum", IntParamVector(-1, -1, -1));
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
    //std::cerr << "Extrema: compute: execcount=" << m_executionCount << std::endl;

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
        auto t = obj->getTransform();
        const Index num = coord->getNumCoords();
        const Scalar *x = &coord->x()[0], *y = &coord->y()[0], *z = &coord->z()[0];
        for (Index i = 0; i < num; ++i) {
            auto p = transformPoint(t, Vector3(x[i], y[i], z[i]));
            for (int c = 0; c < 3; ++c) {
                if (p[c] < min[c]) {
                    min[c] = p[c];
                    minIndex[c] = InvalidIndex;
                    minBlock[c] = obj->getBlock();
                }
                if (p[c] > max[c]) {
                    max[c] = p[c];
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

#ifdef BOUNDINGBOX
    bool perBlock = getIntParameter("per_block");
    if (!perBlock && haveGeometry && rank() == 0) {
        Lines::ptr box = makeBox(gmin, gmax);
        updateMeta(box);
        addObject("grid_out", box);
    }
#endif

    return Module::reduce(timestep);
}

#ifdef BOUNDINGBOX
MODULE_MAIN(BoundingBox)
#else
MODULE_MAIN(Extrema)
#endif
