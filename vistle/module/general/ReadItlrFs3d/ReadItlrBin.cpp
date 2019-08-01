/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <cctype>
#include <fstream>
#include <algorithm>

#include <cstdlib>

#include <boost/algorithm/string/predicate.hpp>
#include <util/filesystem.h>
#include <util/byteswap.h>


#include "ReadItlrBin.h"
#include <core/rectilineargrid.h>

const int SplitDim = 0;

MODULE_MAIN(ReadItlrBin)

class Closer {

    public:
    Closer(FILE *fp): m_fp(fp) {}
    ~Closer() { fclose(m_fp); }

    private:
    FILE *m_fp;
};


using namespace vistle;

namespace
{

const endianness disk_endian = little_endian;

template <typename T>
struct on_disk;

template<>
struct on_disk<float>
{
    typedef double type;
};

template<>
struct on_disk<double>
{
    typedef double type;
};

template<>
struct on_disk<int>
{
    typedef int32_t type;
};

template<>
struct on_disk<unsigned>
{
    typedef uint32_t type;
};

template<>
struct on_disk<long>
{
    typedef int32_t type;
};

template<>
struct on_disk<unsigned long>
{
    typedef uint32_t type;
};

}

template <typename D>
bool readArrayChunk(FILE *fp, D *buf, const size_t num) {

    size_t n = fread(reinterpret_cast<char *>(buf), sizeof(buf[0]), num, fp);
    if (n != num) {
        std::cerr << "readArrayChunk failed: read " << n << " items instead of " << num << std::endl;
        return false;
    }
    if (disk_endian != host_endian) {
       for (size_t i=0; i<num; ++i) {
          buf[i] = byte_swap<disk_endian, host_endian, D>(buf[i]);
       }
    }
    return true;
}

static const size_t bufsiz = 16384;

template <typename T>
bool readArray(FILE *fp, T *p, const size_t num) {
    typedef typename on_disk<T>::type D;
    if (boost::is_same<T, D>::value) {
       return readArrayChunk<T>(fp, p, num);
    } else {
       std::vector<D> buf(bufsiz);
       for (size_t i=0; i<num; i+=bufsiz) {
          const size_t nread = i+bufsiz <= num ? bufsiz : num-i;
          if (!readArrayChunk(fp, &buf[0], nread))
             return false;
          for (size_t j=0; j<nread; ++j) {
             p[i+j] = buf[j];
          }
       }
    }
    return true;
}

template <typename T>
bool skipArray(FILE *fp, const size_t num) {
    typedef typename on_disk<T>::type D;
    return !fseek(fp, sizeof(D)*num, SEEK_CUR);
}

bool readFloatArray(FILE *fp, Scalar *p, const size_t num) {
    if (ferror(fp)) {
       std::cerr << "readFloatArray: file error" << std::endl;
       return false;
    }
    if (feof(fp)) {
       std::cerr << "readFloatArray: already at EOF" << std::endl;
       return false;
    }
    return readArray<Scalar>(fp, p, num);
}

bool skipFloatArray(FILE *fp, const size_t num) {
    if (ferror(fp)) {
       std::cerr << "skipFloatArray: file error" << std::endl;
       return false;
    }
    if (feof(fp)) {
       std::cerr << "skipFloatArray: already at EOF" << std::endl;
       return false;
    }
    return skipArray<Scalar>(fp, num);
}

ReadItlrBin::ReadItlrBin(const std::string &name, int moduleID, mpi::communicator comm)
    : Reader("Read ITLR binary data", name, moduleID, comm)
    , m_nparts(size())
    , m_dims{0,0,0} {

    m_gridFilename = addStringParameter("grid_filename", ".bin file for grid", "/data/itlr/itlmer-Case11114_VISUS/netz_xyz.bin", Parameter::ExistingFilename);
    for (int i=0; i<NumPorts; ++i) {
        if (i == 0)
            m_filename[i] = addStringParameter("filename"+std::to_string(i), ".lst or .bin file for data", "/data/itlr/itlmer-Case11114_VISUS/funs.lst", Parameter::ExistingFilename);
        else
            m_filename[i] = addStringParameter("filename"+std::to_string(i), ".lst or .bin file for data", "", Parameter::ExistingFilename);
        setParameterFilters(m_filename[i], "List Files (*.lst)/Binary Files (*.bin)/All Files (*)");
    }

    m_numPartitions = addIntParameter("num_partitions", "number of partitions (-1: MPI ranks)", -1);
    setParameterRange(m_numPartitions, (Integer)-1, std::numeric_limits<Integer>::max());

#if 0
    m_firstStep = addIntParameter("first_step", "first timestep to read", 0);
    setParameterRange(m_firstStep, (Integer)0, std::numeric_limits<Integer>::max());
    m_lastStep = addIntParameter("last_step", "last timestep to read", -1);
    setParameterRange(m_lastStep, (Integer)-1, std::numeric_limits<Integer>::max());
    m_stepSkip = addIntParameter("skip_step", "number of steps to skip", 99);
    setParameterRange(m_stepSkip, (Integer)0, std::numeric_limits<Integer>::max());
#endif

    for (int i=0; i<NumPorts; ++i) {
        m_dataOut[i] = createOutputPort("data"+std::to_string(i), "data output");
    }

   setParallelizationMode(ParallelizeTimeAndBlocks);
   setAllowTimestepDistribution(true);
   observeParameter(m_numPartitions);
}

