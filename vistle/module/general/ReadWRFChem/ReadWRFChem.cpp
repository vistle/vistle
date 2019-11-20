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


#include "ReadWRFChem.h"
#include <netcdfcpp.h>

#include <core/structuredgrid.h>
#include <boost/filesystem.hpp>

using namespace vistle;
namespace bf = boost::filesystem;


ReadWRFChem::ReadWRFChem(const std::string &name, int moduleID, mpi::communicator comm)
    : vistle::Reader ("Read WRF Chem data files", name, moduleID, comm)
{
    ncFirstFile = NULL;

    m_gridOut = createOutputPort("grid_out", "grid");
    m_filedir = addStringParameter("file_dir", "NC files directory","/mnt/raid/home/hpcleker/Desktop/test_files/NC/test_time", Parameter::Directory);

    m_numPartitionsLat = addIntParameter("num_partitions_lat", "number of partitions in lateral", 1);
    m_numPartitionsVer = addIntParameter("num_partitions_ver", "number of partitions in vertical", 1);

    m_varDim = addStringParameter("var_dim","Dimension of variables","",Parameter::Choice);
    setParameterChoices(m_varDim, varDimList);

    m_trueHGT = addStringParameter("true_height", "Use real ground topology", "", Parameter::Choice);

    /*
    m_gridChoiceX = addStringParameter("GridX", "grid Bottom-Top axis", "", Parameter::Choice);
    m_gridChoiceY = addStringParameter("GridY", "grid Sout-North axis", "", Parameter::Choice);
    m_gridChoiceZ = addStringParameter("GridZ", "grid East-West axis", "", Parameter::Choice);

    std::vector<std::string> choices;
    choices.push_back("(NONE)");
    setParameterChoices(m_gridChoiceX, choices);
    setParameterChoices(m_gridChoiceY, choices);
    setParameterChoices(m_gridChoiceZ, choices);
    */

    char namebuf[50];
    std::vector<std::string> varChoices;
    varChoices.push_back("(NONE)");

    for (int i = 0; i < NUMPARAMS; ++i) {

        sprintf(namebuf, "Variable%d", i);

        std::stringstream s_var;
        s_var << "Variable" << i;
        m_variables[i] = addStringParameter(s_var.str(), s_var.str(), "", Parameter::Choice);

        setParameterChoices(m_variables[i], varChoices);
        observeParameter(m_variables[i]);
        s_var.str("");
        s_var << "data_out" << i;
        m_dataOut[i] = createOutputPort(s_var.str(), "scalar data");

    }
    setParameterChoices(m_trueHGT, varChoices);

    setParallelizationMode(Serial);

    observeParameter(m_filedir);
    observeParameter(m_varDim);
}


ReadWRFChem::~ReadWRFChem() {
    if (ncFirstFile) {
        delete ncFirstFile;
        ncFirstFile = nullptr;
    }
}


// inspectDir: check validity of path and create list of files in directory
bool ReadWRFChem::inspectDir() {
    //TODO :: check if file is NC format!
    std::string sFileDir = m_filedir->getValue();

     if (sFileDir.empty()) {
        sendInfo("WRFChem filename is empty!");
        return false;
    }

    bf::path dir(sFileDir);
    fileList.clear();
    numFiles = 0;

    if (bf::is_directory(dir)) {
        sendInfo("Locating files in %s", dir.string().c_str());
        for (bf::directory_iterator it(dir) ; it != bf::directory_iterator(); ++it) {
            if (bf::is_regular_file(it->path()) &&(bf::extension(it->path().filename())==".nc")) {
                   // std::string fName = it->path().filename().string();
                   std::string fPath = it->path().string();
                   fileList.push_back(fPath);
                   ++numFiles;
            }
        }
    }else if (bf::is_regular_file(dir)) {
        if (bf::extension(dir.filename())==".nc") {
            std::string fName = dir.string();
            sendInfo("Loading file %s", fName.c_str());
            fileList.push_back(fName);
            ++numFiles;
        }else {
            sendError("File does not end with '.nc' ");
        }
    }else {
        sendInfo("Could not find given directory. Please specify a valid path");
        return false;
    }

    if (numFiles > 1) {
        std::sort(fileList.begin(), fileList.end(), [](std::string a, std::string b) {return a<b;}) ;
    }
    sendInfo("Directory contains %d timesteps", numFiles);
    if (numFiles == 0)
            return false;
    return true;
}

