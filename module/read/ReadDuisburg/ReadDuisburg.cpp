/* ------------------------------------------------------------------------------------
           READ Duisburg Simulation Data from Netcdf file
------------------------------------------------------------------------------------ */
#include "ReadDuisburg.h"
#include <boost/algorithm/string.hpp>
#include <vistle/util/filesystem.h>
#include <fstream>

using namespace vistle;
namespace bf = vistle::filesystem;

using namespace PnetCDF;

#if defined(MODULE_THREAD) && PNETCDF_THREAD_SAFE == 0
static std::mutex pnetcdf_mutex; // avoid simultaneous access to PnetCDF library, if not thread-safe
#define LOCK_NETCDF(comm) \
    std::unique_lock<std::mutex> pnetcdf_guard(pnetcdf_mutex, std::defer_lock); \
    if ((comm).rank() == 0) \
        pnetcdf_guard.lock(); \
    (comm).barrier();
#define UNLOCK_NETCDF(comm) \
    (comm).barrier(); \
    if (pnetcdf_guard) \
        pnetcdf_guard.unlock();
#else
#define LOCK_NETCDF(comm)
#define UNLOCK_NETCDF(comm)
#endif

ReadDuisburg::ReadDuisburg(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    //initialize ports
    m_gridFile = addStringParameter("grid_file", "File containing grid and data", "/home/hpcleker/Desktop/duisburg_5t.nc", Parameter::ExistingFilename);
    setParameterFilters(m_gridFile, "NetCDF Grid Files (*.grid.nc)/NetCDF Files (*.nc)/All Files (*)");
   
    m_gridOut = createOutputPort("grid_out", "grid");

    //set other inital options
    setParallelizationMode(Serial);
    setAllowTimestepDistribution(true);
    // setHandlePartitions(Reader::PartitionTimesteps);

    setCollectiveIo(Reader::Collective); 

    observeParameter(m_gridFile);
}

ReadDuisburg::~ReadDuisburg()
{}

bool ReadDuisburg::prepareRead()
{


    return true;
}

bool ReadDuisburg::examine(const vistle::Parameter *param)
{
	if (!param || param == m_gridFile) {
	    LOCK_NETCDF(comm());
	    //extract number of timesteps from file
	    NcmpiFile ncFile(comm(), m_gridFile->getValue().c_str(), NcmpiFile::read);
	    const NcmpiDim timesDim = ncFile.getDim("t");
	    size_t nTimes = timesDim.getSize();
	    UNLOCK_NETCDF(comm());

	    setTimesteps(nTimes);
	    setPartitions(1);
	}
    return true;
}

//cellIsWater: checks if vertex and neighbor vertices are also water e.g. if z-coord is varying
bool ReadDuisburg::cellIsWater(const std::vector<double> &h, int i, int j, int dimX, int dimY) const {
    if ((h[j * dimX + i] > 0.) \
        && (h[(j + 1) * dimX + (i + 1) ] > 0.) \
        && (h[(j + 1) * dimX + i] > 0.) \
        && (h[j * dimX + (i + 1)] > 0.) ) {
        return true;
    }
    return false;
}

bool ReadDuisburg::getDimensions(const NcmpiFile &ncFile, int &dimX, int &dimY) const{
    const NcmpiDim &dimXname = ncFile.getDim("x");
    if (dimXname.isNull()) {
        sendError("Dimension not found in file");
        return false;
    }
    dimX = 7000;//dimXname.getSize(); 
    const NcmpiDim &dimYname = ncFile.getDim("y");
    dimY = 7000;//dimYname.getSize();
    return true;
}