ReadItlrBin::~ReadItlrBin() {


}

bool ReadItlrBin::examine(const Parameter *param)
{
    if (!param || param == m_numPartitions)
        setPartitions(m_numPartitions->getValue());

    return true;
}

bool ReadItlrBin::prepareRead()
{
    int numFiles = -1;
    m_haveListFile = false;
    for (int port=0; port<NumPorts; ++port) {
        m_scalarFile[port] = m_filename[port]->getValue();
        if (m_scalarFile[port].empty())
            continue;

        m_fileList[port].push_back(m_scalarFile[port]);
        bool haveList = false;
        if (boost::algorithm::ends_with(m_scalarFile[port], ".lst")) {
            m_fileList[port] = readListFile(m_scalarFile[port]);
            haveList = true;
        }
        filesystem::path listFilePath(m_scalarFile[port]);
        m_directory[port] = listFilePath.parent_path();

        if (numFiles < 0) {
            m_haveListFile = haveList;
        } else {
            if (haveList != m_haveListFile) {
                sendError("Selection of a .bin or a .list file has to match on all ports");
                return false;
            }
        }

        if (numFiles < 0) {
            numFiles = m_fileList[port].size();
        } else {
            numFiles = std::min<int>(numFiles, m_fileList[port].size());
        }
    }

    std::string gridFile = m_gridFilename->getValue();
    if (m_numPartitions->getValue() < 0)
        m_nparts = size();
    else
        m_nparts = m_numPartitions->getValue();

    m_grids = readGridBlocks(gridFile, m_nparts);
    assert(m_grids.size() == m_nparts);

    return true;
}

bool ReadItlrBin::read(Reader::Token &token, int timestep, int block)
{
    if (timestep < 0) {
        if (m_haveListFile)
            return true;

        timestep = 0;
    }

    if (block<0)
        block = 0;

    for (int port=0; port<NumPorts; ++port) {
        if (m_scalarFile[port].empty())
            continue;
        auto file = m_fileList[port][timestep];
        filesystem::path filename;
        if (m_haveListFile) {
            filename = m_directory[port];
            filename /= file;
        } else {
            filename = m_fileList[port][timestep];
        }

        auto scal = readFieldBlock(filename.string(), block);
        if (scal) {
            scal->setGrid(m_grids[block]);
            token.addObject(m_dataOut[port], scal);
        } else {
            token.addObject(m_dataOut[port], m_grids[block]);
        }
    }

    return true;
}

bool ReadItlrBin::finishRead()
{
    for (int port=0; port<NumPorts; ++port)
        m_fileList[port].clear();
    m_grids.clear();

    return true;
}

ReadItlrBin::Block ReadItlrBin::computeBlock(int part) const {

    int nslices = m_dims[SplitDim];
    int perpart = (nslices+m_nparts-1)/m_nparts;

    int begin = 0, end = nslices;
    int beginghost = 0, endghost = 0;
    if (part >= 0)  {
        begin = part*perpart;
        end = begin+perpart+1;
        if (part > 0) {
            --begin;
            ++beginghost;
        }
        if (part < m_nparts-1) {
            ++end;
            ++endghost;
        } else {
            end = nslices;
        }
    }

    Block block;
    block.part = part;
    block.begin = begin;
    block.end = end;
    block.ghost[0] = beginghost;
    block.ghost[1] = endghost;

    return block;
}

