//-------------------------------------------------------------------------
// STRUCTURED GRID OBJECT BASE CLASS CPP
// *
// * Base class for Structured Grid Objects
//-------------------------------------------------------------------------

#include "structuredgridbase.h"

namespace vistle {


// IS GHOST CELL CHECK
//-------------------------------------------------------------------------
bool StructuredGridBase::isGhostCell(Index elem) const {
      Index dims[3];
      std::array<Index,3> cellCoords;

      for (int c=0; c<3; ++c) {
          dims[c] = getNumDivisions(c);
      }

      cellCoords = cellCoordinates(elem, dims);

      for (int c=0; c<3; ++c) {
          if (cellCoords[c] < getNumGhostLayers(c, Bottom)
                  || cellCoords[c] >= (dims[c]-1)-getNumGhostLayers(c, Top)) {
              return true;
          }
      }

      return false;

  }


} // namespace vistle
