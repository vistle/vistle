/* ------------------------------------------------------------------------------------
           READ MPAS
   
       - read netCDF files containing hexagonal mpas mesh, data and partition info
       - required files: "grid_file" and "part_file"

       - uses pnetcdf

       v1.0: L.Kern 2021

------------------------------------------------------------------------------------ */
#include "ReadMPAS.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <vistle/core/polygons.h>
#include <fstream>

namespace bf = boost::filesystem;
using namespace PnetCDF;
using namespace vistle;

#define MAX_VAL 100000000
#define MAX_EDGES 6 // maximal edges on cell
#define MSL 6371229.0 //sphere radius on mean sea level (earth radius)
#define MAX_VERT 3 // vertex degree
#define RAD_SCALE 20 //scale radius to make atmosphere look thicker


// CONSTRUCTOR
ReadMPAS::ReadMPAS(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    m_gridFile =
        addStringParameter("grid_file", "File containing the grid",
                           "/mnt/raid/home/hpcleker/Desktop/MPAS/test40962/x1.40962.grid.nc", Parameter::Filename);
    m_dataFile = addStringParameter("data_file", "File containing data", "", Parameter::Filename);
    m_zGridFile = addStringParameter("zGrid_file_dir",
                                     "File containing the vertical coordinates (elevation of cell from mean sea level",
                                     "/", Parameter::Filename);
    m_partFile = addStringParameter("part_file", "File containing the grid partitions",
                                    "/mnt/raid/home/hpcleker/Desktop/MPAS/test40962/x1.40962.graph.info.part.8",
                                    Parameter::Filename);

    m_numPartitions = addIntParameter("numParts", "Number of Partitions", Parameter::Integer);
    setIntParameter("numParts", 1);
    m_numLevels = addIntParameter("numLevels", "Number of vertical levels to read", Parameter::Integer);
    m_cellsOnCell = addStringParameter("cellsOnCell", "List of neighboring cells (for ghosts)", "", Parameter::Choice);

    m_gridOut = createOutputPort("grid_out", "grid");
    m_varDim = addStringParameter("var_dim", "Dimension of variables", "", Parameter::Choice);
    setParameterChoices(m_varDim, varDimList);
    setParameter(m_varDim->getName(), varDimList[0]);

    char s_var[50];
    std::vector<std::string> varChoices;
    varChoices.push_back("(NONE)");
    for (int i = 0; i < NUMPARAMS; ++i) {
        sprintf(s_var, "Variable_%d", i);
        m_variables[i] = addStringParameter(s_var, s_var, "", Parameter::Choice);

        setParameterChoices(m_variables[i], varChoices);
        observeParameter(m_variables[i]);
        sprintf(s_var, "data_out_%d", i);
        m_dataOut[i] = createOutputPort(s_var, "scalar data");
    }

    setParallelizationMode(ParallelizeBlocks);
    //setReducePolicy(message::ReducePolicy::ReducePolicy::OverAll);

    observeParameter(m_gridFile);
    observeParameter(m_dataFile);
    observeParameter(m_partFile);
    observeParameter(m_varDim);
}

// DESTRUCTOR
ReadMPAS::~ReadMPAS()
{}

