#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/vec.h>
#include <core/triangles.h>

#include "GendatChecker.h"

MODULE_MAIN(GendatChecker)

GendatChecker::GendatChecker(const std::string &shmname, int rank, int size, int moduleID)
   : Module("GendatChecker", shmname, rank, size, moduleID) {

   createOutputPort("grid_in");
   createOutputPort("data_in");
}

GendatChecker::~GendatChecker() {

}

bool GendatChecker::compute() {

#if 0
   vistle::Vec<vistle::Scalar> *a = vistle::Vec<vistle::Scalar>::create();
   for (unsigned int index = 0; index < 1024 * 1024 * 4; index ++)
      a->x->push_back(index);

   /*
   vistle::Vec3<int> *b = vistle::Vec3<int>::create(16);
   for (unsigned int index = 0; index < b->getSize(); index ++) {
      b->x[index] = index;
      b->y[index] = index;
      b->z[index] = index;
   }
   */
#endif

   while (hasObject("grid_in") && hasObject("data_in")) {

      vistle::Object::const_ptr grid = takeFirstObject("grid_in");
      vistle::Object::const_ptr data = takeFirstObject("data_in");

      vistle::Triangles::const_ptr t = vistle::Triangles::as(grid);
      vassert(t.get() && "expected Triangles");
      vassert(t->cl().size() == 6);
      vassert(t->cl()[0] == 0);
      vassert(t->cl()[1] == 1);
      vassert(t->cl()[2] == 2);
      vassert(t->cl()[3] == 0);
      vassert(t->cl()[4] == 2);
      vassert(t->cl()[5] == 3);
      vassert(t->x().size() == 4);
      vassert(t->y().size() == 4);
      vassert(t->z().size() == 4);
      for(size_t i=0; i<t->x().size(); ++i) {
         vassert(t->x()[i] == 1. + rank());
         vassert(t->y()[i] == 0.);
         vassert(t->z()[i] == 0.);
      }

      vistle::Vec<vistle::Scalar>::const_ptr v = vistle::Vec<vistle::Scalar>::as(data);
      vassert(v.get() && "expected Vec<Scalar>");
      vassert(v->x().size() == 4);
      for (size_t i=0; i<v->x().size(); ++i) {
         vassert(v->x()[i] == vistle::Scalar(i));
      }

#if 0
   vistle::Triangles *t = new vistle::Triangles(6, 4);

   t->cl()[0] = 0;
   t->cl()[1] = 1;
   t->cl()[2] = 2;

   t->cl()[3] = 0;
   t->cl()[4] = 2;
   t->cl()[5] = 3;

   t->x()[0] = 0.0 + rank;
   t->y()[0] = 0.0;
   t->z()[0] = 0.0;

   t->x()[1] = 1.0 + rank;
   t->y()[1] = 0.0;
   t->z()[1] = 0.0;

   t->x()[2] = 1.0 + rank;
   t->y()[2] = 1.0;
   t->z()[2] = 0.0;

   t->x()[3] = 0.0 + rank;
   t->y()[3] = 1.0;
   t->z()[3] = 0.0;

   addObject("grid_out", t);

   vistle::Vec<vistle::Scalar> *v = new vistle::Vec<vistle::Scalar>(4);
   v->x()[0] = 1.;
   v->x()[1] = 2.;
   v->x()[2] = 3.;
   v->x()[3] = 4.;
   addObject("data_out", v);
#endif

   }
   return true;
}
