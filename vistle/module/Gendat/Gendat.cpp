#include <sstream>
#include <iomanip>

#include "object.h"

#include "Gendat.h"

MODULE_MAIN(Gendat)

Gendat::Gendat(int rank, int size, int moduleID)
   : Module("Gendat", rank, size, moduleID) {

   createOutputPort("grid_out");
   createOutputPort("data_out");
}

Gendat::~Gendat() {

}

bool Gendat::compute() {

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
   vistle::Triangles *t = new vistle::Triangles(6, 4);

   t->cl()[0] = 0;
   t->cl()[1] = 1;
   t->cl()[2] = 2;

   t->cl()[3] = 0;
   t->cl()[4] = 2;
   t->cl()[5] = 3;

   t->cl()[0] = 0.0 + rank;
   t->cl()[0] = 0.0;
   t->cl()[0] = 0.0;

   t->cl()[1] = 1.0 + rank;
   t->cl()[1] = 0.0;
   t->cl()[1] = 0.0;

   t->cl()[2] = 1.0 + rank;
   t->cl()[2] = 1.0;
   t->cl()[2] = 0.0;

   t->cl()[3] = 0.0 + rank;
   t->cl()[3] = 1.0;
   t->cl()[3] = 0.0;

   addObject("data_out", t);
   /*
   addObject("data_out", b);
   addObject("data_out", t);
   */
#if 0
   vistle::UnstructuredGrid *usg =
      vistle::UnstructuredGrid::create();

   usg->tl->push_back(vistle::UnstructuredGrid::HEXAHEDRON);
   usg->el->push_back(0);
   for (int index = 0; index < 8; index ++)
      usg->cl->push_back(index);

   usg->x->push_back(0.0);
   usg->y->push_back(0.0);
   usg->z->push_back(0.0);

   usg->x->push_back(1.0);
   usg->y->push_back(0.0);
   usg->z->push_back(0.0);

   usg->x->push_back(0.0);
   usg->y->push_back(1.0);
   usg->z->push_back(0.0);

   usg->x->push_back(1.0);
   usg->y->push_back(1.0);
   usg->z->push_back(0.0);

   usg->x->push_back(0.0);
   usg->y->push_back(0.0);
   usg->z->push_back(1.0);

   usg->x->push_back(1.0);
   usg->y->push_back(0.0);
   usg->z->push_back(1.0);

   usg->x->push_back(0.0);
   usg->y->push_back(1.0);
   usg->z->push_back(1.0);

   usg->x->push_back(1.0);
   usg->y->push_back(1.0);
   usg->z->push_back(1.0);

   addObject("grid_out", usg);
#endif
#if 0
   int *local = new int[1024 * 1024];
   int *global = new int[1024 * 1024];

   for (int count = 0; count < 128; count ++) {
      for (int index = 0; index < 1024 * 1024; index ++)
         local[index] = rand();
      MPI_Allreduce(local, global, 1024 * 1024, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   }
#endif
   return true;
}
