#include "strctrd.h"
#include <iostream>
#include <vector>
#include "mpi.h"

using namespace std;

//Constructor for RequestInformation
structuredGrid::structuredGrid(const std::string& dataFileName_,
                               std::vector<StrctrdLevel>& StrctrdLevel_,
                               std::vector<StrctrdDataset>& StrctrdDataset_,
                               std::vector<StrctrdLevelAttr>& StrctrdLevelAttr_) :
  m_fileName(dataFileName_),
  m_level(&StrctrdLevel_),
  m_levelSelected(&StrctrdDataset_),
  m_levelAttributes(&StrctrdLevelAttr_)
{
  ErrorCode="empty";
  WarningPair.first=false;
  WarningPair.second="nothing";
  m_noDims=0;
  m_hasCoor=false;
  
  m_ioSolution = std::make_shared<ParallelIoHdf5>(m_fileName, maia::parallel_io::PIO_READ, MPI_COMM_WORLD);
}

//Konstruktor RequestData
structuredGrid::structuredGrid(const std::string& gridFileName_,
                               const std::string& dataFileName_,
                               std::vector<StrctrdLevel>& StrctrdLevel_,
                               std::vector<StrctrdDataset>& StrctrdDataset_,
                               std::vector<StrctrdLevelAttr>& StrctrdLevelAttr_,
                               MInt noDims_,
                               MBool hasCoor_,
                               std::string timePath_,
                               Filetype fileType_) :
  m_fileName(dataFileName_),  
  m_gridFileName(gridFileName_),
  m_level(&StrctrdLevel_),
  m_levelSelected(&StrctrdDataset_),
  m_levelAttributes(&StrctrdLevelAttr_),
  m_noDims(noDims_),
  m_hasCoor(hasCoor_),
  m_timePath(timePath_),
  m_FT(fileType_)
{
  ErrorCode="empty";
  WarningPair.first=false;
  WarningPair.second="nothing";

  m_ioSolution = std::make_shared<ParallelIoHdf5>(m_fileName, maia::parallel_io::PIO_READ, MPI_COMM_WORLD);

  if(m_hasCoor) {
    m_ioGrid = m_ioSolution;
  } else {
    m_ioGrid = std::make_shared<ParallelIoHdf5>(m_gridFileName, maia::parallel_io::PIO_READ, MPI_COMM_WORLD);
  }
}


//GetterMethoden
string structuredGrid::getErrorCode() const
{
  return ErrorCode;
}

bool structuredGrid:: getIfWarning() const
{
  return WarningPair.first;
}

string structuredGrid::getWarningCode() const
{
  return WarningPair.second;
}

MInt structuredGrid::getNoStrctrdDataset() const
{
  return m_noStrctrdDataset;
}

MInt structuredGrid::getNoStrctrdLevel() const
{
  return m_noStrctrdLevel;
}


MInt structuredGrid::getNoStrctrdLevelSelected() const
{
  return m_noStrctrdLevelSelected;
}

MInt structuredGrid::getnoDims() const
{
  return m_noDims;
}

bool structuredGrid::gethasCoor() const
{
  return m_hasCoor;
}

string structuredGrid::getTimestep() const
{
  return m_timePath;
}

Filetype structuredGrid::getFiletype() const
{
  return m_FT;
}


//PublicMethods
bool structuredGrid::initialize()
{
  //Determine the Filetype
  MString filetype = "";
  m_ioSolution->getAttribute(&filetype, "filetype", "");

  //Initialize the Filetype
  if (filetype.find("grid")!=std::string::npos) {
    m_FT=Filetype::grid;
    m_gridFileName=m_fileName;
    return openGrid();
  } else if (filetype.find("solution")!=std::string::npos) {
    m_FT=Filetype::solution;
    return openGrid();
  } else if (filetype.find("boxes")!=std::string::npos) {
    m_FT=Filetype::box;
    m_noDims=3;
    if (!checkTimeStep())     { return false; }
    if (!checkCoordinates())  { return false; }
    return openGrid();
  } else if (filetype.find("planes")!=std::string::npos) {
    m_FT=Filetype::plane;
    m_noDims=2;
    if (!checkTimeStep())     {return false; }
    if (!checkCoordinates())  {return false; }
    return openGrid();
  } else if (filetype.find("auxdata")!=std::string::npos) {
    m_FT=Filetype::aux;
    m_noDims=2;
    m_hasCoor=true;
    return openGrid();
  } else {
    ErrorCode="E: Can't read Filetype";
    return false;
  }
}

