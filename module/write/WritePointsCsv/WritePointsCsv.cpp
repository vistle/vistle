#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/alg/objalg.h>
#include <vistle/core/geometry.h>

#include "WritePointsCsv.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(AttachmentPoint, (Bottom)(Middle)(Top))

MODULE_MAIN(WritePointsCsv)

using namespace vistle;

WritePointsCsv::WritePointsCsv(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    m_filename = addStringParameter("filename", "output filename", "points.csv", Parameter::Filename);
    setParameterFilters(m_filename, "CSV files (*.csv)");

    for (int i = 0; i < NumPorts; ++i) {
        std::string name = "data_in";
        if (i > 0)
            name += std::to_string(i);
        m_port[i] = createInputPort(name, "mapped vertex data field (or grid)");
    }

    m_writeHeader = addIntParameter("write_header", "write header line", true, Parameter::Boolean);
    m_writeCoordinates =
        addIntParameter("write_coordinates", "write coordinates of associated point", true, Parameter::Boolean);
    m_writeData = addIntParameter("write_data", "write mapped data", true, Parameter::Boolean);
    m_writeTime = addIntParameter("write_time", "write timestep number", true, Parameter::Boolean);
    m_writeBlock = addIntParameter("write_block", "write block number", true, Parameter::Boolean);
    m_writeIndex = addIntParameter("write_index", "write vertex index", true, Parameter::Boolean);
}

bool WritePointsCsv::prepare()
{
    return true;
}

bool WritePointsCsv::reduce(int t)
{
    if (t < 0) {
        m_csv.close();
    }
    return true;
}

namespace {

class Csv {
    std::ofstream &m_of;
    const std::string m_sep;
    bool m_first = true;

public:
    Csv(std::ofstream &of, const std::string separator = ","): m_of(of), m_sep(separator) { assert(m_of.is_open()); }
    ~Csv() = default;

    void end()
    {
        m_of << std::endl;
        m_first = true;
    }

    void sep()
    {
        if (!m_first) {
            m_of << m_sep;
        }
        m_first = false;
    }

    Csv &operator<<(Csv &(*func)(Csv &)) { return func(*this); }

    Csv &operator<<(const char *val)
    {
        sep();
        m_of << '"' << val << '"';
        return *this;
    }

    Csv &operator<<(const std::string &val)
    {
        sep();
        m_of << '"' << val << '"';
        return *this;
    }

    template<typename T>
    Csv &operator<<(const T &val)
    {
        sep();
        m_of << val;
        return *this;
    }
};


inline static Csv &end(Csv &file)
{
    file.end();
    return file;
}

} // namespace

bool WritePointsCsv::compute()
{
    const std::string sep = ", ";

    int timestep = -1, block = -1;
    std::array<DataBase::const_ptr, NumPorts> data;
    Object::const_ptr grid;
    for (int i = 0; i < NumPorts; ++i) {
        if (m_port[i]->isConnected()) {
            auto in = expect<Object>(m_port[i]);
            if (!in) {
                continue;
            }
            auto split = splitContainerObject(in);
            if (!grid) {
                grid = split.geometry;
            } else if (grid != split.geometry) {
                sendError("multiple geometries");
                return true;
            }

            if (split.mapped) {
                if (split.mapped->guessMapping() != DataBase::Vertex) {
                    auto pname = m_port[i]->getName();
                    sendError("no per-vertex mapping for data on port %s, ignoring", pname.c_str());
                    continue;
                }
                data[i] = split.mapped;
                if (timestep < 0) {
                    timestep = split.timestep;
                }
                if (block < 0) {
                    block = split.block;
                }
            }
        }
    }

    if (!grid) {
        sendError("geometry without grid");
        return true;
    }

    auto geo = grid->getInterface<GeometryInterface>();
    if (!geo) {
        sendError("geometry without geometry interface");
        return true;
    }

    bool needHeader = false;
    if (!m_csv.is_open()) {
        needHeader = m_writeHeader->getValue() != 0;
        auto filename = m_filename->getValue();
        m_csv.open(filename.c_str());

        if (!m_csv.is_open()) {
            sendError("could not open file %s", filename.c_str());
            return true;
        }
    }

    Csv csv(m_csv, sep);

    if (needHeader) {
        std::array<std::string, 4> suffix{"x", "y", "z", "w"};
        if (m_writeTime->getValue()) {
            csv << "time";
        }
        if (m_writeBlock->getValue()) {
            csv << "block";
        }
        if (m_writeIndex->getValue()) {
            csv << "index";
        }
        if (m_writeCoordinates->getValue()) {
            for (int i = 0; i < 3; ++i) {
                csv << suffix[i];
            }
        }
        for (int i = 0; i < NumPorts; ++i) {
            if (data[i]) {
                std::string name = data[i]->getAttribute(attribute::Species);
                if (name.empty()) {
                    name = "data" + std::to_string(i);
                }
                if (data[i]->dimension() == 1) {
                    csv << name;
                } else {
                    for (Index j = 0; j < data[i]->dimension(); ++j) {
                        csv << name + "." + suffix[j];
                    }
                }
            }
        }
        csv << end;
    }

    auto nvert = geo->getNumVertices();
    for (Index i = 0; i < nvert; ++i) {
        if (m_writeTime->getValue()) {
            csv << timestep;
        }
        if (m_writeBlock->getValue()) {
            csv << block;
        }
        if (m_writeIndex->getValue()) {
            csv << i;
        }
        if (m_writeCoordinates->getValue()) {
            auto coords = geo->getVertex(i);
            csv << coords[0] << coords[1] << coords[2];
        }
        for (int j = 0; j < NumPorts; ++j) {
            if (data[j]) {
                for (unsigned k = 0; k < data[j]->dimension(); ++k) {
                    auto val = data[j]->value(i, k);
                    csv << val;
                }
            }
        }
        csv << end;
    }

    return true;
}
