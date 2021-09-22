#include <sstream>
#include <iomanip>

#include <vistle/core/message.h>
#include <vistle/core/object.h>
#include <vistle/core/unstr.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/vec.h>
#include <vistle/core/triangles.h>
#include <vistle/core/vector.h>
#include <vistle/util/math.h>

#include <boost/mpi/collectives.hpp>

#include "IsoSurface.h"
#define ONLY_HEXAHEDRON
#include "../../general/IsoSurface/tables.h"

MODULE_MAIN(IsoSurfaceOld)

using namespace vistle;


IsoSurfaceOld::IsoSurfaceOld(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    setDefaultCacheMode(ObjectCache::CacheDeleteLate);
    setReducePolicy(message::ReducePolicy::OverAll);

    createInputPort("grid_in");
    createInputPort("data_in");

    createOutputPort("grid_out");

    m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);
}

IsoSurfaceOld::~IsoSurfaceOld()
{}

bool IsoSurfaceOld::prepare()
{
    min = std::numeric_limits<Scalar>::max();
    max = -std::numeric_limits<Scalar>::max();
    return Module::prepare();
}

bool IsoSurfaceOld::reduce(int timestep)
{
    min = boost::mpi::all_reduce(comm(), min, boost::mpi::minimum<Scalar>());
    max = boost::mpi::all_reduce(comm(), max, boost::mpi::maximum<Scalar>());

    setParameterRange(m_isovalue, (double)min, (double)max);

    return Module::reduce(timestep);
}

const Scalar EPSILON = 1.0e-10f;

inline Vector interp(Scalar iso, const Vector &p0, const Vector &p1, const Scalar &f0, const Scalar &f1)
{
    Scalar diff = (f1 - f0);

    if (fabs(diff) < EPSILON)
        return p0;

    if (fabs(iso - f0) < EPSILON)
        return p0;

    if (fabs(iso - f1) < EPSILON)
        return p1;

    Scalar t = (iso - f0) / diff;

    return lerp(p0, p1, t);
}

class Leveller {
    UnstructuredGrid::const_ptr m_grid;
    std::vector<Object::const_ptr> m_data;
    Scalar m_isoValue;

    const Byte *tl;
    const Index *el;
    const Index *cl;
    const Scalar *x;
    const Scalar *y;
    const Scalar *z;

    const Scalar *d;

    Index *out_cl;
    Scalar *out_x;
    Scalar *out_y;
    Scalar *out_z;
    Scalar *out_d;

    Triangles::ptr m_triangles;
    Vec<Scalar>::ptr m_outData;

    Scalar gmin, gmax;

public:
    Leveller(UnstructuredGrid::const_ptr grid, const Scalar isovalue)
    : m_grid(grid)
    , m_isoValue(isovalue)
    , tl(nullptr)
    , el(nullptr)
    , cl(nullptr)
    , x(nullptr)
    , y(nullptr)
    , z(nullptr)
    , d(nullptr)
    , out_cl(nullptr)
    , out_x(nullptr)
    , out_y(nullptr)
    , out_z(nullptr)
    , out_d(nullptr)
    , gmin(std::numeric_limits<Scalar>::max())
    , gmax(-std::numeric_limits<Scalar>::max())
    {
        tl = &grid->tl()[0];
        el = &grid->el()[0];
        cl = &grid->cl()[0];
        x = &grid->x()[0];
        y = &grid->y()[0];
        z = &grid->z()[0];

        m_triangles = Triangles::ptr(new Triangles(Object::Initialized));
        m_triangles->setMeta(grid->meta());
    }

    void addData(Object::const_ptr obj)
    {
        m_data.push_back(obj);
        auto data = Vec<Scalar, 1>::as(obj);
        if (data) {
            d = &data->x()[0];

            m_outData = Vec<Scalar>::ptr(new Vec<Scalar>(Object::Initialized));
            m_outData->setMeta(data->meta());
        }
    }

    bool process()
    {
        const Index numElem = m_grid->getNumElements();

        std::vector<Index> outputIdxV(numElem + 1);
        auto outputIdx = outputIdxV.data();
        outputIdx[0] = 0;
#pragma omp parallel
        {
            Scalar tmin = std::numeric_limits<Scalar>::max();
            Scalar tmax = -std::numeric_limits<Scalar>::max();
#pragma omp for
            for (ssize_t elem = 0; elem < ssize_t(numElem); ++elem) {
                Index n = 0;
                switch (tl[elem]) {
                case UnstructuredGrid::HEXAHEDRON: {
                    n = processHexahedron(elem, 0, true /* count only */, tmin, tmax);
                }
                }

                outputIdx[elem + 1] = n;
            }
#pragma omp critical
            {
                if (tmin < gmin)
                    gmin = tmin;
                if (tmax > gmax)
                    gmax = tmax;
            }
        }

        for (Index elem = 0; elem < numElem; ++elem) {
            outputIdx[elem + 1] += outputIdx[elem];
        }

        Index numVert = outputIdx[numElem];

        //std::cerr << "IsoSurfaceOld: " << numVert << " vertices" << std::endl;

        m_triangles->cl().resize(numVert);
        out_cl = m_triangles->cl().data();
        m_triangles->x().resize(numVert);
        out_x = m_triangles->x().data();
        m_triangles->y().resize(numVert);
        out_y = m_triangles->y().data();
        m_triangles->z().resize(numVert);
        out_z = m_triangles->z().data();

        if (m_outData) {
            m_outData->x().resize(numVert);
            out_d = m_outData->x().data();
        }

#pragma omp parallel for
        for (ssize_t elem = 0; elem < ssize_t(numElem); ++elem) {
            switch (tl[elem]) {
            case UnstructuredGrid::HEXAHEDRON: {
                processHexahedron(elem, outputIdx[elem], false, gmin, gmax);
            }
            }
        }

        //std::cerr << "CuttingSurface: << " << m_outData->x().size() << " vert, " << m_outData->x().size() << " data elements" << std::endl;

        return true;
    }