Object::ptr ReadDuisburg::generateTriangleGrid(const NcmpiFile &ncFile, int timestep, int block) const {  
    int dimX, dimY;
    if (!getDimensions(ncFile,dimX,dimY))
        return Object::ptr();

    std::vector<MPI_Offset> start = {0};
    std::vector<MPI_Offset> stopX{dimX};
    std::vector<MPI_Offset> stopY{dimY};
    std::vector<MPI_Offset> startHZ{std::max(timestep,0),0,0};
    std::vector<MPI_Offset> stopHZ{1,dimY, dimX};

    std::vector<double> x_coord(dimX);
    std::vector<double> y_coord(dimY);
    std::vector<double> z_coord(dimY*dimX), h(dimY*dimX);

    const NcmpiVar xVar = ncFile.getVar("x");
    if (xVar.isNull()) {
        sendError("Error with dimension: X not found");
        return nullptr;
    }
    const NcmpiVar yVar = ncFile.getVar("y");
    const NcmpiVar zVar = ncFile.getVar("h+z");    
    const NcmpiVar hVar = ncFile.getVar("h");

    xVar.getVar_all(start,stopX,x_coord.data());
    yVar.getVar_all(start,stopY,y_coord.data());
    zVar.getVar_all(startHZ,stopHZ,z_coord.data());
    hVar.getVar_all(startHZ,stopHZ,h.data());

    size_t nVertices = dimX*dimY;
    // size_t nFaces = (dimX-1)*(dimY-1);
    // size_t nCorners = nFaces*4;
    size_t nFaces = (dimX-1)*(dimY-1)*2;
    size_t nCorners = nFaces*6;

    int nonZero = 0;
    int nonZeroVertices = 0;
    std::vector<int> nonZeroVerticesVec(dimX * dimY, -1);
    for (int j = 0; j<(dimY-1); ++j) {
        for (int i = 0; i<(dimX-1); ++i) {
            if (cellIsWater(h,i,j,dimX,dimY)) {
                ++nonZero;
                for (int i: {j * dimX + i, (j+1) * dimX + i, j * dimX + i + 1, (j + 1) * dimX + i + 1}) {
                    if (nonZeroVerticesVec[i] == -1) {
                        nonZeroVerticesVec[i] = nonZeroVertices;
                        ++nonZeroVertices;
                    }
                }
            }
        }
    }

    //int nonZero = std::count_if(h.begin(),h.end(),[](double i) { return i > 0.; });
    /* int nonZeroVertices = nonZero; */
    int nonZeroCorners = nonZero*6;

    // Polygons::ptr polygons(new Polygons(nFaces,nCorners,nVertices));
    Triangles::ptr polygons(new Triangles(nonZeroCorners, nonZeroVertices));
    // Triangles::ptr polygons(new Triangles(nCorners,nVertices));
    auto ptrOnXcoords = polygons->x().data();
    auto ptrOnYcoords = polygons->y().data();
    auto ptrOnZcoords = polygons->z().data();

    // auto ptrOnEl = polygons->el().data();
    auto ptrOnCl = polygons->cl().data();

    // Vec<Scalar>::ptr dataObj(new Vec<Scalar>(nVertices));
    // dataObj->setMapping(DataBase::Vertex);
    // Scalar *ptrOnScalarData = dataObj->x().data();
    // dataObj->setGrid(polygons);
    // dataObj->setBlock(block);
    // dataObj->addAttribute("_species", "level");
    // dataObj->setTimestep(timestep);
    // dataObj->setNumTimesteps(numTimesteps());

    polygons->setTimestep(timestep);
    polygons->setBlock(block);
    polygons->setNumTimesteps(numTimesteps());

    std::vector<int> localIdx(dimX*dimY);
    std::fill(localIdx.begin(),localIdx.end(),0);

    //fill coordinates, but only for vertices with water
    int idxAll = 0, count = 0;
    float h_idx = 0;
    for (int i = 0; i < nonZeroVerticesVec.size(); ++i) {
        int idx = nonZeroVerticesVec[i];
        if (idx >= 0) {
            auto x = i % dimX;
            auto y = i / dimX;

            ptrOnXcoords[idx] = x_coord[x];
            ptrOnYcoords[idx] = y_coord[y];
            ptrOnZcoords[idx] = z_coord[i]; 
        }
    }

    /* for (int j = 0; j < dimY; ++j) { */
    /*     for (int i = 0; i < dimX; ++i) { */
    /*         idxAll = j*dimX + i; */
    /*         h_idx = z_coord[idxAll]; */
    /*         if (h[idxAll]>0) { */
    /*             ptrOnXcoords[count] = x_coord[i]; */
    /*             ptrOnYcoords[count] = y_coord[j]; */
    /*             ptrOnZcoords[count] = h_idx; */ 
    /*             localIdx[idxAll] = count ; */
    /*             count++; */
    /*         //ptrOnScalarData[idxAll] = h_idx ; */
    /*         // if (h > 0) { */
    /*         //     ptrOnXcoords[count] = x_coord[i]; */
    /*         //     ptrOnYcoords[count] = y_coord[j]; */
    /*         //     ptrOnZcoords[count] = h_idx; */ 
    /*         //     ptrOnScalarData[count] = h_idx ; */
    /*         //     count++; */
    /*         // } */
    /*         } */
    /*     } */
    /* } */

    //create triangle faces for water cells
    // Index currentFace = 0;
    int currentConnection = 0, currentElem = 0;
    for (int j = 0; j<(dimY-1); ++j) {
        for (int i = 0; i<(dimX-1); ++i) {
            if (cellIsWater(h,i,j,dimX,dimY)) {
                ptrOnCl[currentConnection++] = nonZeroVerticesVec[i+(dimX*j)];
                ptrOnCl[currentConnection++] = nonZeroVerticesVec[(i+1)+(dimX*j)];
                ptrOnCl[currentConnection++] = nonZeroVerticesVec[(i+1)+((j+1)*dimX)];

                ptrOnCl[currentConnection++] = nonZeroVerticesVec[i+(dimX*j)];
                ptrOnCl[currentConnection++] = nonZeroVerticesVec[(i+1)+(((j+1)*dimX))];
                ptrOnCl[currentConnection++] = nonZeroVerticesVec[i+((j+1)*dimX)];

                /* // ptrOnEl[currentFace++] = currentConnection; */
                /* ptrOnCl[currentConnection++] = localIdx[i+(dimX*j)]; */
                /* ptrOnCl[currentConnection++] = localIdx[(i+1)+(dimX*j)]; */
                /* ptrOnCl[currentConnection++] = localIdx[(i+1)+((j+1)*dimX)]; */
                /* // ptrOnScalarData[currentElem++] = z_coord[j*dimX + i]; */

                /* // ptrOnEl[currentFace++] = currentConnection; */
                /* ptrOnCl[currentConnection++] = localIdx[i+(dimX*j)]; */
                /* ptrOnCl[currentConnection++] = localIdx[(i+1)+(((j+1)*dimX))]; */
                /* ptrOnCl[currentConnection++] = localIdx[i+((j+1)*dimX)]; */
                /* // ptrOnScalarData[currentElem++] = z_coord[j*dimX + i]; */
            }
        }
    }
    //ptrOnEl[currentFace] = currentConnection;

    return polygons;
}


