#ifndef FOAMTOOLBOX_H
#define FOAMTOOLBOX_H

#include <cstdlib>
#include <istream>
#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

// this header should contain typedefs for index_t and scalar_t
#include "foamtypes.h"

struct HeaderInfo
{
    std::string version;
    std::string format;
    std::string fieldclass;
    std::string location;
    std::string object;
    std::string dimensions;
    std::string internalField;
    index_t lines = 0;
    bool valid;

};

struct DimensionInfo {
   index_t points = 0;
   index_t cells = 0;
   index_t faces = 0;
   index_t internalFaces = 0;
};


struct CaseInfo {

   CaseInfo()
   : numblocks(0), varyingGrid(false), varyingCoords(false), valid(false)
   {}

   std::map<double, std::string> timedirs; //Map of all the Time Directories
   std::map<std::string, int> varyingFields, constantFields;
   int numblocks;
   bool varyingGrid, varyingCoords;
   bool valid;
};

class Boundary {
 public:
   Boundary(const std::string &name, const index_t s, const index_t num, const std::string &t)
      : name(name)
      , startFace(s)
      , numFaces(num)
      , type(t)
      {
      }

   const std::string name;
   const index_t startFace;
   const index_t numFaces;
   const std::string type;
};


class Boundaries {
 public:
   Boundaries() {}

   bool isBoundaryFace(const index_t face) {

      for (auto i = boundaries.begin(); i != boundaries.end(); i ++) {
         if (i->type == "processor")
            continue;
         if (face >= i->startFace && face < i->startFace + i->numFaces)
            return true;
      }
      return false;
   }

   bool isProcessorBoundaryFace(const index_t face) {

      for (auto i = boundaries.begin(); i != boundaries.end(); i ++) {
         if (i->type != "processor")
            continue;
         if (face >= i->startFace && face < i->startFace + i->numFaces)
            return true;
      }
      return false;
   }

   void addBoundary(const Boundary &b) {

      boundaries.push_back(b);
   }

 private:
   std::vector<Boundary> boundaries;
};

CaseInfo getCaseInfo(const std::string &casedir, double mintime, double maxtime);

boost::shared_ptr<std::istream> getStreamForFile(const std::string &filename);
boost::shared_ptr<std::istream> getStreamForFile(const std::string &dir, const std::string &basename);
HeaderInfo readFoamHeader(std::istream &stream);
DimensionInfo readDimensions(const std::string &meshdir);

bool readIndexArray(std::istream &stream, index_t *p, const size_t lines);
bool readIndexListArray(std::istream &stream, std::vector<index_t> *p, const size_t lines);
bool readFloatArray(std::istream &stream, scalar_t *p, const size_t lines);
bool readFloatVectorArray(std::istream &stream, scalar_t *x, scalar_t *y, scalar_t *z, const size_t lines);

Boundaries loadBoundary(const std::string &meshdir);

index_t findVertexAlongEdge(const index_t & point,
      const index_t & homeface,
      const std::vector<index_t> & cellfaces,
      const std::vector<std::vector<index_t> > & faces);
bool isPointingInwards(index_t &face,
      index_t &cell,
      index_t &ninternalFaces,
      std::vector<index_t> &owners,
      std::vector<index_t> &neighbors);
std::vector<index_t> getVerticesForCell(const std::vector<index_t> &cellfaces,
      const std::vector<std::vector<index_t>> &faces);

#endif
