#ifndef FOAMTOOLBOX_H
#define FOAMTOOLBOX_H

#include <cstdlib>
#include <istream>
#include <vector>
#include <string>
#include <map>

#include <boost/shared_ptr.hpp>

// this header should contain typedefs for index_t and scalar_t
#include "foamtypes.h"

struct HeaderInfo
{
   HeaderInfo()
       : lines(0)
       , valid(false)
   {}

   std::string version;
   std::string format;
   std::string fieldclass;
   std::string location;
   std::string object;
   std::string dimensions;
   std::string internalField;
   index_t lines;
   bool valid;
};

struct DimensionInfo {
    DimensionInfo()
        : points(0)
        , cells(0)
        , faces (0)
        , internalFaces(0)
   {}
   index_t points;
   index_t cells;
   index_t faces;
   index_t internalFaces;
};


struct CaseInfo {

   CaseInfo()
   : numblocks(0), varyingGrid(false), varyingCoords(false), valid(false)
   {}

   std::map<double, std::string> timedirs; //Map of all the Time Directories
   std::map<std::string, int> varyingFields, constantFields;
   std::string constantdir;
   int numblocks;
   bool varyingGrid, varyingCoords;
   bool valid;
};

class Boundary {
 public:
   Boundary(const std::string &name, const index_t s, const index_t num, const std::string &t, const index_t &ind)
      : name(name)
      , startFace(s)
      , numFaces(num)
      , type(t)
      , index(ind)
      {
      }

   std::string name;
   index_t startFace;
   index_t numFaces;
   std::string type;
   index_t index;
};


class Boundaries {
 public:
   Boundaries(): valid(false) {}

   bool isBoundaryFace(const index_t face) {

      return isBoundaryFace(boundaries, face);
   }

   bool isProcessorBoundaryFace(const index_t face) {

      return isBoundaryFace(procboundaries, face);
   }

   void addBoundary(const Boundary &b) {

      if (b.type == "processor") {
         procboundaries.push_back(b);
      } else {
         boundaries.push_back(b);
      }
   }

   int findBoundaryIndexByName(const std::string &b) {
      int result=-1;
      for (int i=0;i<boundaries.size();++i) {
         if (!b.compare(boundaries[i].name)) {
            result=i;
            break;
         }
      }
      return result;
   }

   bool valid;

   std::vector<Boundary> boundaries;
   std::vector<Boundary> procboundaries;

 private:
   bool isBoundaryFace(const std::vector<Boundary> &bound, const index_t face) {

      for (std::vector<Boundary>::const_iterator i = bound.begin(); i != bound.end(); ++i) {
         if (face >= i->startFace && face < i->startFace+i->numFaces)
            return true;
      }
      return false;
   }

};

CaseInfo getCaseInfo(const std::string &casedir, double mintime, double maxtime, int skipfactor=1, bool exact=false);

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
      const std::vector<std::vector<index_t>  > &faces);

#endif
