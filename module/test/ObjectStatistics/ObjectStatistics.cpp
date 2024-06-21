#include <sstream>
#include <limits>
#include <algorithm>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>

#include <vistle/module/module.h>
#include <vistle/core/vec.h>
#include <vistle/core/scalars.h>
#include <vistle/core/message.h>
#include <vistle/core/coords.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/indexed.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/alg/objalg.h>

using namespace vistle;

static const int NumDim = 4;

class ObjectStatistics: public vistle::Module {
public:
    ObjectStatistics(const std::string &name, int moduleID, mpi::communicator comm);
    ~ObjectStatistics();

    struct stats {
        Index blocks; //!< no. of blocks/partitions
        Index grids; //! no. of grid attachments
        Index normals; //! no. of normal attachments
        Index elements; //! no. of elements
        Index vertices; //! no. of vertices
        Index coords; //! no. of coordinates
        Index data[NumDim + 1]; //! no. of Scalar data values of corresponding dim
        Index idata[NumDim + 1]; //! no. of Index data values of corresponding dim

        stats(): blocks(0), grids(0), normals(0), elements(0), vertices(0), coords(0)
        {
            for (int d = 0; d <= NumDim; ++d) {
                data[d] = 0;
                idata[d] = 0;
            }
        }

        template<class Archive>
        void serialize(Archive &ar, const unsigned int version)
        {
            ar &blocks;
            ar &grids;
            ar &normals;
            ar &elements;
            ar &vertices;
            ar &coords;
            for (int i = 0; i <= NumDim; ++i) {
                ar &data[i];
            }
            for (int i = 0; i <= NumDim; ++i) {
                ar &idata[i];
            }
        }

        template<class operation>
        void apply(const operation &op, const stats &rhs)
        {
            blocks = op(blocks, rhs.blocks);
            grids = op(grids, rhs.grids);
            normals = op(normals, rhs.normals);
            elements = op(elements, rhs.elements);
            vertices = op(vertices, rhs.vertices);
            coords = op(coords, rhs.coords);
            for (int i = 0; i <= NumDim; ++i) {
                data[i] = op(data[i], rhs.data[i]);
            }
            for (int i = 0; i <= NumDim; ++i) {
                idata[i] = op(idata[i], rhs.idata[i]);
            }
        }

        template<class operation>
        static stats apply(const operation &op, const stats &lhs, const stats &rhs)
        {
            stats result = lhs;
            result.apply(op, rhs);
            return result;
        }

        stats &operator+=(const stats &rhs)
        {
            apply(std::plus<Index>(), rhs);
            return *this;
        }
    };
    vistle::IntParameter *continuousOutput = nullptr;

private:
    static const int MaxDim = ParamVector::MaxDimension;

    virtual bool compute();
    virtual bool reduce(int timestep);

    bool prepare();

    int m_timesteps; //!< no. of time steps
    std::map<int, int> m_objectsInTimestep;
    stats m_cur; //! current timestep
    stats m_min; //! min. values across all timesteps
    stats m_max; //! max. values across all timesteps
    stats m_total; //! accumulated values
    std::map<std::string, Index> m_types; //!< object type counts
};

std::ostream &operator<<(std::ostream &str, const ObjectStatistics::stats &s)
{
    str << "blocks: " << s.blocks << ", with grid: " << s.grids << ", with normals: " << s.normals
        << ", elements: " << s.elements << ", vertices: " << s.vertices << ", coords: " << s.coords
        << ", 1-dim data: " << s.data[1] << ", 3-dim data: " << s.data[3] << ", index data: " << s.idata[1];

    return str;
}

template<typename T>
struct minimum {
    T operator()(const T &lhs, const T &rhs) const { return std::min<T>(lhs, rhs); }
};

template<>
struct minimum<ObjectStatistics::stats> {
    ObjectStatistics::stats operator()(const ObjectStatistics::stats &lhs, const ObjectStatistics::stats &rhs)
    {
        return ObjectStatistics::stats::apply(minimum<Index>(), lhs, rhs);
    }
};

template<typename T>
struct maximum {
    T operator()(const T &lhs, const T &rhs) const { return std::max<T>(lhs, rhs); }
};

template<>
struct maximum<ObjectStatistics::stats> {
    ObjectStatistics::stats operator()(const ObjectStatistics::stats &lhs, const ObjectStatistics::stats &rhs)
    {
        return ObjectStatistics::stats::apply(maximum<Index>(), lhs, rhs);
    }
};

