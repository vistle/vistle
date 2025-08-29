#include <sstream>
#include <iomanip>
#include <string>

#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include <google/profiler.h>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/polygons.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/unstr.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>

#include <boost/algorithm/string/predicate.hpp>

#include <vistle/util/filesystem.h>
#include <vistle/util/fileio.h>

#include <file/covReadFiles.h>

#ifdef READ_DIRECTORY
#define ReadCovise ReadCoviseDirectory
#endif

#include "ReadCovise.h"

MODULE_MAIN(ReadCovise)

#ifdef READ_DIRECTORY
namespace {
const std::string extension(".covise");
const std::string NONE = vistle::Reader::InvalidChoice;
} // namespace
#endif

using namespace vistle;

ReadCovise::ReadCovise(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
#ifdef READ_DIRECTORY
    m_directory =
        addStringParameter("directory", "directory to scan for .covise files", "", Parameter::ExistingDirectory);
    m_fieldFile[0] = addStringParameter("grid", "filename for grid", NONE, Parameter::Choice);
#else
    m_gridFile = addStringParameter("filename", "name of COVISE file", "", Parameter::ExistingFilename);
    setParameterFilters(m_gridFile, "COVISE Files (*.covise)");
    m_fieldFile[0] = m_gridFile;
#endif
    m_out[0] = createOutputPort("grid_out", "grid or geometry");
    linkPortAndParameter(m_out[0], m_fieldFile[0]);

#ifdef READ_DIRECTORY
    m_fieldFile[1] = addStringParameter("normals", "name of COVISE file for normals", NONE, Parameter::Choice);
#else
    m_fieldFile[1] = addStringParameter("normals", "name of COVISE file for normals", "", Parameter::ExistingFilename);
    setParameterFilters(m_fieldFile[1], "COVISE Files (*.covise)");
#endif
    m_out[1] = nullptr;
    for (unsigned i = 2; i < NumPorts; ++i) {
#ifdef READ_DIRECTORY
        m_fieldFile[i] =
            addStringParameter("field" + std::to_string(i - 2), "name of COVISE file for field " + std::to_string(i),
                               NONE, Parameter::Choice);
#else
        m_fieldFile[i] =
            addStringParameter("field" + std::to_string(i - 2), "name of COVISE file for field " + std::to_string(i),
                               "", Parameter::ExistingFilename);
        setParameterFilters(m_fieldFile[i], "COVISE Files (*.covise)");
#endif
        m_out[i] = createOutputPort("field" + std::to_string(i - 2) + "_out", "data mapped to geometry");
        linkPortAndParameter(m_out[i], m_fieldFile[i]);
    }

#ifdef READ_DIRECTORY
    for (unsigned i = 0; i < NumPorts; ++i) {
        setParameterChoices(m_fieldFile[i], std::vector<std::string>({NONE}));
    }
#endif

    for (unsigned i = 0; i < NumPorts; ++i) {
        m_fd[i] = 0;
        m_numObj[i] = 0;
        m_numTime[i] = -1;
    }

    setParallelizationMode(ParallelizeTimeAndBlocks);
    setAllowTimestepDistribution(true);

#ifdef READ_DIRECTORY
    observeParameter(m_directory);
#endif
    for (unsigned port = 0; port < NumPorts; ++port) {
        observeParameter(m_fieldFile[port]);
    }
}

ReadCovise::~ReadCovise()
{}

bool ReadCovise::examine(const Parameter *param)
{
#ifdef READ_DIRECTORY
    if (!param || param == m_directory) {
        std::vector<std::string> choices;
        choices.emplace_back(NONE);
        auto dirstr = m_directory->getValue();
        auto dir = filesystem::path(dirstr);
        try {
            if (!filesystem::is_directory(dir)) {
                sendError("not a directory: %s", dir.string().c_str());
                return false;
            }
        } catch (const filesystem::filesystem_error &e) {
            if (!dirstr.empty())
                sendError("error while opening %s", dir.string().c_str());
            return false;
        }

        for (filesystem::directory_iterator it(dir); it != filesystem::directory_iterator(); ++it) {
            filesystem::path ent(*it);
            if (ent.extension().string() != extension) {
                continue;
            }

            std::string stem = ent.stem().string();
            if (stem.empty()) {
                continue;
            }

            std::string file = ent.string();
            int fd = covOpenInFile(file.c_str());
            if (fd == 0) {
                continue;
            }
            covCloseInFile(fd);

            choices.push_back(stem);
        }

        for (unsigned port = 0; port < NumPorts; ++port) {
            setParameterChoices(m_fieldFile[port], choices);
        }
    }
#else
    for (unsigned i = 1; i < NumPorts; ++i) {
        if (param == m_fieldFile[i]) {
            std::string file = m_fieldFile[i]->getValue();
            int fd = covOpenInFile(file.c_str());
            if (fd == 0) {
                if (!file.empty())
                    sendError("failed to open %s or not a COVISE file", file.c_str());
                return false;
            }
            covCloseInFile(fd);
        }
    }
#endif

    return true;
}

