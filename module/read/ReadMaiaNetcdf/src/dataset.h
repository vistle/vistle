#ifndef MAIAVISDATASET_H_
#define MAIAVISDATASET_H_

#include <iostream>
#include <array>
#include "INCLUDE/maiatypes.h"

namespace maiapv {

/// \brief Auxiliary class used to maintain names and status
/// of datasets that should be loaded.
///
/// \author Michael Schlottke (mic) <mic@aia.rwth-aachen.de>
/// \date 2015-04-20
struct Dataset {
  MString variableName; //!< Name of dataset in file
  MString realName;     //!< Display name
  MBool status;         //!< If true, show dataset
  MBool isGrid;         //!< If true, this is a grid file dataset
  MBool isDgOnly;       //!< If true, this is a DG-only dataset
  std::string port; //!< Name of the output port
  template <typename OStream>
  friend OStream& operator<<(OStream& os, Dataset const& d) {
    os << "name: " << d.variableName << " | real: " << d.realName << " | s: " << d.status
       << " | g: " << d.isGrid << " | dg: " << d.isDgOnly << std::endl;

    return os;
  }
};



// Datasets for grid files hard-coded
//
// Unlike solution variables, grid variables are too numerous to be all loaded
// automatically. Thus, the datasets to be loaded from the grid file are here
// explicitly listed.
// TODO: refresh for newgrid
static const std::array<Dataset, 4> gridsets
    = {{{"globalId", "globalId (grid)", true, true, false},
        {"level", "level (grid)", true, true, false},
        {"nodeId", "nodeId (grid)", true, true, true},
        {"polyDeg", "polyDeg (grid)", true, true, true}}};

} // namespace maiapv

#endif // ifndef MAIAVISDATASET_H_