bool structuredGrid::checkLevels()
{
  switch(m_FT)
  {
  case Filetype::grid:
  {
    if(!saveLevelsGrid_Sol()) {
      return false;
    }
    
    readSelectedLevels();
    std::vector<MString> datasetNames = m_ioGrid->getDatasetNames(("/"+(*m_levelAttributes)[0].name));
    m_noDims = datasetNames.size();
    return true;
  }
  case Filetype::solution:
  {
    if(!saveLevelsGrid_Sol()) {
      return false;
    }
    
    readSelectedLevels();
    std::vector<MString> datasetNames = m_ioGrid->getDatasetNames(("/"+(*m_levelAttributes)[0].name));
    m_noDims = datasetNames.size();
    return true;
  }
  case Filetype::box:
  {
    if(!saveLevelsBox()) {
      return false;
    }
    
    readSelectedLevels();
    if(!m_hasCoor) {
      readSelectedLevelAttributes();
    }
    
    return true;
  }
  case Filetype::plane:
  {
    if(!saveLevelsPlane()) {
      return false;
    }
    
    readSelectedLevels();
    if(!m_hasCoor) {
      readSelectedLevelAttributes();
    }
    return true;

  }
  case Filetype::aux:
  {
    if(!saveLevelsAux()) {
      return false;
    }
    
    readSelectedLevels();
    return true;
  }
  default:
  {
    return false;
  }
  }
}


bool structuredGrid::checkData()
{
  switch(m_FT)
  {
  case Filetype::grid:
  {
    m_levelSelected->resize(1);
    (*m_levelSelected)[0].name="GridFile has no Dataset";
    return true;
  }
  case Filetype::solution:
  {
    const string name="/"+(*m_levelAttributes)[0].name;
    return readDataInformation(name);
  }
  case Filetype::box:
  {
    const string name=m_timePath+"/"+(*m_levelAttributes)[0].name;
    if(!readDataInformation(name)) {
      return false;
    }
    if(m_hasCoor) {
      deleteCoordFromData();
    }
    return true;
  }
  case Filetype::plane:
  {
    const string name=m_timePath+"/"+(*m_levelAttributes)[0].name;
    if(!readDataInformation(name)) {
      return false;
    }
    if(m_hasCoor) {deleteCoordFromData();}
    return true;
  }
  case Filetype::aux:
  {
    const string name="/"+(*m_levelAttributes)[0].name;
    if(!readDataInformation(name)) {
      return false;
    }
    deleteCoordFromData();
    return true;
  }
  default:
  {
    return false;
  }
  }
}

void structuredGrid::setupLevels()
{
  switch(m_FT)
  {
  case Filetype::grid:
  case Filetype::solution:    
  {
    readLevelSizes(m_noDims);
    break;
  }
  case Filetype::box:
  {
    switch(m_hasCoor)
    {
    case false:
    {
      readLevelSizesBox_nc();
      break;
    }
    case true:
    {
      readLevelSizesBox_wc();
      break;
    }
    }
    break;
  }
  case Filetype::plane:
  {
    switch(m_hasCoor)
    {
    case false:
    {
      readLevelSizesPlane_nc();
      break;
    }
    case true:
    {
      readLevelSizesPlane_wc();
      break;
    }
    }
    break;
  }
  case Filetype::aux:
  {
    readLevelSizes2D_A();
  }	break;

  }
}


// void structuredGrid::setupGrid(MInt level, vtkPoints& PS,const MInt (&subEx)[6]) const
// {
//   const MInt l=level;

//   switch(m_FT)
//   {
//   case Filetype::grid:
//   {
//     const string name="/"+(*m_levelAttributes)[l].name;
//     switch(m_noDims)
//     {
//     case 2: {setupGrid2D(name,PS,subEx); break;}
//     case 3: {setupGrid3D(name,PS,subEx); break;}
//     }
//     break;
//   }
//   case Filetype::solution:
//   {
//     const string name="/"+(*m_levelAttributes)[l].name;
//     switch(m_noDims)
//     {
//     case 2: {setupGrid2D(name,PS,subEx); break;}
//     case 3: {setupGrid3D(name,PS,subEx); break;}
//     }
//     break;
//   }
//   case Filetype::box:
//   {
//     //get global offsets from box file
//     const string boxPath=m_timePath+"/"+(*m_levelAttributes)[l].name;
    
//     switch(m_hasCoor)
//     {
//     case false:
//     {
//       if(m_noDims>2) {
//         m_ioSolution->getAttribute(&m_globalOffset[0], "offsetk", "");
//       }
//       m_ioSolution->getAttribute(&m_globalOffset[1], "offsetj", "");      
//       m_ioSolution->getAttribute(&m_globalOffset[0], "offseti", "");
      
