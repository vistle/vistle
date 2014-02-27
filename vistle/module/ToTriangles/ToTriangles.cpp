#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/polygons.h>
#include <core/triangles.h>

#include "ToTriangles.h"

MODULE_MAIN(ToTriangles)

using namespace vistle;

ToTriangles::ToTriangles(const std::string &shmname, int rank, int size, int moduleID)
   : Module("transform Polygons into Triangles", shmname, rank, size, moduleID) {

   createInputPort("grid_in");
   createOutputPort("grid_out");
}

ToTriangles::~ToTriangles() {

}

bool ToTriangles::compute() {

   while (Object::const_ptr obj = takeFirstObject("grid_in")) {

      auto poly = Polygons::as(obj);
      if (!poly) {
         std::cerr << "ToTriangles: incompatible input" << std::endl;
         continue;
      }

      Index nelem = poly->getNumElements();
      Index nvert = poly->getNumCorners();
      Index ntri = nvert-2*nelem;

      //FIXME: reuse poly's coords
      Triangles::ptr tri(new Triangles(3*ntri, poly->getNumCoords()));
      std::copy(poly->x().begin(), poly->x().end(), tri->x().begin());
      std::copy(poly->y().begin(), poly->y().end(), tri->y().begin());
      std::copy(poly->z().begin(), poly->z().end(), tri->z().begin());

      Index i = 0;
      auto el = poly->el().data();
      auto cl = poly->cl().data();
      auto tcl = tri->cl().data();
      for (Index e=0; e<nelem; ++e) {
         const Index begin=el[e], end=el[e+1], last=end-1;
         for (Index v=begin; v<end-2; ++v) {
            const Index v2 = v/2;
            if (v%2) {
               tcl[i++] = cl[begin+v2+1];
               tcl[i++] = cl[last-v2-1];
               tcl[i++] = cl[last-v2];
            } else {
               tcl[i++] = cl[begin+v2+1];
               tcl[i++] = cl[begin+v2];
               tcl[i++] = cl[last-v2];
            }
         }
      }
      vassert(i == 3*ntri);

      addObject("grid_out", tri);
   }

   return true;
}
