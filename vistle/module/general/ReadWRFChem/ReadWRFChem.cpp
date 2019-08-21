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

std::vector<std::string> AxisChoices;
std::vector<std::string> VarDisplayList;

ReadWRFChem::ReadWRFChem(const std::string &name, int moduleID, mpi::communicator comm)
    : vistle::Reader ("Read WRF Chem data files", name, moduleID, comm)
{
    //TODO: axisChoices should be set on start up (if dir given)
    ncFirstFile = NULL;

    m_gridOut = createOutputPort("grid_out", "grid");
    m_filedir = addStringParameter("file_dir", "NC files directory","/mnt/raid/home/hpcleker/Desktop/test_files/NC/test_time", Parameter::Directory);

    m_numPartitionsLat = addIntParameter("num_partitions_lat", "number of partitions in lateral", 1);
    m_numPartitionsVer = addIntParameter("num_partitions_ver", "number of partitions in vertical", 1);
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
    setParallelizationMode(Serial);

    observeParameter(m_filedir);
    observeParameter(m_numPartitionsLat);
    observeParameter(m_numPartitionsVer);
    /*observeParameter(m_gridChoiceX);
    observeParameter(m_gridChoiceY);
    observeParameter(m_gridChoiceZ);*/

}



ReadWRFChem::~ReadWRFChem() {
    if (ncFirstFile) {
        delete ncFirstFile;
    }
}



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

    if (!bf::is_directory(dir)) {
        sendInfo("Could not find given directory. Please specify a valid path");
        return false;
    }
    sendInfo("Locating files in %s", dir.string().c_str());
    for (bf::directory_iterator it(dir) ; it != bf::directory_iterator(); ++it) {
        std::string fName = it->path().filename().string();
        fileList.push_back(fName);
        ++numFiles;
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
        sendError("Please specify a valid directory");
        return false;
    }
    int N_p = static_cast<int>( m_numPartitionsLat->getValue() * m_numPartitionsLat->getValue() * m_numPartitionsVer->getValue());
    setPartitions(N_p);
   return true;
}



bool ReadWRFChem::examine(const vistle::Parameter *param) {

    if (!param || param == m_filedir) {
       if (!inspectDir())
           return false;
       sendInfo("File %s is used as base", fileList.front().c_str());

       if (ncFirstFile)
           delete ncFirstFile;
       std::string sDir = m_filedir->getValue() + "/" + fileList.front();
       ncFirstFile = new NcFile(sDir.c_str(), NcFile::ReadOnly, NULL, 0, NcFile::Offset64Bits);

       if (ncFirstFile->is_valid()) {
            NcVar *var;
            AxisChoices.clear();

            if (!ncFirstFile->is_valid()) {
                sendInfo("Failed to access file");
                return false;
            }
            int num2dVars = 0;
            int nVar = ncFirstFile->num_vars();
            for (int i = 0; i < nVar; ++i) {
                var = ncFirstFile->get_var(i);
                if (var->num_dims() > 3) {          //currently: only 3D (in space) can be read, as o.w. issues in further modules
                    char *newEntry = new char[50];
                    strcpy(newEntry, var->name());
                    AxisChoices.push_back(newEntry);
                    num2dVars++;
                }
            }
            AxisChoices.push_back("NONE");
            for (int i = 0; i < NUMPARAMS; i++)
                setParameterChoices(m_variables[i], AxisChoices);

            /*setParameterChoices(m_gridChoiceX, AxisChoices);
            setParameterChoices(m_gridChoiceY, AxisChoices);
            setParameterChoices(m_gridChoiceZ, AxisChoices);*/
            setTimesteps(numFiles);

        } else {
            std::cerr << "ReadWRFChem: could not open NC file" << std::endl;
            return false;
        }
    }


   /* if ((param == m_numPartitionsLat) || (param == m_numPartitionsVer)) {
       int N_p = static_cast<int>( m_numPartitionsLat->getValue() * m_numPartitionsLat->getValue() * m_numPartitionsVer->getValue());
       setPartitions(N_p);
    }*/
    return true;
}


void ReadWRFChem::setMeta(Object::ptr obj, int blockNr, int totalBlockNr, int timestep) const {
    if(!obj)
        return;

    obj->setTimestep(timestep);
    obj->setNumTimesteps(numFiles);
    obj->setRealTime(0);

    obj->setBlock(blockNr);
    obj->setNumBlocks(totalBlockNr == 0 ? 1 : totalBlockNr);
}

bool ReadWRFChem::emptyValue(vistle::StringParameter *ch) const {
    std::string name = "";
    name = ch->getValue();
    if ((name == "") || (name=="NONE"))
        return true;
    else return false;
}