//       const string name="/block"+to_string((*m_levelAttributes)[l].blockId);
//       setupGrid3D(name,PS,subEx);
//       break;
//     }
//     case true:
//     {      
//       const string name=m_timePath+"/"+(*m_levelAttributes)[l].name;
//       setupGrid3D(name,PS,subEx);
//       break;
//     }
//     }
//     break;
//   }
//   case Filetype::plane:
//   {
//     switch(m_hasCoor)
//     {      
//     case false:
//     {
//       const string name="/block"+to_string((*m_levelAttributes)[l].blockId);
//       setupGridPlaneNC(l,name,PS,subEx);
//       break;
//     }
//     case true:
//     {
//       const string name=m_timePath+"/"+(*m_levelAttributes)[l].name;
//       setupGridPlaneWCuAUX(name,PS,subEx);
//       break;
//     }
//     }
//     break;
//   }
//   case Filetype::aux:
//   {
//     const string name="/"+(*m_levelAttributes)[l].name;
//     setupGridPlaneWCuAUX(name,PS,subEx);
//     break;
//   }
//   }
// }



// void structuredGrid::setupData(MInt level, MInt variable,
//                                vtkDoubleArray& varArray, const MInt (&subEx)[6]) const
// {
//   const MInt l=level;
//   const MInt var=variable;

//   switch(m_FT)
//   {
//   case Filetype::solution:
//   {
//     const string name="/"+(*m_levelAttributes)[l].name+"/"+(*m_levelSelected)[var].name;
//     switch(m_noDims)
//     {
//     case 2:
//     {
//       insertData2D(var,name,varArray,subEx);
//       break;
//     }
//     case 3:
//     {
//       insertData3D(var,name,varArray,subEx);
//       break;
//     }

//     }
//     break;
//   }
//   case Filetype::box:
//   {
//     const string name = m_timePath+"/"+(*m_levelAttributes)[l].name+"/"+(*m_levelSelected)[var].name;
//     insertData3D(var,name,varArray,subEx);
//     break;
//   }
//   case Filetype::plane:
//   {
//     const string name = m_timePath+"/"+(*m_levelAttributes)[l].name+"/"+(*m_levelSelected)[var].name;
//     switch(m_hasCoor)
//     {
//     case false:
//     {
//       insertDataPlaneNC(l,var,name,varArray,subEx);
//       break;
//     }
//     case true:
//     {
//       insertData2D(var,name,varArray,subEx);
//     }
//     }
//     break;
//   }
//   case Filetype::aux:
//   {
//     const string name="/"+(*m_levelAttributes)[l].name+"/"+(*m_levelSelected)[var].name;
//     insertData2D(var,name,varArray,subEx);
//   }
//   default:
//   {
//     //do nothing
//     break;
//   }  
//   }
// }

//privateMethods

bool structuredGrid::checkTimeStep()
{
  if(!m_ioSolution->hasAttribute("globalTimeStep", "")) {
    m_oldStructure = true;
    std::vector<MString> groupNames = m_ioSolution->getGroupNames("");
  
    const string buf_pathnames(groupNames[0]);
    m_timePath.assign(buf_pathnames);
  
    if(m_timePath.empty()) {
      ErrorCode="E: Can't read time step";
      return false;
    }
  } else {
    m_timePath.assign("");
  }
  
  return true;
}

bool structuredGrid::checkCoordinates()
{
  string bufName=m_timePath;
  switch(m_FT)
  {
  case Filetype::box:
  {
    bufName += "/box0"; //If the first Level has Coordnates, all Levels have Coordinates
    break;
  }
  case Filetype::plane:
  {
    bufName += "/plane0"; //If the first Level has Coordinates, all Levels have coordinates
    break;
  }
  default:
  {
    //do nothing
  }
  }

  if(!m_ioSolution->hasAttribute("hasCoordinates", bufName)) {
    ErrorCode="E: Attribute hasCoordinates doesn't exist";
    return false;
  } else {
    MInt hasCoor = -1;
    m_ioSolution->getAttribute(&hasCoor, "hasCoordinates", bufName);
    if(hasCoor==1) {
      m_hasCoor=true;
    } else if (hasCoor==0) {
      m_hasCoor=false;
    } else {
      //If the value is not 1 or 0, there was an error
      ErrorCode="E: Can't read if file has coordinates.";
      return false;
    }
    return true;
  }
}



