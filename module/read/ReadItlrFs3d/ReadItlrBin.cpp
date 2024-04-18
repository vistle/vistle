/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see LICENSE.txt.

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
#include <vistle/util/filesystem.h>
#include <vistle/util/byteswap.h>


#include "ReadItlrBin.h"
#include <vistle/core/rectilineargrid.h>

#ifdef HAVE_HDF5
#include <hdf5.h>
#endif

MODULE_MAIN(ReadItlrBin)

struct File {
    File(const std::string &name): name(name) {}

    ~File()
    {
        if (fp)
            fclose(fp);
        fp = nullptr;
#ifdef HAVE_HDF5
        if (dataset >= 0) {
            H5Dclose(dataset);
        }
        dataset = -1;
        if (file >= 0) {
            H5Fclose(file);
        }
        file = -1;
#endif
    }

    bool isHdf5()
    {
        return boost::algorithm::ends_with(name, ".hdf");
    }

    std::string format()
    {
        if (fp)
            return "plain";
#ifdef HAVE_HDF5
        if (file >= 0)
            return "hdf5";
#endif
        if (isHdf5())
            return "hdf5";
        return "plain";
    }

    bool open()
    {
        if (isHdf5()) {
#ifdef HAVE_HDF5
            file = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            if (file >= 0)
                return true;
            std::cerr << "failed to open HDF5 file " << name << std::endl;
            return false;
#else
            std::cerr << "failed to open HDF5 file " << name << ": no HDF5 support" << std::endl;
            return false;
#endif
        } else {
            fp = fopen(name.c_str(), "rb");
            return fp != nullptr;
        }

        return false;
    }

    bool openDataset(const std::string &dataset)
    {
#ifdef HAVE_HDF5
        if (isHdf5()) {
            std::cerr << "opening dataset " << dataset << std::endl;
            if (this->datasetName == dataset)
                return true;
            offset = 0;
            datasetName.clear();
            if (this->dataset >= 0) {
                H5Dclose(this->dataset);
            }
            this->dataset = H5Dopen2(file, dataset.c_str(), H5P_DEFAULT);
            if (this->dataset < 0) {
                return false;
            }
            this->datatype = H5Dget_type(this->dataset);
            this->dataclass = H5Tget_class(this->datatype);
            this->typesize = H5Tget_size(this->datatype);
            this->byteorder = H5Tget_order(this->datatype);
            this->dataspace = H5Dget_space(this->dataset);
            this->numdims = H5Sget_simple_extent_ndims(this->dataspace);
            dims.resize(numdims);
            H5Sget_simple_extent_dims(dataspace, dims.data(), nullptr);
            std::cerr << "dataset dims:";
            for (const auto &d: dims)
                std::cerr << " " << d;
            std::cerr << std::endl;
        }
#endif
        datasetName = dataset;
        return true;
    }

    std::string name;
    std::string datasetName;
    FILE *fp = nullptr;
#ifdef HAVE_HDF5
    herr_t status = 0;
    hid_t file = -1;
    hid_t dataset = -1;
    hid_t datatype = -1;
    size_t typesize = 0;
    hid_t dataspace = -1;
    H5T_class_t dataclass;
    H5T_order_t byteorder;
    int numdims = 0;
    std::vector<hsize_t> dims;
    size_t offset = 0;
#endif
};

using namespace vistle;

namespace {

const endianness disk_endian = little_endian;

template<typename T>
struct on_disk;

template<>
struct on_disk<float> {
    typedef double type;
};

template<>
struct on_disk<double> {
    typedef double type;
};

template<>
struct on_disk<int> {
    typedef int32_t type;
};

template<>
struct on_disk<unsigned> {
    typedef uint32_t type;
};

template<>
struct on_disk<long> {
    typedef int32_t type;
};

template<>
struct on_disk<unsigned long> {
    typedef uint32_t type;
};

} // namespace

