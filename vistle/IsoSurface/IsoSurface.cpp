#include <sstream>
#include <iomanip>

#include "object.h"
#include "IsoSurface.h"

MODULE_MAIN(IsoSurface)

IsoSurface::IsoSurface(int rank, int size, int moduleID)
   : Module("IsoSurface", rank, size, moduleID) {

   createInputPort("grid_in");
   createInputPort("data_in");

   createOutputPort("grid_out");
}

IsoSurface::~IsoSurface() {

}


vistle::Triangles *
IsoSurface::generateIsoSurface(const vistle::UnstructuredGrid * grid,
                               const vistle::Vec<float> * data,
                               const float isoValue) {

   int numElem = grid->getNumElements();

   for (int elem = 0; elem < numElem; elem ++) {

      switch (grid->tl[elem]) {

         case vistle::UnstructuredGrid::HEXAHEDRON: {

            unsigned int tableIndex = 0;
            char conn[8] = { 5, 6, 2, 1, 4, 7, 3, 0 };
            int shl = 0;
            for (int index = 0; index < 8; index ++) {
               float v = (*data->x)[grid->cl[grid->el[elem] + conn[index]]];
               tableIndex += (((unsigned int)(v < isoValue)) << shl++);
            }
            break;
         }

         default:
            break;
      }
   }
   printf("\n");

   vistle::Triangles *t = vistle::Triangles::create();

   t->cl->push_back(0);
   t->cl->push_back(1);
   t->cl->push_back(2);

   t->cl->push_back(0);
   t->cl->push_back(2);
   t->cl->push_back(3);

   t->x->push_back(0.0 + rank);
   t->y->push_back(0.0);
   t->z->push_back(0.0);

   t->x->push_back(1.0 + rank);
   t->y->push_back(0.0);
   t->z->push_back(0.0);

   t->x->push_back(1.0 + rank);
   t->y->push_back(1.0);
   t->z->push_back(0.0);

   t->x->push_back(0.0 + rank);
   t->y->push_back(1.0);
   t->z->push_back(0.0);

   return t;

   return NULL;
}


bool IsoSurface::compute() {

   std::list<vistle::Object *> gridObjects = getObjects("grid_in");
   std::cout << "IsoSurface: " << gridObjects.size() << " grid objects"
             << std::endl;

   std::list<vistle::Object *> dataObjects = getObjects("data_in");
   std::cout << "IsoSurface: " << dataObjects.size() << " data objects"
             << std::endl;

   while (gridObjects.size() > 0 && dataObjects.size() > 0) {

      if (gridObjects.front()->getType() == vistle::Object::UNSTRUCTUREDGRID &&
          dataObjects.front()->getType() == vistle::Object::VECFLOAT) {

         vistle::UnstructuredGrid *usg =
            static_cast<vistle::UnstructuredGrid *>(gridObjects.front());

         vistle::Vec<float> *array =
            static_cast<vistle::Vec<float> *>(dataObjects.front());

         vistle::Triangles *triangles = generateIsoSurface(usg, array, 0.0);
         if (triangles)
            addObject("grid_out", triangles);
      }

      removeObject("grid_in", gridObjects.front());
      removeObject("data_in", dataObjects.front());

      gridObjects = getObjects("grid_in");
      dataObjects = getObjects("data_in");
   }

   return true;
}
