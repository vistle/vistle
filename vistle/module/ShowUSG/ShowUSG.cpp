#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/unstr.h>
#include <core/lines.h>

#include "ShowUSG.h"

MODULE_MAIN(ShowUSG)

using namespace vistle;

ShowUSG::ShowUSG(const std::string &shmname, const std::string &name, int moduleID)
   : Module("ShowUSG", shmname, name, moduleID) {


   createInputPort("grid_in");
   createOutputPort("grid_out");

   addIntParameter("tetrahedron", "Show tetrahedron", 1, Parameter::Boolean);
   addIntParameter("pyramid", "Show pyramid", 1, Parameter::Boolean);
   addIntParameter("prism", "Show prism", 1, Parameter::Boolean);
   addIntParameter("hexahedron", "Show hexahedron", 1, Parameter::Boolean);
   addIntParameter("polyhedron", "Show polyhedron", 1, Parameter::Boolean);
   m_CellNrMin = addIntParameter("Show Cells from Cell Nr. :", "Show Cell Nr.", -1);
   m_CellNrMax = addIntParameter("Show Cells to Cell Nr. :", "Show Cell Nr.", -1);

}

ShowUSG::~ShowUSG() {

}

bool ShowUSG::compute() {    

   const bool showtet = getIntParameter("tetrahedron");
   const bool showpyr = getIntParameter("pyramid");
   const bool showpri = getIntParameter("prism");
   const bool showhex = getIntParameter("hexahedron");
   const bool showpol = getIntParameter("polyhedron");
   const Integer cellnrmin = getIntParameter("Show Cells from Cell Nr. :");
   const Integer cellnrmax = getIntParameter("Show Cells to Cell Nr. :");

   UnstructuredGrid::const_ptr in = accept<UnstructuredGrid>("grid_in");
   if (in) {

      vistle::Lines::ptr out(new vistle::Lines(Object::Initialized));

      Index x, y;
      if (cellnrmin == -1 || cellnrmax == -1) {
         x = 0;
         y = in->getNumElements();
      } else {
         x = cellnrmin;
         y = cellnrmax;
      }

      for (size_t index = x; index < y; index ++) {
         switch (in->tl()[index]) {

            case vistle::UnstructuredGrid::TETRAHEDRON:
               if(showtet) {
                  out->cl().push_back(in->cl()[in->el()[index]]);
                  out->cl().push_back(in->cl()[in->el()[index] + 1]);
                  out->cl().push_back(in->cl()[in->el()[index] + 3]);
                  out->cl().push_back(in->cl()[in->el()[index]]);
                  out->el().push_back(out->cl().size());

                  out->cl().push_back(in->cl()[in->el()[index] + 1]);
                  out->cl().push_back(in->cl()[in->el()[index] + 2]);
                  out->cl().push_back(in->cl()[in->el()[index] + 3]);
                  out->el().push_back(out->cl().size());

                  out->cl().push_back(in->cl()[in->el()[index] + 2]);
                  out->cl().push_back(in->cl()[in->el()[index]]);
                  out->el().push_back(out->cl().size());
               }
               break;

            case vistle::UnstructuredGrid::PYRAMID:
               if (showpyr) {
                  out->cl().push_back(in->cl()[in->el()[index]]);
                  out->cl().push_back(in->cl()[in->el()[index] + 1]);
                  out->cl().push_back(in->cl()[in->el()[index] + 4]);
                  out->cl().push_back(in->cl()[in->el()[index]]);
                  out->el().push_back(out->cl().size());

                  out->cl().push_back(in->cl()[in->el()[index] + 1]);
                  out->cl().push_back(in->cl()[in->el()[index] + 2]);
                  out->cl().push_back(in->cl()[in->el()[index] + 4]);
                  out->el().push_back(out->cl().size());

                  out->cl().push_back(in->cl()[in->el()[index] + 2]);
                  out->cl().push_back(in->cl()[in->el()[index] + 3]);
                  out->cl().push_back(in->cl()[in->el()[index] + 4]);
                  out->el().push_back(out->cl().size());

                  out->cl().push_back(in->cl()[in->el()[index] + 3]);
                  out->cl().push_back(in->cl()[in->el()[index]]);
                  out->el().push_back(out->cl().size());
               }
               break;

            case vistle::UnstructuredGrid::PRISM:
               if (showpri) {
                  out->cl().push_back(in->cl()[in->el()[index]]);
                  out->cl().push_back(in->cl()[in->el()[index] + 1]);
                  out->cl().push_back(in->cl()[in->el()[index] + 2]);
                  out->cl().push_back(in->cl()[in->el()[index]]);
                  out->cl().push_back(in->cl()[in->el()[index] + 3]);
                  out->cl().push_back(in->cl()[in->el()[index] + 4]);
                  out->cl().push_back(in->cl()[in->el()[index] + 5]);
                  out->cl().push_back(in->cl()[in->el()[index] + 3]);
                  out->el().push_back(out->cl().size());

                  out->cl().push_back(in->cl()[in->el()[index] + 1]);
                  out->cl().push_back(in->cl()[in->el()[index] + 4]);
                  out->el().push_back(out->cl().size());

                  out->cl().push_back(in->cl()[in->el()[index] + 2]);
                  out->cl().push_back(in->cl()[in->el()[index] + 5]);
                  out->el().push_back(out->cl().size());
               }
               break;

            case vistle::UnstructuredGrid::HEXAHEDRON:
               if (showhex) {
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
                  out->el().push_back(out->cl().size());


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
               }
               break;

            case vistle::UnstructuredGrid::POLYHEDRON:
               if (showpol) {
                  Index sidebegin = InvalidIndex;
                  for (Index i = in->el()[index]; i < in->el()[index+1]; i++) {

                     out->cl().push_back(in->cl()[i]);

                     if (in->cl()[i] == sidebegin){// Wenn die Seite endet
                        sidebegin = InvalidIndex;
                        out->el().push_back(out->cl().size());
                     } else if(sidebegin == InvalidIndex) { //Wenn die Neue Seite beginnt
                        sidebegin = in->cl()[i];
                     }
                  }
               }
               break;
         }
      }

      std::cerr << "getNum corners: " << out->getNumCorners() << std::endl;
      std::cerr << "getNum elements: " << out->getNumElements() << std::endl;

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

      out->copyAttributes(in);
   }

   return true;
}
