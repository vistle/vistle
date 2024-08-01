#ifndef MAIAVISTIMERNAMES_H_
#define MAIAVISTIMERNAMES_H_

namespace maiapv{

struct TimerNames {
  enum {
    read,
    readGridFile,
    buildGrid,
    calcDOFs,
    buildVtkGrid,
    loadGridData,
    loadSolutionData,
    _count
  };
};

} // namespace maiapv

#endif
