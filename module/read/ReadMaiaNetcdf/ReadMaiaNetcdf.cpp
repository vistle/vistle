#include "ReadMaiaNetcdf.h"
#include "readerbase.h"
#include "reader.h"

#include <boost/algorithm/string/predicate.hpp>

#include <vistle/core/lines.h>
#include <vistle/core/object.h>
#include <vistle/core/points.h>
#include <vistle/core/polygons.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>
#include <vistle/util/filesystem.h>

#include <mpi.h>

//include maia related header
#include "MEMORY/scratch.h"
#include "compiler_config.h"
#include "IO/parallelio.h"
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

using namespace std;
using vistle::Parameter;

MODULE_MAIN(ReadMaiaNetcdf)
using vistle::Reader;
using vistle::DataBase;
using vistle::Parameter;

const std::string Invalid("(NONE)");

bool isCollectionFile(const std::string &fn)
{
    constexpr const char *collectionEndings[3] = {".pvd", ".vtm", ".pvtu"};
    for (const auto ending: collectionEndings)
        if (boost::algorithm::ends_with(fn, ending))
            return true;

    return false;
}

template<class VO>
std::vector<std::string> getFields(VO *dsa)
{
    std::vector<std::string> fields;
    if (!dsa)
        return fields;
    int na = dsa->GetNumberOfArrays();
    for (int i = 0; i < na; ++i) {
        fields.push_back(dsa->GetArrayName(i));
        //cerr << "field " << i << ": " << fields[i] << endl;
    }
    return fields;
}


ReadMaiaNetcdf::ReadMaiaNetcdf(const std::string &name, int moduleID, mpi::communicator comm)
: Reader(name, moduleID, comm)
{
    createOutputPort("grid_out", "grid or geometry");
    m_filename = addStringParameter("filename", "name of netcdf file", "", Parameter::ExistingFilename);
    setParameterFilters(m_filename, "netcdf files(*.Netcdf)");
    // m_readPieces = addIntParameter("read_pieces", "create block for every piece in an unstructured grid", false,
    //                                Parameter::Boolean);
    // m_ghostCells = addIntParameter("create_ghost_cells", "create ghost cells for multi-piece unstructured grids", true,
    //                                Parameter::Boolean);
    for (size_t i = 0; i < NumPorts; i++)
    {
      createOutputPort("data_out" + std::to_string(i), "data");
      m_cellDataChoice[i] = addStringParameter("cell_field_" + std::to_string(i), "cell data field", "", Parameter::Choice);
    }
    
    observeParameter(m_filename);
}

bool ReadMaiaNetcdf::examine(const vistle::Parameter *param)
{
    int fileId=-1;
    int status = ncmpi_open(comm(), m_filename->getValue().c_str(), NC_NOWRITE, MPI_INFO_NULL, &fileId);
    if (status != NC_NOERR) { cerr << "error in opening the file " << endl;}
    //get if file  has "gridFileName" in it.
    int varId = NC_GLOBAL;
    int attId =-1;
    status = ncmpi_inq_attid(fileId, varId, "gridFile", &attId);
    if (status == NC_NOERR) {
      m_isDataFile = true;
    } else if (status == NC_ENOTATT) {
      m_isDataFile = false;

      int varId = -1;
      ncmpi_inq_varid(fileId, "partCount", &varId);
      if (varId > -1) {
        m_isParticleFile = true;
      }
    } else {
      m_isDataFile = false;
      return 0;
    }
    //procceed if file is a solution file with data
    if (m_isDataFile) {
      m_dataFileName=m_filename->getValue();
      MPI_Offset length;
      status = ncmpi_inq_attlen(fileId, varId, "gridFile", &length);
      char* tmpChar= new char[length];
      status = ncmpi_get_att_text(fileId, varId, "gridFile", tmpChar);
      m_gridFileName.assign(tmpChar, length);
      //open grid file 
      int gridId=-1;
      vistle::filesystem::path p(m_filename->getValue());
      m_gridFileName= (p.parent_path() /  m_gridFileName).string();
      status = ncmpi_open(comm(), m_gridFileName.c_str(), NC_NOWRITE, MPI_INFO_NULL, &gridId);
      status = ncmpi_inq_attid(gridId, varId, "nDim", &attId);
      if (status == NC_NOERR) {
        status=ncmpi_get_att_int(gridId, varId, "nDim", &m_nDim);     
        if (status != NC_NOERR) cerr << " Error in reading nDim" << endl;
      }
      status = ncmpi_close(gridId);
      //proceed if a file is a grid file 
    } else if (m_isParticleFile) {
      m_gridFileName="";
      m_dataFileName=m_filename->getValue();
      m_nDim = 3; //Necessary for reader. Particles do not have dimension!
    } else {
      m_gridFileName=m_filename->getValue();
      //is grid File get nDim directly
      status = ncmpi_inq_attid(fileId, varId, "nDim", &attId);
      if (status == NC_NOERR) {
        status=ncmpi_get_att_int(fileId, varId, "nDim", &m_nDim);     
        if (status != NC_NOERR) cerr << " Error in reading nDim" << endl;
      }
    }
    //close file
    status = ncmpi_close(fileId);
    //build an instance of the reader to retrieve further information from file 
    auto reader = createReader(0);
    
    //get the number of solvers in the file
    m_noSolvers = reader->requestNumberOfSolvers();
    //get the proposed memory increase from the average number of cells
  
    if(MemIncrease < 0 && MemIncrease != reader->getMemoryIncrease()){
      MemIncrease = reader->getMemoryIncrease();
    }
    
    //check if file is DG
    m_isDgFile = reader->requestFiletype();

    // If it is a DG file, set visualization type to VisualizationNodes, otherwise it is ClassicCells
    if (m_isDgFile) {
      m_visType = VisType::VisualizationNodes;
    } else {
      m_visType = VisType::ClassicCells;
    }

    //create the  number of solvers for internal use
    if(m_solvers.size()==0 && static_cast<int>(m_solvers.size())!=m_noSolvers){
      for(int i=0;i<m_noSolvers; ++i){
        stringstream number;
        number<< i+1;
        m_solvers.push_back({"Solver " + number.str(), true, false, i});
      }
    }
  
    //if multi solver and data file only visualize solver which is in datafile
    if(m_solvers.size()>1 && m_isDataFile ){
      MInt solverId = reader->getSolverId();
      for(int i=0; i<m_noSolvers; ++i){
        if(m_solvers[i].solverId==solverId){
          m_solvers[i].status=true; //visualize
        }else{
          m_solvers[i].status=false; //do not visualize
        }
      }
    }
    setPartitions(m_noSolvers);
    setTimesteps(0);
    //get the bounding Box (inital values are shown then in the gui)
    // reader->requestBoundingBoxInformation(m_boundingBox);

    //get the dataset names and number
    reader->requestInformation(m_datasets);
    setChoices(m_datasets);
    //get the level range in the file 
    reader->setLevelInformation(LevelRange);  
    //important for time series
    if(m_isDataFile){
      double time = reader->requestTimeInformation();
      setTimesteps(time);
    }
  
  return true;
}