bool ReadWRFChem::prepareRead() {
    if (!ncFirstFile) {
        ReadWRFChem::examine(nullptr);
    }

 /*   if (ncFirstFile->is_valid()) {
        for (int vi=0; vi<NUMPARAMS; ++vi) {
            std::string name = "";
            int nDim = 0;
            NcToken refDim[4]={"","","",""};

            name = m_variables[vi]->getValue();
            if ((name != "") && (name != "NONE")) {
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
    int N_p = static_cast<int>( m_numPartitionsLat->getValue() * m_numPartitionsLat->getValue() * m_numPartitionsVer->getValue());
    setPartitions(N_p);
   return true;
}


bool ReadWRFChem::examine(const vistle::Parameter *param) {

    if (!param || param == m_filedir || param == m_varDim) {
       if (!inspectDir())
           return false;
       sendInfo("File %s is used as base", fileList.front().c_str());

       if (ncFirstFile) {
           delete ncFirstFile;
           ncFirstFile = nullptr;
       }
       std::string sDir =/* m_filedir->getValue() + "/" + */fileList.front();
       ncFirstFile = new NcFile(sDir.c_str(), NcFile::ReadOnly, NULL, 0, NcFile::Offset64Bits);

       if (ncFirstFile->is_valid()) {

            std::vector<std::string> AxisChoices;
            std::vector<std::string> Axis2dChoices;

            NcVar *var;
            AxisChoices.clear();
            Axis2dChoices.clear();

            /*if (!ncFirstFile->is_valid()) {
                sendInfo("Failed to access file");
                return false;
            }*/
            int num3dVars = 0;
            int num2dVars = 0;
            int nVar = ncFirstFile->num_vars();

            char *newEntry = new char[50];
            bool is_fav = false;
            std::vector<std::string> favVars = {"co","no2","PM10","o3","U","V","W"};

            for (int i = 0; i < nVar; ++i) {
                var = ncFirstFile->get_var(i);
                strcpy(newEntry, var->name());

                if (var->num_dims() > 3) {
                    for (std::string fav : favVars) {
                        if (strcmp(var->name(), fav.c_str()) == 0) {
                            AxisChoices.insert(AxisChoices.begin(), newEntry);
                            is_fav = true;
                            break;
                        }
                    }
                    if (is_fav == false)
                        AxisChoices.push_back(newEntry);
                    num3dVars++;
                    is_fav = false;
                }else if (var->num_dims() > 2) {
                    for (std::string fav : favVars) {
                        if (strcmp(newEntry, fav.c_str()) == 0) {
                            Axis2dChoices.insert(Axis2dChoices.begin(), newEntry);
                            break;
                        }
                    }
                    if (is_fav == false)
                        Axis2dChoices.push_back(newEntry);
                    num2dVars++;
                    is_fav = false;
                }
            }
            delete [] newEntry;

            AxisChoices.insert(AxisChoices.begin(),"NONE");
            Axis2dChoices.insert(Axis2dChoices.begin(),"NONE");

            if (strcmp(m_varDim->getValue().c_str(),varDimList[1].c_str())==0) {//3D
                sendInfo("Variable dimension: 3D");
                for (int i = 0; i < NUMPARAMS; i++)
                    setParameterChoices(m_variables[i], AxisChoices);
            }else if (strcmp(m_varDim->getValue().c_str(),varDimList[0].c_str())==0) { //2D
                sendInfo("Variable dimension: 2D");
                for (int i = 0; i < NUMPARAMS; i++)
                    setParameterChoices(m_variables[i], Axis2dChoices);
            }else {
                sendInfo("Please select the dimension of variables");
            }
            setParameterChoices(m_trueHGT,Axis2dChoices);
            /*setParameterChoices(m_gridChoiceX, AxisChoices);
            setParameterChoices(m_gridChoiceY, AxisChoices);
            setParameterChoices(m_gridChoiceZ, AxisChoices);*/
            setTimesteps(numFiles);

        } else {
            sendError( "Could not open NC file" );
            return false;
        }
    }

    return true;
}

//setMeta: set the meta data
void ReadWRFChem::setMeta(Object::ptr obj, int blockNr, int totalBlockNr, int timestep) const {
    if(!obj)
        return;

    obj->setTimestep(timestep);
    //obj->setNumTimesteps(numFiles);
    obj->setRealTime(0);

    obj->setBlock(blockNr);
    obj->setNumBlocks(totalBlockNr == 0 ? 1 : totalBlockNr);
}

