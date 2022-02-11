#ifndef _ReadMPAS_H
#define _ReadMPAS_H

#include <cstddef>
#include <pnetcdf>
#include <mpi.h>
#include <vistle/module/reader.h>
#include <vistle/core/unstr.h>

#define NUMPARAMS 3

using namespace PnetCDF;
using namespace vistle;

class ReadMPAS: public vistle::Reader {
public:
    ReadMPAS(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadMPAS() override;

private:
    enum FileType { grid_type, data_type, zgrid_type };

    bool prepareRead() override;
    bool read(Token &token, int timestep = -1, int block = -1) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;

    bool emptyValue(vistle::StringParameter *ch) const;
    bool dimensionExists(std::string findName, const NcmpiFile &filenames);
    bool variableExists(std::string findName, const NcmpiFile &filename);
    bool validateFile(std::string fullFileName, std::string &redFileName, FileType type);

    bool addCell(Index elem, Index *el, Index *cl, long vPerC, long numVert, long izVert, Index &idx2,
                 const std::vector<int> &vocList);
    bool getData(const NcmpiFile &filename, std::vector<float> *dataValues, const MPI_Offset &nLevels,
                 const Index dataIdx);
    bool setVariableList(const NcmpiFile &filename, bool setCOC);

    Port *m_gridOut = nullptr;
    Port *m_dataOut[NUMPARAMS];

    IntParameter *m_numPartitions, *m_numLevels;
    FloatParameter *m_altitudeScale;
    StringParameter *m_gridFile, *m_partFile, *m_dataFile, *m_zGridFile;
    StringParameter *m_cellsOnCell;
    StringParameter *m_variables[NUMPARAMS];
    StringParameter *m_varDim;

    std::vector<std::string> varDimList = {"2D", "3D", "other"};
    std::string firstFileName, dataFileName, zGridFileName;
    bool hasDataFile = false;
    bool hasZData = false;
    std::string partsPath = "";

    int numPartsFile = 1;
    int numDataFiles = 1;
    std::vector<std::string> dataFileList;

    std::vector<Object::ptr> gridList;
    std::vector<Index> numCellsB;
    std::vector<Index> numCornB;
    std::vector<size_t> blockIdx;
    std::vector<size_t> numGhosts;
    std::vector<std::vector<int>> isGhost;
    std::vector<std::vector<Index>> idxCellsInBlock;

    std::vector<int> partList;
    Index numLevels = 0;
    Index numCells = 0;

    //std::vector<UnstructuredGrid::ptr> gridList;
    std::vector<int> voc, coc,
        cov; // voc (Vertices on Cell); eoc (edges on Cell); coc (cells on cell); cov (vertices on Cell)
    std::vector<Index> eoc;
    std::vector<float> xCoords, yCoords, zCoords; //coordinates of vertices
    bool ghosts = false;
    int finalNumberOfParts = 1;
    std::mutex mtxPartList;
};

#endif
