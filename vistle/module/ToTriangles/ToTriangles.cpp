#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <core/object.h>
#include <core/triangles.h>
#include <core/normals.h>
#include <core/polygons.h>
#include <core/spheres.h>

#include "ToTriangles.h"

MODULE_MAIN(ToTriangles)

using namespace vistle;

ToTriangles::ToTriangles(const std::string &shmname, const std::string &name, int moduleID)
   : Module("transform Polygons into Triangles", shmname, name, moduleID) {

   createInputPort("grid_in");
   createInputPort("data_in");
   createOutputPort("grid_out");
   createOutputPort("normal_out");
   createOutputPort("data_out");
}

ToTriangles::~ToTriangles() {

}

template<int Dim>
struct ReplicateData {
   Object::const_ptr object;
   Object::ptr &result;
   Index n;
   ReplicateData(Object::const_ptr obj, Object::ptr &result, Index n)
      : object(obj)
      , result(result)
      , n(n)
      {
      }
   template<typename S> void operator()(S) {

      typedef Vec<S,Dim> V;
      typename V::const_ptr in(V::as(object));
      if (!in)
         return;

      typename V::ptr out(new V(in->getSize()*n));
      for (int i=0; i<Dim; ++i) {
         auto din = in->x(i).data();
         auto dout = out->x(i).data();

         for (Index j=0; j<in->getSize(); ++j) {
            for (Index k=0; k<n; ++k) {
               *dout++ = *din;
            }
            ++din;
         }
      }
      result = out;
   }
};

Object::ptr replicateData(Object::const_ptr src, Index n) {

   Object::ptr result;
   boost::mpl::for_each<Scalars>(ReplicateData<1>(src, result, n));
   boost::mpl::for_each<Scalars>(ReplicateData<3>(src, result, n));
   return result;
}

bool ToTriangles::compute() {

   auto obj = expect<Object>("grid_in");
   if (!obj) {
      return false;
   }

   auto data = accept<Object>("data_in");

   if (auto poly = Polygons::as(obj)) {
      Index nelem = poly->getNumElements();
      Index nvert = poly->getNumCorners();
      Index ntri = nvert-2*nelem;

      Triangles::ptr tri(new Triangles(3*ntri, 0));
      for (int i=0; i<3; ++i)
         tri->d()->x[i] = poly->d()->x[i];

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
      if (data)
         passThroughObject("data_out", data);
   }

   if (auto sphere = Spheres::as(obj)) {
      const int NumLat = 8;
      const int NumLong = 13;
      BOOST_STATIC_ASSERT(NumLat >= 3);
      BOOST_STATIC_ASSERT(NumLong >= 3);
      Index TriPerSphere = NumLong * (NumLat-2) * 2;
      Index CoordPerSphere = NumLong * (NumLat - 2) + 2;

      Index n = sphere->getNumSpheres();
      auto x = sphere->x().data();
      auto y = sphere->y().data();
      auto z = sphere->z().data();
      auto r = sphere->r().data();

      Triangles::ptr tri(new Triangles(n*3*TriPerSphere, n*CoordPerSphere));
      auto tx = tri->x().data();
      auto ty = tri->y().data();
      auto tz = tri->z().data();
      auto ti = tri->cl().data();

      Normals::ptr norm(new Normals(n*CoordPerSphere));
      auto nx = norm->x().data();
      auto ny = norm->y().data();
      auto nz = norm->z().data();

      const float psi = M_PI / (NumLat-1);
      const float phi = M_PI * 2 / NumLong;
      for (Index i=0; i<n; ++i) {

         // create normals
         { 
            Index ci = i*CoordPerSphere;
            // south pole
            nx[ci] = ny[ci] = 0.f;
            nz[ci] = -1.f;
            ++ci;

            float Psi = -M_PI*0.5+psi;
            for (Index j=0; j<NumLat-2; ++j) {
               float Phi = j*0.5f*phi;
               for (Index k=0; k<NumLong; ++k) {
                  nx[ci] = sin(Phi) * cos(Psi);
                  ny[ci] = cos(Phi) * cos(Psi);
                  nz[ci] = sin(Psi);
                  ++ci;
                  Phi += phi;
               }
               Psi += psi;
            }
            // north pole
            nx[ci] = ny[ci] = 0.f;
            nz[ci] = 1.f;
            ++ci;
            vassert(ci == (i+1)*CoordPerSphere);
         }

         // create coordinates from normals
         for (Index ci = i*CoordPerSphere; ci<(i+1)*CoordPerSphere; ++ci) {
            tx[ci] = nx[ci] * r[i] + x[i];
            ty[ci] = ny[ci] * r[i] + y[i];
            tz[ci] = nz[ci] * r[i] + z[i];
         }

         // create index list
         {
            Index ii = i*3*TriPerSphere;
            // indices for ring around south pole
            Index ci = i*CoordPerSphere+1;
            for (Index k=0; k<NumLong; ++k) {
               ti[ii++] = ci+(k+1)%NumLong;
               ti[ii++] = ci+k;
               ti[ii++] = i*CoordPerSphere;
            }

            ci = i*CoordPerSphere+1;
            for (Index j=0; j<NumLat-3; ++j) {
               for (Index k=0; k<NumLong; ++k) {

                  ti[ii++] = ci+k;
                  ti[ii++] = ci+(k+1)%NumLong;
                  ti[ii++] = ci+k+NumLong;
                  ti[ii++] = ci+(k+1)%NumLong;
                  ti[ii++] = ci+NumLong+(k+1)%NumLong;
                  ti[ii++] = ci+k+NumLong;
               }
               ci += NumLong;
            }
            vassert(ci == i*CoordPerSphere + 1 + NumLong*(NumLat-3));
            vassert(ci + NumLong + 1 == (i+1)*CoordPerSphere);

            // indices for ring around north pole
            for (Index k=0; k<NumLong; ++k) {
               ti[ii++] = ci+k;
               ti[ii++] = ci+(k+1)%NumLong;
               ti[ii++] = (i+1)*CoordPerSphere-1;
            }
            vassert(ii == (i+1)*3*TriPerSphere);

            for (Index j=0; j<3*TriPerSphere; ++j) {
               vassert(ti[i*3*TriPerSphere + j] >= i*CoordPerSphere);
               vassert(ti[i*3*TriPerSphere + j] < (i+1)*CoordPerSphere);
            }
         }
      }
      addObject("grid_out", tri);
      addObject("normal_out", norm);

      if (data) {
         auto out = replicateData(data, CoordPerSphere);
         addObject("data_out", out);
      }
   }

   return true;
}