bool ReadMaiaNetcdf::prepareRead()
{
    const std::string filename = m_filename->getValue();


    return true;
}

bool ReadMaiaNetcdf::finishRead()
{
    return true;
}

bool ReadMaiaNetcdf::read(Token &token, int timestep, int block)
{
  std::vector<maiapv::Dataset> datasets;
  for(size_t i=0; i<m_cellDataChoice.size(); i++)
  {
    auto dataset = std::find_if(m_datasets.begin(), m_datasets.end(), [i, this](const maiapv::Dataset &dataset){
      return dataset.realName == m_cellDataChoice[i]->getValue();
    });
    if(dataset != m_datasets.end())
    {
      
      dataset->port = "data_out" + std::to_string(i);
      datasets.push_back(*dataset); 
    }
  }
  auto reader = createReader(block);
  if(m_isParticleFile){
    // reader->readParticleData(token.grid(), datasets);
    return true;
  }
  auto grid = reader->readGrid();
  updateMeta(grid);
  token.addObject("grid_out", grid);

  for(const auto &dataset : datasets)
  {
    auto data = reader->readData(dataset);
    data->setGrid(grid);
    updateMeta(data);
    token.addObject(dataset.port, data);
  }
  // reader->read(datasets);
    
  return true;
}

bool ReadMaiaNetcdf::load(Token &token, const std::string &filename, const vistle::Meta &meta, int piece, bool ghost,
                          const std::string &part) const
{
    return true;
}

void ReadMaiaNetcdf::setChoices(const std::vector<maiapv::Dataset> &fileinfo)
{
    const std::string Invalid("(NONE)");
    std::vector<std::string> cellFields({Invalid});
    for(const auto &dataset: fileinfo){
      if(dataset.isGrid)
        continue;
      cellFields.push_back(dataset.realName);
    }
    for (int i = 0; i < NumPorts; ++i) {
        setParameterChoices(m_cellDataChoice[i], cellFields);
    }
}

std::unique_ptr<maiapv::ReaderBase> ReadMaiaNetcdf::createReader(int block)
{
std::unique_ptr<maiapv::ReaderBase> reader;
    switch (m_nDim) {
    case 2:
      reader.reset(new maiapv::Reader<2>(m_gridFileName,m_dataFileName, block, comm()));
      break;

    case 3:
      reader.reset(new maiapv::Reader<3>(m_gridFileName,m_dataFileName, block, comm()));
      break;

    default:
      TERMM(1, "Bad dimension (must be 2 or 3, is: " + to_string(m_nDim) + ")");
    }
    return reader;
}