// PREPARE READING
// set number of partitions and timesteps
bool ReadMPAS::prepareRead()
{
    ReadMPAS::examine(nullptr);
    //set partitions
    std::string sPartFile = m_partFile->getValue();
    if (sPartFile.empty()) {
        sendInfo("No partitioning file given: continue with single block");
        return true;
    }

    partsPath = "";
    numPartsFile = 1;
    bf::path path(sPartFile);
    if (bf::is_regular_file(path)) {
        std::string fEnd = bf::extension(path.filename());
        boost::erase_all(fEnd, ".");
        numPartsFile = atoi(fEnd.c_str());
    }
    int numPartsUser = m_numPartitions->getValue();
    if (numPartsFile < 1) {
        numPartsFile = 1;
        finalNumberOfParts = 1;
        if (rank() == 0)
            sendInfo("Could not determine number of partitions -> using single partition");
    } else {
        finalNumberOfParts = std::min(numPartsUser, numPartsFile);
        if (rank() == 0)
            sendInfo("Using %i partitions", finalNumberOfParts);
        partsPath = path.string();
    }
    setPartitions(finalNumberOfParts);

    //set timesteps
    dataFileList.clear();
    if (hasDataFile) {
        numDataFiles = 0;
        bf::path dataFilePath(dataFileName);
        bf::path dataDir(dataFilePath.parent_path());
        for (bf::directory_iterator it2(dataDir); it2 != bf::directory_iterator(); ++it2) { //all files in dir
            if ((bf::extension(it2->path().filename()) == ".nc") &&
                (strncmp(dataFilePath.filename().c_str(), it2->path().filename().c_str(), 15) == 0)) {
                dataFileList.push_back(it2->path().string());
                numDataFiles++;
            }
        }
        setTimesteps(numDataFiles);
        sendInfo("Set Timesteps from DataFiles to %d", numDataFiles);
    } else {
        setTimesteps(0);
    }


    gridList.clear();
    gridList.resize(finalNumberOfParts);
    numCellsB.clear();
    numCellsB.resize(finalNumberOfParts, 0);
    numCornB.clear();
    numCornB.resize(finalNumberOfParts, 0);
    blockIdx.clear();
    blockIdx.resize(numPartsFile);
    numGhosts.clear();
    numGhosts.resize(finalNumberOfParts, 0);
    isGhost.clear();
    isGhost.resize(finalNumberOfParts, std::vector<int>(1, 0));
    numCells = 0;
    numLevels = 0;
    partList.clear();
    coc.clear();
    eoc.clear();
    voc.clear();
    cov.clear();
    xCoords.clear();
    yCoords.clear();
    zCoords.clear();

    return true;
}

// SET VARIABLE LIST
// fill the drop-down-menu in the module parameters based on the variables available from the grid or data file
bool ReadMPAS::setVariableList(const NcmpiFile &filename, bool setCOC)
{
    std::multimap<std::string, NcmpiVar> varNameListData = filename.getVars();
    char *newEntry = new char[50];
    std::vector<std::string> AxisVarChoices{"NONE"}, Axis1dChoices{"NONE"};
    size_t num3dVars = 0, num2dVars = 0;

    for (auto elem: varNameListData) {
        const NcmpiVar &var = filename.getVar(elem.first.c_str()); //get_var(i);
        strcpy(newEntry, elem.first.c_str()); //var->name());
        if (var.getDimCount() == 1) { //grid axis
            Axis1dChoices.push_back(newEntry);
            num2dVars++;
        } else {
            AxisVarChoices.push_back(newEntry);
            num3dVars++;
        }
    }
    if (setCOC)
        setParameterChoices(m_cellsOnCell, AxisVarChoices);

    delete[] newEntry;
    if (strcmp(m_varDim->getValue().c_str(), varDimList[1].c_str()) == 0) {
        for (int i = 0; i < NUMPARAMS; i++)
            setParameterChoices(m_variables[i], AxisVarChoices);
    } else if (strcmp(m_varDim->getValue().c_str(), varDimList[0].c_str()) == 0) {
        for (int i = 0; i < NUMPARAMS; i++)
            setParameterChoices(m_variables[i], Axis1dChoices);
    }
    return true;
}


//VALIDATE FILE
//check if file is netCDF file and set variables
bool ReadMPAS::validateFile(std::string fullFileName, std::string &redFileName, FileType type)
{
    std::string typeString = (type == grid_type ? "Grid" : (type == data_type ? "Data " : "zGrid"));
    redFileName = "";
    if (!fullFileName.empty()) {
        bf::path dPath(fullFileName);
        if (bf::is_regular_file(dPath)) {
            if (bf::extension(dPath.filename()) == ".nc") {
                redFileName = dPath.string();
                if (!(type == zgrid_type)) {
                    NcmpiFile newFile(comm(), redFileName, NcmpiFile::read);
                    setVariableList(newFile, type == grid_type);
                }


                if (rank() == 0)
                    sendInfo("Loading %s file %s", typeString.c_str(), redFileName.c_str());
                return true;
            }
        }
    }
    //if this failed: use grid file instead
    if (type == data_type) {
        NcmpiFile newFile(comm(), firstFileName, NcmpiFile::read);
        setVariableList(newFile, true);
    }
    if (rank() == 0)
        sendInfo("%s file not found -> using grid file instead", typeString.c_str());

    return false;
}