template<typename D>
bool readArrayChunk(FILE *fp, D *buf, const size_t num)
{
    size_t n = fread(reinterpret_cast<char *>(buf), sizeof(buf[0]), num, fp);
    if (n != num) {
        std::cerr << "readArrayChunk failed: read " << n << " items instead of " << num << std::endl;
        return false;
    }
    if (disk_endian != host_endian) {
        for (size_t i = 0; i < num; ++i) {
            buf[i] = byte_swap<disk_endian, host_endian, D>(buf[i]);
        }
    }
    return true;
}

static const size_t bufsiz = 16384;

#ifdef HAVE_HDF5
template<typename T>
hid_t maptype();

template<>
hid_t maptype<float>()
{
    return H5T_NATIVE_FLOAT;
}

template<>
hid_t maptype<double>()
{
    return H5T_NATIVE_DOUBLE;
}

template<>
hid_t maptype<unsigned int>()
{
    return H5T_NATIVE_UINT;
}

template<>
hid_t maptype<unsigned long>()
{
    return H5T_NATIVE_ULONG;
}
#endif

template<typename T>
bool readArray(File &file, const std::string &name, T *p, const size_t num)
{
    if (!file.openDataset(name)) {
        std::cerr << "failed to open dataset " << name << std::endl;
        return false;
    }
    if (file.fp) {
        typedef typename on_disk<T>::type D;
        if (boost::is_same<T, D>::value) {
            return readArrayChunk<T>(file.fp, p, num);
        } else {
            std::vector<D> buf(bufsiz);
            for (size_t i = 0; i < num; i += bufsiz) {
                const size_t nread = i + bufsiz <= num ? bufsiz : num - i;
                if (!readArrayChunk(file.fp, &buf[0], nread))
                    return false;
                for (size_t j = 0; j < nread; ++j) {
                    p[i + j] = buf[j];
                }
            }
        }
        return true;
#ifdef HAVE_HDF5
    } else if (file.file >= 0) {
        std::vector<hsize_t> offset(file.numdims), count(file.numdims);
        std::copy(file.dims.begin(), file.dims.end(), count.begin());
        size_t begin = file.offset;
        std::vector<size_t> sizes(file.numdims);
        std::copy(file.dims.begin(), file.dims.end(), sizes.begin());
        for (int d = file.numdims - 1; d > 0; --d) {
            sizes[d - 1] *= sizes[d];
        }
        offset[0] = begin;
        count[0] = num;
        for (int d = 1; d < file.numdims; ++d) {
            offset[d] = offset[d - 1] / file.dims[d - 1];
            count[d] = count[d - 1] / file.dims[d - 1];
        }
        for (int d = 0; d < file.numdims - 1; ++d) {
            offset[d] %= file.dims[d];
            count[d] = file.dims[d];
        }
        std::cerr << "selecting hyperslab:";
        for (const auto &d: offset)
            std::cerr << " " << d;
        std::cerr << " plus";
        for (const auto &d: count)
            std::cerr << " " << d;
        std::cerr << std::endl;
#ifndef NDEBUG
        size_t sz = count[file.numdims - 1];
        for (int d = 0; d < file.numdims - 1; ++d) {
            sz *= count[d];
            assert(offset[d] == 0);
            assert(count[d] == file.dims[d]);
        }
        assert(sz == num);
#endif
        if (H5Sselect_hyperslab(file.dataspace, H5S_SELECT_SET, offset.data(), nullptr, count.data(), nullptr) < 0) {
            std::cerr << "H5Sselect_hyperslab from file failed for " << file.name << ":" << file.datasetName
                      << std::endl;
            return false;
        }
        hid_t memspace = H5Screate_simple(file.numdims, count.data(), nullptr);
        std::vector<hsize_t> origin(file.numdims);
        if (H5Sselect_hyperslab(memspace, H5S_SELECT_SET, origin.data(), nullptr, count.data(), nullptr) < 0) {
            std::cerr << "H5Sselect_hyperslab in memory failed for " << file.name << ":" << file.datasetName
                      << std::endl;
            return false;
        }
        if (H5Dread(file.dataset, maptype<T>(), memspace, file.dataspace, H5P_DEFAULT, p) < 0) {
            std::cerr << "H5Dread failed for " << file.name << ":" << file.datasetName << std::endl;
            return false;
        }
        file.offset += num;
        return true;
#endif
    }
    return false;
}

