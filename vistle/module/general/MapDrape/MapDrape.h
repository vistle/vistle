#ifndef MAPDRAPE_H
#define MAPDRAPE_H

#include <module/module.h>
#include <core/vector.h>

using namespace vistle;

class MapDrape: public vistle::Module {

  static const unsigned NumPorts = 5;

 public:
   MapDrape(const std::string &name, int moduleID, mpi::communicator comm);
   ~MapDrape();

 private:
   virtual bool compute();
   StringParameter *p_mapping_from_, *p_mapping_to_;
   IntParameter *p_permutation;
   VectorParameter *p_offset;
   void transformCoordinates(int numCoords, float * xIn, float * yIn, float * zIn, float * xOut, float * yOut, float * zOut);
   float offset[3];
   Port *data_in[NumPorts], *data_out[NumPorts];
};


#endif
