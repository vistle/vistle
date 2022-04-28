/* ------------------------------------------------------------------------------------
           READ MPAS

       - read netCDF files containing hexagonal mpas mesh, data and partition info
       - required files: "grid_file" and "part_file"
       - cf. https://www2.mmm.ucar.edu/projects/mpas/tutorial/Boulder2019/slides/04.mesh_structure.pdf

       - uses pnetcdf

       v1.0: L.Kern 2021

------------------------------------------------------------------------------------ */
#include "ReadMPAS.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <vistle/core/polygons.h>
#include <vistle/core/triangles.h>
#include <vistle/core/unstr.h>
#include <vistle/util/enum.h>
#include <fstream>

namespace bf = boost::filesystem;
using namespace PnetCDF;
using namespace vistle;

#define MAX_VAL 100000000
#define MAX_EDGES 6 // maximal edges on cell
#define MSL 6371229.0 //sphere radius on mean sea level (earth radius)
#define MAX_VERT 3 // vertex degree

DEFINE_ENUM_WITH_STRING_CONVERSIONS(CellMode, (Voronoi)(Delaunay)(DelaunayProjected))

// CONSTRUCTOR
ReadMPAS::ReadMPAS(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    m_gridFile = addStringParameter("grid_file", "File containing the grid", "", Parameter::ExistingFilename);
    m_dataFile = addStringParameter("data_file", "File containing data", "", Parameter::ExistingFilename);
    m_zGridFile = addStringParameter("zGrid_file_dir",
                                     "File containing the vertical coordinates (elevation of cell from mean sea level",
                                     "", Parameter::ExistingFilename);
    for (auto &p: {m_gridFile, m_dataFile, m_zGridFile}) {
        setParameterFilters(p, "NetCDF Files (*.nc)/All Files (*)");
    }
    m_partFile =
        addStringParameter("part_file", "File containing the grid partitions", "", Parameter::ExistingFilename);
    setParameterFilters(m_partFile, "Partitioning Files (*.part.*)/All Files (*)");

    m_numPartitions = addIntParameter("numParts", "Number of partitions per rank", 1);
    m_numLevels = addIntParameter("numLevels", "Number of vertical cell layers to read (0: only 2D base level)", 0);
    setParameterMinimum<Integer>(m_numLevels, 0);
    m_altitudeScale = addFloatParameter("altitudeScale", "value to scale the grid altitude (zGrid)", 20.);

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

    CellMode cm = m_voronoiCells ? Voronoi : m_projectDown ? DelaunayProjected : Delaunay;
    m_cellMode = addIntParameter("cell_mode",
                                 "grid (based on Voronoi cells, Delaunay trianguelation, or Delaunay projected down)",
                                 cm, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_cellMode, CellMode);

    //setParallelizationMode(ParallelizeBlocks);
    setParallelizationMode(Serial); // for MPI usage in PnetCDF
    //setReducePolicy(message::ReducePolicy::ReducePolicy::OverAll);

    observeParameter(m_gridFile);
    observeParameter(m_zGridFile);
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
    int numPartsUser = m_numPartitions->getValue() * this->size();
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
        unsigned nIgnored = 0;
        bf::path dataFilePath(dataFileName);
        bf::path dataDir(dataFilePath.parent_path());
        for (bf::directory_iterator it2(dataDir); it2 != bf::directory_iterator(); ++it2) { //all files in dir
            if ((bf::extension(it2->path().filename()) == ".nc") &&
                (strncmp(dataFilePath.filename().c_str(), it2->path().filename().c_str(), 15) == 0)) {
                auto fn = it2->path().string();
                try {
                    NcmpiFile newFile(comm(), fn, NcmpiFile::read);
                    if (!dimensionExists("nCells", newFile)) {
                        std::cerr << "ReadMpas: ignoring " << fn << ", no nCells dimension" << std::endl;
                        ++nIgnored;
                        continue;
                    }
                    const NcmpiDim dimCells = newFile.getDim("nCells");
                    if (dimCells.getSize() != numGridCells) {
                        std::cerr << "ReadMpas: ignoring " << fn << ", expected nCells=" << numGridCells << ", but got "
                                  << dimCells.getSize() << std::endl;
                        ++nIgnored;
                        continue;
                    }

                    dataFileList.push_back(fn);
                } catch (std::exception &ex) {
                    std::cerr << "ReadMpas: ignoring " << fn << ", exception: " << ex.what() << std::endl;
                    ++nIgnored;
                    continue;
                }
            }
        }
        setTimesteps(dataFileList.size());
        if (rank() == 0)
            sendInfo("Set Timesteps from DataFiles to %u, ignored %u candidates", (unsigned)dataFileList.size(),
                     nIgnored);
    } else {
        setTimesteps(0);
    }

    gridList.clear();
    gridList.resize(finalNumberOfParts);
    numCellsB.clear();
    numCellsB.resize(finalNumberOfParts, 0);
    numCornB.clear();
    numCornB.resize(finalNumberOfParts, 0);
    numTrianglesB.clear();
    numTrianglesB.resize(finalNumberOfParts, 0);
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
    idxCellsInBlock.clear();
    idxCellsInBlock.resize(finalNumberOfParts, std::vector<Index>(0, 0));

    switch (m_cellMode->getValue()) {
    case Voronoi:
        m_voronoiCells = true;
        m_projectDown = false;
        break;
    case Delaunay:
        m_voronoiCells = false;
        m_projectDown = false;
        break;
    case DelaunayProjected:
        m_voronoiCells = false;
        m_projectDown = true;
        break;
    default:
        assert("invalid CellMode" == nullptr);
    }

    ghosts = false;

    return true;
}

