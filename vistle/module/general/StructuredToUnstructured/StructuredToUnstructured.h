//-------------------------------------------------------------------------
// STRUCTURED TO UNSTRUCTURED H
// * converts a structured grid to an unstructured grid
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef STRUCTURED_TO_UNSTRUCTURED_H
#define STRUCTURED_TO_UNSTRUCTURED_H

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
class StructuredToUnstructured : public vistle::Module {
 public:


   StructuredToUnstructured(const std::string &shmname, const std::string &name, int moduleID);
   ~StructuredToUnstructured();

 private:
   // overriden functions
   virtual bool compute();

   // private helper functions
   void compute_uniformVecs(vistle::UniformGrid::const_ptr obj, vistle::UnstructuredGrid::ptr unstrGridOut,
                               const Cartesian3<vistle::Index> numVertices);
   void compute_rectilinearVecs(vistle::RectilinearGrid::const_ptr obj, vistle::UnstructuredGrid::ptr unstrGridOut,
                                   const Cartesian3<vistle::Index> numVertices);
   void compute_structuredVecs(vistle::StructuredGrid::const_ptr obj, vistle::UnstructuredGrid::ptr unstrGridOut,
                               const vistle::Index numVerticesTotal);

   // private constant members
   const vistle::Index M_NUM_CORNERS_HEXAHEDRON = 8;
};



#endif /* STRUCTURED_TO_UNSTRUCTURED_H */