bool structuredGrid::openGrid()
{
  if(m_FT==Filetype::grid) {
    m_ioGrid = std::make_shared<ParallelIoHdf5>(m_gridFileName, maia::parallel_io::PIO_READ, MPI_COMM_WORLD);        
    return true;
  }

  if(m_hasCoor) {
    m_ioGrid = m_ioSolution;
    return true;
  }
  
  //The the Path of file has to be added to the Name of the GridFile,
  //so that it can be opened with io_open..
  MString gridFile = "";
  if(m_ioSolution->hasAttribute("gridFile", "")) {
    m_ioSolution->getAttribute(&gridFile, "gridFile", "");
  } else {
    ErrorCode="E: There is no grid file information in the solution file";
    return false;
  }
 
  //Read the Path
  const string Name=m_fileName; //FileName with Path
  const size_t found = Name.rfind("/");
  const string path = Name.substr(0,found); //Path of the FileName
  m_gridFileName=path+"/"+gridFile;  //the whole gridName
  
  m_ioGrid = std::make_shared<ParallelIoHdf5>(m_gridFileName, maia::parallel_io::PIO_READ, MPI_COMM_WORLD);

  return true;
}



bool structuredGrid::saveLevelsGrid_Sol()
{
  MInt blockcount=0;
  m_ioSolution->getAttribute(&blockcount, "noBlocks", "");

  if(blockcount==0) {
    ErrorCode="Can't read Number of Blocks";
    return false;
  }

  m_noStrctrdLevel=blockcount;
  m_level->resize(m_noStrctrdLevel);

  for(MInt i=0;i<m_noStrctrdLevel;i++) {
    (*m_level)[i].name="block"+to_string(i);
  }

  return true;
}

bool structuredGrid::saveLevelsBox()
{
  //TotalNumber and Names for m_level
  MInt boxcount=0;
  m_ioSolution->getAttribute(&boxcount, "noBoxes", "");

  if(boxcount==0) {
    ErrorCode="Can't read Number of Boxes";
    return false;
  }

  m_noStrctrdLevel=boxcount;
  m_level->resize(m_noStrctrdLevel);

  for(MInt i=0;i<m_noStrctrdLevel;i++) {
    (*m_level)[i].name="box"+to_string(i);
  }

  return true;
}

bool structuredGrid::saveLevelsPlane()
{
  //TotalNumber and Names for m_level
  MInt planecount=0;
  m_ioSolution->getAttribute(&planecount, "noPlanes", "");  

  if(planecount==0) {
    ErrorCode="Can't read Number of Planes";
    return false;
  }

  m_noStrctrdLevel=planecount;
  m_level->resize(m_noStrctrdLevel);

  for(MInt i=0;i<m_noStrctrdLevel;i++) {
    (*m_level)[i].name="plane"+to_string(i);
  }

  return true;
}

bool structuredGrid::saveLevelsAux()
{
  //TotalNumber amd Names for m_level
  //Different than the others as the Windows don't start at zero.
  //MInt auxcount=0;
  std::vector<MString> datasetNames = m_ioSolution->getDatasetNames("");  
  MInt auxcount = datasetNames.size();

  if(auxcount==0) {
    ErrorCode="Can't read Number of Windows";
    return false;
  }

  m_noStrctrdLevel=auxcount;
  m_level->resize(m_noStrctrdLevel);
  
  for(MInt i=0;i<m_noStrctrdLevel;i++) {
    (*m_level)[i].name=datasetNames[i];
  }

  return true;
}

void structuredGrid::readSelectedLevels()
{
  //Number of the selectd Level - is the size of of the m_levelAttributes
  MInt counter=0;
  for(MInt i=0; i<m_noStrctrdLevel; i++) {
    if( (*m_level)[i].status==1) {
      counter+=1;
    }
  }
  
  if(counter==0) {
    //In Order to work at the start-as nothing is selected yet - no Level has Status 1)
    m_levelAttributes->resize(1);
    (*m_levelAttributes)[0].name=(*m_level)[0].name;
    m_noStrctrdLevelSelected=1;
  } else {
    m_noStrctrdLevelSelected=counter;
    m_levelAttributes->resize(m_noStrctrdLevelSelected);
  }
  
  //find the selected Levels
  counter=0;
  for(MInt i=0; i<m_noStrctrdLevel;i++) {
    if( (*m_level)[i].status==1) {
      (*m_levelAttributes)[counter].name=(*m_level)[i].name;
      counter++;
    }
  }
}

