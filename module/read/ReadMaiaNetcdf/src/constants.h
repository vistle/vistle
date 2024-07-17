#ifndef MAIAVISCONSTANTS_H_
#define MAIAVISCONSTANTS_H_

#include "INCLUDE/maiatypes.h"
#include "compiler_config.h"

namespace maiapv {

// 3 digit binary number from 0 to 7
static constexpr MInt binaryId2D[24] = {
    // 2D version
    -1, -1, -1, 1, -1, -1, 1, 1, -1,    -1, 1, -1,
    -1, -1, 1,  1, -1, 1,  1, 1, 1 - 1, 1,  1,
};
static constexpr MInt binaryId3D[24] = {
    // 3D version
    -1, -1, -1, 1, -1, -1, -1, 1, -1, 1, 1, -1,
    -1, -1, 1,  1, -1, 1,  -1, 1, 1,  1, 1, 1};


// Store ids for visualization types
struct VisType {
  enum { ClassicCells = 0, VisualizationNodes = 1 };
};

} // namespace maiapv

#endif

