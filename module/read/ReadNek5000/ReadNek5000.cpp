/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see LICENSE.txt.

 * License: LGPL 2+ */

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#endif

#include "ReadNek5000.h"

#include <vistle/util/enum.h>
#include <vistle/util/byteswap.h>
#include <vistle/util/filesystem.h>

#include "ReadNek5000.h"
#include "PartitionReader.h"

#include <stdio.h>

#include <vistle/core/vec.h>
#include <vistle/core/parameter.h>

using namespace vistle;
using namespace std;
namespace fs = vistle::filesystem;

bool ReadNek::read(Token &token, int timestep, int partition)
{
    if (timestep != -1) {
        if (!myRead(token, timestep, partition)) {
            sendError("nek: could not read: timestep = %d, partition = %d", timestep, partition);
            finishRead();
        }
    }
    return true;
}

bool ReadNek::myRead(Token &token, int timestep, int partition)
{
    auto &partitionReader = getPartitionReader(partition);

    partitionReader.UpdateCyclesAndTimes(timestep);
    if (partitionReader.hasGrid(timestep)) {
        if (!addGridAndBlockNumbers(token, timestep, partitionReader)) {
            return false;
        }
    } else {
        auto grid = m_grids[partitionReader.partition()];
        auto newGrid = grid->clone();
        token.applyMeta(newGrid);
        token.addObject(m_gridPort, newGrid);
    }
    if (!m_geometryOnlyParam->getValue()) {
        if (partitionReader.hasVelocity() && m_velocityPort->isConnected() &&
            !ReadVelocity(token, m_pressurePort, timestep, partitionReader)) {
            return false;
        }
        if (partitionReader.hasPressure() && m_pressurePort->isConnected() &&
            !ReadScalarData(token, m_pressurePort, "pressure", timestep, partitionReader)) {
            return false;
        }
        if (partitionReader.hasTemperature() && m_temperaturePort->isConnected() &&
            !ReadScalarData(token, m_temperaturePort, "temperature", timestep, partitionReader)) {
            return false;
        }
        for (size_t i = 0; i < partitionReader.getNumScalarFields(); i++) {
            if (m_miscPorts[i]->isConnected()) {
                if (!ReadScalarData(token, m_miscPorts[i], "s" + std::to_string(i + 1), timestep, partitionReader)) {
                    return false;
                }
            }
        }
    }
    return true;
}

nek5000::PartitionReader &ReadNek::getPartitionReader(int partition)
{
    lock_guard<mutex> guard(m_readerMapMutex);
    auto r = m_readers.find(partition);
    if (r == m_readers.end()) {
        r = m_readers.emplace(partition, nek5000::PartitionReader{*m_staticData}).first;
        r->second.setPartition(partition, m_numGhostLayersParam->getValue());
    }
    return r->second;
}

Object::ptr ReadNek::readGrid(int timestep, nek5000::PartitionReader &partitionReader)
{
    auto hexes = partitionReader.getHexes();
    auto ghostHexes = partitionReader.getNumGhostHexes();
    partitionReader.constructUnstructuredGrid(timestep);
    UnstructuredGrid::ptr grid =
        UnstructuredGrid::ptr(new UnstructuredGrid(hexes, partitionReader.getNumConn(), partitionReader.getGridSize()));
    Byte elemType = partitionReader.getDim() == 2 ? UnstructuredGrid::QUAD : UnstructuredGrid::HEXAHEDRON;

    partitionReader.fillGrid({grid->x().data(), grid->y().data(), grid->z().data()});
    std::fill_n(grid->tl().data(), hexes - ghostHexes, elemType);
    std::fill_n(grid->ghost().data() + hexes - ghostHexes, ghostHexes, cell::GHOST);
    partitionReader.fillConnectivityList(grid->cl().data());
    int numCorners = partitionReader.getDim() == 2 ? 4 : 8;
    for (size_t i = 0; i < hexes + 1; i++) {
        grid->el().data()[i] = numCorners * i;
    }

    m_grids[partitionReader.partition()] = grid;
    grid->setBlock(partitionReader.partition());
    grid->setTimestep(timestep);
    return grid;
}

Object::ptr ReadNek::generateBlockNumbers(nek5000::PartitionReader &partitionReader, Object::const_ptr grid)
{
    Vec<Index>::ptr blockNumbers(new Vec<Index>(partitionReader.getGridSize()));
    partitionReader.fillBlockNumbers(blockNumbers->x().data());
    blockNumbers->setGrid(grid);
    blockNumbers->setBlock(partitionReader.partition());
    blockNumbers->setMapping(DataBase::Vertex);
    blockNumbers->addAttribute("_species", "blockBumber");
    return blockNumbers;
}

bool ReadNek::addGridAndBlockNumbers(Token &token, int timestep, nek5000::PartitionReader &partitionReader)
{
    if (auto grid = readGrid(timestep, partitionReader)) {
        token.applyMeta(grid);
        token.addObject(m_gridPort, grid);
        auto blockNumbers = generateBlockNumbers(partitionReader, grid);
        token.applyMeta(blockNumbers);
        token.addObject(m_blockIndexPort, blockNumbers);
        return true;
    } else
        return false;
}