std::vector<RectilinearGrid::ptr> ReadItlrBin::readGridBlocks(const std::string &filename, int nparts) {

    std::vector<RectilinearGrid::ptr> result;

    FILE *fp = fopen(filename.c_str(), "r");
    if (!fp) {
        sendError("failed to open grid file %s", filename.c_str());
        return result;
    }
    Closer closeFile(fp);

    char header[80];
    size_t n = fread(header, 80, 1, fp);
    std::string unit = header;

    uint32_t dims[3];
    if (!readArray<uint32_t>(fp, dims, 3)) {
       sendError("failed to read grid dimensions from %s", filename.c_str());
       return result;
    }

    // read coordinates and prepare for transformation avoiding transposition
    std::vector<float> coords[3];
    for (int i=0; i<3; ++i) {
        coords[i].resize(dims[i]);
        if (!readFloatArray(fp, coords[i].data(), dims[i])) {
            sendError("failed to read grid coordinates %d from %s", i, filename.c_str());
            return result;
        }
        if (i==2) {
            for (int j=0; j<dims[i]; ++j) {
                coords[i][j] *= -1;
            }
            std::reverse(coords[i].begin(), coords[i].end());
        }
    }
    std::swap(coords[0], coords[2]);
    for (int i=0; i<3; ++i) {
        m_dims[i] = coords[i].size();
    }
    if (rank() == 0)
       sendInfo("grid dimensions: %d %d %d", (int)m_dims[0], (int)m_dims[1], (int)m_dims[2]);

    for (int part=0; part<nparts; ++part) {
        auto b = computeBlock(part);
        std::cerr << "reading block " << part << ", slices " << b.begin << " to " << b.end << std::endl;

        //RectilinearGrid::ptr grid(new RectilinearGrid(b.end-b.begin, coords[1].size(), coords[2].size()));
        RectilinearGrid::ptr grid(new RectilinearGrid(b.end-b.begin, coords[1].size(), coords[2].size()));
        grid->setNumGhostLayers(SplitDim, RectilinearGrid::Bottom, b.ghost[0]);
        grid->setNumGhostLayers(SplitDim, RectilinearGrid::Top, b.ghost[1]);
        for (int i=0; i<3; ++i) {
            if (i==SplitDim) {
                std::copy(coords[i].begin()+b.begin, coords[i].begin()+b.end, &grid->coords(i)[0]);
            } else {
                std::copy(coords[i].begin(), coords[i].end(), &grid->coords(i)[0]);
            }
        }

        Matrix4 t;
        t << Vector4(0,0,-1,0), Vector4(0,1,0,0), Vector4(1,0,0,0), Vector4(0,0,0,1);
        grid->setTransform(t);

        result.push_back(grid);
    }

    std::cerr << "read " << result.size() << " grid blocks" << std::endl;
    return result;
}

DataBase::ptr ReadItlrBin::readFieldBlock(const std::string &filename, int part) const {

    FILE *fp = fopen(filename.c_str(), "r");
    if (!fp) {
        sendError("failed to open file %s", filename.c_str());
        return DataBase::ptr();
    }
    Closer closeFile(fp);

    char header[80];
    size_t n = fread(header, 80, 1, fp);
    std::string fieldname = header;
    n = fread(header, 80, 1, fp);
    std::string unit = header;

    uint32_t dims[7];
    if (!readArray<uint32_t>(fp, dims, 7)) {
       sendError("failed to read dimensions from %s", filename.c_str());
       return DataBase::ptr();
    }

    if (dims[3] != m_dims[2] || dims[4] != m_dims[1] || dims[5] != m_dims[0]) {
       sendInfo("data with dimensions: %d %d %d", (int)dims[3], (int)dims[4], (int)dims[5]);
       sendError("data set dimensions from %s don't match grid dimensions", filename.c_str());
       return DataBase::ptr();
    }

    auto b = computeBlock(part);
    Index size = 1, offset = 1;
    for (int c=0; c<3; ++c) {
        if (c == SplitDim) {
            size *= Index(b.end-b.begin);
            offset *= b.begin;
        } else {
            size *= m_dims[c];
            offset *= m_dims[c];
        }
    }
    Index rest = m_dims[0]*m_dims[1]*m_dims[2] - offset - size;

    int numComp = 1;
    if (filename.find("velv") == 0 || filename.find("/velv") != std::string::npos) {
        numComp = 3;
    }
    switch(numComp) {
    case 1: {
        if (!skipFloatArray(fp, offset)) {
            sendError("failed to skip data from %s", filename.c_str());
        }
        Vec<Scalar>::ptr vec(new Vec<Scalar>(size));
        if (!readFloatArray(fp, &vec->x()[0], size)) {
            sendError("failed to read data from %s", filename.c_str());
            return DataBase::ptr();
        }
        return vec;
        break;
    }
    case 3: {
        Vec<Scalar,3>::ptr vec3(new Vec<Scalar,3>(size));
        if (!skipFloatArray(fp, offset)) {
            sendError("failed to skip data from %s", filename.c_str());
        }
        if (!readFloatArray(fp, &vec3->x()[0], size)) {
            sendError("failed to read data from %s", filename.c_str());
            return DataBase::ptr();
        }
        if (!skipFloatArray(fp, rest+offset)) {
            sendError("failed to skip data from %s", filename.c_str());
        }
        if (!readFloatArray(fp, &vec3->y()[0], size)) {
            sendError("failed to read data from %s", filename.c_str());
            return DataBase::ptr();
        }
        if (!skipFloatArray(fp, rest+offset)) {
            sendError("failed to skip data from %s", filename.c_str());
        }
        if (!readFloatArray(fp, &vec3->z()[0], size)) {
            sendError("failed to read data from %s", filename.c_str());
            return DataBase::ptr();
        }
        return vec3;
        break;
    }
    }

    sendError("expecting scalar or vector field, have %d dimensions", (int)dims[6]);
    return DataBase::ptr();
}

