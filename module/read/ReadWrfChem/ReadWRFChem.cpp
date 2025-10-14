/**************************************************************************\
 **                                                                        **
 **                                                                        **
 ** Description: Read module for WRFChem data                                **
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


#include "ReadWRFChem.h"

#include <cstddef>
#include <vector>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/uniformgrid.h>

#include <vistle/util/filesystem.h>
/* #include <netcdfcpp.h> */

using namespace vistle;
namespace fs = vistle::filesystem;


ReadWRFChem::ReadWRFChem(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader(name, moduleID, comm)
{
    ncFirstFile = NULL;

    m_gridOut = createOutputPort("grid_out", "grid");
    m_filedir =
        addStringParameter("file_dir", "NC files directory", "/mnt/raid/home/hpcleker/Desktop/test_files/NC/test_time",
                           Parameter::ExistingDirectory);

    m_numPartitionsLat = addIntParameter("num_partitions_lat", "number of partitions in lateral", 1);
    m_numPartitionsVer = addIntParameter("num_partitions_ver", "number of partitions in vertical", 1);

    m_varDim = addStringParameter("var_dim", "Dimension of variables", "", Parameter::Choice);
    setParameterChoices(m_varDim, varDimList);

    m_trueHGT = addStringParameter("true_height", "Use real ground topology", "", Parameter::Choice);
    m_gridLat = addStringParameter("GridX", "grid Sout-North axis", "", Parameter::Choice);
    m_gridLon = addStringParameter("GridY", "grid East_West axis", "", Parameter::Choice);
    m_PH = addStringParameter("pert_gp", "perturbation geopotential", "", Parameter::Choice);
    m_PHB = addStringParameter("base_gp", "base-state geopotential", "", Parameter::Choice);
    m_gridZ = addStringParameter("GridZ", "grid Bottom-Top axis", "", Parameter::Choice);

    std::vector<std::string> varChoices;
    varChoices.push_back(Reader::InvalidChoice);

    for (Index i = 0; i < NUMPARAMS - 3; ++i) {
        std::stringstream s_var;
        s_var << "Variable" << i;
        m_variables[i] = addStringParameter(s_var.str(), s_var.str(), "", Parameter::Choice);

        setParameterChoices(m_variables[i], varChoices);
        observeParameter(m_variables[i]);
        s_var.str("");
        s_var << "data_out" << i;
        m_dataOut[i] = createOutputPort(s_var.str(), "scalar data");
        linkPortAndParameter(m_dataOut[i], m_variables[i]);
    }
    m_variables[NUMPARAMS - 3] = addStringParameter("U", "U", "", Parameter::Choice);
    setParameterChoices(m_variables[NUMPARAMS - 3], varChoices);
    observeParameter(m_variables[NUMPARAMS - 3]);
    m_dataOut[NUMPARAMS - 3] = createOutputPort("data_out_U", "scalar data");
    linkPortAndParameter(m_dataOut[NUMPARAMS - 3], m_variables[NUMPARAMS - 3]);

    m_variables[NUMPARAMS - 2] = addStringParameter("V", "V", "", Parameter::Choice);
    setParameterChoices(m_variables[NUMPARAMS - 2], varChoices);
    observeParameter(m_variables[NUMPARAMS - 2]);
    m_dataOut[NUMPARAMS - 2] = createOutputPort("data_out_V", "scalar data");
    linkPortAndParameter(m_dataOut[NUMPARAMS - 2], m_variables[NUMPARAMS - 2]);

    m_variables[NUMPARAMS - 1] = addStringParameter("W", "W", "", Parameter::Choice);
    setParameterChoices(m_variables[NUMPARAMS - 1], varChoices);
    observeParameter(m_variables[NUMPARAMS - 1]);
    m_dataOut[NUMPARAMS - 1] = createOutputPort("data_out_W", "scalar data");
    linkPortAndParameter(m_dataOut[NUMPARAMS - 1], m_variables[NUMPARAMS - 1]);

    setParameterChoices(m_gridLat, varChoices);
    setParameterChoices(m_gridLon, varChoices);
    setParameterChoices(m_trueHGT, varChoices);
    setParameterChoices(m_PH, varChoices);
    setParameterChoices(m_PHB, varChoices);
    setParameterChoices(m_gridZ, varChoices);

    setParallelizationMode(Serial);
    setAllowTimestepDistribution(true);

    observeParameter(m_filedir);
    observeParameter(m_varDim);
}


ReadWRFChem::~ReadWRFChem()
{
    if (ncFirstFile) {
        delete ncFirstFile;
        ncFirstFile = nullptr;
    }
}


// inspectDir: check validity of path and create list of files in directory
bool ReadWRFChem::inspectDir()
{
    //TODO :: check if file is NC format!
    std::string sFileDir = m_filedir->getValue();

    if (sFileDir.empty()) {
        sendInfo("WRFChem filename is empty!");
        return false;
    }

    try {
        fs::path dir(sFileDir);
        fileList.clear();
        numFiles = 0;

        if (fs::is_directory(dir)) {
            sendInfo("Locating files in %s", dir.string().c_str());
            for (fs::directory_iterator it(dir); it != fs::directory_iterator(); ++it) {
                if (fs::is_regular_file(it->path()) && (it->path().extension() == ".nc")) {
                    // std::string fName = it->path().filename().string();
                    std::string fPath = it->path().string();
                    fileList.push_back(fPath);
                    ++numFiles;
                }
            }
        } else if (fs::is_regular_file(dir)) {
            if (dir.extension() == ".nc") {
                std::string fName = dir.string();
                sendInfo("Loading file %s", fName.c_str());
                fileList.push_back(fName);
                ++numFiles;
            } else {
                sendError("File does not end with '.nc' ");
            }
        } else {
            sendInfo("Could not find given directory %s. Please specify a valid path", sFileDir.c_str());
            return false;
        }
    } catch (std::exception &ex) {
        sendError("Could not read %s: %s", sFileDir.c_str(), ex.what());
        return false;
    }

    if (numFiles > 1) {
        std::sort(fileList.begin(), fileList.end(), [](std::string a, std::string b) { return a < b; });
    }
    sendInfo("Directory contains %d timesteps", numFiles);
    if (numFiles == 0)
        return false;
    return true;
}

bool ReadWRFChem::prepareRead()
{
    if (!ncFirstFile) {
        ReadWRFChem::examine(nullptr);
    }

    /*   if (ncFirstFile->is_valid()) {
        for (int vi=0; vi<NUMPARAMS; ++vi) {
            std::string name = "";
            int nDim = 0;
            NcToken refDim[4]={"","","",""};

            name = m_variables[vi]->getValue();
            if (!name.empty() && name != Reader::InvalidChoice) {
                NcVar* var = ncFirstFile->get_var(name.c_str());
                nDim = var->num_dims();
                if (strcmp(refDim[0],"")!=0) {
                    for(int di=0; di<nDim;++di) {
                         NcDim *dim = var->get_dim(di);
                         if (dim->name() != refDim[di]) {
                             sendInfo("Variables rely on different dimensions. Please select matching variables");
                             return false;
                         }
                    }
                }else {
                    for(int di=0; di<nDim;++di) {
                        refDim[di]=var->get_dim(di)->name();
                    }
                }
            }
        }
    }
*/
    int N_p = static_cast<int>(m_numPartitionsLat->getValue() * m_numPartitionsLat->getValue() *
                               m_numPartitionsVer->getValue());
    setPartitions(N_p);
    return true;
}


bool ReadWRFChem::examine(const vistle::Parameter *param)
{
    if (!param || param == m_filedir || param == m_varDim) {
        if (!inspectDir())
            return false;
        sendInfo("File %s is used as base", fileList.front().c_str());

        if (ncFirstFile) {
            delete ncFirstFile;
            ncFirstFile = nullptr;
        }
        std::string sDir = /* m_filedir->getValue() + "/" + */ fileList.front();

        ncFirstFile = new NcFile(sDir.c_str(), NcFile::read);

        /* if (ncFirstFile->is_valid()) { */
        if (!ncFirstFile->isNull()) {
            std::vector<std::string> AxisChoices;
            std::vector<std::string> Axis2dChoices;

            AxisChoices.clear();
            Axis2dChoices.clear();

            std::string newEntry = "";
            bool is_fav = false;
            std::vector<std::string> favVars = {"co", "no2", "PM10", "o3", "U", "V", "W"};

            for (auto &name_var: ncFirstFile->getVars()) {
                auto name = name_var.first;
                auto var = name_var.second;

                newEntry = name;

                if (var.getDimCount() > 3) {
                    for (std::string fav: favVars) {
                        if (var.getName().compare(fav)) {
                            AxisChoices.insert(AxisChoices.begin(), newEntry);
                            is_fav = true;
                            break;
                        }
                    }
                    if (is_fav == false)
                        AxisChoices.push_back(newEntry);
                    is_fav = false;
                } else if (var.getDimCount() > 2) {
                    for (std::string fav: favVars) {
                        if (newEntry.compare(fav)) {
                            Axis2dChoices.insert(Axis2dChoices.begin(), newEntry);
                            break;
                        }
                    }
                    if (is_fav == false)
                        Axis2dChoices.push_back(newEntry);
                    is_fav = false;
                }
            }

            AxisChoices.insert(AxisChoices.begin(), Reader::InvalidChoice);
            Axis2dChoices.insert(Axis2dChoices.begin(), Reader::InvalidChoice);

            if (strcmp(m_varDim->getValue().c_str(), varDimList[1].c_str()) == 0) { //3D
                sendInfo("Variable dimension: 3D");
                for (Index i = 0; i < NUMPARAMS; i++)
                    setParameterChoices(m_variables[i], AxisChoices);
            } else if (strcmp(m_varDim->getValue().c_str(), varDimList[0].c_str()) == 0) { //2D
                sendInfo("Variable dimension: 2D");
                for (Index i = 0; i < NUMPARAMS; i++)
                    setParameterChoices(m_variables[i], Axis2dChoices);
            } else {
                sendInfo("Please select the dimension of variables");
            }
            setParameterChoices(m_trueHGT, Axis2dChoices);
            setParameterChoices(m_gridLat, Axis2dChoices);
            setParameterChoices(m_gridLon, Axis2dChoices);
            setParameterChoices(m_PHB, AxisChoices);
            setParameterChoices(m_PH, AxisChoices);
            setParameterChoices(m_gridZ, AxisChoices);

            setTimesteps(numFiles);

        } else {
            sendError("Could not open NC file");
            return false;
        }
    }

    return true;
}

//setMeta: set the meta data
void ReadWRFChem::setMeta(Object::ptr obj, Index blockNr, size_t totalBlockNr, int timestep) const
{
    if (!obj)
        return;

    obj->setTimestep(timestep);
    obj->setRealTime(0);

    obj->setBlock(blockNr);
    obj->setNumBlocks(totalBlockNr == 0 ? 1 : totalBlockNr);
}

//emptyValue: check if variable selection is empty
bool ReadWRFChem::emptyValue(vistle::StringParameter *ch) const
{
    std::string name = "";
    name = ch->getValue();
    if (name.empty() || name == Reader::InvalidChoice)
        return true;
    else
        return false;
}

//computeBlock: compute indices of current block and ghost cells
ReadWRFChem::Block ReadWRFChem::computeBlock(Index part, size_t nBlocks, Index blockBegin, size_t cellsPerBlock,
                                             size_t numCellTot) const
{
    Index partIdx = blockBegin / cellsPerBlock;
    Index begin = blockBegin, end = blockBegin + cellsPerBlock + 1;
    size_t numGhostBeg = 0, numGhostEnd = 0;

    if (begin > 0) {
        --begin;
        numGhostBeg = 1;
    }

    if (partIdx == nBlocks - 1) {
        end = numCellTot;
    } else if (end < numCellTot - 1) {
        ++end;
        numGhostEnd = 1;
    }

    Block block;
    block.part = part;
    block.begin = begin;
    block.end = end;
    block.ghost[0] = numGhostBeg;
    block.ghost[1] = numGhostEnd;

    return block;
}

//generateGrid: set grid coordinates for block b and attach ghosts
Object::ptr ReadWRFChem::generateGrid(Block *b) const
{
    size_t bSizeX = b[0].end - b[0].begin, bSizeY = b[1].end - b[1].begin, bSizeZ = b[2].end - b[2].begin;
    Object::ptr geoOut;

    if (!emptyValue(m_gridLat) && !emptyValue(m_gridLon) &&
        ((!emptyValue(m_PH) && !emptyValue(m_PHB)) || !emptyValue(m_gridZ))) {
        //use geographic coordinates
        StructuredGrid::ptr strGrid(new StructuredGrid(bSizeX, bSizeY, bSizeZ));
        auto ptrOnXcoords = strGrid->x().data();
        auto ptrOnYcoords = strGrid->y().data();
        auto ptrOnZcoords = strGrid->z().data();

        std::vector<float> lat(bSizeY * bSizeZ);
        std::vector<float> lon(bSizeY * bSizeZ);

        NcVar varLat = ncFirstFile->getVar(m_gridLat->getValue().c_str());
        NcVar varLon = ncFirstFile->getVar(m_gridLon->getValue().c_str());

        //extract (2D) lat, lon, hgt
        std::vector<size_t> startCorner{0, b[1].begin, b[2].begin};
        std::vector<size_t> count{1, bSizeY, bSizeZ};
        varLat.getVar(startCorner, count, lat.data());
        varLon.getVar(startCorner, count, lon.data());

        if (!emptyValue(m_gridZ)) {
            std::vector<float> z((bSizeX + 1) * bSizeY * bSizeZ);
            NcVar varZ = ncFirstFile->getVar(m_gridZ->getValue().c_str());

            std::vector<size_t> startCorner{0, b[0].begin, b[1].begin, b[2].begin};
            std::vector<size_t> numElem{1, bSizeX + 1, bSizeY, bSizeZ};
            varZ.getVar(startCorner, numElem, z.data());

            Index n = 0;
            for (Index k = 0; k < bSizeZ; k++) {
                for (Index j = 0; j < bSizeY; j++) {
                    for (Index i = 0; i < bSizeX; i++) {
                        Index tmpIdxX1 = (i + 1) * bSizeY * bSizeZ;
                        Index tmpIdxY = j * bSizeZ;
                        ptrOnXcoords[n] = z[tmpIdxX1 + tmpIdxY + k];
                        ptrOnYcoords[n] = lat[tmpIdxY + k];
                        ptrOnZcoords[n] = lon[tmpIdxY + k];
                        ++n;
                    }
                }
            }

        } else if (!emptyValue(m_PH) && !emptyValue(m_PHB)) {
            std::vector<float> ph((bSizeX + 1) * bSizeY * bSizeZ);
            std::vector<float> phb((bSizeX + 1) * bSizeY * bSizeZ);
            NcVar varPH = ncFirstFile->getVar(m_PH->getValue().c_str());
            NcVar varPHB = ncFirstFile->getVar(m_PHB->getValue().c_str());

            //extract (3D) geopotential for z-coord calculation
            /* int numDimElem = 4; */

            std::vector<size_t> startCorner{0, b[0].begin, b[1].begin, b[2].begin};
            std::vector<size_t> numElem{1, bSizeX + 1, bSizeY, bSizeZ};

            varPH.getVar(startCorner, numElem, ph.data());
            varPHB.getVar(startCorner, numElem, phb.data());

            //geopotential height is defined on stagged grid -> one additional layer
            //thus it is evaluated (vertically) inbetween vertices to match lat/lon grid
            Index n = 0;
            for (Index k = 0; k < bSizeZ; k++) {
                for (Index j = 0; j < bSizeY; j++) {
                    for (Index i = 0; i < bSizeX; i++) {
                        Index tmpIdxX = i * bSizeY * bSizeZ;
                        Index tmpIdxY = j * bSizeZ;
                        Index idx = tmpIdxX + tmpIdxY + k;
                        Index idx1 = (i + 1) * bSizeY * bSizeZ + tmpIdxY + k;
                        ptrOnXcoords[n] = (ph[idx] + phb[idx] + ph[idx1] + phb[idx1]) / (2 * 9.81);
                        ptrOnYcoords[n] = lat[tmpIdxY + k];
                        ptrOnZcoords[n] = lon[tmpIdxY + k];
                        ++n;
                    }
                }
            }
        }

        for (Index i = 0; i < 3; ++i) {
            strGrid->setNumGhostLayers(i, StructuredGrid::Bottom, b[i].ghost[0]);
            strGrid->setNumGhostLayers(i, StructuredGrid::Top, b[i].ghost[1]);
        }

        geoOut = strGrid;
    } else if (!emptyValue(m_trueHGT)) {
        //use terrain height
        StructuredGrid::ptr strGrid(new StructuredGrid(bSizeX, bSizeY, bSizeZ));
        auto ptrOnXcoords = strGrid->x().data();
        auto ptrOnYcoords = strGrid->y().data();
        auto ptrOnZcoords = strGrid->z().data();

        /* NcVar *varHGT = ncFirstFile->get_var(m_trueHGT->getValue().c_str()); */
        NcVar varHGT = ncFirstFile->getVar(m_trueHGT->getValue().c_str());
        std::vector<float> hgt(bSizeY * bSizeZ);

        std::vector<size_t> startCorner{0, b[1].begin, b[2].begin};
        std::vector<size_t> count{1, bSizeY, bSizeZ};
        varHGT.getVar(startCorner, count, hgt.data());

        Index n = 0;
        for (Index k = 0; k < bSizeZ; k++) {
            for (Index j = 0; j < bSizeY; j++) {
                for (Index i = 0; i < bSizeX; i++) {
                    Index tmpIdxY = j + b[1].begin;
                    ptrOnXcoords[n] = (i + b[0].begin + hgt[k + bSizeZ * j] / 50); //divide by 50m (=dx of grid cell)
                    ptrOnYcoords[n] = tmpIdxY;
                    ptrOnZcoords[n] = k + b[2].begin;
                    ++n;
                }
            }
        }
        for (Index i = 0; i < 3; ++i) {
            strGrid->setNumGhostLayers(i, StructuredGrid::Bottom, b[i].ghost[0]);
            strGrid->setNumGhostLayers(i, StructuredGrid::Top, b[i].ghost[1]);
        }
        geoOut = strGrid;
    } else {
        //uniform coordinates
        UniformGrid::ptr uniGrid(new UniformGrid(bSizeX, bSizeY, bSizeZ));

        for (Index i = 0; i < 3; ++i) {
            uniGrid->min()[i] = b[i].begin;
            uniGrid->max()[i] = b[i].end;

            uniGrid->setNumGhostLayers(i, StructuredGrid::Bottom, b[i].ghost[0]);
            uniGrid->setNumGhostLayers(i, StructuredGrid::Top, b[i].ghost[1]);
        }
        geoOut = uniGrid;
    }

    updateMeta(geoOut);
    return geoOut;
}


//addDataToPort: read and set values for variable and add them to the output port
bool ReadWRFChem::addDataToPort(Token &token, NcFile *ncDataFile, int vi, Object::ptr outGrid, Block *b, Index block,
                                int t) const
{
    if (!(StructuredGrid::as(outGrid) || UniformGrid::as(outGrid)))
        return true;
    NcVar varData = ncDataFile->getVar(m_variables[vi]->getValue().c_str());
    std::string unit = varData.getAtt("units").getName();
    int numDimElem = varData.getDimCount();
    size_t bSizeX = b[0].end - b[0].begin, bSizeY = b[1].end - b[1].begin, bSizeZ = b[2].end - b[2].begin;
    std::vector<size_t> startCorner(numDimElem);
    startCorner.at(numDimElem - 3) = b[0].begin;
    startCorner.at(numDimElem - 2) = b[1].begin;
    startCorner.at(numDimElem - 1) = b[2].begin;
    startCorner.at(0) = 0;

    std::vector<size_t> numElem(numDimElem);
    numElem.at(numDimElem - 3) = bSizeX;
    numElem.at(numDimElem - 2) = bSizeY;
    numElem.at(numDimElem - 1) = bSizeZ;
    numElem.at(0) = 1;

    Vec<Scalar>::ptr obj(new Vec<Scalar>(bSizeX * bSizeY * bSizeZ));
    vistle::Scalar *ptrOnScalarData = obj->x().data();

    if ((vi == NUMPARAMS - 1) && (numDimElem > 3)) { //W has one level to many: -> read and crop
        numElem[numDimElem - 3] = bSizeX + 1;
        std::vector<float> longdata((bSizeX + 1) * bSizeY * bSizeZ);

        varData.getVar(startCorner, numElem, longdata.data());
        Index n = 0;
        for (Index k = 0; k < bSizeZ; k++) {
            for (Index j = 0; j < bSizeY; j++) {
                for (Index i = 1; i < bSizeX + 1; i++) {
                    Index tmpIdxX = (i)*bSizeY * bSizeZ;
                    Index tmpIdxXY = tmpIdxX + j * bSizeZ;
                    ptrOnScalarData[n] = longdata[tmpIdxXY + k];
                    ++n;
                }
            }
        }
    } else {
        std::vector<float> longdata(bSizeX * bSizeY * bSizeZ);
        varData.getVar(startCorner, numElem, longdata.data());
        Index n = 0;
        for (Index k = 0; k < bSizeZ; k++) {
            for (Index j = 0; j < bSizeY; j++) {
                for (Index i = 0; i < bSizeX; i++) {
                    Index tmpIdxX = (i)*bSizeY * bSizeZ;
                    Index tmpIdxXY = tmpIdxX + j * bSizeZ;
                    ptrOnScalarData[n] = longdata[tmpIdxXY + k];
                    ++n;
                }
            }
        }
    }
    obj->setMapping(DataBase::Vertex);
    obj->setGrid(outGrid);
    setMeta(obj, block, numBlocks, t);
    std::string pVar = m_variables[vi]->getValue();
    obj->describe(pVar, id());
    token.applyMeta(obj);
    token.addObject(m_dataOut[vi], obj);

    return true;
}


bool ReadWRFChem::read(Token &token, int timestep, int block)
{
    Index numBlocksLat = m_numPartitionsLat->getValue();
    Index numBlocksVer = m_numPartitionsVer->getValue();
    if ((numBlocksLat <= 0) || (numBlocksVer <= 0)) {
        sendInfo("Number of partitions cannot be zero!");
        return false;
    }
    numBlocks = numBlocksLat * numBlocksLat * numBlocksVer;

    if (!ncFirstFile->isNull()) {
        NcVar var;
        std::vector<NcDim> edges;
        int numdims = 0;

        //TODO: number of dimensions can be set by any variable, when check in prepareRead is used to ensure matching dimensions of all variables
        for (Index vi = 0; vi < NUMPARAMS; vi++) {
            std::string name = "";
            name = m_variables[vi]->getValue();
            if (!name.empty() && name != Reader::InvalidChoice) {
                var = ncFirstFile->getVar(name);
                if (var.getDimCount() > numdims) {
                    numdims = var.getDimCount();
                    edges = var.getDims();
                }
            }
        }

        if (numdims == 0) {
            sendInfo("Failed to load variables: Dimension is zero");
            return false;
        }

        size_t nx = edges[numdims - 3].getSize() /*Bottom_Top*/, ny = edges[numdims - 2].getSize() /*South_North*/,
               nz = edges[numdims - 1].getSize() /*East_West*/; //, nTime = edges[0] /*TIME*/ ;
        size_t blockSizeVer = (nx) / numBlocksVer, blockSizeLat = (ny) / numBlocksLat;
        if (numdims <= 3) {
            nx = 1;
            blockSizeVer = 1;
            numBlocksVer = 1;
        }

        //set offsets for current block
        size_t blockXbegin = block / (numBlocksLat * numBlocksLat) * blockSizeVer; //vertical direction (Bottom_Top)
        size_t blockYbegin = ((block % (numBlocksLat * numBlocksLat)) / numBlocksLat) * blockSizeLat;
        size_t blockZbegin = (block % numBlocksLat) * blockSizeLat;

        Block b[3];

        b[0] = computeBlock(block, numBlocksVer, blockXbegin, blockSizeVer, nx);
        b[1] = computeBlock(block, numBlocksLat, blockYbegin, blockSizeLat, ny);
        b[2] = computeBlock(block, numBlocksLat, blockZbegin, blockSizeLat, nz);

        if (!outObject[block]) {
            //********* GRID *************
            outObject[block] = generateGrid(b);
            setMeta(outObject[block], block, numBlocks, -1);
        }
        if (timestep == -1) {
            token.applyMeta(outObject[block]);
            token.addObject(m_gridOut, outObject[block]);
        } else {
            // ******** DATA *************
            std::string sDir = fileList.at(timestep);

            NcFile *ncDataFile = new NcFile(sDir, NcFile::read);

            if (ncDataFile->isNull()) {
                sendError("Could not open data file at time %i", timestep);
                return false;
            }

            for (Index vi = 0; vi < NUMPARAMS; ++vi) {
                if (emptyValue(m_variables[vi])) {
                    continue;
                }
                addDataToPort(token, ncDataFile, vi, outObject[block], b, block, timestep);
            }
            delete ncDataFile;
            ncDataFile = nullptr;
        }
        return true;
    }
    return false;
}

bool ReadWRFChem::finishRead()
{
    if (ncFirstFile) {
        delete ncFirstFile;
        ncFirstFile = nullptr;
    }
    return true;
}


MODULE_MAIN(ReadWRFChem)
