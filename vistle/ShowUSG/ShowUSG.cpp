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

   std::list<vistle::Object *> objects = getObjects("grid_in");
   std::cout << "ShowUSG: " << objects.size() << " objects" << std::endl;

   std::list<vistle::Object *>::iterator oit;
   for (oit = objects.begin(); oit != objects.end(); oit ++) {
      vistle::Object *object = *oit;
      switch (object->getType()) {

         case vistle::Object::UNSTRUCTUREDGRID: {

            vistle::UnstructuredGrid *in =
               static_cast<vistle::UnstructuredGrid *>(object);

            int numLines = 0;
            int numCorners = 0;
            for (size_t index = 0; index < in->getNumElements(); index ++)
               switch (in->tl[index]) {
                  case vistle::UnstructuredGrid::HEXAHEDRON:
                     numLines += 4;
                     numCorners += 16;
                     break;

                  default:
                     break;
               }
            printf("ShowUSG: %d, %d\n", numLines, numCorners);

            vistle::Lines *out = vistle::Lines::create(numLines,
                                                       numCorners,
                                                       in->getNumVertices());

            int lineIdx = 0;
            int cornerIdx = 0;

            for (size_t index = 0; index < in->getNumElements(); index ++)
               switch (in->tl[index]) {
                  case vistle::UnstructuredGrid::HEXAHEDRON:
                     out->el[lineIdx++] = cornerIdx;

                     out->cl[cornerIdx++] = in->cl[in->el[index]];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 1];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 2];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 3];
                     out->cl[cornerIdx++] = in->cl[in->el[index]];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 4];

                     out->el[lineIdx++] = cornerIdx;
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 5];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 6];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 7];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 4];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 5];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 1];

                     out->el[lineIdx++] = cornerIdx;
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 2];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 6];

                     out->el[lineIdx++] = cornerIdx;
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 3];
                     out->cl[cornerIdx++] = in->cl[in->el[index] + 7];

                     break;

                  default:
                     break;
               }
            int numVertices = in->getNumVertices();
            for (int index = 0; index < numVertices; index ++) {
               out->x[index] = in->x[index];
               out->y[index] = in->y[index];
               out->z[index] = in->z[index];
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