template<typename T>
bool skipArray(File &file, const std::string &name, const size_t num)
{
    if (!file.openDataset(name)) {
        std::cerr << "failed to open dataset " << name << std::endl;
        return false;
    }
    typedef typename on_disk<T>::type D;
    if (file.fp) {
        return !fseek(file.fp, sizeof(D) * num, SEEK_CUR);
#ifdef HAVE_HDF5
    } else if (file.file) {
        file.offset += num;
        return true;
#endif
    }
    return false;
}

bool readFloatArray(File &file, const std::string &name, Scalar *p, const size_t num)
{
    if (file.fp) {
        if (ferror(file.fp)) {
            std::cerr << "readFloatArray: file error" << std::endl;
            return false;
        }
        if (feof(file.fp)) {
            std::cerr << "readFloatArray: already at EOF" << std::endl;
            return false;
        }
    }
    return readArray<Scalar>(file, name, p, num);
}

bool skipFloatArray(File &file, const std::string &name, const size_t num)
{
    if (file.fp) {
        if (ferror(file.fp)) {
            std::cerr << "skipFloatArray: file error" << std::endl;
            return false;
        }
        if (feof(file.fp)) {
            std::cerr << "skipFloatArray: already at EOF" << std::endl;
            return false;
        }
    }
    return skipArray<Scalar>(file, name, num);
}

ReadItlrBin::ReadItlrBin(const std::string &name, int moduleID, mpi::communicator comm)
: Reader(name, moduleID, comm), m_nparts(size()), m_dims{0, 0, 0}
{
    m_incrementFilename =
        addIntParameter("increment_filename", "replace digits in filename with timestep", false, Parameter::Boolean);

    m_gridFilename = addStringParameter("grid_filename", ".bin file for grid",
                                        "/data/itlr/itlmer-Case11114_VISUS/netz_xyz.bin", Parameter::ExistingFilename);
    setParameterFilters(m_gridFilename, "FS3D Binary Files (*.bin)/FS3D HDF5 Files (*.hdf)/All Files (*)");
    for (int i = 0; i < NumPorts; ++i) {
        if (i == 0)
            m_filename[i] =
                addStringParameter("filename" + std::to_string(i), ".lst or .bin file for data",
                                   "/data/itlr/itlmer-Case11114_VISUS/funs.lst", Parameter::ExistingFilename);
        else
            m_filename[i] = addStringParameter("filename" + std::to_string(i), ".lst or .bin file for data", "",
                                               Parameter::ExistingFilename);
        setParameterFilters(m_filename[i], "List Files (*.lst)/Binary Files (*.bin)/HDF5 Files (*.hdf)/All Files (*)");
        observeParameter(m_filename[i]);
    }

    m_numPartitions = addIntParameter("num_partitions", "number of partitions (-1: MPI ranks)", -1);
    setParameterRange(m_numPartitions, (Integer)-1, std::numeric_limits<Integer>::max());

    for (int i = 0; i < NumPorts; ++i) {
        m_dataOut[i] = createOutputPort("data" + std::to_string(i), "data output");
    }

    setParallelizationMode(ParallelizeBlocks);
    setAllowTimestepDistribution(true);
    observeParameter(m_numPartitions);
}

ReadItlrBin::~ReadItlrBin()
{}

bool ReadItlrBin::examine(const Parameter *param)
{
    if (!param || param == m_numPartitions) {
        int npart = m_numPartitions->getValue();
        if (npart == -1)
            npart = size();
        m_nparts = npart;
        setPartitions(npart);
    }

    for (int i = 0; i < NumPorts; ++i) {
        if (!param || param == m_filename[i]) {
            auto file = m_filename[i]->getValue();
            if (file.empty())
                continue;
            if (boost::algorithm::ends_with(file, ".lst")) {
                auto files = readListFile(file);
                setTimesteps(files.size());
            }
        }
    }

    return true;
}