    Object::ptr result() { return m_triangles; }

    std::pair<Scalar, Scalar> range() { return std::make_pair(gmin, gmax); }

private:
    Index processHexahedron(const Index elem, const Index outIdx, bool numVertsOnly, Scalar &min, Scalar &max)
    {
        const Index p = el[elem];

        Index index[8];
        index[0] = cl[p + 5];
        index[1] = cl[p + 6];
        index[2] = cl[p + 2];
        index[3] = cl[p + 1];
        index[4] = cl[p + 4];
        index[5] = cl[p + 7];
        index[6] = cl[p + 3];
        index[7] = cl[p];

        Scalar field[8];
        for (int idx = 0; idx < 8; idx++) {
            field[idx] = d[index[idx]];
        }

        int tableIndex = 0;
        for (int idx = 0; idx < 8; idx++)
            tableIndex += (((int)(field[idx] < m_isoValue)) << idx);

        const int numVerts = hexaNumVertsTable[tableIndex];

        if (numVertsOnly) {
            for (int idx = 0; idx < 8; ++idx) {
                if (field[idx] < min)
                    min = field[idx];
                if (field[idx] > max)
                    max = field[idx];
            }
            return numVerts;
        }

        if (numVerts > 0) {
            Vector v[8];
            for (int idx = 0; idx < 8; idx++) {
                v[idx][0] = x[index[idx]];
                v[idx][1] = y[index[idx]];
                v[idx][2] = z[index[idx]];
            }

            Vector vertlist[12];
            vertlist[0] = interp(m_isoValue, v[0], v[1], field[0], field[1]);
            vertlist[1] = interp(m_isoValue, v[1], v[2], field[1], field[2]);
            vertlist[2] = interp(m_isoValue, v[2], v[3], field[2], field[3]);
            vertlist[3] = interp(m_isoValue, v[3], v[0], field[3], field[0]);

            vertlist[4] = interp(m_isoValue, v[4], v[5], field[4], field[5]);
            vertlist[5] = interp(m_isoValue, v[5], v[6], field[5], field[6]);
            vertlist[6] = interp(m_isoValue, v[6], v[7], field[6], field[7]);
            vertlist[7] = interp(m_isoValue, v[7], v[4], field[7], field[4]);

            vertlist[8] = interp(m_isoValue, v[0], v[4], field[0], field[4]);
            vertlist[9] = interp(m_isoValue, v[1], v[5], field[1], field[5]);
            vertlist[10] = interp(m_isoValue, v[2], v[6], field[2], field[6]);
            vertlist[11] = interp(m_isoValue, v[3], v[7], field[3], field[7]);

            for (int idx = 0; idx < numVerts; idx += 3) {
                for (int i = 0; i < 3; ++i) {
                    int edge = hexaTriTable[tableIndex][idx + i];
                    Vector &v = vertlist[edge];

                    out_x[outIdx + idx + i] = v[0];
                    out_y[outIdx + idx + i] = v[1];
                    out_z[outIdx + idx + i] = v[2];

                    out_cl[outIdx + idx + i] = outIdx + idx + i;
                }
            }
        }

        return numVerts;
    }
};


Object::ptr IsoSurfaceOld::generateIsoSurface(Object::const_ptr grid_object, Object::const_ptr data_object,
                                              const Scalar isoValue)
{
    if (!grid_object || !data_object)
        return Object::ptr();

    Vec<Scalar>::const_ptr data = Vec<Scalar>::as(data_object);
    if (!data) {
        std::cerr << "IsoSurfaceOld: incompatible data input: no Scalars" << std::endl;
        return Object::ptr();
    }

    RectilinearGrid::const_ptr rgrid = RectilinearGrid::as(grid_object);
    if (rgrid) {
    }

    UnstructuredGrid::const_ptr grid = UnstructuredGrid::as(grid_object);
    if (grid) {
        Leveller l(grid, isoValue);
        l.addData(data);
        l.process();

        auto range = l.range();
        if (range.first < min)
            min = range.first;
        if (range.second > max)
            max = range.second;

        return l.result();
    }

    std::cerr << "IsoSurfaceOld: incompatible grid input: no UnstructuredGrid or RectilinearGrid" << std::endl;
    return Object::ptr();
}


bool IsoSurfaceOld::compute()
{
    const Scalar isoValue = getFloatParameter("isovalue");

    Object::const_ptr grid = expect<Object>("grid_in");
    Object::const_ptr data = expect<Object>("data_in");
    if (!grid || !data)
        return false;

    Object::ptr object = generateIsoSurface(grid, data, isoValue);

    if (object) {
        object->copyAttributes(data);
        object->copyAttributes(grid, false);
        addObject("grid_out", object);
    }

    return true;
}
