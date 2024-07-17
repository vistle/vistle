#ifndef MAIAVISREADERBASE_H_
#define MAIAVISREADERBASE_H_

#include <vector>
#include <vistle/core/unstr.h>
#include "dataset.h"
#include "block.h"
using namespace std;

namespace maiapv {

// Non-template abstract base class to allow easier handling
struct ReaderBase {
  virtual ~ReaderBase() = default;
  virtual vistle::UnstructuredGrid::ptr readGrid() = 0;
  virtual void read(const vector<maiapv::Dataset>& /*datasets*/) = 0;
  virtual vistle::DataBase::ptr readData(const maiapv::Dataset& /*datasets*/) = 0;
  virtual void readParticleData(vistle::UnstructuredGrid::ptr, const vector<Dataset>&){};
  virtual void requestInformation(vector<maiapv::Dataset>& /*datasets*/){};

  virtual MFloat requestTimeInformation(){return -1.0;};
  virtual void setLevelInformation(int* /*levelRange*/)=0;
  virtual void requestBoundingBoxInformation(double (&/*boundingBox*/)[6])=0;
  virtual MBool requestFiletype(){return false;};
  virtual MInt requestNumberOfSolvers(){return -1;};
  virtual MInt getSolverId(){return -1;};
  virtual void setMemoryIncrease(const double /*increase*/)=0;
  virtual MFloat getMemoryIncrease(){return 0.0;};
};
}
#endif
