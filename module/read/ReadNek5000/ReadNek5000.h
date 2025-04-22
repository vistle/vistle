#ifndef VISTLE_READNEK5000_READNEK5000_H
#define VISTLE_READNEK5000_READNEK5000_H

#include "PartitionReader.h"

#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include <vistle/core/index.h>
#include <vistle/module/reader.h>

#include <vector>
#include <map>
#include <string>

#include <mutex>


class ReadNek: public vistle::Reader {
public:
private:
    bool read(Token &token, int timestep = -1, int partition = -1) override;
    bool myRead(Token &token, int timestep = -1, int partition = -1);
    void sendCachedObjects(Token &token, int timestep, int partition);
    nek5000::PartitionReader &getPartitionReader(int partition);
    vistle::Object::ptr readGrid(int timestep, nek5000::PartitionReader &partitionReader);
    vistle::Object::ptr generateBlockNumbers(nek5000::PartitionReader &partitionReader, vistle::Object::const_ptr grid);
    bool addGridAndBlockNumbers(Token &token, int timestep, nek5000::PartitionReader &partitionReader);

    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;
    bool ReadVelocity(Reader::Token &token, vistle::Port *p, int timestep, nek5000::PartitionReader &partitionReader);
    bool ReadScalarData(Reader::Token &token, vistle::Port *p, const std::string &varname, int timestep,
                        nek5000::PartitionReader &partitionReader);

    int numberOfUniqePoints(vistle::UnstructuredGrid::ptr grid);

    std::mutex m_readerMapMutex;

    // Ports
    vistle::Port *m_gridPort = nullptr;
    vistle::Port *m_velocityPort = nullptr;
    vistle::Port *m_pressurePort = nullptr;
    vistle::Port *m_temperaturePort = nullptr;
    vistle::Port *m_blockIndexPort = nullptr;

    std::vector<vistle::Port *> m_miscPorts;
    // Parameters
    vistle::StringParameter *m_filePathParam = nullptr;
    vistle::IntParameter *m_geometryOnlyParam = nullptr;
    vistle::IntParameter *m_numBlocksParam = nullptr;
    vistle::IntParameter *m_numPartitionsParam = nullptr;
    vistle::IntParameter *m_numGhostLayersParam = nullptr;

    std::unique_ptr<nek5000::ReaderBase> m_staticData;
    //one reader for each partition
    std::map<int, nek5000::PartitionReader> m_readers;
    //the last read grid for each partitions, updated by readGrid
    std::vector<vistle::UnstructuredGrid::const_ptr> m_grids;


public:
    ReadNek(const std::string &name, int moduleID, mpi::communicator comm);
};

#endif
