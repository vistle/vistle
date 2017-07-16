#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <core/object.h>
#include <core/unstr.h>

#include "ToPolyhedra.h"

MODULE_MAIN(ToPolyhedra)

using namespace vistle;

ToPolyhedra::ToPolyhedra(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("transform unstructured cells into polyhedral cells", name, moduleID, comm) {

   createInputPort("grid_in");
   createOutputPort("grid_out");
}

ToPolyhedra::~ToPolyhedra() {

}


bool ToPolyhedra::compute() {

   auto data = expect<DataBase>("grid_in");
   if (!data) {
      return true;
   }
   auto obj = data->grid();
   if (!obj) {
      obj = data;
      data.reset();
   }

   auto grid = UnstructuredGrid::as(obj);

   const Index *iel = &grid->el()[0];
   const Index *icl = &grid->cl()[0];
   const unsigned char *itl = &grid->tl()[0];
   const Index nel = grid->getNumElements();
   auto poly = std::make_shared<UnstructuredGrid>(nel, 0, 0);
   poly->d()->x[0] = grid->d()->x[0];
   poly->d()->x[1] = grid->d()->x[1];
   poly->d()->x[2] = grid->d()->x[2];
   Index *oel = &poly->el()[0];
   unsigned char *otl = &poly->tl()[0];
   auto &ocl = poly->cl();
   for (Index elem=0; elem<nel; ++elem) {
       oel[elem] = ocl.size();
       otl[elem] = UnstructuredGrid::POLYHEDRON | (itl[elem]&~UnstructuredGrid::TYPE_MASK);
       unsigned char t = itl[elem]&UnstructuredGrid::TYPE_MASK;
       if (t == UnstructuredGrid::POLYHEDRON) {
           const Index begin = iel[elem], end = iel[elem+1];
           for (Index i=begin; i<end; ++i) {
               ocl.push_back(icl[i]);
           }
       } else {
           switch (t) {
           case UnstructuredGrid::TETRAHEDRON:
           case UnstructuredGrid::PYRAMID:
           case UnstructuredGrid::PRISM:
           case UnstructuredGrid::HEXAHEDRON: {
               const auto numFaces = UnstructuredGrid::NumFaces[t];
               const auto &faces = UnstructuredGrid::FaceVertices[t];
               const Index begin = iel[elem];
               for (int f=0; f<numFaces; ++f) {
                   const auto &face = faces[f];
                   const auto facesize = UnstructuredGrid::FaceSizes[t][f];
                   ocl.push_back(facesize);
                   for (int k=0; k<facesize; ++k) {
                       ocl.push_back(icl[begin+face[k]]);
                   }
               }
               break;
           }
           default:
               std::cerr << "unhandled cell type " << int(t) << std::endl;
           }
       }
   }
   oel[nel] = ocl.size();

   if (poly) {
      poly->setMeta(obj->meta());
      poly->copyAttributes(obj);
   }

   if (data) {
      auto ndata = data->clone();
      ndata->setMeta(data->meta());
      ndata->copyAttributes(data);
         ndata->setGrid(poly);
         addObject("grid_out", ndata);
   } else {
       addObject("grid_out", poly);
   }

   return true;
}