bool ReadItlrBin::prepareRead()
{
    int numFiles = -1;
    m_haveListFile = false;
    m_haveFileList = false;
    for (int port = 0; port < NumPorts; ++port) {
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

        auto species = listFilePath.leaf().string();
        auto pos = species.find_last_of('.');
        if (pos != species.npos) {
            species = species.substr(0, pos);
        }
        m_species[port] = species;
    }

    if (!m_haveListFile && m_incrementFilename->getValue()) {
        int last = m_last->getValue();
        for (int port = 0; port < NumPorts; ++port) {
            m_fileList[port].clear();
            auto file = m_scalarFile[port];
            if (file.empty())
                continue;
            auto len = file.length();
            std::string::size_type digitbegin = std::string::npos;
            std::string::size_type digitend = std::string::npos;
            for (size_t i = len; i > 0; --i) {
                auto &c = file[i - 1];
                if (std::isdigit(c)) {
                    if (digitend == std::string::npos)
                        digitend = i;
                    digitbegin = i - 1;
                } else if (digitend != std::string::npos) {
                    break;
                }
            }
            if (digitbegin != std::string::npos) {
                auto numdigits = digitend - digitbegin;
                for (int i = 0; i <= last; i += 1) {
                    std::stringstream str;
                    str << std::setfill('0') << std::setw(numdigits) << i;
                    for (std::string::size_type i = 0; i < numdigits; ++i) {
                        file[digitbegin + i] = str.str()[i];
                    }
                    std::cerr << "file: " << file << std::endl;
                    m_fileList[port].emplace_back(file);
                }
            }

            if (numFiles < 0) {
                numFiles = m_fileList[port].size();
            } else {
                numFiles = std::min<int>(numFiles, m_fileList[port].size());
            }
        }

        m_haveFileList = true;
    }

    std::string gridFile = m_gridFilename->getValue();
    if (m_numPartitions->getValue() <= 0)
        m_nparts = size();
    else
        m_nparts = m_numPartitions->getValue();

    m_grids = readGridBlocks(gridFile, m_nparts);
    return (ssize_t(m_grids.size()) == m_nparts);
}

bool ReadItlrBin::read(Reader::Token &token, int timestep, int block)
{
    if (timestep < 0) {
        if (m_haveListFile || m_haveFileList)
            return true;

        timestep = 0;
    }

    if (block < 0)
        block = 0;

    for (int port = 0; port < NumPorts; ++port) {
        if (m_scalarFile[port].empty())
            continue;
        if (ssize_t(m_fileList[port].size()) <= timestep)
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
            scal->addAttribute("_species", m_species[port]);
            token.applyMeta(scal);
            token.addObject(m_dataOut[port], scal);
        } else {
            token.addObject(m_dataOut[port], m_grids[block]);
        }
    }

    return true;
}

bool ReadItlrBin::finishRead()
{
    for (int port = 0; port < NumPorts; ++port)
        m_fileList[port].clear();
    m_grids.clear();

    return true;
}