ReadWRFChem::Block ReadWRFChem::computeBlock(int part, int nBlocks, long blockBegin, long cellsPerBlock, long numCellTot) const {
    int partIdx = (blockBegin == 0 ? 0 : (blockBegin/cellsPerBlock));

    int begin = 0, end = nBlocks;
    int beginghost = 0, endghost = 0;
    if (part >= 0)  {
        begin = blockBegin;
        end = begin + cellsPerBlock;
        if (partIdx > 0) {
            --begin;
            beginghost = 1;
        }
        if (partIdx < nBlocks-1) {
            ++end;
            endghost = 1;
        } else {
            end = numCellTot;
        }
    }

    Block block;
    block.part = part;
    block.begin = begin;
    block.end = end;
    block.ghost[0] = beginghost;
    block.ghost[1] = endghost;

    return block;
}

StructuredGrid::ptr ReadWRFChem::generateGrid(Block *b) const {
    int bSizeX = b[0].end - b[0].begin, bSizeY = b[1].end - b[1].begin, bSizeZ = b[2].end - b[2].begin;

    StructuredGrid::ptr outGrid(new StructuredGrid(bSizeX, bSizeY, bSizeZ));
    outGrid->setGrid(outGrid);
    auto ptrOnXcoords = outGrid->x().data();
    auto ptrOnYcoords = outGrid->y().data();
    auto ptrOnZcoords = outGrid->z().data();
    int n = 0;
    for (int i = 0; i < bSizeX; i++) {
        for (int j = 0; j < bSizeY; j++) {
            for (int k = 0; k < bSizeZ; k++, n++) {
                ptrOnXcoords[n] = i+b[0].begin;
                ptrOnYcoords[n] = j+b[1].begin;
                ptrOnZcoords[n] = k+b[2].begin;
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

bool ReadWRFChem::addDataToPort(Token &token, NcFile *ncDataFile, int vi, StructuredGrid::ptr outGrid, Block *b, int block, int t) const {

    NcVar *varData = ncDataFile->get_var(m_variables[vi]->getValue().c_str());
    int numDimElem = varData->num_dims();
    long *curs = varData->edges();
    int bSizeX = b[0].end - b[0].begin, bSizeY = b[1].end - b[1].begin, bSizeZ = b[2].end - b[2].begin;

    curs[numDimElem-1] = b[2].begin;
    curs[numDimElem-2] = b[1].begin;
    curs[numDimElem-3] = b[0].begin;
    curs[0] = 0;

    long *numElem = varData->edges();
    numElem[numDimElem-1] = bSizeZ;
    numElem[numDimElem-2] = bSizeY;
    numElem[numDimElem-3] = bSizeX;
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
    /*TODO:
     *  x better use uniform grid?
    */
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

            //find number of dimensions and sizes
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
            long blockSizeVer = nx/numBlocksVer, blockSizeLat = ny/numBlocksLat;
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


            //set the ghost cells
            Block b[3];
            for (int d = 0; d < 3; ++d) {

                int nBlocks = numBlocksLat;
                int cellsPerBlock = blockSizeLat;
                int blockBegin;
                long numCellTot;

                if ( d == 0) {
                    nBlocks = numBlocksVer;
                    blockBegin = blockXbegin;
                    cellsPerBlock = blockSizeVer;
                    numCellTot = nx;
                } else if ( d == 1) {
                    blockBegin = blockYbegin;
                    numCellTot = ny;
                } else {
                    blockBegin = blockZbegin;
                    numCellTot = nz;
                }

                b[d] = computeBlock(block, nBlocks, blockBegin, cellsPerBlock, numCellTot);
            }

            //********* GRID *************
            StructuredGrid::ptr outDataGrid = generateGrid(b);
            setMeta(outDataGrid, block, numBlocks, -1);
            outDataGrid->setNumTimesteps(-1);
            token.addObject(m_gridOut, outDataGrid);

            // ******** DATA *************
            for (int t = 0; t < numFiles; ++t) {
                std::string sDir = m_filedir->getValue() + "/" + fileList.at(t);
                NcFile *ncDataFile = new NcFile(sDir.c_str(), NcFile::ReadOnly, NULL, 0, NcFile::Offset64Bits);
                if (! ncDataFile->is_valid()) {
                    sendError("Could not open data file at time %i", t);
                    return false;
                }

                for (int vi = 0; vi < NUMPARAMS; ++vi) {
                    if (emptyValue(m_variables[vi])) {
                        continue;
                    }
                    addDataToPort(token, ncDataFile, vi, outDataGrid, b,  block, t);
                }
                delete ncDataFile;
            }

        }
    return true;
}

bool ReadWRFChem::finishRead() {

    return true;
}


MODULE_MAIN(ReadWRFChem)
