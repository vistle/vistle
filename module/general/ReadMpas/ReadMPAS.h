#ifndef _ReadMPAS_H
#define _ReadMPAS_H

#include <cstddef>
#include <pnetcdf>
#include <mpi.h>
#include <vistle/module/reader.h>

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

    bool addCell(Index elem, bool ghost, Index &curElem, Index *el, Byte *tl, Index *cl, long vPerC, long numVert,
                 long izVert, Index &idx2, const std::vector<Index> &vocList);
    bool addPoly(Index elem, Index &curElem, Index *el, Index *cl, long vPerC, long numVert, long izVert, Index &idx2,
                 const std::vector<Index> &vocList);
    bool addWedge(bool ghost, Index &curElem, Index center, Index n1, Index n2, Index layer, Index nVertPerLayer,
                  Index *el, Byte *tl, Index *cl, Index &idx2);
    bool addTri(Index &curElem, Index center, Index n1, Index n2, Index *cl, Index &idx2);
    bool getData(const NcmpiFile &filename, std::vector<float> *dataValues, const MPI_Offset &nLevels,
                 const Index dataIdx);
    bool setVariableList(const NcmpiFile &filename, FileType ft, bool setCOC);

    Port *m_gridOut = nullptr;
    Port *m_dataOut[NUMPARAMS];

    IntParameter *m_numPartitions, *m_numLevels;
    FloatParameter *m_altitudeScale;
    StringParameter *m_gridFile, *m_partFile, *m_dataFile, *m_zGridFile;
    StringParameter *m_cellsOnCell;
    StringParameter *m_variables[NUMPARAMS];
    StringParameter *m_varDim;
    bool m_voronoiCells = false;
    bool m_projectDown = true;
    IntParameter *m_cellMode = nullptr;

    std::vector<std::string> varDimList = {"2D", "3D"};
    std::string firstFileName, dataFileName, zGridFileName;
    std::map<std::string, FileType> m_2dChoices, m_3dChoices;
    bool hasDataFile = false;
    bool hasZData = false;
    std::string partsPath = "";

    int numPartsFile = 1;
    std::vector<std::string> dataFileList;
    Index numGridCells = 0;

    std::vector<Object::ptr> gridList;
    std::vector<Index> numCellsB;
    std::vector<Index> numTrianglesB; // block with lowest numbered center as corner of Delaunay triangle owns it
    std::vector<Index> numCornB;
    std::vector<std::vector<Index>> idxCellsInBlock;

    std::vector<int> partList;
    unsigned numLevels = 0;
    Index numCells = 0;

    std::vector<unsigned> voc; // voc (Vertices on Cell)
    std::vector<unsigned> coc; // coc (cells on cell)
    std::vector<unsigned> cov; // cov (vertices on Cell)
    std::vector<unsigned char> eoc; // eoc (edges on Cell)
    std::vector<float> xCoords, yCoords, zCoords; //coordinates of vertices
    bool ghosts = false;
    int finalNumberOfParts = 1;
    std::mutex mtxPartList;
};

#endif