ReadItlrBin::Block ReadItlrBin::computeBlock(int part) const
{
    int nslices = m_dims[SplitDim];
    int perpart = (nslices + m_nparts - 1) / m_nparts;

    int begin = 0, end = nslices;
    int beginghost = 0, endghost = 0;

    if (part >= 0) {
        begin = part * perpart;
        end = begin + perpart + 1;
        if (part > 0) {
            --begin;
            ++beginghost;
        }
        if (part < m_nparts - 1) {
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

bool readHdf5GridSize(File &file, uint32_t dims[3])
{
#ifdef HAVE_HDF5
    hid_t attr = H5Aopen(file.file, "gridsize", H5P_DEFAULT);
    if (attr < 0) {
        std::cerr << "failed to open attribute 'gridsize'" << std::endl;
        return false;
    }
    hid_t type = H5Aget_type(attr);
    if (H5Aread(attr, type, &dims[0]) < 0) {
        H5Aclose(attr);
        return false;
    }
    std::cerr << "grid dimensions: " << dims[0] << " " << dims[1] << " " << dims[2] << std::endl;
    H5Aclose(attr);
    return true;
#else
    return false;
#endif
}

bool readGridSizeFromGrid(File &file, uint32_t dims[3])
{
    if (file.fp) {
        char header[80];
        size_t n = fread(header, 80, 1, file.fp);
        if (n != 1) {
            std::cerr << "failed to read grid dimension" << std::endl;
            return false;
        }

        return readArray<uint32_t>(file, "gridsize", dims, 3);
    }
    return readHdf5GridSize(file, dims);
}

bool readGridSizeFromField(File &file, uint32_t dims[3])
{
    if (file.fp) {
        char header[80];
        size_t n = fread(header, 80, 1, file.fp);
        if (n != 1) {
            std::cerr << "failed to read grid dimension" << std::endl;
            return false;
        }
        n = fread(header, 80, 1, file.fp);

        std::vector<uint32_t> d(7);
        if (!readArray<uint32_t>(file, "dims", d.data(), 7)) {
            return false;
        }
        for (int i = 0; i < 3; ++i)
            dims[i] = d[i + 3];
        return true;
    }

    return readHdf5GridSize(file, dims);
}

std::vector<RectilinearGrid::ptr> ReadItlrBin::readGridBlocks(const std::string &filename, int nparts)
{
    std::vector<RectilinearGrid::ptr> result;

    File file(filename);
    if (!file.open()) {
        sendError("failed to open grid file %s in format %s", filename.c_str(), file.format().c_str());
        return result;
    }

    uint32_t dims[3]{0, 0, 0};
    if (!readGridSizeFromGrid(file, dims)) {
        sendError("failed to read grid dimensions from %s", filename.c_str());
        return result;
    }

    const std::string axis[3]{"xval", "yval", "zval"};

    if (file.isHdf5()) {
        SplitDim = 0;
    } else {
        SplitDim = 2;
    }

    // read coordinates and prepare for transformation avoiding transposition
    std::vector<Scalar> coords[3];
    for (int i = 0; i < 3; ++i) {
        if (file.isHdf5())
            coords[i].resize(dims[i] + 1);
        else
            coords[i].resize(dims[i]);
        if (!readFloatArray(file, axis[i], coords[i].data(), coords[i].size())) {
            sendError("failed to read grid coordinates %d from %s", i, filename.c_str());
            return result;
        }
        if (file.isHdf5()) {
            // per-cell data was read, map to dual grid
            for (size_t j = 0; j < coords[i].size() - 1; ++j) {
                coords[i][j] += coords[i][j + 1];
                coords[i][j] *= 0.5;
            }
            coords[i].resize(dims[i]);
        }
    }
    if (file.isHdf5()) {
        std::swap(coords[0], coords[2]);
    }
    for (int i = 0; i < 3; ++i) {
        m_dims[i] = coords[i].size();
    }
    if (rank() == 0)
        sendInfo("grid dimensions: %d %d %d", (int)m_dims[0], (int)m_dims[1], (int)m_dims[2]);

    for (int part = 0; part < nparts; ++part) {
        auto b = computeBlock(part);
        std::cerr << "reading block " << part << ", slices " << b.begin << " to " << b.end << std::endl;

        RectilinearGrid::ptr grid(new RectilinearGrid(SplitDim == 0 ? b.end - b.begin : coords[0].size(),
                                                      SplitDim == 1 ? b.end - b.begin : coords[1].size(),
                                                      SplitDim == 2 ? b.end - b.begin : coords[2].size()));
        grid->setNumGhostLayers(SplitDim, RectilinearGrid::Bottom, b.ghost[0]);
        grid->setNumGhostLayers(SplitDim, RectilinearGrid::Top, b.ghost[1]);
        grid->setGlobalIndexOffset(SplitDim, b.begin);
        for (int i = 0; i < 3; ++i) {
            if (i == SplitDim) {
                std::copy(coords[i].begin() + b.begin, coords[i].begin() + b.end, &grid->coords(i)[0]);
            } else {
                std::copy(coords[i].begin(), coords[i].end(), &grid->coords(i)[0]);
            }
        }

        if (file.isHdf5()) {
            Matrix4 t;
            t << Vector4(0, 0, -1, 0), Vector4(0, 1, 0, 0), Vector4(1, 0, 0, 0), Vector4(0, 0, 0, 1);
            grid->setTransform(t);
        }
        updateMeta(grid);

        result.push_back(grid);
    }

    std::cerr << "read " << result.size() << " grid blocks" << std::endl;
    return result;
}

DataBase::ptr ReadItlrBin::readFieldBlock(const std::string &filename, int part) const
{
    File file(filename);
    if (!file.open()) {
        sendError("failed to open file %s in format %s", filename.c_str(), file.format().c_str());
        return DataBase::ptr();
    }

    uint32_t dims[3]{0, 0, 0};
    if (!readGridSizeFromField(file, dims)) {
        sendError("failed to read grid dimensions from %s", filename.c_str());
        return DataBase::ptr();
    }
    if (dims[0] != m_dims[0] || dims[1] != m_dims[1] || dims[2] != m_dims[2]) {
        sendInfo("data with dimensions: %d %d %d", (int)dims[0], (int)dims[1], (int)dims[2]);
        sendError("data set dimensions from %s don't match grid dimensions", filename.c_str());
        return DataBase::ptr();
    }

    auto b = computeBlock(part);
    Index size = 1, offset = 1;
    for (int c = 0; c < 3; ++c) {
        if (c == SplitDim) {
            size *= Index(b.end - b.begin);
            offset *= b.begin;
        } else {
            size *= m_dims[c];
            offset *= m_dims[c];
        }
    }
    //Index rest = m_dims[0]*m_dims[1]*m_dims[2] - offset - size;

    const std::string arrayname1{"funs"};
    const std::string arraynames3[3]{"xval", "yval", "zval"};
    int numComp = 1;
    if (filename.find("velv") == 0 || filename.find("/velv") != std::string::npos) {
        numComp = 3;
    }
    switch (numComp) {
    case 1: {
        if (!skipFloatArray(file, arrayname1, offset)) {
            sendError("failed to skip data from %s", filename.c_str());
            return DataBase::ptr();
        }
        Vec<Scalar>::ptr vec(new Vec<Scalar>(size));
        if (!readFloatArray(file, arrayname1, &vec->x()[0], size)) {
            sendError("failed to read data from %s", filename.c_str());
            return DataBase::ptr();
        }
        return vec;
        break;
    }
    case 3: {
        Vec<Scalar, 3>::ptr vec3(new Vec<Scalar, 3>(size));
        for (int c = 0; c < 3; ++c) {
            if (!skipFloatArray(file, arraynames3[c], offset)) {
                sendError("failed to skip data from %s", filename.c_str());
                return DataBase::ptr();
            }
            if (!readFloatArray(file, arraynames3[c], &vec3->x(c)[0], size)) {
                sendError("failed to read data from %s", filename.c_str());
                return DataBase::ptr();
            }
        }
        return vec3;
        break;
    }
    }

    sendError("expecting scalar or 3-dim vector field, didn't find any in %s", filename.c_str());
    //sendError("have %d dimensions", (int)dims[6]);
    return DataBase::ptr();
}

std::vector<std::string> ReadItlrBin::readListFile(const std::string &filename) const
{
    std::ifstream file(filename);

    std::vector<std::string> files;
    std::string line;
    while (std::getline(file, line)) {
        files.push_back(line);
    }
    return files;
}