void structuredGrid::readSelectedLevelAttributes()
{
  //Read the selected Level properties
  switch(m_FT)
  {
  case Filetype::box:
  {
    for(MInt i=0; i<m_noStrctrdLevelSelected; i++)
    {
      MInt bufBlock;
      string bufName=m_timePath+"/"+(*m_levelAttributes)[i].name;
      m_ioSolution->getAttribute(&bufBlock, "blockId", "");
      (*m_levelAttributes)[i].blockId=bufBlock;
    }
    break;
  }
  case Filetype::plane:
  {
    for(MInt i=0; i<m_noStrctrdLevelSelected; i++)
    {
      MInt bufIntVar; //bufferIntergerVariable
      string bufName=m_timePath+"/"+(*m_levelAttributes)[i].name;
      m_ioSolution->getAttribute(&bufIntVar, "blockId", bufName);
      (*m_levelAttributes)[i].blockId=bufIntVar;

      m_ioSolution->getAttribute(&bufIntVar, "normal", bufName);
      (*m_levelAttributes)[i].normal=bufIntVar;
    }
    break;
  }
  default:
  {
    //do nothing
  }  
  }
}


bool structuredGrid::readDataInformation(const std::string& name)
{
  std::vector<MString> datasetNames = m_ioSolution->getDatasetNames(name);
  m_noStrctrdDataset = datasetNames.size();
  m_levelSelected->resize(m_noStrctrdDataset);
  
  if(m_noStrctrdDataset<0) {
    ErrorCode="E: Couldn't read Number of Datasets";
    return false;
  }

  for(MInt i=0;i<m_noStrctrdDataset;i++) {
    (*m_levelSelected)[i].name.assign(datasetNames[i]);
  }

  return true;
}

void structuredGrid::deleteCoordFromData()
{
  vector<StrctrdDataset> ::iterator it;
  MInt i=0;
  for(it=m_levelSelected->begin() ;it!=m_levelSelected->end() ;it++,i++) {
    if      ( (*m_levelSelected)[i].name.compare("z")==0){m_levelSelected->erase(it);it--;i--;}
    else if ( (*m_levelSelected)[i].name.compare("y")==0){m_levelSelected->erase(it);it--;i--;}
    else if ( (*m_levelSelected)[i].name.compare("x")==0){m_levelSelected->erase(it);it--;i--;}
  }
  m_noStrctrdDataset = m_levelSelected->size();
}

void structuredGrid::readLevelSizes2D_A()
{
  //Loop over all selected Levels
  for(MInt i=0; i<m_noStrctrdLevelSelected; i++) {
    const string Name =  "/"+(*m_levelAttributes)[i].name+"/x";    
    std::vector<ParallelIo::size_type> size(2, 0);
    m_ioSolution->getArraySize(Name, "", &size[0]);
    (*m_levelAttributes)[i].sizeX=size[1];
    (*m_levelAttributes)[i].sizeY=size[0];
  }
}

void structuredGrid::readLevelSizes(const MInt nDim)
{
  //Loop over all selected Levels
  for(MInt i=0; i<m_noStrctrdLevelSelected; i++) {
    const string Name = "/"+(*m_levelAttributes)[i].name+"/x";
    std::vector<ParallelIo::size_type> size(nDim, 0);
    cout << "Reading grid array size..." << endl;
    m_ioGrid->getArraySize(Name, "", &size[0]);
    cout << "Reading grid array size... DONE" << endl;
    (*m_levelAttributes)[i].sizeX=size[nDim-1];
    (*m_levelAttributes)[i].sizeY=size[nDim-2];
    if(nDim==3) {
      (*m_levelAttributes)[i].sizeZ=size[nDim-3];
    }
  }
}


void structuredGrid::readLevelSizesBox_nc()
{
  //Loop over all selected Levels
  for(MInt i=0; i<m_noStrctrdLevelSelected; i++) {    
    const string Name = m_timePath+"/"+(*m_levelAttributes)[i].name+"/"+(*m_levelSelected)[0].name;    
    std::vector<ParallelIo::size_type> size(3, 0);
    m_ioGrid->getArraySize(Name, "", &size[0]);     
    (*m_levelAttributes)[i].sizeX=size[2];
    (*m_levelAttributes)[i].sizeY=size[1];
    (*m_levelAttributes)[i].sizeZ=size[0];
  }
}


