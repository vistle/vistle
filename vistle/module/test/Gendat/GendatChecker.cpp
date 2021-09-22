#include <iomanip>
#include <sstream>

#include <vistle/core/object.h>
#include <vistle/core/triangles.h>
#include <vistle/core/vec.h>

#include "GendatChecker.h"

MODULE_MAIN(GendatChecker)

GendatChecker::GendatChecker(const std::string &name, int moduleID, mpi::communicator comm)
: Module("GendatChecker", name, moduleID, comm)
{
    createOutputPort("grid_in");
    createOutputPort("data_in");
}

GendatChecker::~GendatChecker()
{}

bool GendatChecker::compute()
{
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

    vistle::Object::const_ptr grid = takeFirstObject("grid_in");
    vistle::Object::const_ptr data = takeFirstObject("data_in");

    vistle::Triangles::const_ptr t = expect<vistle::Triangles>("grid_in");
    assert(t.get() && "expected Triangles");
    //assert(t->cl().size() == 6);
    assert(t->cl()[0] == 0);
    assert(t->cl()[1] == 1);
    assert(t->cl()[2] == 2);
    assert(t->cl()[3] == 0);
    assert(t->cl()[4] == 2);
    assert(t->cl()[5] == 3);
    //assert(t->x().size() == 4);
    //assert(t->y().size() == 4);
    //assert(t->z().size() == 4);
    for (size_t i = 0; i < 4; ++i) {
        assert(t->x()[i] == 1. + rank());
        assert(t->y()[i] == 0.);
        assert(t->z()[i] == 0.);
    }

    vistle::Vec<vistle::Scalar>::const_ptr v = expect<vistle::Vec<vistle::Scalar>>("data_in");
    assert(v.get() && "expected Vec<Scalar>");
    //assert(v->x().size() == 4);
    for (size_t i = 0; i < 4; ++i) {
        assert(v->x()[i] == vistle::Scalar(i));
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

    return true;
}
