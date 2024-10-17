#ifndef _ReadMPAS_H
#define _ReadMPAS_H

#include <cstddef>
#include <vistle/module/reader.h>
#ifdef USE_NETCDF
#include <netcdf.h>
#ifdef NETCDF_PARALLEL
#include <netcdf_par.h>
#define ReadMPAS ReadMpasNetcdf
#endif
#else
#include <pnetcdf>
#include <mpi.h>
#define ReadMPAS ReadMpasPnetcdf
#endif
#include <vistle/core/unstr.h>

#define NUMPARAMS 4

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
#ifndef USE_NETCDF
    bool dimensionExists(std::string findName, const PnetCDF::NcmpiFile &filenames);
    bool variableExists(std::string findName, const PnetCDF::NcmpiFile &filename);
#endif
    bool validateFile(std::string fullFileName, std::string &redFileName, FileType type);

    bool addCell(Index elem, bool ghost, Index &curElem, UnstructuredGrid::ptr uGrid, long vPerC, long numVert,
                 long izVert, Index &idx2, const std::vector<Index> &vocList);
    bool addPoly(Index elem, Index &curElem, Index *el, Index *cl, long vPerC, long numVert, long izVert, Index &idx2,
                 const std::vector<Index> &vocList);
    bool addWedge(bool ghost, Index &curElem, Index center, Index n1, Index n2, Index layer, Index nVertPerLayer,
                  UnstructuredGrid::ptr uGrid, Index &idx2);
    bool addTri(Index &curElem, Index center, Index n1, Index n2, Index *cl, Index &idx2);
#ifdef USE_NETCDF
    std::vector<vistle::Scalar> getData(int ncid, Index startLevel, Index nLevels, Index dataIdx,
                                        bool velocity = false);
    bool setVariableList(int ncid, FileType ft, bool setCOC);
#else
    bool getData(const PnetCDF::NcmpiFile &filename, std::vector<Scalar> *dataValues, const MPI_Offset &startLevel,
                 const MPI_Offset &nLevels, const Index dataIdx, bool velocity = false);
    bool setVariableList(const PnetCDF::NcmpiFile &filename, FileType ft, bool setCOC);
#endif
    bool readVariable(Reader::Token &token, int timestep, int block, Index dataIdx, unsigned nLevels,
                      std::vector<Scalar> *dataValues, bool velocity);

    Port *m_gridOut = nullptr;
    Port *m_dataOut[NUMPARAMS];
    Port *m_velocityOut = nullptr;

    IntParameter *m_numPartitions, *m_numLevels, *m_bottomLevel;
    FloatParameter *m_altitudeScale;
    StringParameter *m_gridFile, *m_partFile, *m_dataFile, *m_zGridFile;
    StringParameter *m_variables[NUMPARAMS];
    StringParameter *m_velocityVar[3];
    IntParameter *m_velocityScaleRad = nullptr;
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

    size_t numPartsFile = 1;
    std::vector<std::string> dataFileList;
    Index numGridCells = 0;

    std::vector<Object::ptr> gridList;
    std::vector<Index> numCellsB;
    std::vector<Index> numTrianglesB; // block with lowest numbered center as corner of Delaunay triangle owns it
    std::vector<Index> numCornB;
    std::vector<std::vector<Index>> idxCellsInBlock;

    std::vector<int> partList;
    unsigned numLevels = 0;
    size_t numCells = 0;

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