void structuredGrid::readLevelSizesPlane_nc()
{
  //Loop over all selected Levels
  for(MInt i=0; i<m_noStrctrdLevelSelected; i++) {
    const string sBlockName = "/block"+to_string((*m_levelAttributes)[i].blockId)+"/x";
    std::vector<ParallelIo::size_type> size(3, 0);
    m_ioGrid->getArraySize(sBlockName, "", &size[0]);    

    if((*m_levelAttributes)[i].normal==0) {
      (*m_levelAttributes)[i].sizeX=size[2];
      (*m_levelAttributes)[i].sizeY=size[1];
    }

    if((*m_levelAttributes)[i].normal==1) {
      (*m_levelAttributes)[i].sizeX=size[2];
      (*m_levelAttributes)[i].sizeZ=size[0];
    }

    if((*m_levelAttributes)[i].normal==2) {
      (*m_levelAttributes)[i].sizeY=size[1];
      (*m_levelAttributes)[i].sizeZ=size[0];
    }
  }
}

void structuredGrid::readLevelSizesBox_wc()
{
  //Loop over all selected Levels
  for(MInt i=0; i<m_noStrctrdLevelSelected; i++) {
    const string Name = m_timePath+"/"+(*m_levelAttributes)[i].name+"/x";
    std::vector<ParallelIo::size_type> size(3, 0);
    m_ioSolution->getArraySize(Name, "", &size[0]);   
    (*m_levelAttributes)[i].sizeX=size[2];
    (*m_levelAttributes)[i].sizeY=size[1];
    (*m_levelAttributes)[i].sizeZ=size[0];
  }
}

void structuredGrid::readLevelSizesPlane_wc()
{
  //Loop over all selected Levels
  for(MInt i=0; i<m_noStrctrdLevelSelected; i++) {
    const string Name = m_timePath+"/"+(*m_levelAttributes)[i].name+"/x";
    std::vector<ParallelIo::size_type> size(3, 0);
    m_ioSolution->getArraySize(Name, "", &size[0]);    
    (*m_levelAttributes)[i].sizeX=size[1];
    (*m_levelAttributes)[i].sizeY=size[0];
  }
}

// void structuredGrid::setupGrid2D(const std:: string& name,
//                                  vtkPoints& points,const MInt (&subEx)[6]) const
// {
//   const ParallelIo::size_type size[2] = {subEx[3]-subEx[2]+1,
//                                              subEx[1]-subEx[0]+1};
//   const ParallelIo::size_type offset[2] = {subEx[2],
//                                                subEx[0]};
//   const MInt noGridPoints=size[0]*size[1];
  
//   std::unique_ptr<MFloat[]> x = std::make_unique<MFloat[]>(noGridPoints);
//   std::unique_ptr<MFloat[]> y = std::make_unique<MFloat[]>(noGridPoints);

//   const string xLevelName = name+"/x";
//   const string yLevelName = name+"/y";
  
//   m_ioGrid->readArray(&x[0], "", xLevelName, 2, offset, size);
//   m_ioGrid->readArray(&y[0], "", yLevelName, 2, offset, size);

//   for(MInt k=0; k<size[0]; k++ ) {
//     for(MInt j=0; j<size[1]; j++ ) {
//       const MInt pointId=j+k*size[1];
//       points.InsertNextPoint(x[pointId],y[pointId],0);
//     }
//   }
// }


// void structuredGrid::setupGrid3D(const std:: string& name,
//                                  vtkPoints& points,const MInt (&subEx)[6]) const
// {
//   const ParallelIo::size_type size[3] = {subEx[5]-subEx[4]+1,
//                                              subEx[3]-subEx[2]+1,
//                                              subEx[1]-subEx[0]+1};

//   const ParallelIo::size_type offset[3] = {m_globalOffset[0] + subEx[4],
//                                                m_globalOffset[1] + subEx[2],
//                                                m_globalOffset[2] + subEx[0]};

//   const MInt noGridPoints=size[0]*size[1]*size[2];

//   std::unique_ptr<MFloat[]> x = std::make_unique<MFloat[]>(noGridPoints);
//   std::unique_ptr<MFloat[]> y = std::make_unique<MFloat[]>(noGridPoints);
//   std::unique_ptr<MFloat[]> z = std::make_unique<MFloat[]>(noGridPoints);

//   const string xLevelName = name+"/x";
//   const string yLevelName = name+"/y";
//   const string zLevelName = name+"/z";
  
//   m_ioGrid->readArray(&x[0], "", xLevelName, 3, offset, size);
//   m_ioGrid->readArray(&y[0], "", yLevelName, 3, offset, size);
//   m_ioGrid->readArray(&z[0], "", zLevelName, 3, offset, size);    

