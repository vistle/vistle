#ifndef VISTLE_READWRFCHEM_READWRFCHEM_H
#define VISTLE_READWRFCHEM_READWRFCHEM_H

/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for WRFChem data         	                   **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:    Leyla Kern                                                  **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Date:  31.07.2019                                                      **
\**************************************************************************/

#include <cstddef>
#include <vistle/core/structuredgrid.h>
#include <vistle/module/reader.h>

/* #include <netcdfcpp.h> */

#include <ncFile.h>
#include <ncVar.h>
#include <ncDim.h>

#define NUMPARAMS 6

using namespace vistle;

class ReadWRFChem: public vistle::Reader {
public:
    ReadWRFChem(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadWRFChem() override;

private:
    bool prepareRead() override;
    bool read(Token &token, int timestep = -1, int block = -1) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;

    void setMeta(Object::ptr obj, Index blockNr, size_t totalBlockNr, int timestep) const;
    bool emptyValue(vistle::StringParameter *ch) const;

    struct Block {
        Index part = 0;
        size_t begin = 0, end = 0;
        int ghost[2] = {0, 0};
    };

    bool inspectDir();
    bool addDataToPort(Reader::Token &token, netCDF::NcFile *ncDataFile, int vi, Object::ptr outGrid, Block *b,
                       Index block, int t) const;
    Object::ptr generateGrid(Block *b) const;
    Block computeBlock(Index part, size_t nBlocks, Index blockBegin, size_t cellsPerBlock, size_t numCellTot) const;

    std::map<int, Object::ptr> outObject;

    std::vector<std::string> fileList;
    std::vector<vistle::Float> dataOut;
    std::vector<std::string> varDimList = {"2D", "3D", "other"};

    int numFiles = 0;
    int numBlocks = 1;
    Index numBlocksLat = 0;
    Index numBlocksVer = 0;

    Port *m_gridOut = nullptr;
    Port *m_dataOut[NUMPARAMS];

    StringParameter *m_filedir;
    StringParameter *m_gridLat, *m_gridLon;
    StringParameter *m_variables[NUMPARAMS];
    StringParameter *m_varDim;
    StringParameter *m_trueHGT;
    StringParameter *m_PHB;
    StringParameter *m_PH;
    StringParameter *m_gridZ;

    IntParameter *m_numPartitionsLat, *m_numPartitionsVer;

    mutable std::mutex m_ncMutex;
    std::unique_ptr<netCDF::NcFile> m_ncFirstFile;
};

#endif
