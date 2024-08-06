/**########################################################
 *
 *                AIA READER 
 *
 *########################################################
 *
 *
 * This file constitutes the reader which enables the 
 * communication with maia and retrieves the information
 * from maia, i.e., the full access to the cartesian-
 * grid is granted
 * 
 * This function is based in its orginal form on Micheals
 * concept. The reader was cleaned and heavily extended by
 * the multi solver approach
 * 
 * @author: Pascal S. Meysonnat
 * @date:   back in the 90
 * 
 * If you have any questions --> p.meysonnat@aia.rwth-aachen.de
 */


#ifndef MAIAVISREADER_H_
#define MAIAVISREADER_H_

// #define MAIA_DEBUG_OUTPUT
// #define MAIA_GLOBAL_VARIABLES_
// #define DEBUG_H
#include <algorithm>
#include <vector>
#include <string>
#include <bitset>
#include "mpi.h"
#include <iostream>

using namespace maiapv;
# define TRACE()
//static std::ostream& m_log = std::cerr;

#define MAIA_SWALLOW_EXCEPTIONS(RET_VAL)                         \
  catch (const std::exception& e) {                             \
    vtkErrorMacro("Exception thrown: " << e.what());            \
    std::cerr << "Exception thrown: " << e.what() << std::endl; \
    return RET_VAL;                                             \
  }

#ifndef NDEBUG
#define ASSERT_BOUNDS(i, lower, upper)                        \
  if (i < lower || i >= upper) {                              \
    mTerm(1, FUN_, "index " + to_string(i)   \
                + " out-of-bounds [" + to_string(lower) + ", " \
                + to_string(upper) + ")");                    \
  }
#else
#define ASSERT_BOUNDS(i, lower, upper)
#endif

//include vistle related header
#include <vistle/core/unstr.h>

using namespace std;

//include maia related header
#include "compiler_config.h"
#include "IO/parallelio.h"
#include "IO/parallelio_pnetcdf.h"
#include "DG/dgcartesianinterpolation.h"
#include "DG/sbpcartesianinterpolation.h"
#include "MEMORY/collector.h"
#include "FV/fvcartesiancellproperties.h"
#include "GRID/cartesiangridcellproperties.h"
#include "GRID/cartesiangridio.h"
#include "GRID/cartesiangrid.h"
#include "GRID/cartesiangridtree.h"
#include "GRID/cartesiangridproxy.h"
#include "typetraits.h"
#include "UTIL/debug.h"
#include "INCLUDE/maiamacro.h"

//include reader related header
#include "box.h"
#include "constants.h"
#include "dataset.h"
#include "block.h"
#include "filesystem.h"
#include "point.h"
#include "timers.h"
#include "timernames.h"
#include "../readerbase.h"

namespace maiapv {

template <MInt nDim>
  struct VisBlock{
    MString solverName;
    MInt solverId = 0;
    MBool isDgOnly;
    MBool isActive;
    MInt noCellsVisualized = 0;
    std::set<MInt> vertices;
    vector<MInt> vertexIds;
    vector<MInt> visCellIndices;
    MInt visCellHaloOffset = -1;
  };

