/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

 /**************************************************************************\
  **                                                           (C)1995 RUS  **
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


#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#endif

#include "ReadNek5000.h"

#include <util/enum.h>
#include <util/byteswap.h>

#include "ReadNek5000.h"
#include "PartitionReader.h"

#include <stdio.h>

#include <core/vec.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>


using namespace vistle;
using namespace std;
namespace fs = boost::filesystem;


bool ReadNek::prepareRead() {

    return true;
}

bool ReadNek::read(Token& token, int timestep, int partition) {
    ++numReads;
    if (!myRead(token, timestep, partition)) {
        sendError("nek: could not read: timestep = " + to_string(timestep) + ", partition = " + to_string(partition));
        finishRead();
    }
    return true;
}

bool ReadNek::myRead(Token& token, int timestep, int partition) {
    auto r = readers.find(partition);
    if (r == readers.end()) {
        r = readers.insert(std::pair<int, nek5000::PartitionReader>(partition, nek5000::PartitionReader{ *readerBase })).first;
        r->second.setPartition(partition, p_useMap->getValue());

    }
    nek5000::PartitionReader* pReader = &r->second;
    if (timestep == -1) {//there is only one grid for all timesteps, so we read it in advance


        int hexes = pReader->getHexes();
        UnstructuredGrid::ptr grid = UnstructuredGrid::ptr(new UnstructuredGrid(hexes, pReader->getNumConn(), pReader->getGridSize()));
        if (pReader->getDim() == 2) {
            std::fill_n(grid->tl().data(), hexes, (const unsigned char)UnstructuredGrid::QUAD);
        } else {
            std::fill_n(grid->tl().data(), hexes, (const unsigned char)UnstructuredGrid::HEXAHEDRON);
        }
        pReader->getConnectivityList(grid->cl().data());

        for (int i = 0; i < hexes + 1; i++) {
            if (pReader->getDim() == 2) {
                grid->el().data()[i] = 4 * i;
            } else {
                grid->el().data()[i] = 8 * i;
            }
        }
        if (!pReader->fillMesh(grid->x().data(), grid->y().data(), grid->z().data())) {
            cerr << "nek: failed to fill mesh" << endl;
            return false;
        }

        mGrids[partition] = grid;
        grid->setBlock(partition);
        grid->setTimestep(-1);
        grid->applyDimensionHint(grid);
        token.addObject(p_grid, grid);
    } else if (!p_only_geometry->getValue()) {
        {//velocities
            if (pReader->hasVelocity()) {
                auto grid = mGrids.find(partition);
                if (grid == mGrids.end()) {
                    sendError(".nek5000 did not find a matching grid for block %d", partition);
                    return false;
                }
                Vec<float, 3>::ptr velocity(new Vec<Scalar, 3>(pReader->getGridSize()));
                velocity->setGrid(grid->second);
                velocity->setTimestep(timestep);
                velocity->setBlock(partition);
                velocity->addAttribute("_species", "velocity");
                velocity->applyDimensionHint(velocity);
                auto x = velocity->x().data();
                auto y = velocity->y().data();
                auto z = velocity->z().data();
                pReader->fillVelocity(timestep, x, y, z);

                grid->second->updateInternals();
                token.addObject(p_velocity, velocity);
            }
        }
        if (pReader->hasPressure()) {


            if (!ReadScalarData(token, p_pressure, "pressure", timestep, partition)) {
                return false;
            }
        }
        if (pReader->hasTemperature()) {
            if (!ReadScalarData(token, p_temperature, "temperature", timestep, partition)) {
                return false;
            }
        }
        for (size_t i = 0; i < pReader->getNumScalarFields(); i++) {
            if (!ReadScalarData(token, pv_misc[i], "s" + std::to_string(i + 1), timestep, partition)) {
                return false;
            }
        }
    }
    return true;
}

bool ReadNek::examine(const vistle::Parameter* param) {
    (void)param;
    if (!fs::exists(p_data_path->getValue())) {
        return false;
    }
    readerBase.reset(new nek5000::ReaderBase(p_data_path->getValue(),p_numPartitions->getValue(), p_numBlocks->getValue()));
    if(!readerBase->init())
        return false;
    size_t oldNumSFields = pv_misc.size();

    setTimesteps(readerBase->getNumTimesteps());
    setPartitions(p_numPartitions->getValue());
    for (size_t i = oldNumSFields; i < readerBase->getNumScalarFields(); i++) {
        pv_misc.push_back(createOutputPort("scalarFiled" + std::to_string(i + 1), "scalar data " + std::to_string(i + 1)));
    }
    for (size_t i = oldNumSFields; i > readerBase->getNumScalarFields(); --i) {
        destroyPort(pv_misc[i]);
    }
    return true;

}

bool ReadNek::finishRead() {
    mGrids.clear();
    readers.clear();
    std::cerr << "_________________________________________________________" << std::endl;
    std::cerr << "read was called "<< numReads << " times" << std::endl;
    std::cerr << "_________________________________________________________" << std::endl;
    return true;
}


bool ReadNek::ReadScalarData(Reader::Token &token, vistle::Port *p, const std::string& varname, int timestep, int partition) {

    auto r = readers.find(partition);
    if(r == readers.end())
    {
        sendError("nek: ReadScalar failed to find reader for partition " + std::to_string(partition));
        return false;
    }
    nek5000::PartitionReader *pReader = &r->second;
    auto grid = mGrids.find(partition);
    if (grid == mGrids.end()) {
        sendError(".nek5000 did not find a matching grid for block %d", partition);
        return false;
    }
    Vec<Scalar>::ptr scal(new Vec<Scalar>(pReader->getGridSize()));
    if(!pReader->fillScalarData(varname, timestep, scal->x().data()))
    {
        sendError("nek: ReadScalar failed to get data for " + varname + " for partition " + to_string(partition) + " in timestep " + to_string(timestep));
        return false;
    }
    scal->setGrid(grid->second);
    scal->setTimestep(timestep);
    scal->setBlock(partition);
    scal->setMapping(false ? DataBase::Element : DataBase::Vertex);
    scal->addAttribute("_species", varname);
    token.addObject(p, scal);
    return true;
}

int ReadNek::numberOfUniqePoints(vistle::UnstructuredGrid::ptr grid)     {
    std::set<std::array<float, 3>> uniquePts;
    for (size_t i = 0; i < grid->getNumCoords(); i++) {
        uniquePts.insert(std::array<float, 3>{grid->x().data()[i], grid->y().data()[i], grid->z().data()[i]});
    }
    return uniquePts.size();
}

ReadNek::ReadNek(const std::string& name, int moduleID, mpi::communicator comm)
   :vistle::Reader("Read .nek5000 files", name, moduleID, comm)
{
   // Output ports
   p_grid = createOutputPort("grid_out", "grid");
   p_velocity = createOutputPort("velosity_out", "velocity");
   p_pressure = createOutputPort("pressure_out", "pressure data");
   p_temperature = createOutputPort("temperature_out", "temperature data");
   
   // Parameters
   p_data_path = addStringParameter("filename", "Geometry file path", "", Parameter::Filename);
   //p_byteswap = addIntParameter("byteswap", "Perform Byteswapping", Auto, Parameter::Choice);
   //V_ENUM_SET_CHOICES(p_byteswap, ByteSwap);
   p_only_geometry = addIntParameter("OnlyGeometry", "Reading only Geometry? yes|no", false, Parameter::Boolean);
   p_useMap = addIntParameter("useMap", "use .map file to partition mesh", false, Parameter::Boolean);

   //p_readOptionToGetAllTimes = addIntParameter("ReadOptionToGetAllTimes", "Read all times and cycles", false, Parameter::Boolean);
   //p_duplicateData = addIntParameter("DublicateData", "Duplicate data for particle advection(slower for all other techniques)", false, Parameter::Boolean);
   p_numBlocks = addIntParameter("number of blocks", "number of blocks, <= 0 to read all", 0);
   p_numPartitions = addIntParameter("numerOfPartitions", "number of partitions", 1);


   observeParameter(p_data_path);
   //observeParameter(p_byteswap);
   observeParameter(p_only_geometry);
   //observeParameter(p_readOptionToGetAllTimes);
   //observeParameter(p_duplicateData);
   observeParameter(p_numBlocks);
   observeParameter(p_numPartitions);

   setParallelizationMode(ParallelizationMode::Serial);

}

ReadNek::~ReadNek() {
}
MODULE_MAIN(ReadNek)