//   for(MInt k=0; k<size[0]; k++ ) {
//     for(MInt j=0; j<size[1]; j++ ) {
//       for(MInt ii=0;ii<size[2];ii++) {
//         const MInt pointId=ii+(j+k*size[1])*size[2];
//         points.InsertNextPoint(x[pointId],y[pointId],z[pointId]);
//       }
//     }
//   }
// }

// void structuredGrid::setupGridPlaneNC(const MInt level, const std:: string& name,
//                                       vtkPoints& points,const MInt (&subEx)[6]) const
// {
//   const ParallelIo::size_type size[3] = {subEx[5]-subEx[4]+1,
//                                              subEx[3]-subEx[2]+1,
//                                              subEx[1]-subEx[0]+1};

//   const ParallelIo::size_type offset[3] = {subEx[4],
//                                                subEx[2],
//                                                subEx[0]};

//   const MInt noGridPoints=size[0]*size[1]*size[2];
//   std::unique_ptr<MFloat[]> x = std::make_unique<MFloat[]>(noGridPoints);
//   std::unique_ptr<MFloat[]> y = std::make_unique<MFloat[]>(noGridPoints);
//   std::unique_ptr<MFloat[]> z = std::make_unique<MFloat[]>(noGridPoints);
  
//   const string xLevelName = name+"/x";
//   const string yLevelName = name+"/y";
//   const string zLevelName = name+"/z";
  
//   m_ioGrid->readArray(&x[0], "", xLevelName, 3, offset, size);
//   m_ioGrid->readArray(&y[0], "", yLevelName, 3, offset, size);
//   m_ioGrid->readArray(&z[0], "", zLevelName, 3, offset, size);

//   if((*m_levelAttributes)[level].normal==0) {
//     for(MInt i=0; i<size[1]; i++ ) {
//       for( MInt j=0; j<size[2]; j++) {
//         const MInt pointId=j+i*size[2];
//         points.InsertNextPoint(x[pointId],y[pointId],z[pointId]);
//       }
//     }
//   }

//   if((*m_levelAttributes)[level].normal==1) {
//     for(MInt i=0; i<size[0]; i++ ) {
//       for( MInt j=0; j<size[2]; j++) {
//         const MInt pointId=j+i*size[2];
//         points.InsertNextPoint(x[pointId],y[pointId],z[pointId]);
//       }
//     }
//   }

//   if((*m_levelAttributes)[level].normal==2) {
//     for(MInt i=0; i<size[0]; i++)  {
//       for( MInt j=0; j<size[1]; j++) {
//         const MInt pointId=j+i*size[1];
//         points.InsertNextPoint(x[pointId],y[pointId],z[pointId]);
//       }
//     }
//   }
// }

// void structuredGrid::setupGridPlaneWCuAUX(const std:: string& name,
//                                           vtkPoints& points, const MInt (&subEx)[6]) const
// {
//   const ParallelIo::size_type size[2] = {subEx[3]-subEx[2]+1,
//                                              subEx[1]-subEx[0]+1};

//   const ParallelIo::size_type offset[2] = {subEx[2],
//                                                subEx[0]};

//   const MInt noGridPoints=size[0]*size[1];
//   std::unique_ptr<MFloat[]> x = std::make_unique<MFloat[]>(noGridPoints);
//   std::unique_ptr<MFloat[]> y = std::make_unique<MFloat[]>(noGridPoints);
//   std::unique_ptr<MFloat[]> z = std::make_unique<MFloat[]>(noGridPoints);  

//   const string xLevelName = name+"/x";
//   const string yLevelName = name+"/y";
//   const string zLevelName = name+"/z";
  
//   m_ioSolution->readArray(&x[0], "", xLevelName, 2, offset, size);
//   m_ioSolution->readArray(&y[0], "", yLevelName, 2, offset, size);
//   m_ioSolution->readArray(&z[0], "", zLevelName, 2, offset, size);
  
//   for(MInt k=0; k<size[0]; k++ ) {
//     for(MInt j=0; j<size[1]; j++ ) {
//       const MInt pointId=j+k*size[1];
//       points.InsertNextPoint(x[pointId],y[pointId],z[pointId]);
//     }
//   }
// }

