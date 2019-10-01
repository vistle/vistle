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

#include <util/coRestraint.h>

#include <core/vec.h>
//#include <core/structuredgrid.h>
#include <core/unstr.h>
#include<core/index.h>
#include <module/reader.h>

#include <vector>
#include <map>
#include <string>
#include <iostream>    
#include <fstream>
#include <cstdio>

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

    // Ports
    vistle::Port* p_grid = nullptr;
    vistle::Port* p_velocity = nullptr;
    vistle::Port* p_pressure = nullptr;
    vistle::Port* p_temperature = nullptr;

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
    vistle::VectorParameter* p_testConnectivity = nullptr;
    float minVeclocity = 0, maxVelocity = 0;
    int numReads = 0;
    std::unique_ptr<nek5000::ReaderBase> readerBase;
    std::map<int, nek5000::PartitionReader> readers;
    //maps the grids to the blocks
    std::map<int, vistle::UnstructuredGrid::ptr> mGrids;


public:
    ReadNek(const std::string& name, int moduleID, mpi::communicator comm);
    void testConnectivity(int dimX, int dimY, int dimZ);
    ~ReadNek() override;

};

#endif // _READNEK5000_H