bool ReadCovise::prepareRead()
{
    m_transposed.clear();
#ifdef READ_DIRECTORY
    auto dir = filesystem::path(m_directory->getValue());
#endif
    for (unsigned port = 0; port < NumPorts; ++port) {
        m_numTime[port] = -1;

        m_objects[port].clear();

        assert(m_fd[port] == 0);

#ifdef READ_DIRECTORY
        std::string name;
        m_species[port] = m_fieldFile[port]->getValue();
        if (m_fieldFile[port]->getValue() != NONE) {
            filesystem::path path(dir);
            path += path.preferred_separator;
            path += m_fieldFile[port]->getValue();
            path += extension;
            name = path.string();
        }
#else
        const std::string name = m_fieldFile[port]->getValue();
        std::string species = name;
        if (boost::algorithm::ends_with(species, ".covise")) {
            species = species.substr(0, species.length() - std::string(".covise").length());
        }
        auto last_slash = species.find_last_of('/');
        if (last_slash != std::string::npos) {
            species = species.substr(last_slash + 1);
        }
        m_species[port] = species;
#endif
        m_filename[port] = name;
        if (name.empty())
            continue;

        m_fd[port] = covOpenInFile(name.c_str());
        if (!m_fd[port]) {
            if (!name.empty())
                sendError("failed to open %s or not a COVISE file", name.c_str());
            m_filename[port].clear();
        }
    }

    for (unsigned port = 0; port < NumPorts; ++port) {
        if (m_fd[port] == 0)
            continue;

        try {
            readSkeleton(port, &m_rootElement[port]);
        } catch (vistle::exception &e) {
            finishRead();
            throw(e);
            return false;
        }

        if (port > 0) {
            if (m_numTime[port] != m_numTime[0]) {
                sendError("number of timesteps in %s (%d) and %s (%d) do not match", m_filename[0].c_str(),
                          m_numTime[0], m_filename[port].c_str(), m_numTime[port]);
                finishRead();
                return false;
            }
        }
    }

    for (unsigned port = 0; port < NumPorts; ++port) {
        if (m_fd[port] != 0) {
            covCloseInFile(m_fd[port]);
            m_fd[port] = 0;
        }
    }

    sendInfo("reading %d timesteps", (int)m_numTime[0]);

    setTimesteps(m_numTime[0]);

    return true;
}

bool ReadCovise::read(Reader::Token &token, int timestep, int block)
{
    std::cerr << "reading t=" << timestep << ", block=" << block << std::endl;
    Element *elem[NumPorts];
    int fd[NumPorts];
    for (unsigned port = 0; port < NumPorts; ++port) {
        elem[port] = &m_rootElement[port];
        if (m_filename->empty()) {
            fd[port] = 0;
        } else {
            fd[port] = covOpenInFile(m_filename[port].c_str());
#if 0
            if (fd[port] == 0)
                return false;
#endif
        }
    }
    bool ret = readRecursive(token, fd, elem, -1, timestep);
    for (unsigned port = 0; port < NumPorts; ++port) {
        if (fd[port])
            covCloseInFile(fd[port]);
    }
    return ret;
}

bool ReadCovise::finishRead()
{
    m_transposed.clear();
    for (unsigned port = 0; port < NumPorts; ++port) {
        if (m_fd[port]) {
            covCloseInFile(m_fd[port]);
            m_fd[port] = 0;
        }
        deleteRecursive(m_rootElement[port]);
        m_numObj[port] = 0;
        m_objects[port].clear();
    }
    return true;
}

static off_t mytell(const int fd)
{
    return lseek(abs(fd), 0, SEEK_CUR);
}

static off_t seek(const int fd, off_t off)
{
    return lseek(abs(fd), off, SEEK_SET);
}

static int findBlockNum(const Element &elem)
{
    bool isTimestep = false;
    for (size_t i = 0; i < elem.attribs.size(); ++i) {
        const std::pair<std::string, std::string> &att = elem.attribs[i];
        if (att.first == "TIMESTEP")
            isTimestep = true;
    }
    int block = elem.index;
    if (elem.parent && (isTimestep || block < 0)) {
        block = findBlockNum(*elem.parent);
    }
    return block;
}

void ReadCovise::applyAttributes(Token &token, Object::ptr obj, const Element &elem, int index)
{
    if (elem.parent) {
        applyAttributes(token, obj, *elem.parent, elem.index);
    }

    bool isTimestep = false;
    for (size_t i = 0; i < elem.attribs.size(); ++i) {
        const std::pair<std::string, std::string> &att = elem.attribs[i];
        if (att.first == "TIMESTEP") {
            isTimestep = true;
        } else if (att.first == "COLOR") {
            obj->addAttribute(attribute::Color, att.second);
        } else {
            obj->addAttribute(att.first, att.second);
        }
    }

    if (index != -1) {
        if (isTimestep) {
            if (obj->getTimestep() != -1) {
                std::cerr << "ReadCovise: multiple TIMESTEP attributes in object hierarchy" << std::endl;
            }
#if 1
            if (elem.parent)
                obj->setNumTimesteps(elem.parent->subelems.size());
#endif
        } else if (obj->getBlock() == -1) {
            obj->setBlock(index);
#if 0
         if (elem.parent)
            obj->setNumBlocks(elem.parent->subelems.size());
#endif
        }
        token.applyMeta(obj);

        if (!isTimestep) {
            std::string set = obj->getAttribute(attribute::PartOf);
            if (!set.empty())
                set = set + "_";
            std::stringstream idx;
            idx << index;
            set = set + idx.str();
            obj->addAttribute(attribute::PartOf, set);
        }
    }
}