std::vector<std::string> ReadItlrBin::readListFile(const std::string &filename) const {

    std::ifstream file(filename);

    std::vector<std::string> files;
    std::string line;
    while (std::getline(file, line)) {
        files.push_back(line);
    }
    return files;
}

#if 0
bool ReadItlrBin::compute() {

    std::string gridFile = m_gridFilename->getValue();
    if (m_numPartitions->getValue() < 0)
        m_nparts = size();
    else
        m_nparts = m_numPartitions->getValue();

    auto grids = readGridBlocks(gridFile, m_nparts);
    assert(grids.size() == m_nparts);

    std::string scalarFile[NumPorts];
    std::vector<std::string> fileList[NumPorts];
    filesystem::path directory[NumPorts];
    int numFiles = -1;
    bool haveListFile = false;
    for (int port=0; port<NumPorts; ++port) {
        scalarFile[port] = m_filename[port]->getValue();
        if (scalarFile[port].empty())
            continue;

        fileList[port].push_back(scalarFile[port]);
        bool haveList = false;
        if (boost::algorithm::ends_with(scalarFile[port], ".lst")) {
            fileList[port] = readListFile(scalarFile[port]);
            haveList = true;
        }
        filesystem::path listFilePath(scalarFile[port]);
        directory[port] = listFilePath.parent_path();

        if (numFiles < 0) {
            haveListFile = haveList;
        } else {
            if (haveList != haveListFile) {
                sendError("Selection of a .bin or a .list file has to match on all ports");
                return true;
            }
        }

        if (numFiles < 0) {
            numFiles = fileList[port].size();
        } else {
            numFiles = std::min<int>(numFiles, fileList[port].size());
        }
    }

    int first = m_firstStep->getValue();
    int inc = m_stepSkip->getValue()+1;
    int last = m_lastStep->getValue();
    if (last < 0)
        last = numFiles-1;
    int step = numFiles >= 1 ? 0 : -1;
    int numSteps = (last-first)/inc+1;
    if (!haveListFile) {
        first = 0;
        last = 0;
        inc = 1;
    }
    for (int idx = first; idx <= last; idx += inc) {
        for (int block=0; block<m_nparts; ++block) {
            if (rankForBlockAndTimestep(block, step) == rank()) {
                for (int port=0; port<NumPorts; ++port) {
                    if (scalarFile[port].empty())
                        continue;
                    auto file = fileList[port][idx];
                    filesystem::path filename;
                    if (haveListFile) {
                        filename = directory[port];
                        filename /= file;
                    } else {
                        filename = fileList[port][idx];
                    }

                    auto scal = readFieldBlock(filename.string(), block);
                    if (scal) {
                        scal->setGrid(grids[block]);
                        if (numSteps > 1) {
                            scal->setTimestep(step);
                            scal->setNumTimesteps(numSteps);
                        }
                        addObject(m_dataOut[port], scal);
                    }
                }
            }
        }
        ++step;
    }

    return true;
}

int ReadItlrBin::rankForBlockAndTimestep(int block, int timestep) {

    bool distTime = m_distributeTimesteps->getValue();

    if (block < 0) {
        block = 0;
    }

    if (distTime) {
        block += timestep;
    }

    return block % size();
}
#endif