ObjectStatistics::stats operator+(const ObjectStatistics::stats &lhs, const ObjectStatistics::stats &rhs)
{
    return ObjectStatistics::stats::apply(std::plus<Index>(), lhs, rhs);
}

using namespace vistle;

ObjectStatistics::ObjectStatistics(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    setReducePolicy(message::ReducePolicy::OverAll);
    continuousOutput =
        addIntParameter("continuous_output", "additionally write info of received objects as they arrive", false,
                        vistle::Parameter::Boolean);
    createInputPort("data_in", "input data", Port::MULTI);
}

ObjectStatistics::~ObjectStatistics()
{}

bool ObjectStatistics::compute()
{
    //std::cerr << "ObjectStatistics: compute: generation=" << m_generation << std::endl;

    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    auto split = splitContainerObject(obj);

    if (split.timestep + 1 > m_timesteps)
        m_timesteps = split.timestep + 1;

    stats s;
    if (split.normals) {
        ++s.normals;
    }
    if (split.geometry) {
        ++s.grids;
    }
    if (auto i = Indexed::as(split.geometry)) {
        s.elements = i->getNumElements();
        s.vertices = i->getNumCorners();
    } else if (auto t = Triangles::as(obj)) {
        s.elements = t->getNumElements();
        s.vertices = t->getNumCorners();
    } else if (auto q = Quads::as(obj)) {
        s.elements = q->getNumElements();
        s.vertices = q->getNumCorners();
    } else if (auto sg = StructuredGridBase::as(obj)) {
        s.elements = sg->getNumElements();
        s.vertices = sg->getNumVertices();
    }
    if (auto c = Coords::as(split.geometry)) {
        s.coords = c->getNumCoords();
    }

    if (auto d = split.mapped) {
        if (auto v = Vec<Scalar, 4>::as(d)) {
            s.data[4] = v->getSize();
        } else if (auto v = Vec<Scalar, 3>::as(d)) {
            s.data[3] = v->getSize();
        } else if (auto v = Vec<Scalar, 2>::as(d)) {
            s.data[2] = v->getSize();
        } else if (auto v = Vec<Scalar, 1>::as(d)) {
            s.data[1] = v->getSize();
        }
        if (auto v = Vec<Index, 4>::as(d)) {
            s.idata[4] = v->getSize();
        } else if (auto v = Vec<Index, 3>::as(d)) {
            s.idata[3] = v->getSize();
        } else if (auto v = Vec<Index, 2>::as(d)) {
            s.idata[2] = v->getSize();
        } else if (auto v = Vec<Index, 1>::as(d)) {
            s.idata[1] = v->getSize();
        }
    }
    s.blocks = 1;
    m_cur += s;
    if (continuousOutput->getValue()) {
        int count = m_objectsInTimestep[obj->getTimestep()]++;
        std::stringstream msg;
        msg << "ObjectAnalysis received " << Object::toString(obj->getType()) << std::endl;
        ;
        msg << "[" << rank() << "/" << size() << "] " << count << "th object in timestep " << obj->getTimestep()
            << std::endl;
        msg << s;
        msg << "___________________________________________________" << std::endl;
        sendInfo(msg.str());
    }
    return true;
}

bool ObjectStatistics::prepare()
{
    m_timesteps = 0;
    m_types.clear();

    m_total = stats();
    m_min = stats();
    m_max = stats();
    m_cur = stats();

    return true;
}

bool ObjectStatistics::reduce(int timestep)
{
    //std::cerr << "reduction for timestep " << timestep << std::endl;
    boost::mpi::reduce(comm(), m_cur, m_min, minimum<stats>(), 0);
    boost::mpi::reduce(comm(), m_cur, m_max, maximum<stats>(), 0);
    stats total;
    boost::mpi::reduce(comm(), m_cur, total, std::plus<stats>(), 0);
    m_total += total;

    int timesteps = 0;
    boost::mpi::reduce(comm(), m_timesteps, timesteps, maximum<int>(), 0);

    if (rank() == 0) {
        std::stringstream str;

        if (timestep >= 0) {
            str << "timestep " << timestep << ": " << total << std::endl;
        } else {
            str << "total";
            if (timesteps > 0)
                str << " (" << timesteps << " timesteps)";
            str << ": ";
            str << m_total << std::endl;
            str << "  rank min: " << m_min << std::endl;
            str << "  rank max: " << m_max << std::endl;
        }

        sendInfo(str.str());
    }

    m_cur = stats();

    return Module::reduce(timestep);
}

MODULE_MAIN(ObjectStatistics)