// SET VARIABLE LIST
// fill the drop-down-menu in the module parameters based on the variables available from the grid or data file
bool ReadMPAS::setVariableList(const NcmpiFile &filename, FileType ft, bool setCOC)
{
    std::multimap<std::string, NcmpiVar> varNameListData = filename.getVars();
    std::vector<std::string> AxisVarChoices{"NONE"}, Axis1dChoices{"NONE"};

    for (auto elem: varNameListData) {
        const NcmpiVar var = filename.getVar(elem.first); //get_var(i);
        if (var.getDimCount() == 1) { //grid axis
            Axis1dChoices.push_back(elem.first);
            m_2dChoices[elem.first] = ft;
        } else {
            AxisVarChoices.push_back(elem.first);
            m_3dChoices[elem.first] = ft;
        }
    }
    if (setCOC)
        setParameterChoices(m_cellsOnCell, AxisVarChoices);

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
                try {
                    NcmpiFile newFile(comm(), redFileName, NcmpiFile::read);
                    if (!dimensionExists("nCells", newFile)) {
                        if (rank() == 0)
                            sendInfo("File %s does not have dimension nCells, no MPAS file", redFileName.c_str());
                        return false;
                    }
                    const NcmpiDim dimCells = newFile.getDim("nCells");
                    if (type == grid_type) {
                        numGridCells = dimCells.getSize();
                    } else {
                        Index nCells = dimCells.getSize();
                        if (numGridCells != nCells) {
                            if (rank() == 0)
                                sendInfo("nCells=%lu from %s does not match nCells=%lu from grid file",
                                         (unsigned long)nCells, redFileName.c_str(), (unsigned long)numGridCells);
                            return false;
                        }
                    }
                    setVariableList(newFile, type, type == grid_type);
                } catch (std::exception &ex) {
                    if (rank() == 0)
                        sendInfo("Exception during Pnetcdf call on %s: %s", redFileName.c_str(), ex.what());
                    return false;
                }

                if (rank() == 0)
                    sendInfo("To load: %s file %s", typeString.c_str(), redFileName.c_str());
                return true;
            }
        }
    }
    //if this failed: use grid file instead
    if (type == data_type) {
        NcmpiFile newFile(comm(), firstFileName, NcmpiFile::read);
        setVariableList(newFile, grid_type, true);
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
    if (!param || param == m_gridFile || param == m_dataFile || param == m_zGridFile) {
        m_2dChoices.clear();
        m_3dChoices.clear();

        if (!validateFile(m_gridFile->getValue(), firstFileName, grid_type))
            return false;
        hasDataFile = validateFile(m_dataFile->getValue(), dataFileName, data_type);
        hasZData = validateFile(m_zGridFile->getValue(), zGridFileName, zgrid_type);
    }

    std::vector<std::string> choices{"(NONE)"};
    if (m_varDim->getValue() == varDimList[0]) {
        for (const auto &var: m_2dChoices)
            choices.push_back(var.first);
    } else {
        for (const auto &var: m_3dChoices)
            choices.push_back(var.first);
    }
    for (int i = 0; i < NUMPARAMS; i++)
        setParameterChoices(m_variables[i], choices);
    return true;
}

