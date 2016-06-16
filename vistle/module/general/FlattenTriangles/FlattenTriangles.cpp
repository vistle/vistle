#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <core/object.h>
#include <core/triangles.h>
#include <core/normals.h>
#include <core/polygons.h>
#include <core/spheres.h>
#include <core/tubes.h>
#include <core/texture1d.h>

#include "FlattenTriangles.h"

MODULE_MAIN(FlattenTriangles)

using namespace vistle;

FlattenTriangles::FlattenTriangles(const std::string &shmname, const std::string &name, int moduleID)
   : Module("transform Polygons into Triangles", shmname, name, moduleID) {

   createInputPort("grid_in");
   createOutputPort("grid_out");
}

FlattenTriangles::~FlattenTriangles() {

}

template<int Dim>
struct Flatten {
   Triangles::const_ptr tri;
   DataBase::const_ptr object;
   DataBase::ptr result;
   Flatten(Triangles::const_ptr tri, DataBase::const_ptr obj, DataBase::ptr result)
      : tri(tri)
      , object(obj)
      , result(result)
      {
      }
   template<typename S> void operator()(S) {

      typedef Vec<S,Dim> V;
      typename V::const_ptr in(V::as(object));
      if (!in)
         return;
      typename V::ptr out(boost::dynamic_pointer_cast<V>(result));
      if (!out)
          return;
      if (out->getSize() != tri->getNumCorners())
          return;

      Index N = tri->getNumCorners();
      auto cl = tri->cl();
      for (int c=0; c<Dim; ++c) {
         auto din = &in->x(c)[0];
         auto dout = out->x(c).data();

         for (Index i=0; i<N; ++i) {
             dout[i] = din[cl[i]];
         }
      }
   }
};

DataBase::ptr flatten(Triangles::const_ptr tri, DataBase::const_ptr src, DataBase::ptr result) {

   boost::mpl::for_each<Scalars>(Flatten<1>(tri, src, result));
   boost::mpl::for_each<Scalars>(Flatten<3>(tri, src, result));
   return result;
}

bool FlattenTriangles::compute() {

   auto data = expect<DataBase>("grid_in");
   if (!data) {
      return true;
   }
   auto grid = data->grid();
   if (!grid) {
      grid = data;
      data.reset();
   }

   auto in = Triangles::as(grid);
   if (!in) {
       sendError("did not receive Triangles");
       return true;
   }

   if (in->getNumCorners() == 0) {
       // already flat
       passThroughObject("grid_out", in);
       return true;
   }

   Triangles::ptr tri = in->createEmpty();
   tri->setSize(in->getNumCorners());
   flatten(in, in, tri);
   tri->setMeta(in->meta());
   tri->copyAttributes(in);
   if (data) {
       DataBase::ptr dout = data->createEmpty();
       flatten(in, data, dout);
       dout->setMeta(data->meta());
       dout->copyAttributes(data);
       dout->setGrid(tri);
       addObject("grid_out", dout);
   } else {
       addObject("grid_out", tri);
   }

   return true;
}