// Object::ptr ReadDuisburg::generateLayerGrid(const NcmpiFile &ncFile, int timestep, int block) const {  

//     //initialize and get dimensions
    // int dimX, dimY;
    // if (!getDimensions(ncFile,dimX,dimY))
    //     return Object::ptr();

//     std::vector<MPI_Offset> start = {0};
//     std::vector<MPI_Offset> stopX{dimX};
//     std::vector<MPI_Offset> stopY{dimY};
//     std::vector<MPI_Offset> startHZ{std::max(timestep,0),0,0};
//     std::vector<MPI_Offset> stopHZ{1,dimY, dimX};

//     std::vector<double> x_coord(dimX);
//     std::vector<double> y_coord(dimY);
//     std::vector<double> z_coord(dimY*dimX);

//     const NcmpiVar xVar = ncFile.getVar("x");
//     if (xVar.isNull()) {
//         sendError("Error with dimension: X not found");
//         return nullptr;
//     }
//     const NcmpiVar yVar = ncFile.getVar("y");
//     const NcmpiVar zVar = ncFile.getVar("h+z");

//     xVar.getVar_all(start,stopX,x_coord.data());
//     yVar.getVar_all(start,stopY,y_coord.data());
//     zVar.getVar_all(startHZ,stopHZ,z_coord.data());

//     size_t nVertices = dimX*dimY;

//     //create grid object
//     LayerGrid::ptr grid(new LayerGrid(dimX,dimY,1));
//     grid->min()[0] = *x_coord.begin();
//     grid->min()[1] = *y_coord.begin();
//     auto z = grid->z().begin();  
//     grid->max()[0] = *(x_coord.end()-1);
//     grid->max()[1] = *(y_coord.end()-1);

//     //create scalar data object
//    // Vec<Scalar>::ptr dataObj(new Vec<Scalar>(nVertices));
//     //dataObj->setMapping(DataBase::Vertex);
//     //Scalar *ptrOnScalarData = dataObj->x().data();

//     //set meta data
//    // dataObj->setGrid(grid);
//     //dataObj->setBlock(block);
//    // dataObj->addAttribute("_species", "level");
//     //dataObj->setTimestep(timestep);
//     //dataObj->setNumTimesteps(numTimesteps());

//     grid->setTimestep(timestep);
//     grid->setBlock(block);
//     grid->setNumTimesteps(numTimesteps());
 
//     //fill timevarying values (h)
//     //TODO: replace this copy operation
//     Index idx = 0;
//     for (Index j = 0; j < dimY; ++j) {
//      for (Index i = 0; i < dimX; ++i) {
//         idx = j*dimX + i;
//         //ptrOnScalarData[idx] = z_coord[idx];
//         z[idx] = z_coord[idx];
//      }
//     }

//     return grid;
// }

bool ReadDuisburg::read(Reader::Token &token, int timestep, int block)
{
    Object::ptr grid;
     if (timestep < 0) return true;
    try{
        LOCK_NETCDF(*token.comm());
        printf("I am rank %i  at block %i at time %i", rank(), block,timestep);

        NcmpiFile ncGridFile(*token.comm(), m_gridFile->getValue().c_str(), NcmpiFile::read);

        //grid = generateLayerGrid(ncGridFile,timestep,block);
        grid = generateTriangleGrid(ncGridFile,timestep,block);
    
        if (grid) {
            token.applyMeta(grid);
            token.addObject("grid_out", grid);
        }
        
        UNLOCK_NETCDF(*token.comm());

    } catch (std::exception &ex) {
        std::cerr << "ReadDuisburg exception: " << ex.what() << std::endl;
    }
    
    return true;
}

bool ReadDuisburg::finishRead()
{
    return true;
}

MODULE_MAIN(ReadDuisburg)