//EMPTY VALUE
//check if module parameter choice is valid
bool ReadMPAS::emptyValue(vistle::StringParameter *ch) const
{
    std::string name = "";
    name = ch->getValue();
    return ((name == "") || (name == "NONE") || (name == "(NONE)"));
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
bool ReadMPAS::addCell(Index elem, bool ghost, Index &curElem, Index *el, Byte *tl, Index *cl, long vPerC, long numVert,
                       long izVert, Index &idx2, const std::vector<Index> &vocList)
{
    Index elemRow = elem * vPerC; //vPerC is set to MAX_EDGES if vocList is reducedVOC
    el[curElem] = idx2;

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
    tl[curElem] = ghost ? (UnstructuredGrid::POLYHEDRON | UnstructuredGrid::GHOST_BIT) : UnstructuredGrid::POLYHEDRON;
    ++curElem;
    return true;
}

bool ReadMPAS::addPoly(Index elem, Index &curElem, Index *el, Index *cl, long vPerC, long numVert, long izVert,
                       Index &idx2, const std::vector<Index> &vocList)
{
    el[curElem] = idx2;
    Index elemRow = elem * vPerC; //vPerC is set to MAX_EDGES if vocList is reducedVOC

    for (Index d = 0; d < eoc[elem]; ++d) { // add vertices on surface (crosssectional face)
        cl[idx2++] = ((vocList)[elemRow + d] - 1) + izVert;
    }
    cl[idx2++] = (vocList)[elemRow] - 1 + izVert;
    ++curElem;

    return true;
}

bool ReadMPAS::addWedge(bool ghost, Index &curElem, Index center, Index n1, Index n2, Index layer, Index nVertPerLayer,
                        Index *el, Byte *tl, Index *cl, Index &idx2)
{
    el[curElem] = idx2;
    Index off = layer * nVertPerLayer;
    cl[idx2++] = center + off;
    cl[idx2++] = n1 + off;
    cl[idx2++] = n2 + off;
    off += nVertPerLayer;
    cl[idx2++] = center + off;
    cl[idx2++] = n1 + off;
    cl[idx2++] = n2 + off;
    tl[curElem] = UnstructuredGrid::PRISM;
    if (ghost)
        tl[curElem] |= UnstructuredGrid::GHOST_BIT;
    ++curElem;

    return true;
}

bool ReadMPAS::addTri(Index &curElem, Index center, Index n1, Index n2, Index *cl, Index &idx2)
{
    cl[idx2++] = center;
    cl[idx2++] = n1;
    cl[idx2++] = n2;
    ++curElem;

    return true;
}

// GET DATA
// read 2D or 3D data from data or grid file into a vector
bool ReadMPAS::getData(const NcmpiFile &filename, std::vector<float> *dataValues, const MPI_Offset &nLevels,
                       const Index dataIdx)
{
    const NcmpiVar varData = filename.getVar(m_variables[dataIdx]->getValue());
    std::vector<MPI_Offset> numElem, startElem;
    for (auto elem: varData.getDims()) {
        if (elem.getName() == "nCells") {
            numElem.push_back(elem.getSize());
        } else if (elem.getName() == "nVertLevels") {
            numElem.push_back(std::min(elem.getSize(), nLevels));
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
    assert(gridList.size() == Index(numPartitions()));
    auto &idxCells = idxCellsInBlock[block];
    if (timestep >= 0) {
        assert(gridList[block]);
    }

    if (!gridList[block]) { //if grid has not been created -> do it
        assert(timestep == -1);

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

        const NcmpiDim dimCells = ncFirstFile.getDim("nCells");
        const NcmpiDim dimVert = ncFirstFile.getDim("nVertices");
        const NcmpiDim dimVPerC = ncFirstFile.getDim("maxEdges");

        const NcmpiVar xC = m_voronoiCells ? ncFirstFile.getVar("xVertex") : ncFirstFile.getVar("xCell");
        const NcmpiVar yC = m_voronoiCells ? ncFirstFile.getVar("yVertex") : ncFirstFile.getVar("yCell");
        const NcmpiVar zC = m_voronoiCells ? ncFirstFile.getVar("zVertex") : ncFirstFile.getVar("zCell");
        const NcmpiVar verticesPerCell = ncFirstFile.getVar("verticesOnCell");
        const NcmpiVar nEdgesOnCell = ncFirstFile.getVar("nEdgesOnCell");

        const MPI_Offset vPerC = dimVPerC.getSize(); // vertices on each cell
        if (numCells == 0)
            numCells = dimCells.getSize(); //number of cells in 2D
        const MPI_Offset numVert = dimVert.getSize();

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
            numLevels = std::min(numMaxLevels, numLevelsUser + 1);
        if (numLevels < 1)
            return false;

        float dH = 0.001;
        size_t numPartsUser = m_numPartitions->getValue() * size();

        if (numPartsFile == 1) {
            sendInfo("please use a grid partition file");
            return false;
        } else {
            if (block == 0)
                sendInfo("Reading started...");

            // PARTITIONING: read partitions file and redistribute partition to number of user-defined partitions
            size_t FileBlocksPerUserBlocks = (numPartsFile + numPartsUser - 1) / numPartsUser;

            std::unique_lock<std::mutex> guardPartList(mtxPartList);
            if (partList.size() < 1) {
                FILE *partsFile = fopen(partsPath.c_str(), "r");

                if (partsFile == nullptr) {
                    if (block == 0)
                        sendInfo("Could not read parts file %s", partsPath.c_str());
                    return false;
                }


                char buffer[10];
                Index xBlockIdx = 0, idxp = 0;
                partList.reserve(numCells);
                while ((fgets(buffer, sizeof(buffer), partsFile) != NULL) && (idxp < MAX_VAL)) {
                    unsigned x;
                    sscanf(buffer, "%u", &x);
                    xBlockIdx = x / FileBlocksPerUserBlocks;
                    numCellsB[xBlockIdx]++;
                    numCornB[xBlockIdx] += eoc[idxp]; //TODO: std instead: more efficient?
                    partList.push_back(xBlockIdx);
                    ++idxp;
                }
                if (partsFile) {
                    fclose(partsFile);
                    partsFile = nullptr;
                }

                assert(partList.size() == numCells);
            }

            //READ VERTEX COORDINATES given for base level (a unit sphere)
            if (xCoords.size() < 1 || yCoords.size() < 1 || zCoords.size() < 1) {
                std::vector<MPI_Offset> start = {0};
                std::vector<MPI_Offset> stop;
                if (m_voronoiCells) {
                    stop.push_back(numVert);
                    xCoords.resize(numVert);
                    yCoords.resize(numVert);
                    zCoords.resize(numVert);
                } else {
                    stop.push_back(numCells);
                    xCoords.resize(numCells);
                    yCoords.resize(numCells);
                    zCoords.resize(numCells);
                }
                xC.getVar_all(start, stop, xCoords.data());
                yC.getVar_all(start, stop, yCoords.data());
                zC.getVar_all(start, stop, zCoords.data());
            }

            // set coc (index of neighboring cells on each cell) to determine ghost cells
            if ((numLevels > 1 || (!m_voronoiCells && numLevels == 1)) && !emptyValue(m_cellsOnCell) &&
                coc.size() < 1) {
                NcmpiVar cellsOnCell = ncFirstFile.getVar(m_cellsOnCell->getValue().c_str());
                coc.resize(numCells * vPerC);
                cellsOnCell.getVar_all(coc.data());
                ghosts = numLevels > 1;
            }
            guardPartList.unlock();

            size_t numCornGhost = 0;
            size_t numGhosts = 0;
            std::vector<unsigned char> isGhost(numCells, 0);
            std::vector<Index> reducedVOC;
            Index idx = 0;
            std::vector<Index> reducedCenter;

            //DETERMINE VERTICES used for this partition
            idxCells.reserve(numCells / numPartsUser);
            if (m_voronoiCells) {
                std::vector<Index> vocIdxUsedTmp(numCells * MAX_EDGES, InvalidIndex);
                reducedVOC.resize(numCells * MAX_EDGES, InvalidIndex);
                for (Index i = 0; i < numCells; ++i) {
                    if (partList[i] != block)
                        continue;
                    idxCells.push_back(i);
                    for (Index d = 0; d < eoc[i]; ++d) {
                        if (vocIdxUsedTmp[voc[i * vPerC + d] - 1] == InvalidIndex) {
                            vocIdxUsedTmp[voc[i * vPerC + d] - 1] = idx++;
                            reducedVOC[i * MAX_EDGES + d] =
                                idx; // is set to idx + 1 before it is increased -> idx+1-1 =idx
                        } else {
                            reducedVOC[i * MAX_EDGES + d] = vocIdxUsedTmp[voc[i * vPerC + d] - 1] + 1;
                        }

                        if (ghosts) {
                            Index neighborIdx = coc[i * vPerC + d] - 1;
                            // if (neighborIdx < 0 || neighborIdx >= numCells)
                            //   continue;
                            if (partList[neighborIdx] != block) {
                                bool isNew = false;
                                for (Index dn = 0; dn < eoc[neighborIdx]; ++dn) {
                                    if (vocIdxUsedTmp[voc[neighborIdx * vPerC + dn] - 1] == InvalidIndex) {
                                        vocIdxUsedTmp[voc[neighborIdx * vPerC + dn] - 1] = idx++;
                                        reducedVOC[neighborIdx * MAX_EDGES + dn] = idx;
                                        isNew = true;
                                    } else {
                                        reducedVOC[neighborIdx * MAX_EDGES + dn] =
                                            vocIdxUsedTmp[voc[neighborIdx * vPerC + dn] - 1] + 1;
                                    }
                                }

                                if (isNew) {
                                    isGhost[neighborIdx] = 1;
                                    idxCells.push_back(neighborIdx);
                                    numGhosts++;
                                    numCornGhost = numCornGhost + eoc[neighborIdx];
                                }
                            }
                        }
                    }
                }
            } else {
                reducedCenter.resize(numCells, InvalidIndex);
                std::vector<Index> outstandingGhosts;
                auto addVertex = [this, &reducedCenter, &idx, &idxCells](Index i) {
                    if (reducedCenter[i] != InvalidIndex)
                        return;
                    idxCells.push_back(i);
                    reducedCenter[i] = idx;
                    ++idx;
                };
                // ghost triangles around cell-centers in this partition
                auto addOwnedGhostTriangles = [this, &addVertex, &block, &vPerC, &reducedCenter, &idxCells, &idx,
                                               &isGhost, &numGhosts](Index i) {
                    assert(partList[i] == block);
                    if (!ghosts)
                        return;
                    for (Index d = 0; d < eoc[i]; ++d) {
                        Index n1 = coc[i * vPerC + d] - 1;
                        Index n2 = coc[i * vPerC + (d + 1) % eoc[i]] - 1;
                        Index smallest = std::min(n1, n2);
                        if (i < smallest) {
                            // no ghost: triangle owned by this center vertex
                            continue;
                        }
                        if (partList[smallest] == block) {
                            // no ghost: triangle owned by another center vertex of this block
                            continue;
                        }
                        Index largest = std::max(n1, n2);
                        if (partList[largest] == block) {
                            if (i > largest) {
                                // ghost, but other center vertex of this block is responsible for it
                                continue;
                            }
                        }
                        isGhost[i] = 1;
                        numGhosts++;
                        if (partList[n1] != block)
                            addVertex(n1);
                        if (partList[n2] != block)
                            addVertex(n2);
                    }
                };
                // ghost triangles around cell-centers added for owned triangles with vertices/cell-centers from other partitions
                auto addBorrowedGhostTriangles = [this, &addVertex, &block, &vPerC, &reducedCenter, &idxCells, &isGhost,
                                                  &numGhosts](Index i) {
                    assert(partList[i] != block);
                    if (!ghosts)
                        return;
                    assert(isGhost[i]);
                    for (Index d = 0; d < eoc[i]; ++d) {
                        Index n1 = coc[i * vPerC + d] - 1;
                        Index n2 = coc[i * vPerC + (d + 1) % eoc[i]] - 1;
                        if (partList[n1] == block || partList[n2] == block) {
                            // non-borrowed cell-center is responsible for this triangle
                            continue;
                        }
                        if (isGhost[n1] && n1 < i) {
                            // ghost, but n1 is responsible
                            continue;
                        }
                        if (isGhost[n2] && n2 < i) {
                            // ghost, but n2 is responsible
                            continue;
                        }
                        numGhosts++;
                        addVertex(n1);
                        addVertex(n2);
                    }
                };
                // add base triangles (together with vertices borrowed from other blocks) and resulting cells to block, if their lowest numbered vertex is owned by block
                for (Index i = 0; i < numCells; ++i) {
                    if (partList[i] != block)
                        continue;
                    assert(reducedCenter[i] == InvalidIndex);
                    addVertex(i);
                    for (Index d = 0; d < eoc[i]; ++d) {
                        Index n1 = coc[i * vPerC + d] - 1;
                        Index n2 = coc[i * vPerC + (d + 1) % eoc[i]] - 1;
                        if (i < n1 && i < n2) {
                            // cell-center i owns this triangle
                            ++numTrianglesB[block];
                            if (partList[n1] != block) {
                                if (reducedCenter[n1] == InvalidIndex) {
                                    addVertex(n1);
                                    if (ghosts) {
                                        outstandingGhosts.push_back(n1);
                                        isGhost[n1] = 1;
                                    }
                                }
                            }
                            if (partList[n2] != block) {
                                if (reducedCenter[n2] == InvalidIndex) {
                                    addVertex(n2);
                                    if (ghosts) {
                                        outstandingGhosts.push_back(n2);
                                        isGhost[n2] = 1;
                                    }
                                }
                            }
                        }
                    }
                    addOwnedGhostTriangles(i);
                    assert(idx == idxCells.size());
                }
                std::cerr << "block " << block << ": "
                          << "owned ghosts=" << numGhosts;
                for (auto i: outstandingGhosts)
                    addBorrowedGhostTriangles(i);
                outstandingGhosts.clear();
                assert(idx == idxCells.size());
            }

            size_t numVertB = idx;
            size_t numCornPlusOne = numCornB[block] + numCellsB[block] + numCornGhost + numGhosts;

            //ASSEMBLE GRID
            if (block == 0)
                sendInfo("numLevels is %u", numLevels);

            Coords::ptr grid;
            float altScale = m_altitudeScale->getValue();
            assert(numLevels >= 1);

            unsigned numZLevels = (m_voronoiCells || m_projectDown) ? numLevels : numLevels + 1;
            std::vector<float> zGrid(Index(numCells) * numZLevels);
            if (hasZData) {
                if (m_voronoiCells) {
                    NcmpiVar cellsOnVertex = ncFirstFile.getVar("cellsOnVertex");
                    cov.resize(numVert * MAX_VERT);
                    cellsOnVertex.getVar_all(cov.data());
                }

                std::vector<MPI_Offset> startZ{0, 0};
                std::vector<MPI_Offset> stopZ{MPI_Offset(numCells), numZLevels};
                NcmpiFile ncZGridFile(comm(), zGridFileName.c_str(), NcmpiFile::read);
                const NcmpiVar zGridVar = ncZGridFile.getVar("zgrid");
                zGridVar.getVar_all(startZ, stopZ, zGrid.data());
                if (numLevels < numZLevels) {
                    // average z levels to Voronoi cell centers
                    float *z = zGrid.data();
                    for (Index i = 0; i < numCells; ++i) {
                        for (Index l = 0; l < numZLevels; ++l) {
                            if (l < numLevels)
                                *z = 0.5f * (*z + *(z + 1));
                            ++z;
                        }
                    }
                    assert(z == zGrid.data() + zGrid.size());
                }
            }

            if (m_voronoiCells) {
                Index *cl = nullptr;
                Index *el = nullptr;
                Byte *tl = nullptr;
                if (numLevels > 1) {
                    UnstructuredGrid::ptr p(new UnstructuredGrid(
                        (numCellsB[block] + numGhosts) * (numLevels - 1),
                        ((numLevels - 1) * numCornPlusOne * 2 + (numCornB[block] + numCornGhost) * (numLevels - 1) * 5),
                        numVertB * numLevels));
                    grid = p;
                    cl = p->cl().data();
                    el = p->el().data();
                    tl = p->tl().data();
                } else {
                    Polygons::ptr p(new Polygons(numCellsB[block], numCornPlusOne, numVertB));
                    grid = p;
                    cl = p->cl().data();
                    el = p->el().data();
                }
                Scalar *ptrOnX = grid->x().data();
                Scalar *ptrOnY = grid->y().data();
                Scalar *ptrOnZ = grid->z().data();

                // SET GRID COORDINATES:
                // if zGrid is given: calculate level height from it
                // o.w. use constant offsets between levels
                Index idx2 = 0, currentElem = 0;
                if (hasZData) {
                    for (Index iz = 0; iz < numLevels; ++iz) {
                        Index izVert = numVertB * iz;
                        for (Index k = 0; k < idxCells.size(); ++k) {
                            Index i = idxCells[k];
                            for (Index d = 0; d < eoc[i]; ++d) {
                                Index iv = reducedVOC[i * MAX_EDGES + d];
                                assert(iv != InvalidIndex);
                                assert(iv > 0);
                                --iv;
                                Index iVOC = voc[i * vPerC + d] - 1; //current vertex index
                                Index i_v1 = cov[iVOC * MAX_VERT + 0] - 1; //cell index
                                Index i_v2 = cov[iVOC * MAX_VERT + 1] - 1;
                                Index i_v3 = cov[iVOC * MAX_VERT + 2] - 1;
                                float radius = altScale * (1. / 3.) *
                                                   (zGrid[(numZLevels)*i_v1 + iz] + zGrid[(numZLevels)*i_v2 + iz] +
                                                    zGrid[(numZLevels)*i_v3 + iz]) +
                                               MSL; //compute vertex z from average of neighbouring cells
                                ptrOnX[izVert + iv] = radius * xCoords[iVOC];
                                ptrOnY[izVert + iv] = radius * yCoords[iVOC];
                                ptrOnZ[izVert + iv] = radius * zCoords[iVOC];
                            }
                            if (tl) {
                                if (iz < numLevels - 1) {
                                    addCell(i, isGhost[i] > 0, currentElem, el, tl, cl, MAX_EDGES, numVertB, izVert,
                                            idx2, reducedVOC);
                                }
                            } else {
                                addPoly(i, currentElem, el, cl, MAX_EDGES, numVertB, izVert, idx2, reducedVOC);
                            }
                        }
                    }
                } else {
                    for (Index iz = 0; iz < numLevels; ++iz) {
                        Index izVert = numVertB * iz;
                        float radius = altScale * dH * iz + 1.; // FIXME: MSL?
                        for (Index k = 0; k < idxCells.size(); ++k) {
                            Index i = idxCells[k];
                            for (Index d = 0; d < eoc[i]; ++d) {
                                Index iv = reducedVOC[i * MAX_EDGES + d];
                                assert(iv != InvalidIndex);
                                assert(iv > 0);
                                --iv;
                                Index iVOC = voc[i * vPerC + d] - 1;
                                ptrOnX[izVert + iv] = radius * xCoords[iVOC];
                                ptrOnY[izVert + iv] = radius * yCoords[iVOC];
                                ptrOnZ[izVert + iv] = radius * zCoords[iVOC];
                            }
                            if (tl) {
                                if (iz < numLevels - 1) {
                                    addCell(i, isGhost[i] > 0, currentElem, el, tl, cl, MAX_EDGES, numVertB, izVert,
                                            idx2, reducedVOC);
                                }
                            } else {
                                addPoly(i, currentElem, el, cl, MAX_EDGES, numVertB, izVert, idx2, reducedVOC);
                            }
                        }
                    }
                }

                // add sentinel
                el[currentElem] = idx2;
            } else {
                assert(idxCells.size() == numVertB);
                // build dual triangle/wedge grid
                Byte *tl = nullptr;
                Index *cl = nullptr;
                Index *el = nullptr;
                if (numLevels > 1) {
                    auto ntri = numTrianglesB[block];
                    ntri += numGhosts;
                    std::cerr << "block " << block << ": base triangles: " << ntri << "=" << numTrianglesB[block] << "+"
                              << numGhosts << std::endl;
                    UnstructuredGrid::ptr p(
                        new UnstructuredGrid(ntri * (numLevels - 1), ntri * 6 * (numLevels - 1), numVertB * numLevels));
                    grid = p;
                    tl = p->tl().data();
                    cl = p->cl().data();
                    el = p->el().data();
                } else {
                    Triangles::ptr p(new Triangles(numTrianglesB[block] * 3, numVertB));
                    grid = p;
                    cl = p->cl().data();
                }
                Scalar *ptrOnX = grid->x().data();
                Scalar *ptrOnY = grid->y().data();
                Scalar *ptrOnZ = grid->z().data();

                // SET GRID COORDINATES:
                // if zGrid is given: calculate level height from it
                // o.w. use constant offsets between levels
                for (Index k = 0; k < idxCells.size(); ++k) {
                    Index i = idxCells[k];
                    for (Index iz = 0; iz < numLevels; ++iz) {
                        Index izVert = numVertB * iz;
                        float radius = altScale * (hasZData ? zGrid[numZLevels * i + iz] : dH * iz) + MSL;
                        ptrOnX[izVert + k] = radius * xCoords[i];
                        ptrOnY[izVert + k] = radius * yCoords[i];
                        ptrOnZ[izVert + k] = radius * zCoords[i];
                    }
                }

                Index idx2 = 0, currentElem = 0;
                for (Index k = 0; k < idxCells.size(); ++k) {
                    Index i = idxCells[k];
                    bool haveGhost = ghosts && isGhost[i];
                    bool borrowed = partList[i] != block;
                    if (borrowed && !haveGhost)
                        continue;
                    for (Index d = 0; d < eoc[i]; ++d) {
                        Index n1 = coc[i * vPerC + d] - 1;
                        Index n2 = coc[i * vPerC + (d + 1) % eoc[i]] - 1;
                        assert(n1 != InvalidIndex);
                        assert(n2 != InvalidIndex);
                        bool ghost = false;
                        Index smallest = std::min(n1, n2);
                        if (borrowed) {
                            if (partList[n1] == block)
                                // other cell-center is responsible
                                continue;
                            if (partList[n2] == block)
                                // other cell-center is responsible
                                continue;
                            if (n1 < i && isGhost[n1])
                                // ghost, but owned by another borrowed cell-center
                                continue;
                            if (n2 < i && isGhost[n2])
                                // ghost, but owned by another borrowed cell-center
                                continue;
                            assert(tl);
                            ghost = true;
                        } else if (i < smallest) {
                            // no ghost, owned by this cell-center
                        } else if (haveGhost) {
                            assert(i > smallest);
                            if (partList[smallest] == block) {
                                // no ghost, owned by another cell-center belonging to this block
                                continue;
                            }
                            Index largest = std::max(n1, n2);
                            if (partList[largest] == block) {
                                if (i > largest) {
                                    // ghost, but another cell-center belonging to this block is responsible
                                    continue;
                                }
                            }
                            assert(tl);
                            ghost = true;
                        } else {
                            // no ghost, owned by another cell-center belonging to another block
                            continue;
                        }
                        Index rn1 = reducedCenter[n1];
                        assert(rn1 != InvalidIndex);
                        Index rn2 = reducedCenter[n2];
                        assert(rn2 != InvalidIndex);
                        if (tl) {
                            for (Index iz = 0; iz < numLevels - 1; ++iz) {
                                addWedge(ghost, currentElem, k, rn1, rn2, iz, numVertB, el, tl, cl, idx2);
                            }
                        } else {
                            assert(!ghost);
                            addTri(currentElem, k, rn1, rn2, cl, idx2);
                        }
                    }
                }
                // add sentinel
                if (el)
                    el[currentElem] = idx2;
            }

            if (grid) {
                grid->setBlock(block);
                grid->setTimestep(-1);
                grid->setNumBlocks(numPartitions());
                token.applyMeta(grid);
                gridList[block] = grid;
            }
        }
    }
    assert(gridList[block]);

    if (timestep < 0) {
        token.addObject("grid_out", gridList[block]);
    }

    // Read data
    NcmpiVar varData;
    unsigned nLevels = m_voronoiCells ? std::max(1u, numLevels - 1) : numLevels;
    for (Index dataIdx = 0; dataIdx < NUMPARAMS; ++dataIdx) {
        if (!emptyValue(m_variables[dataIdx])) {
            std::string pVar = m_variables[dataIdx]->getValue();
            Vec<Scalar>::ptr dataObj(new Vec<Scalar>(idxCells.size() * nLevels));
            Scalar *ptrOnScalarData = dataObj->x().data();

            auto ft = m_varDim->getValue() == varDimList[0] ? m_2dChoices[pVar] : m_3dChoices[pVar];

            std::vector<float> dataValues(Index(numCells) * nLevels, 0.);
            if (ft == data_type) {
                if (timestep < 0)
                    continue;
                NcmpiFile ncDataFile(comm(), dataFileList.at(timestep).c_str(), NcmpiFile::read);
                getData(ncDataFile, &dataValues, nLevels, dataIdx);
            } else if (ft == zgrid_type) {
                if (timestep >= 0)
                    continue;
                NcmpiFile ncFirstFile2(comm(), zGridFileName, NcmpiFile::read);
                getData(ncFirstFile2, &dataValues, nLevels, dataIdx);
            } else {
                if (timestep >= 0)
                    continue;
                NcmpiFile ncFirstFile2(comm(), firstFileName, NcmpiFile::read);
                getData(ncFirstFile2, &dataValues, nLevels, dataIdx);
            }
            Index currentElem = 0;
            for (Index iz = 0; iz < nLevels; ++iz) {
                for (Index k = 0; k < idxCells.size(); ++k) {
                    ptrOnScalarData[currentElem++] = dataValues[iz + idxCells[k] * nLevels];
                }
            }

            dataObj->setGrid(gridList[block]);
            if (m_voronoiCells)
                dataObj->setMapping(DataBase::Element);
            else
                dataObj->setMapping(DataBase::Vertex);
            dataObj->setBlock(block);
            dataObj->addAttribute("_species", pVar);
            dataObj->setTimestep(timestep);
            token.applyMeta(dataObj);
            token.addObject(m_dataOut[dataIdx], dataObj);

            dataValues.clear();
        }
    }
    return true;
}

bool ReadMPAS::finishRead()
{
    numCells = 0;
    numLevels = 0;

    gridList.clear();
    numCellsB.clear();
    numCornB.clear();
    numTrianglesB.clear();
    idxCellsInBlock.clear();
    partList.clear();

    voc.clear();
    coc.clear();
    cov.clear();
    eoc.clear();
    xCoords.clear();
    yCoords.clear();
    zCoords.clear();

    return true;
}

MODULE_MAIN(ReadMPAS)
