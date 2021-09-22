#ifndef _READWRFCHEM_H
#define _READWRFCHEM_H
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

#ifdef OLD_NETCDFCXX
#include <netcdfcpp>
#else
#include <ncFile.h>
#include <ncVar.h>
#include <ncDim.h>
using namespace netCDF;
#endif
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

    void setMeta(Object::ptr obj, int blockNr, int totalBlockNr, int timestep) const;
    bool emptyValue(vistle::StringParameter *ch) const;

    struct Block {
        int part = 0;
        size_t begin = 0, end = 0;
        int ghost[2] = {0, 0};
    };

    bool inspectDir();
    bool addDataToPort(Reader::Token &token, NcFile *ncDataFile, int vi, Object::ptr outGrid, Block *b, int block,
                       int t) const;
    Object::ptr generateGrid(Block *b) const;
    Block computeBlock(int part, int nBlocks, long blockBegin, long cellsPerBlock, long numCellTot) const;

    std::map<int, Object::ptr> outObject;

    std::vector<std::string> fileList;
    std::vector<vistle::Float> dataOut;
    std::vector<std::string> varDimList = {"2D", "3D", "other"};

    int numFiles = 0;
    int numBlocks = 1;

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

    IntParameter *m_numPartitions, *m_numPartitionsLat, *m_numPartitionsVer;

    NcFile *ncFirstFile;
};

#endif
