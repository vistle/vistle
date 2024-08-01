#ifndef MAIASTRCTRD_H_
#define MAIASTRCTRD_H_

#include <string>
#include <vector>
#include <memory>
#include "mpi.h"
#include "strctrd_structs.h"
#include "IO/parallelio.h"
#include "IO/parallelio_hdf5.h"
#include "INCLUDE/maiatypes.h"
//#include "IO/parallelio_hdf5.h"

class structuredGrid
{

public:
  //Constructor in RequestInformation
  structuredGrid(const std::string& Filename_,
                 std::vector<StrctrdLevel>&,
                 std::vector<StrctrdDataset>&,
                 std::vector<StrctrdLevelAttr>&);

  //Constructor in RequestData, needs the buffered informatons.
  structuredGrid(const std::string& gridFileName_,
                 const std::string& dataFileName_,
                 std::vector<StrctrdLevel>&,
                 std::vector<StrctrdDataset>&,
                 std::vector<StrctrdLevelAttr>&,
                 MInt noDims_,
                 MBool hasCoor_,
                 std::string timePath_,
                 Filetype fileType_);

  bool initialize();  //determines Filetype (grid,box,...) and basic informations for each Filetype
  bool checkLevels(); //read necessary grid information of the level.
  bool checkData();   //read basic information about the datasets(names, number)
  void setupLevels(); //store the sizes of the Grids
  // void setupGrid(MInt level,vtkPoints& PS,const MInt (&subEx)[6]) const; //provide vtk the actual grid.
  // void setupData(MInt level,MInt variable, vtkDoubleArray& vA,const MInt (&subEx)[6]) const; //provide vtk the actual datasets


//getter
  std::string getErrorCode() const;
  bool getIfWarning() const; //return if there is a warning to be shown in the vtk-part
  std::string getWarningCode() const;
  MInt getNoStrctrdLevel() const;
  MInt getNoStrctrdLevelSelected() const;
  MInt getNoStrctrdDataset() const;
  MInt getnoDims() const;
  bool gethasCoor() const;
  std::string getTimestep() const;
  Filetype getFiletype() const;
  std::string getGridFileName() const { return m_gridFileName; }

private:

  std::string m_fileName = "";                         //Name of the file
  std::string m_gridFileName = "";                         //Name of the grid File
  MBool m_oldStructure = false;

  std:: string ErrorCode; //holds the error code to be returned to the vtk-part.
  std::pair <bool, std::string> WarningPair; //pair.first is if there is a warning, pair.second is the message.
  MInt m_noStrctrdLevel;                    //Number of all Levels in the file
  MInt m_noStrctrdLevelSelected;                    //Number of the selected Levels
  MInt m_noStrctrdDataset;                   //Number of the Datasets;

  std::vector<StrctrdLevel>* m_level;  //Vector of all Levels and their status.
  std::vector<StrctrdDataset>* m_levelSelected;  //Vector holds all selected Levels and their important attributes 
  std::vector<StrctrdLevelAttr>* m_levelAttributes;  //Vector of all Datasets and their status.
  
  MInt m_noDims;                                         //Number of Dimensions
  bool m_hasCoor;                               //If the file has Coordinates or not
  std::string m_timePath;	//holds the needed subfolder of the file structure. Box/Plane only
  Filetype m_FT;					//Filetype of the file

  std::shared_ptr<ParallelIoHdf5> m_ioSolution;
  std::shared_ptr<ParallelIoHdf5> m_ioGrid;

  MInt m_globalOffset[3] = {0,0,0};

  bool checkTimeStep(); //get the timestep. Is a "subfolder" of the hdf5 file structure.
  bool checkCoordinates(); //check if Box/Plane has Coordinates
  bool openGrid(); //get the Gridname, check UIDs
  bool saveLevelsGrid_Sol();  //read and save the number of solvers
  bool saveLevelsBox();       //read and save the number of Boxes
  bool saveLevelsPlane();           //read and save the number of Planes
  bool saveLevelsAux();     //read and save the number of Windows
  void readSelectedLevels(); //determine which Levels should be visualizied
  void readSelectedLevelAttributes(); //read Attributes of Levels (Box/Plane)
  bool readDataInformation(const std::string& name); //read number and names of the Datasets
  void deleteCoordFromData();	//Box/Plane/Aux, the cordinates are grid informations and not Datasets
  void readLevelSizes2D_A(); //2D Gridssizes Grid,Solution,Aux
  void readLevelSizes(const MInt);	//3d Gridsizes of Solution
  void readLevelSizesBox_nc();	//Gridsizes Box without Coordinats
  void readLevelSizesPlane_nc();	//Gridsizes Plane without Coordinates
  void readLevelSizesBox_wc();	//Gridsizes Plane with Coordinates
  void readLevelSizesPlane_wc();	//Gridsizes Plane with Coordinates

  // void setupGrid2D(const std:: string& name, vtkPoints& PS,const MInt (&subEx)[6]) const;
  // void setupGrid3D(const std:: string& name, vtkPoints& PS,const MInt (&subEx)[6]) const;
  // void setupGridPlaneNC(const MInt level, const std:: string& name, vtkPoints& PS,const MInt (&subEx)[6]) const; // read and set the Gridpoints of Plane without coordinates
  // void setupGridPlaneWCuAUX(const std:: string& name, vtkPoints& PS, const MInt (&subEx)[6]) const; //read and set the Gridpoints of Plane with Coordinates and Aux

  // void insertData2D(const MInt var,const std::string& name, vtkDoubleArray& vA,const MInt (&subEx)[6]) const;
  // void insertData3D(const MInt var,const std::string& name, vtkDoubleArray& vA,const MInt (&subEx)[6]) const;
  // void insertDataPlaneNC(const MInt level,const MInt var,const std::string& name, vtkDoubleArray& vA,const MInt (&subEx)[6]) const; //read and set the Datapoints of Plane without Coordinates


};

#endif
