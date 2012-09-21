#include <sstream>
#include <iomanip>

#include "object.h"

#include "ShowUSG.h"

MODULE_MAIN(ShowUSG)

ShowUSG::ShowUSG(int rank, int size, int moduleID)
   : Module("ShowUSG", rank, size, moduleID) {

   createInputPort("grid_in");
   createOutputPort("grid_out");
}

ShowUSG::~ShowUSG() {

}

bool ShowUSG::compute() {

   ObjectList objects = getObjects("grid_in");
   std::cout << "ShowUSG: " << objects.size() << " objects" << std::endl;

   ObjectList::iterator oit;
   for (oit = objects.begin(); oit != objects.end(); oit ++) {
      vistle::Object::const_ptr object = *oit;

      switch (object->getType()) {

         case vistle::Object::UNSTRUCTUREDGRID: {

            vistle::Lines::ptr out(new vistle::Lines);
            vistle::UnstructuredGrid::const_ptr in = vistle::UnstructuredGrid::as(object);

            for (size_t index = 0; index < in->getNumElements(); index ++)
               switch (in->tl()[index]) {
                  case vistle::UnstructuredGrid::HEXAHEDRON:

                     out->el().push_back(out->cl().size());
                     out->cl().push_back(in->cl()[in->el()[index]]);
                     out->cl().push_back(in->cl()[in->el()[index] + 1]);
                     out->cl().push_back(in->cl()[in->el()[index] + 2]);
                     out->cl().push_back(in->cl()[in->el()[index] + 3]);
                     out->cl().push_back(in->cl()[in->el()[index]]);
                     out->cl().push_back(in->cl()[in->el()[index] + 4]);

                     out->el().push_back(out->cl().size());
                     out->cl().push_back(in->cl()[in->el()[index] + 5]);
                     out->cl().push_back(in->cl()[in->el()[index] + 6]);
                     out->cl().push_back(in->cl()[in->el()[index] + 7]);
                     out->cl().push_back(in->cl()[in->el()[index] + 4]);
                     out->cl().push_back(in->cl()[in->el()[index] + 5]);
                     out->cl().push_back(in->cl()[in->el()[index] + 1]);

                     out->el().push_back(out->cl().size());
                     out->cl().push_back(in->cl()[in->el()[index] + 2]);
                     out->cl().push_back(in->cl()[in->el()[index] + 6]);

                     out->el().push_back(out->cl().size());
                     out->cl().push_back(in->cl()[in->el()[index] + 3]);
                     out->cl().push_back(in->cl()[in->el()[index] + 7]);

                     /*
                     (*out->el)[lineIdx++] = cornerIdx;

                     (*out->cl)[cornerIdx++] = in->cl[in->el[index]];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 1];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 2];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 3];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index]];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 4];

                     (*out->el)[lineIdx++] = cornerIdx;
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 5];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 6];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 7];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 4];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 5];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 1];

                     (*out->el)[lineIdx++] = cornerIdx;
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 2];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 6];

                     (*out->el)[lineIdx++] = cornerIdx;
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 3];
                     (*out->cl)[cornerIdx++] = in->cl[in->el[index] + 7];
                     */
                     break;

                  default:
                     break;
               }

            int numVertices = in->getNumVertices();
            for (int index = 0; index < numVertices; index ++) {
               out->x().push_back(in->x()[index]);
               out->y().push_back(in->y()[index]);
               out->z().push_back(in->z()[index]);
               /*
               (*out->x)[index] = in->x[index];
               (*out->y)[index] = in->y[index];
               (*out->z)[index] = in->z[index];
               */
            }

            addObject("grid_out", out);
            break;
         }
         default:
            break;
      }
      removeObject("grid_in", object);
   }

   return true;
}
