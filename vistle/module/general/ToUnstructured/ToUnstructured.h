//-------------------------------------------------------------------------
// TO UNSTRUCTURED H
// * converts a grid to an unstructured grid
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef TO_UNSTRUCTURED_H
#define TO_UNSTRUCTURED_H

#include <module/module.h>
#include <core/object.h>
#include <core/unstr.h>
#include <core/structuredgridbase.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>

#include "Cartesian3.h"


//-------------------------------------------------------------------------
// STRUCTURED TO UNSTRUCTURED CLASS DECLARATION
//-------------------------------------------------------------------------
class ToUnstructured : public vistle::Module {
 public:


   ToUnstructured(const std::string &name, int moduleID, mpi::communicator comm);
   ~ToUnstructured();

 private:
   // overriden functions
   virtual bool compute() override;

   // private helper functions
   void compute_uniformVecs(vistle::UniformGrid::const_ptr obj, vistle::UnstructuredGrid::ptr unstrGridOut,
                               const Cartesian3<vistle::Index> numVertices);
   void compute_rectilinearVecs(vistle::RectilinearGrid::const_ptr obj, vistle::UnstructuredGrid::ptr unstrGridOut,
                                   const Cartesian3<vistle::Index> numVertices);

   // private constant members
   const vistle::Index M_NUM_CORNERS_HEXAHEDRON = 8;
};



#endif /* TO_UNSTRUCTURED_H */