//EXAMINE
// TODO: remove duplicate code and collect all into functions
// react to changes of module parameters and set them; validate files
bool ReadMPAS::examine(const vistle::Parameter *param)
{
    if (!param || param == m_gridFile || param == m_varDim || param == m_dataFile) {
        if (!validateFile(m_gridFile->getValue(), firstFileName, grid_type))
            return false;
        hasDataFile = validateFile(m_dataFile->getValue(), dataFileName, data_type);
        hasZData = validateFile(m_zGridFile->getValue(), zGridFileName, zgrid_type);
    }
    return true;
}

//EMPTY VALUE
//check if module parameter choice is valid
bool ReadMPAS::emptyValue(vistle::StringParameter *ch) const
{
    std::string name = "";
    name = ch->getValue();
    return ((name == "") || (name == "NONE"));
}

//DIMENSION EXISTS
//check if a given dimension exists in the NC-files
bool ReadMPAS::dimensionExists(std::string findName, const NcmpiFile &filename)
{
    const NcmpiDim &findDim = filename.getDim(findName.c_str());
    return !(findDim.isNull());
}
//VARIABLE EXISTS
//check if a given variable exists in the NC-files
bool ReadMPAS::variableExists(std::string findName, const NcmpiFile &filename)
{
    const NcmpiVar &findVar = filename.getVar(findName.c_str());
    return !(findVar.isNull());
}

//ADD CELL
//compute vertices of a cell and add the cells to the cell list
bool ReadMPAS::addCell(Index elem, Index *el, Index *cl, long vPerC, long numVert, long izVert, Index &idx2,
                       const std::vector<int> &vocList)
{
    Index elemRow = elem * vPerC; //vPerC is set to MAX_EDGES if vocList is reducedVOC

    for (Index d = 0; d < eoc[elem]; ++d) { // add vertices on surface (crosssectional face)
        cl[idx2++] = ((vocList)[elemRow + d] - 1) + izVert;
    }
    cl[idx2++] = (vocList)[elemRow] - 1 + izVert;

    for (Index d = 0; d < eoc[elem]; ++d) { // add coating vertices for all coating sides
        cl[idx2++] = ((vocList)[elemRow + d] - 1) + izVert;
        cl[idx2++] = ((vocList)[elemRow + d] - 1) + numVert + izVert;
        if (d == (eoc[elem] - 1)) {
            cl[idx2++] = ((vocList)[elemRow] - 1) + numVert + izVert;
            cl[idx2++] = ((vocList)[elemRow] - 1) + izVert;
        } else {
            cl[idx2++] = ((vocList)[elemRow + d + 1] - 1) + numVert + izVert;
            cl[idx2++] = ((vocList)[elemRow + d + 1] - 1) + izVert;
        }
        cl[idx2++] = ((vocList)[elemRow + d] - 1) + izVert;
    }

    for (Index d = 0; d < eoc[elem]; ++d) { //add caping (bottom crossectional face)
        cl[idx2++] = ((vocList)[elemRow + d] - 1) + numVert + izVert;
    }
    cl[idx2++] = (vocList)[elemRow] - 1 + numVert + izVert;
    return true;
}

// GET DATA
// read 2D or 3D data from data or grid file into a vector
bool ReadMPAS::getData(const NcmpiFile &filename, std::vector<float> *dataValues, const MPI_Offset &nLevels, const Index dataIdx)
{
    const NcmpiVar &varData = filename.getVar(m_variables[dataIdx]->getValue().c_str());
    std::vector<MPI_Offset> numElem, startElem;
    for (auto elem: varData.getDims()) {
        if (strcmp(elem.getName().c_str(), "nCells") == 0) {
            numElem.push_back(elem.getSize());
        } else if (strcmp(elem.getName().c_str(), "nVertLevels") == 0) {
            numElem.push_back(std::min(elem.getSize(), nLevels - 1));
        } else {
            numElem.push_back(1);
        }
        startElem.push_back(0);
    }

    varData.getVar_all(startElem, numElem, dataValues->data());
    return true;
}

