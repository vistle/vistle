/* This file is part of Vistle.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */


 /**************************************************************************\
  **                                                           (C)2019 HLRS **
  **                                                                        **
  ** Description: Read module for Nek5000 data                              **
  **                                                                        **
  **                                                                        **
  **                                                                        **
  **                                                                        **
  **                                                                        **
  ** Author:                                                                **
  **                                                                        **
  **                             Dennis Grieger                             **
  **                                 HLRS                                   **
  **                            Nobelstraße 19                              **
  **                            70550 Stuttgart                             **
  **                                                                        **
  ** Date:  08.07.19  V1.0                                                  **
 \**************************************************************************/
#ifndef _READNEK5000_H
#define _READNEK5000_H

#include "PartitionReader.h"

#include <vistle/util/coRestraint.h>

#include <vistle/core/vec.h>
//#include <vistle/core/structuredgrid.h>
#include <vistle/core/unstr.h>
#include <vistle/core/index.h>
#include <vistle/module/reader.h>

#include <vector>
#include <map>
#include <string>

#include <mutex>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ByteSwap, (Off)(On)(Auto))

class ReadNek : public vistle::Reader {
public:
private:
    bool prepareRead() override;
    bool read(Token& token, int timestep = -1, int partition = -1) override;
    bool myRead(Token& token, int timestep = -1, int partition = -1);
    bool examine(const vistle::Parameter* param) override;
    bool finishRead() override;

    bool ReadScalarData(Reader::Token &token, vistle::Port *p, const std::string& varname, int timestep, int partition);

    int numberOfUniqePoints(vistle::UnstructuredGrid::ptr grid);

    std::mutex readerMapMutex, gridMapMutex;

    // Ports
    vistle::Port* p_grid = nullptr;
    vistle::Port* p_velocity = nullptr;
    vistle::Port* p_pressure = nullptr;
    vistle::Port* p_temperature = nullptr;
    vistle::Port* p_blockNumber = nullptr;

    std::vector<vistle::Port*> pv_misc;
    // Parameters
    vistle::StringParameter* p_data_path = nullptr;
    vistle::IntParameter* p_only_geometry = nullptr;
    vistle::IntParameter* p_useMap = nullptr;
    vistle::IntParameter* p_byteswap = nullptr;
    vistle::IntParameter* p_readOptionToGetAllTimes = nullptr;
    vistle::IntParameter* p_duplicateData = nullptr;
    vistle::IntParameter* p_numBlocks = nullptr;
    vistle::IntParameter* p_numPartitions = nullptr;
    vistle::IntParameter* p_numGhostLayers = nullptr;
    float minVeclocity = 0, maxVelocity = 0;
    int numReads = 0;
    bool parameterChanged = true;
    vistle::UnstructuredGrid::ptr readGrid; //the grid to use if params haven't changed
    vistle::Vec<vistle::Index>::ptr readBlockNumbers; //the blockNumbers to use if params haven't changed
    std::map<vistle::Port*, std::vector<vistle::Object::ptr>> readData; //the data to use if params haven't changed
    std::unique_ptr<nek5000::ReaderBase> readerBase;
    std::map<int, nek5000::PartitionReader> readers;
    //maps the grids to the blocks
    std::vector<vistle::UnstructuredGrid::const_ptr> mGrids;


public:
    ReadNek(const std::string& name, int moduleID, mpi::communicator comm);
    ~ReadNek() override;

};

#endif // _READNEK5000_H
