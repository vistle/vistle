#ifndef MAIAVISSOLVER_H_
#define MAIAVISSOLVER_H_

#include <iostream>
#include <array>
#include "INCLUDE/maiatypes.h"




namespace maiapv {
/** \brief solver object to store relevant information for visualization
 *
 * @author: Pascal S. Meysonnat
 * @date:   back in the 90
 *
 */
struct SolverPV {
  MString solverName; //!< Name of dataset in file
  MBool status;         //!< If true, show dataset
  MBool isDgOnly;       //!< If true, this is a DG-only dataset
  MInt solverId;          //!< solver Id
  template <typename OStream>
  friend OStream& operator<<(OStream& os, SolverPV const& d) {
    os << "name: " << d.solverName << " | s: " << d.status << " | dg: " << d.isDgOnly << std::endl;
    return os;
  }
};


} // namespace maiapv

#endif // ifndef MAIAVISSOLVER_H_