//READ
//start all the reading
bool ReadMPAS::read(Reader::Token &token, int timestep, int block)
{
    assert(gridList.size() == numPartitions());
    if (!gridList[block]) { //if grid has not been created -> do it

        NcmpiFile ncFirstFile(comm(), firstFileName, NcmpiFile::read);
        assert(dimensionExists("nCells", ncFirstFile));
        // first of all: validate that all neccesary dimensions and variables exist
        if (!dimensionExists("nCells", ncFirstFile) || !dimensionExists("nVertices", ncFirstFile) ||
            !dimensionExists("maxEdges", ncFirstFile)) {
            sendError("Missing dimension info -> quit");
            return false;
        }
        if (!variableExists("xVertex", ncFirstFile) || !variableExists("verticesOnCell", ncFirstFile) ||
            !variableExists("nEdgesOnCell", ncFirstFile)) {
            sendError("Missing variable info -> quit");
            return false;
        }

        const NcmpiDim &dimCells = ncFirstFile.getDim("nCells");
        const NcmpiDim &dimVert = ncFirstFile.getDim("nVertices");
        const NcmpiDim &dimVPerC = ncFirstFile.getDim("maxEdges");

        const NcmpiVar &xC = ncFirstFile.getVar("xVertex");
        const NcmpiVar &yC = ncFirstFile.getVar("yVertex");
        const NcmpiVar &zC = ncFirstFile.getVar("zVertex");
        const NcmpiVar &verticesPerCell = ncFirstFile.getVar("verticesOnCell");
        const NcmpiVar &nEdgesOnCell = ncFirstFile.getVar("nEdgesOnCell");

        const MPI_Offset &vPerC = dimVPerC.getSize(); // vertices on each cell
        if (numCells == 0)
            numCells = dimCells.getSize(); //number of cells in 2D
        const MPI_Offset &numVert = dimVert.getSize();
        std::vector<size_t> numVOC;

        // set eoc (number of Edges On each Cells), voc (Vertex index On each Cell)
        if (eoc.size() < 1) {
            eoc.resize(numCells);
            nEdgesOnCell.getVar_all(eoc.data());
        }
        if (voc.size() < 1) {
            voc.resize(numCells * vPerC);
            verticesPerCell.getVar_all(voc.data());
        }

        size_t numLevelsUser = m_numLevels->getValue();
        size_t numMaxLevels = 1;
        //verify that dimensions in grid file and data file are matching
        if (hasDataFile) {
            NcmpiFile ncDataFile(comm(), /*dataFileName.c_str()*/ dataFileList.at(0), NcmpiFile::read);
            // const NcmpiDim &dimCellsData = ncDataFile.getDim("nCells");
            // numCells = dimCellsData.getSize();
            if (dimensionExists("nVertLevels", ncDataFile)) {
                const NcmpiDim &dimLevel = ncDataFile.getDim("nVertLevels");
                numMaxLevels = dimLevel.getSize();
            } else {
                hasDataFile = false;
                if (block == 0)
                    sendInfo("No vertical dimension found -> number of levels set to one");
            }
        }

        if (numLevels == 0)
            numLevels = std::min(numMaxLevels, numLevelsUser) + 1;
        if (numLevels < 2)
            return false;

        float dH = 0.001;
        size_t numPartsUser = m_numPartitions->getValue();

        if (numPartsFile == 1) {
            sendInfo("please use a grid partition file");
            return false;
        } else {
            if (block == 0)
                sendInfo("Reading started...");

            // PARTITIONING: read partitions file and redistribute partition to number of user-defined partitions
            size_t FileBlocksPerUserBlocks = numPartsFile / numPartsUser;
            size_t remParts = numPartsFile - (numPartsUser * FileBlocksPerUserBlocks);

            if (partList.size() < 1) {
                mtxPartList.lock();
                FILE *partsFile = fopen(partsPath.c_str(), "r");

                if (partsFile == nullptr) {
                    if (block == 0)
                        sendInfo("Could not read parts file");
                    return false;
                }

                for (Index i = 0; i < numPartsUser; ++i) {
                    std::fill(blockIdx.begin() + (i * FileBlocksPerUserBlocks),
                              blockIdx.begin() + ((i + 1) * FileBlocksPerUserBlocks), i);
                }
                if (remParts != 0) {
                    for (Index i = 0; i < remParts; ++i) {
                        std::fill(blockIdx.begin() + ((numPartsUser * FileBlocksPerUserBlocks) + i),
                                  blockIdx.begin() + ((numPartsUser * FileBlocksPerUserBlocks) + (i + 1)), i);
                    }
                }

                char buffer[10];
                Index x = 0, xBlockIdx = 0, idxp = 0;
                while ((fgets(buffer, sizeof(buffer), partsFile) != NULL) && (idxp < MAX_VAL)) {
                    sscanf(buffer, "%u", &x);
                    xBlockIdx = blockIdx[x];
                    numCellsB[xBlockIdx]++;
                    numCornB[xBlockIdx] = numCornB[xBlockIdx] + eoc[idxp]; //TODO: std instead: more effiecient?
                    partList.push_back(xBlockIdx);
                    ++idxp;
                }
                blockIdx.clear();
                if (partsFile) {
                    fclose(partsFile);
                    partsFile = nullptr;
                }
                mtxPartList.unlock();

                assert(partList.size() == numCells);
            }
            //READ VERTEX COORDINATES given for base level (a unit sphere)
            if (xCoords.size() < 1 || yCoords.size() < 1 || zCoords.size() < 1) {
                xCoords.resize(numVert);
                yCoords.resize(numVert);
                zCoords.resize(numVert);
                std::vector<MPI_Offset> start = {0};
                std::vector<MPI_Offset> stop = {numVert};
                xC.getVar_all(start, stop, xCoords.data());
                yC.getVar_all(start, stop, yCoords.data());
                zC.getVar_all(start, stop, zCoords.data());
            }

            // set coc (index of neighboring cells on each cell) to determine ghost cells
            if (!emptyValue(m_cellsOnCell) && coc.size() < 1) {
                NcmpiVar cellsOnCell = ncFirstFile.getVar(m_cellsOnCell->getValue().c_str());
                coc.resize(numCells * vPerC);
                cellsOnCell.getVar_all(coc.data());
                ghosts = true;
            }
            size_t numCornGhost = 0;
            std::vector<int> reducedVOC(numCells * MAX_EDGES, -1);
            Index idx = 0, neighborIdx = 0;
            bool isNew = false;
            std::vector<int> vocIdxUsedTmp(numCells * MAX_EDGES, -1);
            if (isGhost[block].size() <= 1)
                isGhost[block].resize(numCells, 0);

            //DETERMINE VERTICES used for this partition
            for (Index i = 0; i < numCells; ++i) {
                if (partList[i] == block) {
                    for (Index d = 0; d < eoc[i]; ++d) {
                        if (vocIdxUsedTmp[voc[i * vPerC + d] - 1] < 0) {
                            vocIdxUsedTmp[voc[i * vPerC + d] - 1] = idx++;
                            reducedVOC[i * MAX_EDGES + d] =
                                idx; // is set to idx + 1 before it is increased -> idx+1-1 =idx
                        } else {
                            reducedVOC[i * MAX_EDGES + d] = vocIdxUsedTmp[voc[i * vPerC + d] - 1] + 1;
                        }

                        if (ghosts) {
                            neighborIdx = coc[i * vPerC + d] - 1;
                            // if (neighborIdx < 0 || neighborIdx >= numCells)
                            //   continue;
                            if (partList[neighborIdx] != block) {
                                isNew = false;
                                for (Index dn = 0; dn < eoc[neighborIdx]; ++dn) {
                                    if (vocIdxUsedTmp[voc[neighborIdx * vPerC + dn] - 1] < 0) {
                                        vocIdxUsedTmp[voc[neighborIdx * vPerC + dn] - 1] = idx++;
                                        reducedVOC[neighborIdx * MAX_EDGES + dn] = idx;
                                        isNew = true;
                                    } else {
                                        reducedVOC[neighborIdx * MAX_EDGES + dn] =
                                            vocIdxUsedTmp[voc[neighborIdx * vPerC + dn] - 1] + 1;
                                    }
                                }

                                if (isNew) {
                                    isGhost[block][neighborIdx] = 1;
                                    numGhosts[block]++;
                                    numCornGhost = numCornGhost + eoc[neighborIdx];
                                }
                            }
                        }
                    }
                }
            }
            vocIdxUsedTmp.clear();

            size_t numVertB = idx;
            size_t numCornPlusOne = numCornB[block] + numCellsB[block] + numCornGhost + numGhosts[block];

            //ASSEMBLE GRID
            UnstructuredGrid::ptr p(new UnstructuredGrid(
                (numCellsB[block] + numGhosts[block]) * (numLevels - 1),
                ((numLevels - 1) * numCornPlusOne * 2 + (numCornB[block] + numCornGhost) * (numLevels - 1) * 5),
                numVertB * numLevels));
            Index *cl = p->cl().data();
            Index *el = p->el().data();
            auto tl = p->tl().data();
            Scalar *ptrOnX = p->x().data();
            Scalar *ptrOnY = p->y().data();
            Scalar *ptrOnZ = p->z().data();
            int iv = 0, iVOC = 0; // might be negative!
            float radius = 1.;

            if (block == 0)
                sendInfo("numLevels is %d", numLevels);

            // SET GRID COORDINATES:
            // if zGrid is given: calculate level height from it
            // o.w. use constant offsets between levels
            Index idx2 = 0, currentElem = 0, izVert = 0;

            if (hasZData) {
                NcmpiVar cellsOnVertex = ncFirstFile.getVar("cellsOnVertex");
                cov.resize(numVert * MAX_VERT);
                cellsOnVertex.getVar_all(cov.data());

                std::vector<MPI_Offset> startZ{0, 0};
                std::vector<MPI_Offset> stopZ{numCells, numLevels};
                NcmpiFile ncZGridFile(comm(), zGridFileName.c_str(), NcmpiFile::read);
                const NcmpiVar &zGridVar = ncZGridFile.getVar("zgrid");
                std::vector<float> zGrid(numCells * numLevels, 0.);
                zGridVar.getVar_all(startZ, stopZ, zGrid.data());

                int i_v1 = 0, i_v2 = 0, i_v3 = 0;
                for (Index iz = 0; iz < numLevels; ++iz) {
                    izVert = numVertB * iz;
                    for (Index i = 0; i < numCells; ++i) {
                        //radius = zGrid[i+numCells*iz]+MSL;
                        if ((partList[i] == block) || (ghosts && (isGhost[block][i] > 0))) {
                            for (Index d = 0; d < eoc[i]; ++d) {
                                iv = reducedVOC[i * MAX_EDGES + d] - 1;
                                if (iv >= 0) {
                                    iVOC = voc[i * vPerC + d] - 1; //current vertex index
                                    i_v1 = cov[iVOC * MAX_VERT + 0]; //cell index
                                    i_v2 = cov[iVOC * MAX_VERT + 1];
                                    i_v3 = cov[iVOC * MAX_VERT + 2];
                                    radius = RAD_SCALE * (1. / 3.) *
                                                 (zGrid[(numLevels)*i_v1 + iz] + zGrid[(numLevels)*i_v2 + iz] +
                                                  zGrid[(numLevels)*i_v3 + iz]) +
                                             MSL; //compute vertex z from average of neighbouring cells
                                    ptrOnX[izVert + iv] = radius * xCoords[iVOC];
                                    ptrOnY[izVert + iv] = radius * yCoords[iVOC];
                                    ptrOnZ[izVert + iv] = radius * zCoords[iVOC];
                                }
                            }
                            if (iz < numLevels - 1) {
                                el[currentElem] = idx2;
                                addCell(i, el, cl, MAX_EDGES, numVertB, izVert, idx2, reducedVOC);
                                tl[currentElem++] = isGhost[block][i] > 0
                                                        ? (UnstructuredGrid::CPOLYHEDRON | UnstructuredGrid::GHOST_BIT)
                                                        : UnstructuredGrid::CPOLYHEDRON;
                            }
                        }
                    }
                }
            } else {
                for (Index iz = 0; iz < numLevels; ++iz) {
                    izVert = numVertB * iz;
                    radius = dH * iz + 1.;
                    for (Index i = 0; i < numCells; ++i) {
                        if ((partList[i] == block) || (ghosts && (isGhost[block][i] > 0))) {
                            for (Index d = 0; d < eoc[i]; ++d) {
                                iv = reducedVOC[i * MAX_EDGES + d] - 1;
                                if (iv >= 0) {
                                    iVOC = voc[i * vPerC + d] - 1;
                                    ptrOnX[izVert + iv] = radius * xCoords[iVOC];
                                    ptrOnY[izVert + iv] = radius * yCoords[iVOC];
                                    ptrOnZ[izVert + iv] = radius * zCoords[iVOC];
                                }
                            }
                            if (iz < numLevels - 1) {
                                el[currentElem] = idx2;
                                addCell(i, el, cl, MAX_EDGES, numVertB, izVert, idx2, reducedVOC);
                                tl[currentElem++] = isGhost[block][i] > 0
                                                        ? (UnstructuredGrid::CPOLYHEDRON | UnstructuredGrid::GHOST_BIT)
                                                        : UnstructuredGrid::CPOLYHEDRON;
                            }
                        }
                    }
                }
            }

            el[currentElem] = idx2;
            p->setBlock(block);
            p->setTimestep(-1);
            p->setNumBlocks(numPartitions());
            p->updateInternals();
            gridList[block] = p;
        }
    }

    if (timestep < 0) {
        token.addObject("grid_out", gridList[block]);
        return true;
    } else {
        // Read data

        NcmpiVar varData;
        //Index dataIdx = 0;
        for (Index dataIdx = 0; dataIdx < NUMPARAMS; ++dataIdx) {
            if (!emptyValue(m_variables[dataIdx])) {
                Vec<Scalar>::ptr dataObj(new Vec<Scalar>((numCellsB[block] + numGhosts[block]) * (numLevels - 1)));
                Scalar *ptrOnScalarData = dataObj->x().data();

                std::vector<float> dataValues(numCells * (numLevels - 1), 0.);
                if (hasDataFile) {
                    NcmpiFile ncDataFile(comm(), dataFileList.at(timestep).c_str(), NcmpiFile::read);
                    getData(ncDataFile, &dataValues, numLevels, dataIdx);
                } else {
                    NcmpiFile ncFirstFile2(comm(), firstFileName, NcmpiFile::read);
                    getData(ncFirstFile2, &dataValues, numLevels, dataIdx);
                }

                assert(partList.size() == numCells);

                Index currentElem = 0;
                for (Index iz = 0; iz < numLevels - 1; ++iz) {
                    for (Index i = 0; i < numCells; ++i) {
                        if ((partList[i] == block) || (ghosts && (isGhost[block][i] > 0))) {
                            ptrOnScalarData[currentElem++] = dataValues[iz + i * (numLevels - 1)]; //numMaxLevels
                        }
                    }
                }

                dataObj->setGrid(gridList[block]);
                dataObj->setMapping(DataBase::Element);
                dataObj->setBlock(block);
                std::string pVar = m_variables[dataIdx]->getValue();
                dataObj->addAttribute("_species", pVar);
                dataObj->updateInternals();
                dataObj->setTimestep(timestep);
                token.addObject(m_dataOut[dataIdx], dataObj);

                dataValues.clear();
            }
        }
    }
    return true;
}

bool ReadMPAS::finishRead()
{
    return true;
}

MODULE_MAIN(ReadMPAS)