//emptyValue: check if variable selction is empty
bool ReadWRFChem::emptyValue(vistle::StringParameter *ch) const {
    std::string name = "";
    name = ch->getValue();
    if ((name == "") || (name=="NONE"))
        return true;
    else return false;
}

//computeBlock: compute indices of current block and ghost cells
ReadWRFChem::Block ReadWRFChem::computeBlock(int part, int nBlocks, long blockBegin, long cellsPerBlock, long numCellTot) const {
    int partIdx = blockBegin/cellsPerBlock;
    int begin = blockBegin, end = blockBegin + cellsPerBlock + 1;
    int numGhostBeg = 0, numGhostEnd = 0;

    if (begin > 0) {
        --begin;
        numGhostBeg = 1;
    }

    if (partIdx == nBlocks - 1) {
        end = numCellTot;
    }else if (end < numCellTot - 1) {
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
StructuredGrid::ptr ReadWRFChem::generateGrid(Block *b) const {
    int bSizeX = b[0].end - b[0].begin, bSizeY = b[1].end - b[1].begin, bSizeZ = b[2].end - b[2].begin;
    StructuredGrid::ptr outGrid(new StructuredGrid(bSizeX, bSizeY, bSizeZ));
    auto ptrOnXcoords = outGrid->x().data();
    auto ptrOnYcoords = outGrid->y().data();
    auto ptrOnZcoords = outGrid->z().data();

    if (!emptyValue(m_trueHGT)) {
        //add real ground height to grid
        NcVar *varHGT = ncFirstFile->get_var(m_trueHGT->getValue().c_str());
        float * hgt = new float[bSizeY*bSizeZ];

        varHGT->set_cur(0,b[1].begin,b[2].begin);
        varHGT->get(hgt, 1,bSizeY, bSizeZ);

        int n = 0;
        for (int i = 0; i < bSizeX; i++) {
            for (int j = 0; j < bSizeY; j++) {
                for (int k = 0; k < bSizeZ; k++, n++) {
                    ptrOnXcoords[n] = -(i+b[0].begin+hgt[k+bSizeZ*j]/50);  //divide by 50m (=dx of grid cell)
                    ptrOnYcoords[n] = j+b[1].begin;
                    ptrOnZcoords[n] = k+b[2].begin;
                }
            }
        }
        delete [] hgt;
    }else {
        int n = 0;
        for (int i = 0; i < bSizeX; i++) {
            for (int j = 0; j < bSizeY; j++) {
                for (int k = 0; k < bSizeZ; k++, n++) {
                    ptrOnXcoords[n] = -(i+b[0].begin);
                    ptrOnYcoords[n] = j+b[1].begin;
                    ptrOnZcoords[n] = k+b[2].begin;
                }
            }
        }
    }
    for (int d = 0; d < 3; ++d) {
        outGrid->setNumGhostLayers(d, StructuredGrid::Bottom, b[d].ghost[0] );
        outGrid->setNumGhostLayers(d, StructuredGrid::Top, b[d].ghost[1]);
    }

    outGrid->updateInternals();
    return outGrid;

}

//addDataToPort: read and set values for variable and add them to the output port
bool ReadWRFChem::addDataToPort(Token &token, NcFile *ncDataFile, int vi, StructuredGrid::ptr outGrid, Block *b, int block, int t) const {

    NcVar *varData = ncDataFile->get_var(m_variables[vi]->getValue().c_str());
    int numDimElem = varData->num_dims();
    long *curs = varData->edges();
    int bSizeX = b[0].end - b[0].begin, bSizeY = b[1].end - b[1].begin, bSizeZ = b[2].end - b[2].begin;

    curs[numDimElem-3] = b[0].begin;
    curs[numDimElem-2] = b[1].begin;
    curs[numDimElem-1] = b[2].begin;
    curs[0] = 0;

    long *numElem = varData->edges();
    numElem[numDimElem-3] = bSizeX;
    numElem[numDimElem-2] = bSizeY;
    numElem[numDimElem-1] = bSizeZ;
    numElem[0] = 1;

    Vec<Scalar>::ptr obj(new Vec<Scalar>(bSizeX*bSizeY*bSizeZ));
    vistle::Scalar *ptrOnScalarData = obj->x().data();

    varData->set_cur(curs);
    varData->get(ptrOnScalarData, numElem);

    obj->setGrid(outGrid);
    setMeta(obj, block, numBlocks, t);
    obj->setMapping(DataBase::Vertex);
    std::string pVar = m_variables[vi]->getValue();
    obj->addAttribute("_species", pVar);
    token.addObject(m_dataOut[vi], obj);

    return true;
}


bool ReadWRFChem::read(Token &token, int timestep, int block) {
    int numBlocksLat = m_numPartitionsLat->getValue();
    int numBlocksVer = m_numPartitionsVer->getValue();
    if ((numBlocksLat <= 0) || (numBlocksVer <= 0)) {
        sendInfo("Number of partitions cannot be zero!");
        return false;
    }
    numBlocks = numBlocksLat*numBlocksLat*numBlocksVer;

    if (ncFirstFile->is_valid()) {

            NcVar *var;
            long *edges;
            int numdims = 0;

            //TODO: number of dimensions can be set by any variable, when check in prepareRead is used to ensure matching dimensions of all variables
            for (int vi = 0; vi < NUMPARAMS; vi++) {
               std::string name = "";
               name = m_variables[vi]->getValue();
               if ((name != "") && (name != "NONE")) {
                    var = ncFirstFile->get_var(name.c_str());
                    if (var->num_dims() > numdims) {
                        numdims = var->num_dims();
                        edges = var->edges();
                    }
                }
            }

            if (numdims == 0) {
                sendInfo("Failed to load variables: Dimension is zero");
                return false;
            }

            int nx = edges[numdims - 3] /*Bottom_Top*/, ny = edges[numdims - 2] /*South_North*/, nz = edges[numdims - 1]/*East_West*/;//, nTime = edges[0] /*TIME*/ ;
            long blockSizeVer = (nx)/numBlocksVer, blockSizeLat = (ny)/numBlocksLat;
            if (numdims <= 3) {
                nx = 1;
                blockSizeVer = 1;
                numBlocksVer = 1;
            }

            /*if (emptyValue(m_gridChoiceX) || emptyValue(m_gridChoiceY) || emptyValue(m_gridChoiceZ)) {
                sendInfo("Found empty grid variable -- stop reading");
                return false;
            }*/

            //set offsets for current block
            int blockXbegin = block /(numBlocksLat*numBlocksLat) * blockSizeVer;  //vertical direction (Bottom_Top)
            int blockYbegin = ((static_cast<long>((block % (numBlocksLat*numBlocksLat)) / numBlocksLat)) * blockSizeLat);
            int blockZbegin = (block % numBlocksLat)*blockSizeLat;

            Block b[3];

            b[0] = computeBlock(block, numBlocksVer, blockXbegin, blockSizeVer, nx );
            b[1] = computeBlock(block, numBlocksLat, blockYbegin, blockSizeLat, ny );
            b[2] = computeBlock(block, numBlocksLat, blockZbegin, blockSizeLat, nz );

            if (timestep < 0) {
                //********* GRID *************
                outDataGrid[block] = generateGrid(b);
                setMeta(outDataGrid[block], block, numBlocks, -1);
                //outDataGrid->setNumTimesteps(-1);
                token.addObject(m_gridOut, outDataGrid[block]);
            }else {
                // ******** DATA *************
                std::string sDir = /*m_filedir->getValue() + "/" + */ fileList.at(timestep);
                NcFile *ncDataFile = new NcFile(sDir.c_str(), NcFile::ReadOnly, NULL, 0, NcFile::Offset64Bits);

                if (! ncDataFile->is_valid()) {
                    sendError("Could not open data file at time %i", timestep);
                    return false;
                }

                for (int vi = 0; vi < NUMPARAMS; ++vi) {
                    if (emptyValue(m_variables[vi])) {
                        sendInfo("No data for variable found");
                        return false;
                    }
                    addDataToPort(token, ncDataFile, vi, outDataGrid[block], b,  block, timestep);
                }
                delete ncDataFile;
                ncDataFile = nullptr;
            }
            return true;
        }
    return false;

}

bool ReadWRFChem::finishRead() {
    if(ncFirstFile) {
        delete ncFirstFile;
        ncFirstFile = nullptr;
    }
    return true;
}


MODULE_MAIN(ReadWRFChem)
