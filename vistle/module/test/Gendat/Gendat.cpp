#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/vec.h>
#include <core/triangles.h>
#include <core/polygons.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>

#include "Gendat.h"

MODULE_MAIN(Gendat)

using namespace vistle;

Gendat::Gendat(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Gendat", shmname, name, moduleID) {

   createOutputPort("grid_out");
   createOutputPort("data_out");

   std::vector<std::string> choices;
   m_geoMode = addIntParameter("geo_mode", "geometry generation mode", 0, Parameter::Choice);
   choices.clear();
   choices.push_back("Triangles");
   choices.push_back("Polygons");
   choices.push_back("Uniform");
   choices.push_back("Rectilinear");
   choices.push_back("Structured");
   setParameterChoices(m_geoMode, choices);

   m_dataMode = addIntParameter("data_mode", "data generation mode", 0, Parameter::Choice);
   choices.clear();
   choices.push_back("uniform");
   choices.push_back("add MPI rank");
   setParameterChoices(m_dataMode, choices);
}

Gendat::~Gendat() {

}

bool Gendat::compute() {

#if 0
   Vec<Scalar> *a = Vec<Scalar>::create();
   for (unsigned int index = 0; index < 1024 * 1024 * 4; index ++)
      a->x->push_back(index);

   /*
   Vec3<int> *b = Vec3<int>::create(16);
   for (unsigned int index = 0; index < b->getSize(); index ++) {
      b->x[index] = index;
      b->y[index] = index;
      b->z[index] = index;
   }
   */
#endif
   // output data: first if statement seperates coord-derived objects
   if (m_geoMode->getValue() == M_TRIANGLES || m_geoMode->getValue() == M_POLYGONS) {
       Coords::ptr geo;
       if (m_geoMode->getValue() == M_TRIANGLES) {

          Triangles::ptr t(new Triangles(6, 4));
          geo = t;

          t->cl()[0] = 0;
          t->cl()[1] = 1;
          t->cl()[2] = 3;

          t->cl()[3] = 0;
          t->cl()[4] = 3;
          t->cl()[5] = 2;
       } else if (m_geoMode->getValue() == M_POLYGONS) {

          Polygons::ptr p(new Polygons(1, 4, 4));
          geo = p;

          p->cl()[0] = 0;
          p->cl()[1] = 1;
          p->cl()[2] = 3;
          p->cl()[3] = 2;

          p->el()[0] = 0;
          p->el()[1] = 4;
       }

       geo->x()[0] = 0.0 + rank();
       geo->y()[0] = 0.0;
       geo->z()[0] = 0.0;

       geo->x()[1] = 1.0 + rank();
       geo->y()[1] = 0.0;
       geo->z()[1] = 0.0;

       geo->x()[2] = 0.0 + rank();
       geo->y()[2] = 1.0;
       geo->z()[2] = 0.0;

       geo->x()[3] = 1.0 + rank();
       geo->y()[3] = 1.0;
       geo->z()[3] = 0.0;
       addObject("grid_out", geo);
   } else {
       if (rank() == 0 && m_geoMode->getValue() == M_UNIFORM) {
           UniformGrid::ptr u(new UniformGrid(2,3,4));

           // generate test data
           for (unsigned i = 0; i < 3; i++) {
               u->min()[i] = i;
               u->max()[i] = i + 1;
           }

           addObject("grid_out", u);

       } else if (rank() == 0 && m_geoMode->getValue() == M_RECTILINEAR){
           const unsigned eg_x = 5;
           const unsigned eg_y = 8;
           const unsigned eg_z = 9;
           RectilinearGrid::ptr r(new RectilinearGrid(eg_x, eg_y, eg_z));

           // generate test data
           for (unsigned i = 0; i < eg_x + 1; i++) {
               r->coords(0)[i] = i;
           }

           for (unsigned i = 0; i < eg_y + 1; i++) {
               r->coords(1)[i] = i;
           }

           for (unsigned i = 0; i < eg_z + 1; i++) {
               r->coords(2)[i] = i;
           }

           addObject("grid_out", r);

       } else if (rank() == 0 && m_geoMode->getValue() == M_STRUCTURED){
           const unsigned eg_x = 8;
           const unsigned eg_y = 2;
           const unsigned eg_z = 3;
           const unsigned size = eg_x * eg_y * eg_z;
           StructuredGrid::ptr s(new StructuredGrid(eg_x, eg_y, eg_z));

           // generate test data
           for (unsigned i = 0; i < eg_x + 1; i++) {
               s->x()[i] = i;
           }

           for (unsigned i = 0; i < eg_y + 1; i++) {
               s->y()[i] = i;
           }

           for (unsigned i = 0; i < eg_z + 1; i++) {
               s->z()[i] = i;
           }

           addObject("grid_out", s);
       }
   }

   // add data_out objects
   Scalar add = m_dataMode->getValue() ? rank() : 0.;
   Vec<Scalar>::ptr v(new Vec<Scalar>(4));
   for (int i=0; i<4; ++i) {
      v->x()[i] = add + i*0.2;
   }

   addObject("data_out", v);
   
   /*
   addObject("data_out", b);
   addObject("data_out", t);
   */
#if 0
   UnstructuredGrid *usg =
      UnstructuredGrid::create();

   usg->tl->push_back(UnstructuredGrid::HEXAHEDRON);
   usg->el->push_back(0);
   for (int index = 0; index < 8; index ++)
      usg->cl->push_back(index);
   usg->el->push_back(usg->cl->size());

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
