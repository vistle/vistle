#ifndef MAIASTRCTRD_STRUCTS_H_
#define MAIASTRCTRD_STRUCTS_H_

#include <string>

//Needed for Interaction with GUI. Stores Name and Status
//Stores the solvers which are to be displayed
struct StrctrdLevel
{
  std::string name;
  int status;
  StrctrdLevel()
    {
      name="Start";
      status=1; //If 1 then the Level should be loaded
    }
};

//Needed for Interaction with GUI. Stores Name and Status
//Stores the datasets which are to be displayed, by default is set to 1
struct StrctrdDataset
{
  std::string name;
  int status;
  StrctrdDataset()
    {
      name="Start";
      status=1; //If 1 then the Dataset should be loaded
    }
};

//Struct with all the needed Attributes of a Level
struct StrctrdLevelAttr
{
  std::string name;
  int blockId;
  int normal;
  int sizeX;
  int sizeY;
  int sizeZ;
  StrctrdLevelAttr()
    {
      name="Start";
      blockId=-1;
      normal=-1;
      sizeX=0;
      sizeY=0;
      sizeZ=0;
    }
};

enum class Filetype { grid, solution, box, plane, aux };

#endif