static void parseAttributes(Element *elem)
{
    for (size_t i = 0; i < elem->attribs.size(); ++i) {
        const std::pair<std::string, std::string> &att = elem->attribs[i];
        if (att.first == "TIMESTEP") {
            std::cerr << "timesteps " << att.second << " on type " << elem->type << std::endl;
            elem->is_timeset = true;
            break;
        }
    }
}


AttributeList ReadCovise::readAttributes(const int fd)
{
    std::vector<std::pair<std::string, std::string>> attributes;
    int num, size;
    covReadNumAttributes(fd, &num, &size);
    assert(num >= 0);
    if (num > 0) {
        assert(size >= 0);
    }

    if (num > 0 && size > 0) {
        std::vector<char *> key(num), value(num);
        std::vector<char> buf(size);
        key[0] = &buf[0];
        covReadAttributes(fd, &key[0], &value[0], num, size);

        for (int i = 0; i < num; ++i) {
            attributes.push_back(std::pair<std::string, std::string>(key[i], value[i]));
        }
    }

    return attributes;
}

bool ReadCovise::readSETELE(Token &token, const int port, int fd, Element *parent)
{
    int num = -1;
    covReadSetBegin(fd, &num);
    assert(num >= 0);

    for (int index = 0; index < num; index++) {
        Element *elem = new Element();
        elem->parent = parent;
        elem->index = index;
        readSkeleton(port, elem);
        parent->subelems.push_back(elem);
    }

    return true;
}