  using namespace maiapv;

/**  Main class for building a VTK multi solver grid from Cartesiangrid 
 **  real dependency from maia!!!!!!!! 
 **  based on Mic's test plugin
 **  Improved "features" and complete overhoaled structure
 **
 **  @author: Pascal Meysonnat
 **  @date: back in the 90
 **
 */
template <MInt nDim> class Reader : public ReaderBase {
public:
  //constructor
  Reader(const MString& gridFileName, const MString& dataFileName, const MPI_Comm comm);

  //member funcitons
  //function to dermine the filetype and other important information (,e.g., dimensions, number of cells, bounding box)
  void determineFileType(const MString&, const MString&);
  //function setting the parallelization information (,e.g., the rank, the mpi size)
  void getParallelInfo();
  //function to return the MPI communicator (not really necessary but for the sake of similarity to maia it is used)
  MPI_Comm mpiComm() const { return m_mpiComm; }
  //function to return the MPI rank (not really necessary but for the sake of similarity to maia it is used)
  MInt domainId() const { return m_mpiRank; }
  //function to return the MPI size (not really necessary but for the sake of similarity to maia it is used)
  MInt noDomains() const { return m_mpiSize; }
  //function to return if the rank is the root process
  MBool isMpiRoot() const { return domainId() == 0; }
  //function to return the solver Id
  MInt getSolverId();
  //function to request the information of the datasets
   void requestInformation(vector<Dataset>& datasets);
   //function to set the visualization level information
   void setLevelInformation(int *levelRange);
   //function to return the bounding box from file
   void requestBoundingBoxInformation(double (&boundingBox)[6]);
   //function to return the time series information
   MFloat requestTimeInformation(); 
   //determine the levels up to which the visualization should take place
   void determineVisualizationLevelIndex(MInt* levels);
   //function to determine the file type and file information
   MBool requestFiletype();
   //function to return the memory increase proposed due to the average cell distribution
   double getMemoryIncrease();
   //function to set the memory increase for the average cell distribution
   void setMemoryIncrease(double increase);
   //function to return the number of solvers
   MInt requestNumberOfSolvers();
   //function to compute the number degrees of freedom (Dolce & Gabana)
   void computeDofDg();
   //function to compute the vertices
   void calcVertices();
   //function to read the grid
   vistle::UnstructuredGrid::ptr readGrid() override;

   //function to read data on the grid
   vistle::DataBase::ptr readData(const maiapv::Dataset& dataset) override;
   //function to read the datasets
   void read(const vector<Dataset>& datasets) override;
   //function to load the data from the solution file
   vistle::DataBase::ptr loadSolutionData(ParallelIo& datafile, const maiapv::Dataset& dataset);
   //function to load information of Lagrange particles
   void readParticleData(vistle::UnstructuredGrid::ptr, const vector<Dataset>&);
   //function to load additional data for the grid, e.g., levelId
   void loadGridAuxillaryData(vistle::UnstructuredGrid::ptr &grid, const MInt b);
   //function to build the VTK multi solver grid
   vistle::UnstructuredGrid::ptr buildVistleGrid();
   //function to init the interpolation (Dolce & Gabana)
   void initInterpolation(const MInt maxNoNodes1D);
   //function to return min/max point of the domain extent
   void determineDomainExtent(Point<nDim>& min, Point<nDim>& max);
   //function to add data to the cells
   template <typename T>
   void addCellData(const MString& name, const T* data, vistle::UnstructuredGrid::ptr &grid, MInt solverId,
                    const MBool solverLocalData = false);

  private:
    long m_noCells =-1;
    vector<long>m_noCellsBySolver{};
    MString m_gridFileName{};
    MString m_dataFileName{};
    MBool m_isParticleFile = false;
    MBool m_isDataFile = false;
    MBool m_isDgFile = false;
    MFloat m_boundingBox[2*nDim];
    MPI_Comm m_mpiComm = MPI_COMM_NULL;
    int m_mpiRank=-1;
    int m_mpiSize=-1;
    MLong m_32BitOffset=0;
    //solver related
    MInt m_noSolvers=-1;
    MInt m_solverId=-1;
    maiapv::SolverPV m_solver{};
    maiapv::VisBlock<nDim> m_visBlock{};

   // cell-related
   vector<MInt> m_visCellIndices{};
   const MInt* const m_binaryId = (nDim == 2) ? binaryId2D : binaryId3D;
   MBool m_useHaloCells = false;
   MInt m_noHaloLevels=0;
   //grids
   std::vector<vistle::UnstructuredGrid::ptr> m_vistleGrids;
   std::unique_ptr<CartesianGrid<nDim>> m_maiagrid;

   // Timer-related
   Timers timers;
   array<MInt, TimerNames::_count> m_timers{};

   //memory related
   MFloat m_memIncrease=-1.0;

   //visualization related
   //MInt m_noDOFsVisualized = -1;
   MInt m_visType = VisType::ClassicCells;
   MFloat m_visualizationBoxMinMax[2*nDim];
   MBool m_useVisualizationBox = false;
   vector<Dataset> m_datasets{};
   //DG related stuff
   MInt  m_offsetLocalDG = -1;
   MInt  m_dofDg[2]={-1,-1};
   MString m_intMethod{};
   MString m_polyType{};
   MInt* m_polyDeg= nullptr;
   MInt* m_noNodes1D = nullptr;
   vector<DgInterpolation> m_interp{};
   MInt m_noVisNodes = -1;
   MBool m_visualizeSbpData = false;
   MInt m_visLevels[2]={-1,0};
   MBool m_leafCellsOn = true;
   vector<vector<MFloat>> m_vandermonde{};
   std::vector<std::unique_ptr<maia::grid::Proxy<nDim>>> m_maiaproxys;
   vector<Point<nDim>> m_vertices;
  void resetTimers();

};

/** \brief Constructor of the reading class
 *
 * @author Pascal S. Meysonnat
 * @date back in the 90
 *  
 */
template <MInt nDim>
  Reader<nDim>::Reader(const MString& gridFileName, const MString& dataFileName, const MPI_Comm comm)
  : m_noCells(0),
    m_mpiComm(comm)
    {

    
    TRACE();
    m_useVisualizationBox =false;
    // get MPI rank and size and encapsulate them in function
    getParallelInfo();
    
    m_useHaloCells = (noDomains() > 1) ? true : false;
    if(m_useHaloCells){m_noHaloLevels=1;}
    determineFileType(gridFileName, dataFileName);
    m_solver = maiapv::SolverPV{"Solver" + std::to_string(m_solverId + 1), true, false, m_solverId};
    m_visBlock = maiapv::VisBlock<nDim>{m_solver.solverName, m_solver.solverId, m_solver.isDgOnly, true, -1};
    

  }

/** \brief Function to determine the filetype and extract basic information
 *
 * @author Pascal S. Meysonnat
 * @date back in the 90
 *  
 */
template <MInt nDim>
void Reader<nDim>::determineFileType(const MString& gridFileName, const MString& dataFileName) {
  TRACE();
  // Extract path and filename from full String
  m_gridFileName = gridFileName;
  m_dataFileName = dataFileName;
  if(!dataFileName.empty()){
    //we have a datafile 
    m_isDataFile=true;
  }else{
    m_isDataFile = false;
  }
  if(gridFileName.empty()){
    if(!dataFileName.empty()) {
      m_isParticleFile = true;
      m_isDataFile = false;
    } else {
      mTerm(1, FUN_, "Error no gridfile is known");
    }
  }


  // Particle files have differnet format. They do not have a grid.
  if ( m_isParticleFile ) {
    m_noSolvers = -1;
    m_solverId = -1;
    for ( MInt d=0; d<2*nDim; d++ ) {
      m_boundingBox[d] = F0;
    }
    return;
  }

  ParallelIo gridfile(m_gridFileName, maia::parallel_io::PIO_READ, mpiComm());
  gridfile.getAttribute(&m_noCells,"noCells");
  //according to the number of cells/core propose a memory increase
  m_memIncrease=1.0;
  MFloat noCellsPerCore=m_noCells/noDomains();
  if(noCellsPerCore > 50000000){
    m_memIncrease=1.1;
  } else if (noCellsPerCore > 10000000 && noCellsPerCore <= 50000000 ){
    m_memIncrease=1.25;
  } else if (noCellsPerCore >2000000 && noCellsPerCore <= 10000000){
    m_memIncrease =1.4;
  } else if (noCellsPerCore <=2000000){
    m_memIncrease =1.6;
  }
  if(noDomains()==1){ m_memIncrease=1.0;}




  if(gridfile.hasAttribute("bitOffset")){gridfile.getAttribute(&m_32BitOffset, "bitOffset");}
  //read the bounding box
  gridfile.getAttribute(m_boundingBox, "boundingBox", 2*nDim);

  // TODO add support for multisolver bounding box
  MFloat centerOfGravity[3];
  gridfile.getAttribute(&centerOfGravity[0], "centerOfGravity", nDim);
  for (MInt i = 0; i < nDim; i++) {
    const MFloat cog = 0.5 * (m_boundingBox[nDim + i] + m_boundingBox[i]);
    const MFloat eps = 1e-8;
    if (centerOfGravity[i] > cog + eps || centerOfGravity[i] < cog - eps) {
      std::cerr << "WARNING: center of gravity mismatch " << centerOfGravity[i] << " " << cog
                << std::endl;
    }
  }

  //get the number of solvers in the file 
  gridfile.getAttribute(&m_noSolvers, "noSolvers");
  if (!gridfile.hasAttribute("noPartitionCells")) {
    mTerm(1, FUN_, "This reader does not support older MAIA file formats");
  }

  if(m_isDataFile){
    ParallelIo datafile(m_dataFileName, maia::parallel_io::PIO_READ, mpiComm());
    if(datafile.hasAttribute("solverId") && m_noSolvers >1){
      datafile.getAttribute(&m_solverId, "solverId");
    }else if (m_noSolvers == 1){
      m_solverId=0;
    }else{
      mTerm(-1, FUN_, "ERROR: We have a multi solver grid but no solver id in solution file");
    }

    // Check if data file is DG file
    m_isDgFile = datafile.hasDataset("polyDegs");
  }

  //check for the cells in each solver (except DG)
  if (m_isDataFile && m_noSolvers > 1 && !m_isDgFile) {
    ParallelIo datafile(m_dataFileName, maia::parallel_io::PIO_READ, mpiComm());
    for (MInt b = 0; b < m_noSolvers; b++) {
      long noCells = 0;

      stringstream solverName;
      solverName << b;
      string noCellName = "noCells_" + solverName.str();
      if (gridfile.hasAttribute(noCellName)) {
        gridfile.getAttribute(&noCells, noCellName);
      } else if (b == m_solverId) {
        // Get the number of cells from the datafile for the selected solver
        // TODO fix this for the visualization of multiple solvers
        datafile.getAttribute(&noCells, "noCells");
      }

      m_noCellsBySolver.push_back(noCells);
    }

    // Note: this assumes the number of cells for each solver is stored in the grid file, which is
    // only true if multisolverGrid is enabled during grid generation
    //for(MInt b=0; b<m_noSolvers; b++){
    //  stringstream solverName;
    //  solverName << b;
    //  string noCellName="noCells_"+solverName.str();
    //  long noCells=0;
    //  gridfile.getAttribute(&noCells,noCellName);
    //  m_noCellsBySolver.push_back(noCells);
    //}
  } else if (m_isDgFile) {
    m_noCellsBySolver.push_back(-1);
  } else {
    m_noCellsBySolver.push_back(m_noCells);
  }
}


/** \brief Function to return the number of solvers (single/multi solver =1/>1)
 *
 * @author Pascal S. Meysonnat
 * @date back in the 90
 *  
 */
template <MInt nDim>
  MInt Reader<nDim>::requestNumberOfSolvers(){
  return m_noSolvers;
}

/** \brief Function to return the dataset names in the file 
 *
 * @author Pascal S. Meysonnat, Michael Schlottke-Lakemper
 * @date back in the 90, 
 *
 * Comment: from old plugin
 *  
 */
template <MInt nDim>
void  Reader<nDim>::requestInformation(vector<Dataset>& datasets){
  MBool datasetsAvailable = (datasets.size()>0) ? 1:0;
  if(m_isDataFile){
    ParallelIo datafile(m_dataFileName, maia::parallel_io::PIO_READ, mpiComm());
    if(!datasetsAvailable){
      datasets.clear();
      MInt datasetSize = 0;
      // Determine the required size of the variables
      if (m_isDgFile) {
        // Use the size of the first dataset as the required size for DG cases
        // to circumvent the use/requirement of the polyDegs array
        if (datafile.hasDataset("variables0")) {
          datasetSize = datafile.getArraySize("variables0");
        } else {
          TERMM(1,"Dataset 'variables0' doesn't exits and therefore the required size cannot be determined.");
        }
      } else {
        datasetSize=m_noCellsBySolver.at(m_solverId);
      }
      for (auto&& variableName : datafile.getDatasetNames()) {
        // Check if the dataset is an array or scalar
        if (datafile.getDatasetNoDims(variableName) > 0) {
          if (datafile.getArraySize(variableName) == datasetSize) {
            // Query the actual variable name and save both names to the
            // respective vectors
            MString realName;
            if (datafile.hasAttribute("name", variableName)) {
              datafile.getAttribute(&realName, "name", variableName);
            } else {
              realName = variableName;
            }
            // Just tick the checkboxes for variablesN
            MBool status = false;
            if (variableName.find("variables") != MString::npos) {
              status = true;
            }
            datasets.push_back(
              {variableName, realName, status, false, false});
          }
        }
      }
    }
  } else if (m_isParticleFile) {
    ParallelIo partFile(m_dataFileName, maia::parallel_io::PIO_READ, mpiComm());
    if(!datasetsAvailable){
      datasets.clear();
      MLong datasetSize = 0;
      if ( partFile.hasDataset("partDia") ) {
        datasetSize = partFile.getArraySize("partDia");
      } else {
        TERMM(1,"Dataset 'partDia' doesn't exits and therefore the required size cannot be determined.");
      }
      for (auto&& variableName : partFile.getDatasetNames()) {
        // Check if the dataset is an array or scalar
        if (partFile.getDatasetNoDims(variableName) == 1) {
          if (partFile.getArraySize(variableName) == datasetSize ||
              partFile.getArraySize(variableName) == nDim*datasetSize) {
            // Just tick the checkboxes for variablesN
            MBool status = true;
            datasets.push_back(
              {variableName, variableName, status, false, false});
          }
        }
      }
    }
    return; // Nothing further to be done for particles
  }

  // Only load grid dataset information if not yet present
  if (!datasetsAvailable) {
    // Add grid datasets
    for (auto&& dataset : gridsets) {
      // Skip DG datasets for non-DG files
      if (!m_isDgFile && dataset.isDgOnly) {
        continue;
      }
      // Otherwise, add grid dataset to list of available datasets
      datasets.push_back(dataset);
      // When loading data file, grid datasets unchecked by default.
      if (m_isDataFile) {
        datasets.back().status = false;
      }
    }
  }
}

/** \brief Function to return the bounding box values
 *
 * @author Pascal S. Meysonnat
 * @date back in the 90
 *  
 */  
template <MInt nDim>
void Reader<nDim>::requestBoundingBoxInformation(double (&boundingBox)[6]){
  for(MInt i=0; i<2*nDim; i++) boundingBox[i]=m_boundingBox[i];
}

/** \brief Function to return the time series information
 *
 * @author Pascal S. Meysonnat, Michael Schlottke-Lakemper
 * @date back in the 90, 
 *
 * Comment: from old plugin
 *  
 */

template <MInt nDim>
MFloat Reader<nDim>::requestTimeInformation(){
  ParallelIo datafile(m_dataFileName, maia::parallel_io::PIO_READ, mpiComm());
  MFloat time=0.000000;
  if (datafile.hasAttribute("time")) {
    datafile.getAttribute(&time, "time");
  } else if (datafile.hasDataset("time")) {
    datafile.readScalar(&time, "time");
  } else {
    time = -std::numeric_limits<MFloat>::infinity();
  }
  return time;
}

/** \brief Function to return the min/max level in the file
 *
 * @author Pascal S. Meysonnat
 * @date back in the 90
 *  
 */  
template <MInt nDim>
void Reader<nDim>::setLevelInformation(int* levelRange){

  // Particles do not have a gird
  if ( m_isParticleFile ) return;
  
  ParallelIo gridfile(m_gridFileName, maia::parallel_io::PIO_READ, mpiComm());\
  //get the min level
  if (gridfile.hasAttribute("minLevel")) {
    int a=0;
    gridfile.getAttribute(&a, "minLevel");
    levelRange[0]=(double)a;
  }
  //get the max level
  if (gridfile.hasAttribute("maxLevel")) {
    int a=0;
    gridfile.getAttribute(&a, "maxLevel");
    levelRange[1]=(double)a;
  }
}

/** \brief Retrunt the proposed memory increase determined from average cells
 *
 * @author Pascal S. Meysonnat
 * @date back in the 90
 *  
 */
template <MInt nDim>
MFloat Reader<nDim>::getMemoryIncrease(){
  return m_memIncrease;
}

/** \brief Set memory increase from average cels
 *
 * @author Pascal S. Meysonnat
 * @date back in the 90
 *  
 */
template <MInt nDim>
void Reader<nDim>::setMemoryIncrease(double increase){
  m_memIncrease=increase;
}




/** \brief Function to the visualization information
 *
 * @author Pascal S. Meysonnat
 * @date back in the 90
 *  
 */  
template <MInt nDim>
MBool Reader<nDim>::requestFiletype(){
  return m_isDgFile;
}




/** \brief Function to return the solver id (e.g., the solver id in multi solver grids)
 *
 * @author Pascal S. Meysonnat
 * @date back in the 90
 *  
 */  
template <MInt nDim>
  MInt Reader<nDim>::getSolverId(){
  return m_solverId;
}


/** \brief Get the rank of the processor and size of the parallel communicator
 **
 ** @author Pascal Meysonnat, Michael Schlottke-Lakemper
 * @date back in the 90
 **
 */
template <MInt nDim> void Reader<nDim>::getParallelInfo() {
  TRACE();

  MPI_Comm_rank(mpiComm(), &m_mpiRank);
  MPI_Comm_size(mpiComm(), &m_mpiSize);
}

/** \brief Function to compute the degrees of freedom (Dolce and Gabana)
 **
 ** @author Pascal S. Meysonnat, Ansgar Niemoeller, Michael Schlottke-Lakemper
 ** @date back in the 90
 **
 */
template <MInt nDim>
  void Reader<nDim>::computeDofDg() {
  if(m_isDgFile){
    // Exchange polyDegs of halo/window cells --> Ansgar Niemoeller
    // Note: this function should only be called by the active ranks for this solver!
    std::cerr << "exchange polynomial degrees of window/halo cells" << std::endl;
   m_maiaproxys[m_solverId]->exchangeHaloCellsForVisualization(&m_polyDeg[0]);

    if(m_visualizeSbpData){
      std::cerr << "exchange number of nodes of window/halo cells" << std::endl;
     m_maiaproxys[m_solverId]->exchangeHaloCellsForVisualization(&m_noNodes1D[0]);
    }

    m_dofDg[0] = 0;
    for (MInt i = 0; i <m_maiaproxys[m_solverId]->noInternalCells(); i++) {
      const MInt noNodes1D = m_visualizeSbpData ? m_noNodes1D[i] : m_polyDeg[i]+1; 
      m_dofDg[0] += ipow(noNodes1D, nDim);
    }
    m_dofDg[1] = m_dofDg[0];
    for (MInt i =m_maiaproxys[m_solverId]->noInternalCells(); i <m_maiaproxys[m_solverId]->noCells();
         i++) {
      const MInt noNodes1D = m_visualizeSbpData ? m_noNodes1D[i] : m_polyDeg[i]+1;
      m_dofDg[1] += ipow(noNodes1D, nDim);
    }
    // Calculate offset for DOFs
    MPI_Exscan(&m_dofDg[0], &m_offsetLocalDG, 1, maia::type_traits<MInt>::mpiType(), MPI_SUM,
              m_maiaproxys[m_solverId]->mpiComm());
    if (m_maiaproxys[m_solverId]->domainId() == 0) {
      m_offsetLocalDG = 0;
    }
    // Calculate DOFs of visualized cells // unused
    //m_noDOFsVisualized = 0;
    //for (size_t i = 0; i < m_visCellIndices.size(); i++) {
    //  const MInt cellId = m_visCellIndices.at(i);
    //  m_noDOFsVisualized += ipow(m_polyDeg[cellId] + 1, nDim);
    //}
  }
}

/** \brief Store datasets and call main read() method.
 **
 ** @author Michael Schlottke, Pascal S. Meysonnat 
 ** @date back in the 90
 **
 ** \param.at(in) datasets List of available datasets with information on whether
 **                     they should be loaded.
 */

template <MInt nDim>
void Reader<nDim>::read(const vector<Dataset>& datasets) {
  TRACE();

  // Create timer for the measurements
  m_timers.at(TimerNames::read) = timers.create("Read");

  m_timers.at(TimerNames::readGridFile)
    = timers.create("ReadGridFile", m_timers.at(TimerNames::read));

  m_timers.at(TimerNames::buildGrid)
    = timers.create("BuildGrid", m_timers.at(TimerNames::read));
  m_timers.at(TimerNames::calcDOFs)
    = timers.create("CalcDOFs", m_timers.at(TimerNames::buildGrid));
  m_timers.at(TimerNames::buildVtkGrid)
    = timers.create("BuildVtkGrid", m_timers.at(TimerNames::buildGrid));
  m_timers.at(TimerNames::loadGridData)
    = timers.create("LoadGridData", m_timers.at(TimerNames::buildGrid));
  m_timers.at(TimerNames::loadSolutionData)
    = timers.create("LoadSolutionData", m_timers.at(TimerNames::read));

  timers.start(m_timers.at(TimerNames::read));

  m_datasets = datasets;
  // If this is a data file, also load data, e.g., the solution data
  if (m_isDataFile) {
    //check if the number of solvers is the same as in the whole vector
    //for the moment the solution can only be read if every solver is present

    ParallelIo datafile(m_dataFileName, maia::parallel_io::PIO_READ, mpiComm());
    timers.start(m_timers.at(TimerNames::loadSolutionData));
    //load the solution data
    for(auto&& dataset : datasets){
      if(dataset.status){
        loadSolutionData(datafile, dataset);
      }
    }
    timers.stop(m_timers.at(TimerNames::loadSolutionData));
  }
  timers.print(mpiComm());
}

template <MInt nDim> 
void Reader<nDim>::resetTimers()
{
  m_timers.at(TimerNames::read) = timers.create("Read");

  m_timers.at(TimerNames::readGridFile)
    = timers.create("ReadGridFile", m_timers.at(TimerNames::read));

  m_timers.at(TimerNames::buildGrid)
    = timers.create("BuildGrid", m_timers.at(TimerNames::read));
  m_timers.at(TimerNames::calcDOFs)
    = timers.create("CalcDOFs", m_timers.at(TimerNames::buildGrid));
  m_timers.at(TimerNames::buildVtkGrid)
    = timers.create("BuildVtkGrid", m_timers.at(TimerNames::buildGrid));
  m_timers.at(TimerNames::loadGridData)
    = timers.create("LoadGridData", m_timers.at(TimerNames::buildGrid));
  m_timers.at(TimerNames::loadSolutionData)
    = timers.create("LoadSolutionData", m_timers.at(TimerNames::read));
}

/** \brief Main method reading in the data
 **  based on Michaels test paraview plugin
 **  now real dependency on MAIA 
 **  and new features
 **
 **  @author Pascal Meysonnat
 **  @date back in the 90
 **
 */
template <MInt nDim> vistle::UnstructuredGrid::ptr Reader<nDim>::readGrid() {
  TRACE();
  resetTimers();

  // initialize cell collector:
  
  // Read grid file
  timers.start(m_timers.at(TimerNames::readGridFile));

  //we need to create a cartesian grid
  (m_noSolvers > 1) ? g_multiSolverGrid = 1 : g_multiSolverGrid = 0;

  //create a cartesian grid instance
  m_maiagrid = std::make_unique<CartesianGrid<nDim>>((MInt)m_noCells, m_boundingBox, mpiComm(), m_gridFileName);
  m_maiaproxys.clear();
  for(MInt b=0; b<m_noSolvers; b++){
    m_maiaproxys.emplace_back(std::make_unique<maia::grid::Proxy<nDim>>(b, *m_maiagrid));
  }
  m_visBlock.isActive = m_maiaproxys[m_solverId]->isActive();
  //m_cells_ = m_maiagrid->tree(); //original tree should be replaced by proxy
  timers.stop(m_timers.at(TimerNames::readGridFile));

  // Build up grid structures with VTK
  timers.start(m_timers.at(TimerNames::buildGrid));

  // If this is a data file with DG data, we first need to read the polynomial degree info
  if (m_isDgFile) {
    ParallelIo datafile(m_dataFileName, maia::parallel_io::PIO_READ, mpiComm());
    datafile.getAttribute(&m_intMethod, "dgIntegrationMethod", "polyDegs");
    datafile.getAttribute(&m_polyType, "dgPolynomialType", "polyDegs");

    const MInt isActive = m_maiaproxys[m_solverId]->isActive();
    const MInt noCells = (isActive) ?m_maiaproxys[m_solverId]->noCells() : 0;
    const MInt noInternalCells = (isActive) ?m_maiaproxys[m_solverId]->noInternalCells() : 0;
    const MInt solverDomainId = (isActive) ?m_maiaproxys[m_solverId]->domainId() : -1;
    const MInt offset = (isActive) ?m_maiaproxys[m_solverId]->domainOffset(solverDomainId) : 0;

    mAlloc(m_polyDeg, std::max(noCells, 1), "dgPolynomialDeg", -1, AT_);
    datafile.setOffset(noInternalCells, offset);
    MUcharScratchSpace polyDegs(std::max(noInternalCells, 1), AT_, "polyDegs");
    datafile.readArray(polyDegs.data(), "polyDegs");
    std::copy_n(polyDegs.data(), noInternalCells, m_polyDeg);

    // Additioanlly read noNodes in sbpMode
    if(m_visualizeSbpData){
      mAlloc(m_noNodes1D, std::max(noCells, 1), "dgNoNodes1D", -1, AT_);
      datafile.setOffset(noInternalCells, offset);
      MUcharScratchSpace noNodes1D(std::max(noInternalCells, 1), AT_, "noNodes1D");
      datafile.readArray(noNodes1D.data(), "noNodes1D");
      std::copy_n(noNodes1D.data(), noInternalCells, m_noNodes1D);
    }
  }

  auto grid = buildVistleGrid();
  timers.stop(m_timers.at(TimerNames::buildGrid));
  return grid;
  
}

template <MInt nDim> vistle::DataBase::ptr Reader<nDim>::readData(const maiapv::Dataset& dataset){
  // If this is a data file, also load data, e.g., the solution data
  vistle::DataBase::ptr data;
  if (m_isDataFile) {
    //check if the number of solvers is the same as in the whole vector
    //for the moment the solution can only be read if every solver is present

    ParallelIo datafile(m_dataFileName, maia::parallel_io::PIO_READ, mpiComm());
    timers.start(m_timers.at(TimerNames::loadSolutionData));
    //load the solution data
    data = loadSolutionData(datafile, dataset);
    timers.stop(m_timers.at(TimerNames::loadSolutionData));
  }
  return data;
}


template <MInt nDim> vistle::UnstructuredGrid::ptr Reader<nDim>::buildVistleGrid() {
  TRACE();

  
  //MOVE THIS FUNCTION TO other part 

  // Determine which cells are to be visualized (,e.g., cells without child cells)
  // that is for every solver you get cellIds to be visualized
  determineVisualizationLevelIndex(m_visLevels); 

  // Initialize interpolation for DG files
  if (m_isDgFile) {
    // TODO fixme for visualization of multiple DG solvers
      // Exchange the halo-cell polynomial degrees and compute the degrees of freedom
    computeDofDg();

    // Determine the maximum (local) polynomial degree (including halo cells) for interpolation
    const MInt maxPolyDeg = *max_element(
        &m_polyDeg[0], &m_polyDeg[0] + m_maiaproxys[m_solverId]->noCells());
    // Set for DG case
    MInt maxNoNodes1D = maxPolyDeg + 1;
    // Correct for SBP mode
    if(m_visualizeSbpData){
      maxNoNodes1D = *max_element(
        &m_noNodes1D[0], &m_noNodes1D[0] + m_maiaproxys[m_solverId]->noCells());
    }
    initInterpolation(maxNoNodes1D);

  }

  timers.start(m_timers.at(TimerNames::buildVtkGrid));
  // building the vtkGrid
  // Calculate the vertices (cell corner points) and vertex ids
  calcVertices();//vertices, vertexIds);
  
  const MInt noVertices = m_visBlock.vertices.size();

  //calculate numCorners
  vistle::Index numCorners = 0, numElements = m_visBlock.noCellsVisualized;
  
  for (vistle::Index i = 0; i < numElements; i++) {
    // Create cell pointer and appropriate cell type, calculate number of
    // vertices per cell
    numCorners += IPOW2(nDim);
  }

  //build the unstructured grid
  vistle::UnstructuredGrid::ptr localUnstructuredGrid = vistle::make_ptr<vistle::UnstructuredGrid>(numElements ,numCorners, noVertices);
  std::array<vistle::shm_array<float, vistle::shm_allocator<float>>*, 3> points = {&localUnstructuredGrid->x(), &localUnstructuredGrid->y(), &localUnstructuredGrid->z()};
  //set the points of the grid
  MInt cnt = 0;
  map<MInt, MInt> vertexMapping;
  for (auto it=m_visBlock.vertices.begin(); it != m_visBlock.vertices.end(); ++it) {
    for(MInt d=0; d<nDim; d++){
      (*points[d])[cnt] = m_vertices.at(*it)[d];
    }
    vertexMapping.insert({*it, cnt});
    cnt++;
  }  
  // Loop over all cells that are visualized
  MInt vertexId = 0;
  size_t elementPos = 0;
  for (vistle::Index i = 0; i < numElements; i++) {
    // Create cell pointer and appropriate cell type, calculate number of
    // vertices per cell
    MInt noVerticesPerCell = IPOW2(nDim);

    // Assign points to cell
    for (MInt j = 0; j < noVerticesPerCell; j++) {
      constexpr vistle::Index vtkVoxelOrder[] = {0, 1, 3, 2, 4, 5, 7, 6};
      vertexId = vertexMapping[m_visBlock.vertexIds[vtkVoxelOrder[j] + elementPos]];
      localUnstructuredGrid->cl()[j + elementPos] = vertexId;
    }
    localUnstructuredGrid->tl()[i] = nDim == 2 ? vistle::UnstructuredGrid::QUAD : vistle::UnstructuredGrid::HEXAHEDRON;
    localUnstructuredGrid->el()[i] = elementPos;
    elementPos += noVerticesPerCell;
  }
  localUnstructuredGrid->el()[numElements] = elementPos;

  //add ghost cell data necessary for proper visualization of iso contours
  if (m_useHaloCells) {
    for (MInt i = 0; i < (MInt)m_visBlock.visCellIndices.size(); i++) {
      const MInt cellId = m_visBlock.visCellIndices.at(i);
      if (m_maiaproxys[m_solverId]->tree().hasProperty(cellId, GridCell::IsHalo)) {
        localUnstructuredGrid->ghost()[cellId] =  vistle::cell::GHOST;
      }
    }

  }
  timers.stop(m_timers.at(TimerNames::buildVtkGrid));
  
  timers.start(m_timers.at(TimerNames::loadGridData));
  // Add all grid-specific data to VTK grid
  loadGridAuxillaryData(localUnstructuredGrid, m_solverId);
  timers.stop(m_timers.at(TimerNames::loadGridData));
    
  return localUnstructuredGrid;
}


/** /brief Determine the cellIndices of the cells to visualize
 **
 ** @author Pascal Meysonnat
 ** @date back in the 90
 **
 **
 */
template <MInt nDim> void Reader<nDim>::determineVisualizationLevelIndex(MInt* visLevels /*=-1*/) {
  TRACE();

  
  m_visBlock.visCellIndices.reserve(m_maiaproxys[m_solverId]->noCells());

  //set the min and max visualization levels
  const MInt minVisLevel = visLevels[0];
  const MInt maxVisLevel = visLevels[1];

  // Visualization box
  Point<nDim> min;
  Point<nDim> max;
  if (m_useVisualizationBox) {
    for (MInt d = 0; d < nDim; d++) {
      min.at(d) = m_visualizationBoxMinMax[d];
      max.at(d) = m_visualizationBoxMinMax[nDim + d];
    }
  } else {
    min.fill(std::numeric_limits<MFloat>::infinity());
    max.fill(std::numeric_limits<MFloat>::infinity());
  }
  Box<nDim> visualizationBox(min, max);

  //std::cerr << "determine visualization cells: useBox=" << m_useVisualizationBox
  //          << ", levelRange=" << (minVisLevel != -1)
  //          << ", leafCells=" << m_leafCellsOn << std::endl;

  MBool onlyVisGridLeafCells = false;
  if ( !m_isDataFile ) onlyVisGridLeafCells = true;

  std::vector<MInt> isVisCell(m_maiaproxys[m_solverId]->noCells(), 0);
  for(MInt b=0; b<m_noSolvers;b++){
    MInt solverId = b;

    for (MInt i = 0; i <m_maiaproxys[solverId]->noInternalCells(); i++) {
      MBool vis = true;

      // Check for visualization box
      if (m_useVisualizationBox) {
        Point<nDim> p;
        for (MInt d = 0; d < nDim; d++) {
          p.at(d) =m_maiaproxys[solverId]->tree().coordinate(i, d);
        }

        // Cell is outside the visualization box
        if (!visualizationBox.isPointInBox(p)) {
          vis = false;
        }
      }

      // Check for visualization level range
      if (minVisLevel != -1) {
        const MInt level =m_maiaproxys[solverId]->tree().level(i);

        // Check if cell is outside the level range
        if (level < minVisLevel || level > maxVisLevel) {
          vis = false;
        }

        // Only show non-leaf cells on the maximum visualization level
        if (m_leafCellsOn &&m_maiaproxys[solverId]->tree().hasChildren(i) && level != maxVisLevel) {
          vis = false;
        }
      } else {
        // Default: visualize all leaf cells
        if (m_leafCellsOn &&m_maiaproxys[solverId]->tree().hasChildren(i)) {
          vis = false;
        }
      }

      // // Check if cell is not leaf cell on other solver
      if ( onlyVisGridLeafCells && vis == true ) {
        MInt gridId = m_maiaproxys[solverId]->tree().solver2grid(i);

        for(MInt solverId_2=0; solverId_2<m_noSolvers;solverId_2++){
          if (solverId_2 == m_solverId) continue;
          if (!m_maiaproxys[solverId_2]->solverFlag(gridId, solverId_2)) continue;

          MInt solverCellId = m_maiaproxys[solverId_2]->tree().grid2solver(gridId);
          // Only show non-leaf cells on the maximum visualization level
          if (m_leafCellsOn && (m_maiaproxys[solverId_2]->tree().noChildren(solverCellId) == m_maiaproxys[solverId_2]->m_maxNoChilds)) {

            vis = false;
          }
        }
      }

      // Add cell if it needs to be visualized
      if (vis){
        m_visBlock.visCellIndices.push_back(i);
        isVisCell.at(i) = 1;
      }
    }

    const MInt visCellHaloOffset = m_visBlock.visCellIndices.size();
    m_visBlock.visCellHaloOffset = visCellHaloOffset;

    // TODO fix this for 'visualizing' inactive ranks
    const MInt tmpSolverId = solverId;

    // Exchange information which halo cells need to be 'visualized' (i.e. the corresponding
    // internal cell is visualized on another domain)
    m_maiaproxys[tmpSolverId]->exchangeHaloCellsForVisualization(&isVisCell[0]);

    // Add halo cells to be 'visualized'
    if(tmpSolverId == m_solverId) continue;
    for (MInt i =m_maiaproxys[tmpSolverId]->noInternalCells(); i <m_maiaproxys[tmpSolverId]->noCells(); i++) {
      if (isVisCell.at(i)) {
        m_visBlock.visCellIndices.push_back(i);
      }
    }
  }
}

/** \brief Function to compute the vertices depending on visualization level and visualization box
 *
 * @author Pascal S. Meysonnat, Michael Schlottke-Lakemper
 * @date back in the 90
 *
 */
template <MInt nDim>
void Reader<nDim>::calcVertices() {
  TRACE();

  // Calculate the bounding box for the grid by determining the maximum
  // coordinate extent, and determine
  // the maximum number of cell/integration nodes/visualization nodes
  Point<nDim> min, max;
  determineDomainExtent(min, max);
  Box<nDim> box(min, max);

  MLong totalNoCellsVisualized = 0;
  switch (m_visType) {
  case VisType::ClassicCells: {
    // Set size to match the number of cells without children (i.e. all
    // classic cells that are to be visualized)
    m_visBlock.noCellsVisualized = m_visBlock.visCellIndices.size();
    totalNoCellsVisualized += m_visBlock.noCellsVisualized;
    
    // Clear containers and reserve memory for efficient adding of elements
    m_visBlock.vertexIds.clear();
    m_visBlock.vertexIds.reserve(m_visBlock.noCellsVisualized * IPOW2(nDim));
  } break;

  case VisType::VisualizationNodes: {
    // Determine number of cells to be visualized by iterating over all cells
    m_visBlock.noCellsVisualized = 0;
    for (size_t i = 0; i < m_visBlock.visCellIndices.size(); i++) {
      const MInt cellId = m_visBlock.visCellIndices.at(i);
      const MInt noNodes1D = (m_visualizeSbpData ? m_noNodes1D[cellId] : (m_polyDeg[cellId]+1)) + m_noVisNodes;
      const MInt noNodesXD = ipow(noNodes1D, nDim);
      m_visBlock.noCellsVisualized += noNodesXD;
    }
    totalNoCellsVisualized += m_visBlock.noCellsVisualized;

    // Resize containers and drop all previous contents
    m_visBlock.vertexIds.clear();
    m_visBlock.vertexIds.reserve(m_visBlock.noCellsVisualized * IPOW2(nDim));
  } break;

  default:
    mTerm(1, AT_, "Bad value");
  }
  
  m_vertices.clear();
  m_vertices.reserve(totalNoCellsVisualized * IPOW2(nDim));

  switch (m_visType) {
  case VisType::ClassicCells: {
    // Loop over all cells that should be visualized
    for (const MInt cellId : m_visBlock.visCellIndices) {
      const MFloat cellLength =m_maiaproxys[m_solverId]->cellLengthAtLevel(m_maiaproxys[m_solverId]->tree().level(cellId));//m_maiagrid->cellLengthAtLevel(m_cells_.level(cellId));

      // Calculate the location of the 4 (2D) or 8 (3D) corner points of the
      // cell, and save them to the box
      for (MInt i = 0; i < IPOW2(nDim); i++) {
        Point<nDim> vertex;
        for (MInt d = 0; d < nDim; d++) {
          vertex.at(d) =m_maiaproxys[m_solverId]->tree().coordinate(cellId, d)
            + 0.5 * m_binaryId[(3 * i) + d] * cellLength;
        }

        MInt vertexId = box.insert(vertex, m_vertices);
        m_visBlock.vertices.insert(vertexId);
        m_visBlock.vertexIds.push_back(vertexId);
      }
    }
  } break;

  case VisType::VisualizationNodes: {
    // Loop over all actual cells and create all visualization node elements
    for (const MInt cellId : m_visBlock.visCellIndices) {
      const MInt noNodes1D = (m_visualizeSbpData ? m_noNodes1D[cellId] : (m_polyDeg[cellId]+1)) + m_noVisNodes;
      const MInt noNodes1D3 = (nDim == 3) ? noNodes1D : 1;
      const MFloat cellLength =m_maiaproxys[m_solverId]->cellLengthAtLevel(m_maiaproxys[m_solverId]->tree().level(cellId));     
      const MFloat nodeLength = cellLength / noNodes1D;
      // Calculate the node center at the -ve corner (i.e. most negative node
      // in x,y,z-directions)
      Point<nDim> node0;
      for (MInt d = 0; d < nDim; d++) {
        node0.at(d) =m_maiaproxys[m_solverId]->tree().coordinate(cellId, d)
                    + 0.5 * (-cellLength + nodeLength);
      }

      // Calculate all vertices and add them to the Box
      array<MInt, 3> index{};
      for (index.at(0) = 0; index.at(0) < noNodes1D; index.at(0)++) {
        for (index.at(1) = 0; index.at(1) < noNodes1D; index.at(1)++) {
          for (index.at(2) = 0; index.at(2) < noNodes1D3; index.at(2)++) {
            Point<nDim> node;
            for (MInt d = 0; d < nDim; d++) {
              node.at(d) = node0.at(d) + nodeLength * index.at(d);
            }

            for (MInt l = 0; l < IPOW2(nDim); l++) {
              Point<nDim> vertex;
              for (MInt d = 0; d < nDim; d++) {
                vertex.at(d)
                    = node.at(d) + (0.5 * m_binaryId[(3 * l) + d] * nodeLength);
              }
              MInt vertexId = box.insert(vertex, m_vertices);
              m_visBlock.vertices.insert(vertexId);
              m_visBlock.vertexIds.push_back(vertexId);
            }
          }
        }
      }
    }
  } break;

  default:
    mTerm(1, AT_, "Bad value");
  }
}


/** \brief Function to init the interpolation (DG)
 *
 * @author Michael Schlottke-Lakemper
 * @date back in the 90
 *  
 */
template <MInt nDim>
void Reader<nDim>::initInterpolation(const MInt maxNoNodes1D) {
  TRACE();

  // Do not initialize for noNodes1D = 1 with Gauss-Lobatto, as this is not defined
  const MInt start = (string2enum(m_intMethod) == DG_INTEGRATE_GAUSS_LOBATTO) ? 2 : 1;

  // Init interpolation objects
  vector<DgInterpolation>(maxNoNodes1D+1).swap(m_interp);
  for (MInt noNodes1D = start; noNodes1D <= maxNoNodes1D; noNodes1D++) {
    const MInt polyDeg = noNodes1D-1; // Irrelevant for SBP
    const MString sbpOperator = ""; // Irrelevant for interpolation 
    m_interp.at(noNodes1D).initInterpolation(
        polyDeg,
        static_cast<DgPolynomialType>(string2enum(m_polyType)),
        noNodes1D,
        static_cast<DgIntegrationMethod>(string2enum(m_intMethod)),
        m_visualizeSbpData,
        sbpOperator);
  }

  // Calculate polynomial interpolation matrices
  vector<vector<MFloat>>(maxNoNodes1D+1).swap(m_vandermonde);
  for (MInt noNodesIn = start; noNodesIn <= maxNoNodes1D; noNodesIn++) {
    const MInt noNodesOut = noNodesIn + m_noVisNodes;
    
    const MFloat nodeLength = F2 / noNodesOut;
    vector<MFloat> nodesOut(noNodesOut);
    for (MInt j = 0; j < noNodesOut; j++) {
      nodesOut.at(j) = -F1 + F1B2 * nodeLength + j * nodeLength;
    }

    m_vandermonde.at(noNodesIn).resize(noNodesIn * noNodesOut);
   
    if(m_visualizeSbpData){
      maia::dg::interpolation::calcLinearInterpolationMatrix(
          noNodesIn, &m_interp.at(noNodesIn).m_nodes[0], noNodesOut, &nodesOut.at(0),
          &m_vandermonde.at(noNodesIn).at(0));
    }else{
      maia::dg::interpolation::calcPolynomialInterpolationMatrix(
          noNodesIn, &m_interp.at(noNodesIn).m_nodes[0], noNodesOut, &nodesOut.at(0),
          &m_interp.at(noNodesIn).m_wBary[0], &m_vandermonde.at(noNodesIn).at(0));
    }
  }
}


/** \brief Function to determine the domain extents
 *
 * @author Michael Schlottke-Lakemper
 * @date back in the 90
 *  
 */
template <MInt nDim>
void Reader<nDim>::determineDomainExtent(Point<nDim>& min, Point<nDim>& max) {
  TRACE();

  // Determine minimum and maximum extent (on a global scale the orginial grid
  // is taken to evaluate the maximumn box extent) 
  for (MInt d = 0; d < nDim; d++) {
    min.at(d) =m_maiaproxys[0]->raw().treeb().coordinate(0, d);
    max.at(d) =m_maiaproxys[0]->raw().treeb().coordinate(0, d);  
  }
  
  for (MInt i = 0; i < (MInt)m_maiaproxys[0]->raw().treeb().size(); i++) {
    const MFloat cellLength =m_maiaproxys[0]->raw().cellLengthAtLevel(
                               m_maiaproxys[0]->raw().a_level(i));
    const MFloat cellLengthB2 = cellLength/2.0;
    for (MInt d = 0; d < nDim; d++) {
      min.at(d) = std::min(min.at(d), 
                 m_maiaproxys[0]->raw().treeb().coordinate(i, d)-cellLengthB2);
      max.at(d) = std::max(max.at(d), 
                 m_maiaproxys[0]->raw().treeb().coordinate(i, d)+cellLengthB2);
    }
  }

  // Increase the min/max range by 5% of the extent in each dimension
  // to avoid numerical errors in the box algorithm for boundary nodes (DG/SBP)
  for (MInt d = 0; d < nDim; d++) {
    const MFloat extension = 0.05 * (max.at(d) - min.at(d));
    min.at(d) -= extension;
    max.at(d) += extension;
  }
}


/** \brief Function to add cell data to the grid
 *
 * @author Pascal S. Meysonnat, Michael Schlottke-Lakemper
 * @date back in the 90
 *  
 */
template <MInt nDim>
template <typename T>
  void Reader<nDim>::addCellData(const MString& name, const T* data, vistle::UnstructuredGrid::ptr &grid, const MInt b, const MBool solverLocalData) {
  TRACE();

  // If data is local for the solver it is already in the correct order in memory, i.e., we do not
  // convert local cell ids into grid cell ids to access the data
  const MInt solverId = (solverLocalData) ? -1 : m_visBlock.solverId;
  //std::cerr << domainId() << " " << isActive << " addCellData " << name
  //          << " b=" << b << " solverId=" << solverId << " "
  //          << m_visBlock.noCellsVisualized << std::endl;


  auto vistleArray = vistle::make_ptr<vistle::Vec<vistle::Scalar, 1>>(m_visBlock.noCellsVisualized); 
  vistleArray->addAttribute("_species", name);

  // Copy data from member variables to vistle array
  switch (m_visType) {
  case VisType::ClassicCells: {
    if (solverId > -1) {
      for (size_t i = 0; i < m_visBlock.visCellIndices.size(); i++) {
        // Visualization of global data on a solver, convert solver cell ids to grid ids
        const MInt cellId = m_visBlock.visCellIndices.at(i);
        const MInt gridCellId =m_maiaproxys[m_solverId]->tree().solver2grid(cellId);
        vistleArray->x().data()[i] = data[gridCellId];
      }
    } else {
      // Visualization of data that is local to a solver, i.e., sorted by the local cells
      for (size_t i = 0; i < m_visBlock.visCellIndices.size(); i++) {
        const MInt cellId = m_visBlock.visCellIndices.at(i);
        vistleArray->x().data()[i] = data[cellId];
      }
    }
    break;
  }  
  case VisType::VisualizationNodes: {
    MInt visNodeId = 0;
    if (solverId > -1) {
      for (size_t i = 0; i < m_visBlock.visCellIndices.size(); i++) {
        // Visualization of global data on a solver, convert solver cell ids to grid ids
        const MInt cellId = m_visBlock.visCellIndices.at(i);
        const MInt gridCellId =m_maiaproxys[m_solverId]->tree().solver2grid(cellId);
        const MInt noNodes1D = (m_visualizeSbpData ? m_noNodes1D[cellId] : (m_polyDeg[cellId]+1)) + m_noVisNodes;
        const MInt noNodesXD = ipow(noNodes1D, nDim);
        for (MInt j = 0; j < noNodesXD; j++) {
          vistleArray->x().data()[visNodeId] = data[gridCellId];
          visNodeId++;
        }
      }
    } else {
      // Visualization of data that is local to a solver, i.e., sorted by the local cells
      for (size_t i = 0; i < m_visBlock.visCellIndices.size(); i++) {
        const MInt cellId = m_visBlock.visCellIndices.at(i);
        const MInt noNodes1D = (m_visualizeSbpData ? m_noNodes1D[cellId] : (m_polyDeg[cellId]+1)) + m_noVisNodes;
        const MInt noNodesXD = ipow(noNodes1D, nDim);
        for (MInt j = 0; j < noNodesXD; j++) {
          vistleArray->x().data()[visNodeId] = data[cellId];
          visNodeId++;
        }
      }
    }
    break;
  }
  default:
    mTerm(1, AT_, "Bad value");
  }

  vistleArray->setGrid(grid);
  vistleArray->setBlock(b);
}


/** \brief Function to add auxiallary data to the grid
 *
 * @author Pascal S. Meysonnat, Michael Schlottke-Lakemper
 * @date back in the 90
 *  
 */
template <MInt nDim>
void Reader<nDim>::loadGridAuxillaryData(vistle::UnstructuredGrid::ptr &grid, const MInt b) {
  TRACE();

  // Container for min cell offsets
  vector<MInt> partitionCellIdOffsets;

  for (auto&& dataset : m_datasets) {
    if (!dataset.isGrid) {
      continue;
    }

    // Skip datasets for which the status is zero (i.e. the user chose to not
    // load them)
    if (!dataset.status) {
      continue;
    }

    if (dataset.variableName == "globalId") {
      addCellData(dataset.realName, &m_maiaproxys[m_solverId]->raw().treeb().globalId(0), grid, b);
      //m_vtkGrid->GetCellData()->SetActiveScalars(dataset.realName.c_str());
      continue;
    }

    if (dataset.variableName == "level") {
      addCellData(dataset.realName, &m_maiaproxys[m_solverId]->raw().treeb().level(0), grid, b);
      continue;
    }

    //DG related stuff
    // if (m_isDgFile){
    //   // Data only for visualization nodes
    //   if (m_visType == VisType::VisualizationNodes && dataset.variableName == "nodeId") {
    //     auto vistleArray = vistle::make_ptr<vistle::Vec<vistle::Scalar, nDim>>(m_visBlock.noCellsVisualized); 
    //     vistleArray->addAttribute("_species", dataset.realName.c_str());

    //     MInt visNodeId = 0;
    //     for (size_t i = 0; i < m_visBlock.visCellIndices.size(); i++) {
    //       const MInt cellId = m_visBlock.visCellIndices.at(i);
    //       const MInt noNodes1D = (m_visualizeSbpData ? m_noNodes1D[cellId] : (m_polyDeg[cellId]+1)) + m_noVisNodes;
    //       const MInt noNodes1D3 = (nDim == 3) ? noNodes1D : 1;
    //       for (MInt ii = 0; ii < noNodes1D; ii++) {
    //       for (MInt jj = 0; jj < noNodes1D; jj++) {
    //         for (MInt kk = 0; kk < noNodes1D3; kk++) {
    //           vistleArray[visNodeId].x() = ii;
    //           vistleArray[visNodeId].y() = jj;
    //           vistleArray[visNodeId].z() = kk;
    //           visNodeId++;
    //         }
    //       }
    //       }
    //     }
    //     vistleArray->setGrid(grid);
    //     continue;
    //   } else if (dataset.variableName == "polyDeg") {
    //     // Polynomial degree (data is local for solver)
    //     addCellData(dataset.realName, &m_polyDeg[0], grid, b, true);
    //     continue;
    //   } else if (m_visualizeSbpData && dataset.variableName == "noNodes1D"){
    //     addCellData(dataset.realName, &m_noNodes1D[0], grid, b, true);
    //     continue;
    //   }
    // }
  }

  // // DG reletaed stuff
  // if (m_isDgFile) {
  //   // Integration method
  //   vtkSmartPointer<vtkStringArray> intMethod
  //     = vtkSmartPointer<vtkStringArray>::New();
  //   intMethod->SetNumberOfComponents(1);
  //   intMethod->SetName("integration method (grid)");
  //   intMethod->InsertNextValue(m_intMethod.c_str());
  //   grid->GetFieldData()->AddArray(intMethod);

  //   // Polynomial type
  //   vtkSmartPointer<vtkStringArray> polyType
  //     = vtkSmartPointer<vtkStringArray>::New();
  //   polyType->SetNumberOfComponents(1);
  //   polyType->SetName("polynomial type (grid)");
  //   polyType->InsertNextValue(m_polyType.c_str());
  //   grid->GetFieldData()->AddArray(polyType);
  // }

}

/** \brief Function to load the solution data
 *
 * @author Pascal S. Meysonnat, Michael Schlottke-Lakemper
 * @date back in the 90
 *  
 */
template <MInt nDim> vistle::DataBase::ptr Reader<nDim>::loadSolutionData(ParallelIo& datafile, const maiapv::Dataset& dataset) {
  TRACE();

  const MBool isActive =m_maiaproxys[m_solverId]->isActive();
  const MInt solverDomainId =m_maiaproxys[m_solverId]->domainId();

  //if (!isActive) {
  //  std::cerr << "inactive rank " << domainId() << " "
  //            << m_visBlocks[0].visCellIndices.size() << " "
  //            << m_visBlocks[0].noCellsVisualized << std::endl;
  //} else {
  //  std::cerr << "active rank " << domainId() << " "
  //            << m_visBlocks[0].visCellIndices.size() << " "
  //            << m_visBlocks[0].noCellsVisualized << std::endl;
  //}

  //copy the offsets so that we can use one call independent of dg/fv/lb
  MInt cellOffsets[2]={m_maiaproxys[m_solverId]->noInternalCells(),m_maiaproxys[m_solverId]->noCells()};
  if(m_isDgFile){
    cellOffsets[0]=m_dofDg[0];
    cellOffsets[1]=m_dofDg[1];
  }

  //set the offset correctly
  if (!isActive) {
    cellOffsets[0] = 0;
    cellOffsets[1] = 0;
    datafile.setOffset(0, 0);
  } else if (m_isDgFile) {
    datafile.setOffset(m_dofDg[0], m_offsetLocalDG);
  } else {
    datafile.setOffset(m_maiaproxys[m_solverId]->noInternalCells(),
                      m_maiaproxys[m_solverId]->domainOffset(solverDomainId));
    //datafile.setOffset(m_maiagrid->noInternalCells(), m_maiagrid->domainOffset(domainId())-m_32BitOffset);
  }

  // Calculate the offsets for the cell data based on the polynomial degree (DG)
  // or trivially based on the number of cells (FV/LB)
  const MInt noOffsets = (isActive) ?m_maiaproxys[m_solverId]->noCells() + 1 : 1;
  vector<MInt> dataOffsets(noOffsets);

  if (m_isDgFile) {
    dataOffsets.at(0) = 0;
    for (MInt i = 1; i < noOffsets; i++) {
      const MInt noNodes1D = (m_visualizeSbpData ? m_noNodes1D[i-1] : (m_polyDeg[i-1]+1));
      const MInt noNodesXD = ipow(noNodes1D, nDim);
      dataOffsets.at(i)
          = dataOffsets.at(i - 1) + noNodesXD;
    }
  }

    // Load all valid datasets into the grid
  // MBool activeScalarsSet = false;
    // Skip grid datasets
    // Skip datasets for which the status is zero (i.e. the user chose to not load them)
  if (dataset.isGrid || !dataset.status) {return nullptr;}
  
  // Read in data
  std::vector<MFloat> rawdata(std::max(cellOffsets[1], 1), 0.0);

  datafile.readArray(&rawdata[0], dataset.variableName);

  // MPI communication
  if (noDomains() > 1 && isActive) {
    if(!m_isDgFile) { //needs to be implemented for DG style
     m_maiaproxys[m_solverId]->exchangeHaloCellsForVisualization(&rawdata[0]);
      // exchangeHaloDataset(scalars, dataOffsets);
    } else {
      if(m_visualizeSbpData){
        std::cerr << "exchange SBP window/halo cell data" << std::endl;
       m_maiaproxys[m_solverId]->exchangeHaloCellsForVisualizationSBP(&rawdata[0], &m_noNodes1D[0],
                                                                    &dataOffsets[0]);
      } else {
        std::cerr << "exchange DG window/halo cell data" << std::endl;
       m_maiaproxys[m_solverId]->exchangeHaloCellsForVisualizationDG(&rawdata[0], &m_polyDeg[0],
                                                                    &dataOffsets[0]);
      }
    }
  }
  // Create Vistle array to hold data values
  auto vistleArray = vistle::make_ptr<vistle::Vec<vistle::Scalar, 1>>(m_visBlock.noCellsVisualized); 
  vistleArray->addAttribute("_species", dataset.realName.c_str());

  
  // Add solution data to VTK array
  std::cerr << "add solution data to Vistle..." << std::endl;
  if (m_visType == VisType::VisualizationNodes && isActive) {
    // Normal DG visualization (non-SBP data)
    // Interpolate data to cells
    std::cerr << "interpolate data to cells..." << std::endl;
    size_t nodeId = 0;
    for (size_t i = 0; i < m_visBlock.visCellIndices.size(); i++) {
      const MInt cellId = m_visBlock.visCellIndices.at(i);
      const MInt noNodesIn = (m_visualizeSbpData ? m_noNodes1D[cellId] : (m_polyDeg[cellId]+1));
      const MInt noNodesOut = noNodesIn + m_noVisNodes;
      const MInt noNodesXDOut = ipow(noNodesOut, nDim);
      const MInt noVariables = 1;
      vector<MFloat> interpolated(noNodesXDOut);
      using namespace maia::dg::interpolation;
      (nDim < 3)
        ? interpolateNodes<2>(&rawdata[dataOffsets.at(cellId)],
                              &m_vandermonde.at(noNodesIn).at(0), noNodesIn,
                              noNodesOut, noVariables, &interpolated.at(0))
        : interpolateNodes<3>(&rawdata[dataOffsets.at(cellId)],
                              &m_vandermonde.at(noNodesIn).at(0), noNodesIn,
                              noNodesOut, noVariables, &interpolated.at(0));
      
      for (MInt j = 0; j < noNodesXDOut; j++) {
        vistleArray->x().data()[nodeId] = interpolated.at(j);
        nodeId++;
      }
    }
    std::cerr << "interpolate data to cells: OK" << std::endl;
  } else if (m_visType == VisType::VisualizationNodes && m_visualizeSbpData && isActive) {
    //Visualization of SBP data generated by DG solver
    std::cerr << "interpolate SBP data to cells..." << std::endl;
    MInt nodeId = 0;
    for (size_t idx = 0; idx < m_visBlock.visCellIndices.size(); idx++) {
      const MInt cellId = m_visBlock.visCellIndices.at(idx);
      const MInt noNodesIn = (m_visualizeSbpData ? m_noNodes1D[cellId] : (m_polyDeg[cellId]+1));
      const MInt noNodesOut = noNodesIn;
      const MInt noNodesXDOut = ipow(noNodesOut, nDim);
      vector<MFloat> interpolated(noNodesXDOut);

      if (nDim == 2) {
        MFloatTensor in(&rawdata[dataOffsets.at(cellId)], noNodesIn, noNodesIn);
        MFloatTensor out(&interpolated.at(0), noNodesOut, noNodesOut);
        for (MInt i = 0; i < noNodesOut; i++) {
          for (MInt j = 0; j < noNodesOut; j++) {
            out(i, j) = (in(i, j) + in(i, j + 1) + in(i + 1, j) + in(i + 1, j + 1)) / 4;
          }
        }
      } else {
        MFloatTensor in(&rawdata[dataOffsets.at(cellId)], noNodesIn, noNodesIn, noNodesIn);
        MFloatTensor out(&interpolated.at(0), noNodesOut, noNodesOut, noNodesOut);
        for (MInt i = 0; i < noNodesOut; i++) {
          for (MInt j = 0; j < noNodesOut; j++) {
            for (MInt k = 0; k < noNodesOut; k++) {
              out(i, j, k) = (in(i, j, k) + in(i, j, k + 1) +
                              in(i, j + 1, k) + in(i, j + 1, k + 1) +
                              in(i + 1, j, k) + in(i + 1, j, k + 1) +
                              in(i + 1, j + 1, k) + in(i + 1, j + 1, k + 1)) / 8;
            }
          }
        }
      }

      for (MInt j = 0; j < noNodesXDOut; j++) {
        vistleArray->x().data()[nodeId] = interpolated.at(j);
        nodeId++;
      }
    }
    std::cerr << "interpolate SBP data to cells: OK" << std::endl;
  } else {
    std::cerr << "copying raw data..." << std::endl;
    // For classic cells, just copy values from raw data
    for (size_t i = 0; i < m_visBlock.visCellIndices.size(); i++) {
      const MInt cellId = m_visBlock.visCellIndices.at(i);
      vistleArray->x().data()[i] = rawdata[cellId];
    }
    std::cerr << "copying raw data: OK" << std::endl;
  }

    /*
    // Set the first data array to be the "active" one
    if (!activeScalarsSet) {
      m_vtkGrid->GetCellData()->SetActiveScalars(dataset.realName.c_str());
      activeScalarsSet = true;
    }
    */
    std::cerr << "loadSolutionData: OK" << std::endl;
    return vistleArray;

}

/** \brief Function to load information of Lagrange particles
 *
 * @author Thomas Hoesgen
 * @date 28.05.2020
 *  
 */
template <MInt nDim>
  void Reader<nDim>::readParticleData(vistle::UnstructuredGrid::ptr grid, const vector<Dataset>& datasets) {
  TRACE();

  
  // m_datasets = datasets;


  // cerr << "Build particle vtkgrid...";
  
  // //Build the grid
  // m_vistleGrids=grid;
  // m_vistleGrids.resize(1);
  
  // vistle::UnstructuredGrid::ptr localGrid = viste::make_ptr<vistle::UnstructuredGrid>();
  // std::array<vistle::shm_array<float>&, 3> partPoints = {localGrid->x(), localGrid->y(), localGrid->z()};
  // vtkPoints* partPoints = vtkPoints::New();
  // localGrid->Initialize();
  // localGrid->Allocate(0,0);
  // localGrid->SetPoints(partPoints);
  
  // ParallelIo partFile(m_dataFileName, maia::parallel_io::PIO_READ, mpiComm());
  // MLong noGlobalParticles = (partFile.getArrayDims("partDia")).at(0);

  // MInt avgNoParticles = noGlobalParticles / noDomains();
 
  // MInt localOffset = domainId() * avgNoParticles;
  // MInt noParticles = avgNoParticles;

  // if (domainId() == noDomains() - 1) {
  //   noParticles = noGlobalParticles - domainId() * avgNoParticles;
  // }

  // // Set offsets
  // partFile.setOffset(nDim*noParticles, localOffset);

  // // Read in coordinates
  // std::vector<MFloat> rawdata(std::max(nDim*noParticles, 1), 0.0);
  // partFile.readArray(&rawdata[0], "partPos");
  // for (MInt i = 0; i < noParticles; i++) {
  //   MInt index = localOffset + i*nDim;
  //   MFloat coord[3] = {rawdata[index], rawdata[index+1], F0};
  //   if (nDim == 3) coord[2] = rawdata[index+2];

  //   partPoints->InsertNextPoint(coord[0], coord[1], coord[2]);
  // }
  // for (MInt i = 0; i < noParticles; i++) {
  //   vtkVertex * cell = vtkVertex::New();
  //   cell->GetPointIds()->SetId(0, i);
  //   localGrid->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
  //   cell->Delete();
  // }

  // cerr << "   done." << endl;
  // // Build grid done
  
  // // Load data
  // cerr << "Load paricle data";
  // for (auto&& dataset : m_datasets) {
  //   if (!dataset.status) {continue;}

  //   // Read data
  //   MLong dataSize = partFile.getArraySize(dataset.variableName);
  //   if ( dataSize / noGlobalParticles == nDim ) {

  //     // Set offsets
  //     partFile.setOffset(nDim*noParticles, localOffset);

  //     partFile.readArray(&rawdata[0], dataset.variableName);

  //     MFloat* values_0 = new MFloat [noParticles];
  //     MFloat* values_1 = new MFloat [noParticles];
  //     MFloat* values_2 = new MFloat [noParticles];
  
  //     for (MInt i = 0; i < noParticles; i++) {
  //       values_0[i] = rawdata[nDim*i];
  //       values_1[i] = rawdata[nDim*i + 1];
  //       if ( nDim == 3 )
  //         values_2[i] = rawdata[nDim*i + 2];
  //       else
  //         values_2[i] = F0;
  //     }

  //     for (MInt d = 0; d < nDim; d++) {
  //       vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
  //       data->SetNumberOfValues(noParticles);
        
  //       stringstream dim;
  //       dim << d;
  //       data->SetName( (dataset.variableName + dim.str()).c_str());
  //       for (MInt i = 0; i < noParticles; i++) {
  //         data->SetValue(i, rawdata[i*nDim+d]);
  //       }
  //       localGrid->GetCellData()->AddArray(data);

  //     }
  //     delete[] values_0;
  //     delete[] values_1;
  //     delete[] values_2;

  //   } else {

  //     // Set offsets
  //     partFile.setOffset(noParticles, localOffset);

  //     partFile.readArray(&rawdata[0], dataset.variableName);

  //     vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
  //     data->SetNumberOfValues(noParticles);

  //     data->SetName( dataset.variableName.c_str() );
  //     for (MInt i = 0; i < noParticles; ++i) {
  //       data->SetValue(i, rawdata[i]);
  //     }
  //     localGrid->GetCellData()->AddArray(data);
  //   }
  // }
  // cerr << "   done." << endl;

  // // Add to grid
  // m_vistleGrids->SetBlock(0, localGrid);
  // m_vistleGrids->GetMetaData((MInt) 0)->Set(vtkCompositeDataSet::NAME(), "particles");

  // return;
}


}//namespace maiapv
#endif