bool ReadNek::examine(const vistle::Parameter *param)
{
    if (param && param != m_filePathParam && param != m_numPartitionsParam && param != m_numBlocksParam) {
        // nothing changed
        return true;
    }

    if (!param || param == m_filePathParam) {
        if (!fs::exists(m_filePathParam->getValue())) {
            cerr << "file " << m_filePathParam->getValue() << " does not exist" << endl;
            return false;
        }
    }

    vistle::Integer numPartitions = m_numPartitionsParam->getValue() == 0 ? size() : m_numPartitionsParam->getValue();
    m_staticData.reset(
        new nek5000::ReaderBase(m_filePathParam->getValue(), numPartitions, m_numBlocksParam->getValue()));
    if (!m_staticData->init()) {
        for (auto p: m_miscPorts)
            destroyPort(p);
        m_miscPorts.clear();
        m_grids.clear();
        return false;
    }
    setPartitions(numPartitions);
    m_grids.resize(numPartitions);
    setTimesteps(m_staticData->getNumTimesteps());

    while (m_miscPorts.size() < m_staticData->getNumScalarFields()) {
        auto i = m_miscPorts.size();
        m_miscPorts.push_back(
            createOutputPort("scalarField" + std::to_string(i + 1), "scalar data " + std::to_string(i + 1)));
    }
    while (m_miscPorts.size() > m_staticData->getNumScalarFields()) {
        destroyPort(m_miscPorts.back());
        m_miscPorts.pop_back();
    }
    return true;
}

bool ReadNek::finishRead()
{
    m_readers.clear();
    return true;
}

bool ReadNek::ReadVelocity(Reader::Token &token, vistle::Port *p, int timestep,
                           nek5000::PartitionReader &partitionReader)
{
    auto grid = m_grids[partitionReader.partition()];
    if (!grid) {
        sendError(".nek5000 did not find a matching grid for partition %zu", partitionReader.partition());
        return false;
    }
    Vec<Scalar, 3>::ptr velocity(new Vec<Scalar, 3>(partitionReader.getGridSize()));
    velocity->setMapping(DataBase::Vertex);
    velocity->setGrid(grid);
    velocity->setTimestep(timestep);
    velocity->setBlock(partitionReader.partition());
    velocity->addAttribute("_species", "velocity");
    partitionReader.fillVelocity(timestep, {velocity->x().data(), velocity->y().data(), velocity->z().data()});
    token.applyMeta(velocity);
    token.addObject(m_velocityPort, velocity);
    return true;
}


bool ReadNek::ReadScalarData(Reader::Token &token, vistle::Port *p, const std::string &varname, int timestep,
                             nek5000::PartitionReader &partitionReader)
{
    auto grid = m_grids[partitionReader.partition()];
    if (!grid) {
        sendError(".nek5000 did not find a matching grid for block %zu", partitionReader.partition());
        return false;
    }
    Vec<Scalar>::ptr scal(new Vec<Scalar>(partitionReader.getGridSize()));
    if (!partitionReader.fillScalarData(varname, timestep, scal->x().data())) {
        sendError("nek: ReadScalar failed to get data for " + varname + " for partition " +
                  to_string(partitionReader.partition()) + " in timestep " + to_string(timestep));
        return false;
    }
    scal->setMapping(DataBase::Vertex);
    scal->setGrid(grid);
    scal->setTimestep(timestep);
    scal->setBlock(partitionReader.partition());
    scal->addAttribute("_species", varname);
    token.applyMeta(scal);
    token.addObject(p, scal);
    return true;
}

int ReadNek::numberOfUniqePoints(vistle::UnstructuredGrid::ptr grid)
{
    std::set<std::array<vistle::Scalar, 3>> uniquePts;
    for (size_t i = 0; i < grid->getNumCoords(); i++) {
        uniquePts.insert(std::array<vistle::Scalar, 3>{grid->x().data()[i], grid->y().data()[i], grid->z().data()[i]});
    }
    return uniquePts.size();
}

ReadNek::ReadNek(const std::string &name, int moduleID, mpi::communicator comm): vistle::Reader(name, moduleID, comm)
{
    // Output ports
    m_gridPort = createOutputPort("grid_out", "grid");
    m_velocityPort = createOutputPort("velosity_out", "velocity");
    m_pressurePort = createOutputPort("pressure_out", "pressure data");
    m_temperaturePort = createOutputPort("temperature_out", "temperature data");
    m_blockIndexPort = createOutputPort("blockNumber_out", "Nek internal block numbers");

    // Parameters
    m_filePathParam = addStringParameter("filename", "Geometry file path", "", Parameter::ExistingFilename);
    setParameterFilters(m_filePathParam, "Nek5000 geometry (*.nek5000)");
    m_geometryOnlyParam = addIntParameter("OnlyGeometry", "Reading only Geometry? yes|no", false, Parameter::Boolean);
    m_numGhostLayersParam = addIntParameter(
        "num_ghost_layers", "number of ghost layers around eeach partition, a layer consists of whole blocks", 1);
    setParameterMinimum(m_numGhostLayersParam, Integer{0});
    m_numBlocksParam = addIntParameter("num_blocks", "number of blocks to read from file, <= 0 to read all", 0);
    m_numPartitionsParam = addIntParameter(
        "num_partitions", "number of parallel partitions to use for reading, 0 = one partition for each rank", 0);
    setParameterMinimum(m_numPartitionsParam, Integer{0});
    observeParameter(m_numGhostLayersParam);
    observeParameter(m_filePathParam);
    observeParameter(m_geometryOnlyParam);
    observeParameter(m_numBlocksParam);
    observeParameter(m_numPartitionsParam);

    setParallelizationMode(ParallelizationMode::ParallelizeBlocks);
}

MODULE_MAIN(ReadNek)