Object::ptr ReadCovise::readUNIGRD(Token &token, const int port, int fd, const bool skeleton)
{
    int dim[3];
    float min[3], max[3];

    covReadUNIGRD(fd, &dim[0], &dim[1], &dim[2], &min[0], &max[0], &min[1], &max[1], &min[2], &max[2]);
    if (!skeleton) {
        UniformGrid::ptr uni(new UniformGrid(dim[0], dim[1], dim[2]));
        for (int i = 0; i < 3; ++i) {
            uni->min()[i] = min[i];
            uni->max()[i] = max[i];
        }

        return uni;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readRCTGRD(Token &token, const int port, int fd, const bool skeleton)
{
    int dim[3];

    covReadSizeRCTGRD(fd, &dim[0], &dim[1], &dim[2]);

    if (skeleton) {
        covSkipRCTGRD(fd, dim[0], dim[1], dim[2]);
    } else {
        std::vector<float> coord[3];
        for (int i = 0; i < 3; ++i)
            coord[i].resize(dim[i]);
        covReadRCTGRD(fd, dim[0], dim[1], dim[2], coord[0].data(), coord[1].data(), coord[2].data());

        RectilinearGrid::ptr rect(new RectilinearGrid(dim[0], dim[1], dim[2]));
        for (int i = 0; i < 3; ++i) {
            for (int k = 0; k < dim[i] + 1; ++k) {
                rect->coords(i)[k] = coord[i][k];
            }
        }

        return rect;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readSTRGRD(Token &token, const int port, int fd, const bool skeleton)
{
    int dim[3];
    covReadSizeSTRGRD(fd, &dim[0], &dim[1], &dim[2]);

    if (skeleton) {
        covSkipSTRGRD(fd, dim[0], dim[1], dim[2]);
    } else {
        size_t numVertices = dim[0] * dim[1] * dim[2];
        std::vector<float> v_x(numVertices), v_y(numVertices), v_z(numVertices);
        float *_x = v_x.data(), *_y = v_y.data(), *_z = v_z.data();
        covReadSTRGRD(fd, dim[0], dim[1], dim[2], _x, _y, _z);
        StructuredGrid::ptr str(new StructuredGrid(dim[0], dim[1], dim[2]));
        Scalar *x = str->x().data(), *y = str->y().data(), *z = str->z().data();
        size_t idx = 0;
        const Index vdim[3] = {static_cast<Index>(dim[0]), static_cast<Index>(dim[1]), static_cast<Index>(dim[2])};
        for (Index k = 0; k < vdim[0]; ++k) {
            for (Index j = 0; j < vdim[1]; ++j) {
                for (Index i = 0; i < vdim[2]; ++i) {
                    auto vidx = StructuredGrid::vertexIndex(k, j, i, vdim);
                    x[vidx] = _x[idx];
                    y[vidx] = _y[idx];
                    z[vidx] = _z[idx];
                    ++idx;
                }
            }
        }
        return str;
    }
    return Object::ptr();
}

Object::ptr ReadCovise::readUNSGRD(Token &token, const int port, int fd, const bool skeleton)
{
    int numElements = -1;
    int numCorners = -1;
    int numVertices = -1;

    covReadSizeUNSGRD(fd, &numElements, &numCorners, &numVertices);
    assert(numElements >= 0);
    assert(numCorners >= 0);
    assert(numVertices >= 0);

    if (skeleton) {
        covSkipUNSGRD(fd, numElements, numCorners, numVertices);
    } else {
        UnstructuredGrid::ptr usg(new UnstructuredGrid(numElements, numCorners, numVertices));

        enum COVISE_ELEM_TYPE {
            TYPE_NONE = 0,
            TYPE_BAR = 1,
            TYPE_TRIANGLE = 2,
            TYPE_QUAD = 3,
            TYPE_TETRAHEDER = 4,
            TYPE_PYRAMID = 5,
            TYPE_PRISM = 6,
            TYPE_HEXAGON = 7,
            TYPE_HEXAEDER = 7,
            TYPE_POINT = 10,
            TYPE_POLYHEDRON = 11
        };

        static uint8_t coviseToVtkm[] = {
            vistle::cell::NONE,        vistle::cell::BAR,     vistle::cell::TRIANGLE, vistle::cell::QUAD,
            vistle::cell::TETRAHEDRON, vistle::cell::PYRAMID, vistle::cell::PRISM,    vistle::cell::HEXAHEDRON,
            vistle::cell::NONE,        vistle::cell::NONE,    vistle::cell::POINT,    vistle::cell::POLYHEDRON,
        };

        std::vector<int> v_tl(numElements);
        std::vector<int> v_el(numElements);
        std::vector<int> v_cl(numCorners);
        auto _tl = v_tl.data();
        auto _el = v_el.data();
        auto _cl = v_cl.data();

        std::vector<float> v_x(numVertices), v_y(numVertices), v_z(numVertices);
        float *_x = v_x.data(), *_y = v_y.data(), *_z = v_z.data();
        covReadUNSGRD(fd, numElements, numCorners, numVertices, _el, _cl, _tl, _x, _y, _z);

        Index *el = usg->el().data();
        Byte *tl = &usg->tl()[0];
        for (int index = 0; index < numElements; index++) {
            el[index] = _el[index];
            assert(_tl[index] <= TYPE_POLYHEDRON);
            tl[index] = coviseToVtkm[_tl[index]];
        }
        el[numElements] = numCorners;

        Index *cl = usg->cl().data();
        for (int index = 0; index < numCorners; index++)
            cl[index] = _cl[index];

        auto x = usg->x().data(), y = usg->y().data(), z = usg->z().data();
        for (int index = 0; index < numVertices; ++index) {
            x[index] = _x[index];
            y[index] = _y[index];
            z[index] = _z[index];
        }

        return usg;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readBYTEDT(Token &token, const int port, int fd, const bool skeleton)
{
    int numElements = -1;
    covReadSizeBYTEDT(fd, &numElements);
    assert(numElements >= 0);

    if (skeleton) {
        covSkipBYTEDT(fd, numElements);
    } else {
        Vec<Byte>::ptr array(new Vec<Byte>(numElements));
        std::vector<Byte> _x(numElements);
        covReadBYTEDT(fd, numElements, &_x[0]);
        auto x = array->x().data();
        for (int i = 0; i < numElements; ++i)
            x[i] = _x[i];

        return array;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readINTDT(Token &token, const int port, int fd, const bool skeleton)
{
    int numElements = -1;
    covReadSizeINTDT(fd, &numElements);
    assert(numElements >= 0);

    if (skeleton) {
        covSkipINTDT(fd, numElements);
    } else {
        Vec<Index>::ptr array(new Vec<Index>(numElements));
        std::vector<int> _x(numElements);
        covReadINTDT(fd, numElements, &_x[0]);
        auto x = array->x().data();
        for (int i = 0; i < numElements; ++i)
            x[i] = _x[i];

        return array;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readUSTSDT(Token &token, const int port, int fd, const bool skeleton)
{
    int numElements = -1;
    covReadSizeUSTSDT(fd, &numElements);
    assert(numElements >= 0);

    if (skeleton) {
        covSkipUSTSDT(fd, numElements);
    } else {
        Vec<Scalar>::ptr array(new Vec<Scalar>(numElements));
        std::vector<float> _x(numElements);
        covReadUSTSDT(fd, numElements, &_x[0]);
        auto x = array->x().data();
        for (int i = 0; i < numElements; ++i)
            x[i] = _x[i];

        return array;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readUSTVDT(Token &token, const int port, int fd, const bool skeleton)
{
    int numElements = -1;
    covReadSizeUSTVDT(fd, &numElements);
    assert(numElements >= 0);

    if (skeleton) {
        covSkipUSTVDT(fd, numElements);
    } else {
        Vec<Scalar, 3>::ptr array(new Vec<Scalar, 3>(numElements));
        std::vector<float> _x(numElements), _y(numElements), _z(numElements);
        covReadUSTVDT(fd, numElements, &_x[0], &_y[0], &_z[0]);
        Scalar *x = array->x().data();
        Scalar *y = array->y().data();
        Scalar *z = array->z().data();
        for (int i = 0; i < numElements; ++i) {
            x[i] = _x[i];
            y[i] = _y[i];
            z[i] = _z[i];
        }

        return array;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readSTRSDT(Token &token, const int port, int fd, const bool skeleton)
{
    int numElements = -1;
    int sx = -1, sy = -1, sz = -1;
    covReadSizeSTRSDT(fd, &numElements, &sx, &sy, &sz);
    assert(sx >= 0);
    assert(sy >= 0);
    assert(sz >= 0);
    size_t n = sx * sy * sz;

    if (skeleton) {
        covSkipSTRSDT(fd, numElements, sx, sy, sz);
    } else {
        Vec<Scalar>::ptr array(new Vec<Scalar>(n));
        std::vector<float> _x(n);
        covReadSTRSDT(fd, n, &_x[0], sx, sy, sz);
        auto x = array->x().data();
        const Index vdim[3] = {static_cast<Index>(sx), static_cast<Index>(sy), static_cast<Index>(sz)};
        Index idx = 0;
        for (Index k = 0; k < vdim[0]; ++k) {
            for (Index j = 0; j < vdim[1]; ++j) {
                for (Index i = 0; i < vdim[2]; ++i) {
                    auto vidx = StructuredGrid::vertexIndex(k, j, i, vdim);
                    x[vidx] = _x[idx];
                    ++idx;
                }
            }
        }

        m_transposed.insert(array);

        return array;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readSTRVDT(Token &token, const int port, int fd, const bool skeleton)
{
    int numElements = -1;
    int sx = -1, sy = -1, sz = -1;
    covReadSizeSTRVDT(fd, &numElements, &sx, &sy, &sz);
    assert(sx >= 0);
    assert(sy >= 0);
    assert(sz >= 0);
    size_t n = sx * sy * sz;

    if (skeleton) {
        covSkipSTRVDT(fd, numElements, sx, sy, sz);
    } else {
        Vec<Scalar, 3>::ptr array(new Vec<Scalar, 3>(n));
        std::vector<float> _x(n), _y(n), _z(n);
        covReadSTRVDT(fd, n, &_x[0], &_y[0], &_z[0], sx, sy, sz);
        auto x = array->x().data();
        auto y = array->y().data();
        auto z = array->z().data();
        const Index vdim[3] = {static_cast<Index>(sx), static_cast<Index>(sy), static_cast<Index>(sz)};
        Index idx = 0;
        for (Index k = 0; k < vdim[0]; ++k) {
            for (Index j = 0; j < vdim[1]; ++j) {
                for (Index i = 0; i < vdim[2]; ++i) {
                    auto vidx = StructuredGrid::vertexIndex(k, j, i, vdim);
                    x[vidx] = _x[idx];
                    y[vidx] = _y[idx];
                    z[vidx] = _z[idx];
                    ++idx;
                }
            }
        }

        m_transposed.insert(array);

        return array;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readPOINTS(Token &token, const int port, int fd, const bool skeleton)
{
    int numCoords = -1;
    covReadSizePOINTS(fd, &numCoords);
    assert(numCoords >= 0);

    if (skeleton) {
        covSkipPOINTS(fd, numCoords);
    } else {
        Points::ptr points(new Points(numCoords));

        std::vector<float> _x(numCoords), _y(numCoords), _z(numCoords);

        covReadPOINTS(fd, numCoords, &_x[0], &_y[0], &_z[0]);

        auto *x = points->x().data();
        auto *y = points->y().data();
        auto *z = points->z().data();
        for (int index = 0; index < numCoords; index++) {
            x[index] = _x[index];
            y[index] = _y[index];
            z[index] = _z[index];
        }

        return points;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readLINES(Token &token, const int port, int fd, const bool skeleton)
{
    int numElements = -1, numCorners = -1, numVertices = -1;
    covReadSizeLINES(fd, &numElements, &numCorners, &numVertices);
    assert(numElements >= 0);
    assert(numCorners >= 0);
    assert(numVertices >= 0);

    if (skeleton) {
        covSkipLINES(fd, numElements, numCorners, numVertices);
    } else {
        Lines::ptr lines(new Lines(numElements, numCorners, numVertices));

        std::vector<int> _el(numElements);
        std::vector<int> _cl(numCorners);
        std::vector<float> _x(numVertices), _y(numVertices), _z(numVertices);

        covReadLINES(fd, numElements, &_el[0], numCorners, &_cl[0], numVertices, &_x[0], &_y[0], &_z[0]);

        auto el = lines->el().data();
        for (int index = 0; index < numElements; index++)
            el[index] = _el[index];
        el[numElements] = numCorners;

        auto cl = lines->cl().data();
        for (int index = 0; index < numCorners; index++)
            cl[index] = _cl[index];

        auto x = lines->x().data(), y = lines->y().data(), z = lines->z().data();
        for (int index = 0; index < numVertices; index++) {
            x[index] = _x[index];
            y[index] = _y[index];
            z[index] = _z[index];
        }

        return lines;
    }

    return Object::ptr();
}

Object::ptr ReadCovise::readPOLYGN(Token &token, const int port, int fd, const bool skeleton)
{
    int numElements = -1, numCorners = -1, numVertices = -1;
    covReadSizePOLYGN(fd, &numElements, &numCorners, &numVertices);
    assert(numElements >= 0);
    assert(numCorners >= 0);
    assert(numVertices >= 0);

    if (skeleton) {
        covSkipPOLYGN(fd, numElements, numCorners, numVertices);
    } else {
        Polygons::ptr polygons(new Polygons(numElements, numCorners, numVertices));

        std::vector<int> _el(numElements);
        std::vector<int> _cl(numCorners);
        std::vector<float> _x(numVertices), _y(numVertices), _z(numVertices);

        covReadPOLYGN(fd, numElements, &_el[0], numCorners, &_cl[0], numVertices, &_x[0], &_y[0], &_z[0]);

        auto el = polygons->el().data();
        for (int index = 0; index < numElements; index++)
            el[index] = _el[index];
        el[numElements] = numCorners;

        auto cl = polygons->cl().data();
        for (int index = 0; index < numCorners; index++)
            cl[index] = _cl[index];

        auto x = polygons->x().data(), y = polygons->y().data(), z = polygons->z().data();
        for (int index = 0; index < numVertices; index++) {
            x[index] = _x[index];
            y[index] = _y[index];
            z[index] = _z[index];
        }

        return polygons;
    }

    return Object::ptr();
}

bool ReadCovise::readGEOTEX(Token &token, const int port, int fd, Element *elem)
{
    assert(elem);
    assert(port == 0);

    const size_t ncomp = 4;
    int contains[ncomp] = {0, 0, 0, 0};
    covReadGeometryBegin(fd, &contains[0], &contains[1], &contains[2], &contains[3]);
    elem->is_geometry = true;

    bool ok = true;
    for (size_t i = 0; i < ncomp; ++i) {
        Element *e = new Element();
        e->offset = mytell(fd);
        if (contains[i])
            ok &= readSkeleton(port, e);
        elem->subelems.push_back(e);
    }

    return ok;
}

bool ReadCovise::readGEOMET(Token &token, const int port, int fd, Element *elem)
{
    assert(elem);
    assert(port == 0);

    const size_t ncomp = 3;
    int contains[ncomp] = {0, 0, 0};
    covReadOldGeometryBegin(fd, &contains[0], &contains[1], &contains[2]);
    elem->is_geometry = true;

    bool ok = true;
    for (size_t i = 0; i < ncomp; ++i) {
        Element *e = new Element();
        e->offset = mytell(fd);
        if (contains[i])
            ok &= readSkeleton(port, e);
        elem->subelems.push_back(e);
    }

    return ok;
}

vistle::Object::ptr ReadCovise::readOBJREF(Token &token, const int port, int fd, bool skeleton, Element *elem)
{
    if (skeleton) {
        int objNum = -1;
        if (covReadOBJREF(fd, &objNum) == -1) {
            std::cerr << "ReadCovise: failed to read OBJREF" << std::endl;
            return Object::ptr();
        }

        if (objNum < 0 || size_t(objNum) >= m_objects[port].size()) {
            std::cerr << "ReadCovise: invalid OBJREF" << std::endl;
            return Object::ptr();
        }

        elem->referenced = m_objects[port][objNum];
    } else {
        token.wait();
    }

    if (!elem->referenced->obj) {
        std::cerr << "ReadCovise: OBJREF to SETELE - not supported" << std::endl;
    }
    return elem->referenced->obj;
}

Object::ptr ReadCovise::readObjectIntern(Token &token, const int port, int fd, const bool skeleton, Element *elem,
                                         int timestep)
{
    Object::ptr object;

    if (!skeleton) {
        if (elem->objnum < 0)
            return object;

#if 0
      int block = findBlockNum(*elem);
      if (rankForTimestepAndPartition(timestep, block) != rank())
         return object;
#endif

        if (elem->obj) {
            if (elem->obj->getTimestep() == token.meta().timeStep())
                return elem->obj;
            object = elem->obj->clone();
            token.applyMeta(object);
            return object;
        }
    }

    if (skeleton) {
        elem->offset = mytell(fd);
    } else {
        seek(fd, elem->offset);
    }

    char buf[7];
    if (covReadDescription(fd, buf) == -1) {
        std::cerr << "ReadCovise: not a COVISE file?" << std::endl;
        return object;
    }
    buf[6] = '\0';
    std::string type(buf);
    //std::cerr << "ReadCovise::readObject " << type << " @ " << mytell(fd) << std::endl;

    if (skeleton)
        elem->type = type;
    if (type == "OBJREF") {
        object = readOBJREF(token, port, fd, skeleton, elem);
        elem->objnum = -1;
        return object;
    } else if (skeleton) {
        m_objects[port].push_back(elem);
    }

    bool handled = false;
#define HANDLE(t) \
    if (!handled && type == "" #t) { \
        handled = true; \
        object = read##t(token, port, fd, skeleton); \
    }
    HANDLE(UNIGRD);
    HANDLE(RCTGRD);
    HANDLE(STRGRD);
    HANDLE(UNSGRD);
    HANDLE(USTSDT);
    HANDLE(USTVDT);
    HANDLE(BYTEDT);
    HANDLE(INTDT);
    HANDLE(STRSDT);
    HANDLE(STRVDT);
    HANDLE(POLYGN);
    HANDLE(LINES);
    HANDLE(POINTS);
#undef HANDLE

    if (handled) {
        if (skeleton) {
            elem->objnum = m_numObj[port];
            ++m_numObj[port];
        }
    } else {
        if (type == "GEOTEX") {
            if (skeleton) {
                readGEOTEX(token, port, fd, elem);
            }
        } else if (type == "GEOMET") {
            if (skeleton) {
                readGEOMET(token, port, fd, elem);
            }
        } else if (type == "SETELE") {
            if (skeleton) {
                readSETELE(token, port, fd, elem);
            }
        } else {
            std::stringstream str;
            str << "Object type not supported: " << buf;
            std::cerr << "ReadCovise: " << str.str() << std::endl;
            throw vistle::exception(str.str());
        }
    }

    if (skeleton) {
        elem->attribs = readAttributes(fd);
        parseAttributes(elem);
        if (elem->is_timeset && m_numTime[port] < int(elem->subelems.size())) {
            m_numTime[port] = elem->subelems.size();
            std::cerr << "ReadCovise: skeleton: " << type << ", #time=" << elem->subelems.size() << std::endl;
        } else {
            std::cerr << "ReadCovise: skeleton: " << type << ", #sub=" << elem->subelems.size() << std::endl;
        }
    } else {
        if (object) {
            applyAttributes(token, object, *elem);
            token.applyMeta(object);
            elem->obj = object;
            std::cerr << "ReadCovise: " << type << " [ b# " << object->getBlock() << ", t# " << object->getTimestep()
                      << " ]" << std::endl;
        }
    }

    return object;
}

Object::ptr ReadCovise::readObject(Token &token, const int port, int fd, Element *elem, int timestep)
{
    return readObjectIntern(token, port, fd, false, elem, timestep);
}

bool ReadCovise::readSkeleton(const int port, Element *elem)
{
    Token invalid(this, std::shared_ptr<Token>());
    readObjectIntern(invalid, port, m_fd[port], true, elem, -1);
    return true;
}

namespace {

template<typename S>
void transposeArray(S *d, const S *s, const Index dim[3])
{
    Index idx = 0;
    for (Index k = 0; k < dim[0]; ++k) {
        for (Index j = 0; j < dim[1]; ++j) {
            for (Index i = 0; i < dim[2]; ++i) {
                auto vidx = StructuredGrid::vertexIndex(k, j, i, dim);
                d[vidx] = s[idx];
                ++idx;
            }
        }
    }
}

template<class V>
void transposeData(typename V::ptr data, vistle::StructuredGridBase::ptr str)
{
    const Index dim[3] = {str->getNumDivisions(0), str->getNumDivisions(1), str->getNumDivisions(2)};

    ShmVector<typename V::Scalar> x[V::Dimension];
    for (unsigned d = 0; d < V::Dimension; ++d) {
        x[d] = data->d()->x[d];
    }
    data->resetArrays();
    for (unsigned d = 0; d < V::Dimension; ++d) {
        transposeArray(data->x(d).data(), x[d]->data(), dim);
    }
}

} // namespace

// transpose arrays attached to structured grids from COVISE indexing to Vistle/VTK indexing
void ReadCovise::transpose(vistle::DataBase::ptr data, vistle::StructuredGridBase::ptr str)
{
    if (m_transposed.find(data) != m_transposed.end())
        return;
    if (auto s1 = Vec<Scalar>::as(data)) {
        transposeData<Vec<Scalar>>(s1, str);
    } else if (auto s2 = Vec<Scalar, 2>::as(data)) {
        transposeData<Vec<Scalar, 2>>(s2, str);
    } else if (auto s3 = Vec<Scalar, 3>::as(data)) {
        transposeData<Vec<Scalar, 3>>(s3, str);
    } else if (auto b1 = Vec<Byte>::as(data)) {
        transposeData<Vec<Byte>>(b1, str);
    } else if (auto i1 = Vec<Index>::as(data)) {
        transposeData<Vec<Index>>(i1, str);
    }
    m_transposed.insert(data);
}

bool ReadCovise::readRecursive(Token &token, int fd[], Element *elem[], int timestep, int targetTimestep)
{
    if (!elem[0]->is_geometry && timestep != -1 && timestep != targetTimestep) {
        std::cerr << "ReadCovise: skipping read: timstep " << timestep << " != targetTimestep " << targetTimestep
                  << std::endl;
        return true;
    }

    Object::ptr obj[NumPorts];
    Object::ptr grid;
    std::future<Object::ptr> fut[NumPorts];
    if (elem[0]->is_geometry) {
        if (!fd[0])
            return false;
        int subfd[NumPorts];
        for (unsigned port = 0; port < NumPorts; ++port) {
            if (port >= elem[0]->subelems.size() || !elem[0]->subelems[port]) {
                subfd[port] = 0;
            } else if (port == 0) {
                subfd[0] = fd[0];
            } else if (fd[0]) {
                subfd[port] = covOpenInFile(m_filename[0].c_str());
            }
        }
        bool ok = readRecursive(token, subfd, elem[0]->subelems.data(), timestep, targetTimestep);
        for (unsigned port = 1; port < NumPorts; ++port) {
            if (subfd[port]) {
                covCloseInFile(subfd[port]);
            }
        }
        return ok;
    }

    if (timestep == targetTimestep) {
        // obj is regular
        // do not recurse as subelems are abused for Geometry components

        bool gridOnPort0 = false;
        for (unsigned port = 0; port < NumPorts; ++port) {
            if (fd[port]) {
                assert(elem[port]);
                fut[port] = std::async(std::launch::async, [this, &token, port, fd, elem, timestep]() -> Object::ptr {
                    auto obj = readObject(token, port, fd[port], elem[port], timestep);
                    if (obj) {
                        updateMeta(obj);
                        std::cerr << "read obj " << obj->getName() << " for timestep " << timestep << " on port "
                                  << port << std::endl;
                    } else {
                        std::cerr << "no data for timestep " << timestep << " on port " << port << std::endl;
                    }
                    return obj;
                });
            }
        }
        for (unsigned port = 0; port < NumPorts; ++port) {
            if (fd[port]) {
                obj[port] = fut[port].get();
            }
            if (port == 0) {
                grid = obj[port];
                if (auto coords = Coords::as(grid)) {
                    gridOnPort0 = true;
                } else if (auto str = StructuredGridBase::as(grid)) {
                    gridOnPort0 = true;
                }
            } else if (port == 1) {
                if (auto normals = Normals::as(obj[port])) {
                    if (auto str = StructuredGridBase::as(grid)) {
                        transpose(normals, str);
                    }
                    if (auto coords = Coords::as(grid)) {
                        coords->setNormals(normals);
                    } else if (auto str = StructuredGridBase::as(grid)) {
                        str->setNormals(normals);
                    }
                }
            } else {
                if (auto data = DataBase::as(obj[port])) {
                    if (auto str = StructuredGridBase::as(grid)) {
                        transpose(data, str);
                    }
                    data->setGrid(grid);
                }
            }
        }
        token.wait();
        for (unsigned port = 0; port < NumPorts; ++port) {
            if (m_out[port] && obj[port]) {
                if (port > 1 || !gridOnPort0)
                    obj[port]->addAttribute(attribute::Species, m_species[port]);
                updateMeta(obj[port]);
                addObject(m_out[port], obj[port]);
            }
        }
    }

    if (!grid) {
        const bool inTimeset = elem[0]->is_timeset;
        //std::cerr << "processing SET w/ " << elem->subelems.size() << " elements, timeset=" << inTimeset << std::endl;
        // obj corresponds to a Set, recurse
        bool ok = true;
        for (size_t i = 0; i < elem[0]->subelems.size(); ++i) {
            Element *subelems[NumPorts];
            for (unsigned port = 0; port < NumPorts; ++port) {
                if (!fd[port])
                    continue;

                if (!elem[port] || elem[port]->subelems.empty()) {
                    fd[port] = 0;
                    continue;
                }

                if (elem[port]->subelems.size() != elem[0]->subelems.size()) {
                    sendError("mismatch in object structure (size): %d != %d", (int)elem[port]->subelems.size(),
                              (int)elem[0]->subelems.size());
                    return false;
                }
                if (inTimeset != elem[port]->is_timeset) {
                    sendError("mismatch in object structure (time)");
                    return false;
                }
                if (elem[port]->subelems[i]->referenced) {
                    subelems[port] = elem[port]->subelems[i]->referenced;
                } else {
                    subelems[port] = elem[port]->subelems[i];
                }
            }
            ok &= readRecursive(token, fd, subelems, inTimeset ? i : timestep, targetTimestep);
        }

        return ok;
    }

    return true;
}

void ReadCovise::deleteRecursive(Element &elem)
{
    for (size_t i = 0; i < elem.subelems.size(); ++i) {
        deleteRecursive(*elem.subelems[i]);
        delete elem.subelems[i];
    }
    elem = Element();
}