// void structuredGrid::insertData2D(const MInt var,const std::string& name,
//                                   vtkDoubleArray& varArray,const MInt (&subEx)[6]) const
// {
//   ParallelIo::size_type size[2];
//   switch(m_hasCoor)
//   {
//   case false:
//   {
//     size[0]=subEx[3]-subEx[2];
//     size[1]=subEx[1]-subEx[0];
//     break;
//   }
//   case true:
//   {
//     size[0]=subEx[3]-subEx[2]+1;
//     size[1]=subEx[1]-subEx[0]+1;
//   }
//   }
//   ParallelIo::size_type offset[2];
//   offset[0]=subEx[2];
//   offset[1]=subEx[0];
//   const MInt cellsize=size[0]*size[1];

//   varArray.SetNumberOfComponents(1);
//   varArray.SetNumberOfTuples(cellsize);
//   varArray.SetName((*m_levelSelected)[var].name.c_str());  

//   std::unique_ptr<MFloat[]> dummyArray = std::make_unique<MFloat[]>(cellsize);  
//   m_ioSolution->readArray(&dummyArray[0], "", name, 2, offset, size);  

//   for(MInt k=0; k<size[0]; k++ ) {
//     for(MInt j=0; j<size[1]; j++ ) {
//       const MInt cellId=j+k*size[1];
//       varArray.SetValue(cellId, dummyArray[cellId]);
//     }
//   }  
// }

// void structuredGrid::insertData3D(const MInt var,const std::string& name,
//                                   vtkDoubleArray& varArray,const MInt (&subEx)[6]) const
// {
//   ParallelIo::size_type size[3] = {0,0,0};
//   switch(m_hasCoor)
//   {
//   case false:
//   {
//     size[0]=subEx[5]-subEx[4];
//     size[1]=subEx[3]-subEx[2];
//     size[2]=subEx[1]-subEx[0];
//     break;
//   }
//   case true:
//   {
//     size[0]=subEx[5]-subEx[4]+1;
//     size[1]=subEx[3]-subEx[2]+1;
//     size[2]=subEx[1]-subEx[0]+1;

//   }
//   }
//   ParallelIo::size_type offset[3];
//   offset[0]=subEx[4];
//   offset[1]=subEx[2];
//   offset[2]=subEx[0];

//   const MInt cellsize=size[0]*size[1]*size[2];
  
//   varArray.SetNumberOfComponents(1);
//   varArray.SetNumberOfTuples(cellsize);
//   varArray.SetName((*m_levelSelected)[var].name.c_str());

//   std::unique_ptr<MFloat[]> dummyArray = std::make_unique<MFloat[]>(cellsize);
  
//   m_ioSolution->readArray(&dummyArray[0], "", name, 3, offset, size);    

//   for(MInt k=0; k<size[0]; k++ ) {
//     for(MInt j=0; j<size[1]; j++ ) {
//       for(MInt ii=0;ii<size[2];ii++) {
//         const MInt cellId=ii+(j+k*size[1])*size[2];
//         varArray.SetValue(cellId, dummyArray[cellId]);
//       }
//     }
//   }
// }

// void structuredGrid::insertDataPlaneNC(const MInt level,const MInt var,
//                                        const std::string& name,
//                                        vtkDoubleArray& varArray,const MInt (&subEx)[6]) const
// {
//   ParallelIo::size_type size[2] = {0,0};
//   ParallelIo::size_type offset[2] = {0,0};
  
//   switch((*m_levelAttributes)[level].normal)
//   {
//   case 0:
//   {
//     size[0]=subEx[3]-subEx[2];
//     size[1]=subEx[1]-subEx[0];
//     offset[0]=subEx[2];
//     offset[1]=subEx[0];
//     break;
//   }
//   case 1:
//   {
//     size[0]=subEx[5]-subEx[4];
//     size[1]=subEx[1]-subEx[0];
//     offset[0]=subEx[4];
//     offset[1]=subEx[0];
//     break;
//   }
//   case 2:
//   {
//     size[0]=subEx[5]-subEx[4];
//     size[1]=subEx[3]-subEx[2];
//     offset[0]=subEx[4];
//     offset[1]=subEx[2];
//   }

//   }
//   const MInt cellsize=size[0]*size[1];

//   varArray.SetNumberOfComponents(1);
//   varArray.SetNumberOfTuples(cellsize);
//   varArray.SetName((*m_levelSelected)[var].name.c_str());

//   std::unique_ptr<MFloat[]> dummyArray = std::make_unique<MFloat[]>(cellsize);  

//   m_ioSolution->readArray(&dummyArray[0], "", name, 2, offset, size);

//   for(MInt k=0; k<size[0]; k++){
//     for(MInt j=0; j<size[1]; j++ ) {
//       const MInt cellId=j+k*size[1];
//       varArray.SetValue(cellId, dummyArray[cellId]);
//     }
//   }
// }
