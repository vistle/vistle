
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

#include <module/reader.h>
#include <netcdfcpp.h>
#include <core/structuredgrid.h>

#define NUMPARAMS 2

using namespace vistle;

class ReadWRFChem: public vistle::Reader
{
public:
    ReadWRFChem(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadWRFChem() override;
private:
    bool prepareRead() override;
    bool read(Token &token, int timestep=-1, int block=-1) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;

    void setMeta(Object::ptr obj, int blockNr, int totalBlockNr, int timestep) const;
    bool emptyValue(vistle::StringParameter *ch) const;

    struct Block {
        int part=0;
        int begin=0, end=0;
        int ghost[2]={0,0};
    };

    bool inspectDir();
    bool addDataToPort(Reader::Token &token, NcFile *ncDataFile, int vi, StructuredGrid::ptr outGrid, Block *b, int block, int t) const;
    StructuredGrid::ptr generateGrid(Block *b) const;


    std::vector<std::string> fileList;
    int numFiles = 0;
    Block computeBlock(int part, int nBlocks, long blockBegin, long cellsPerBlock, long numCellTot) const;

    Port *m_gridOut = nullptr;
    Port *m_dataOut[NUMPARAMS];

    StringParameter *m_filedir;
    StringParameter *m_gridChoiceX, *m_gridChoiceY, *m_gridChoiceZ;
    StringParameter *m_variables[NUMPARAMS];

    IntParameter *m_numPartitions, *m_numPartitionsLat, *m_numPartitionsVer;

    NcFile *ncFirstFile;

    std::vector<vistle::Float> dataOut;

    int numBlocks = 1;
};

#endif
