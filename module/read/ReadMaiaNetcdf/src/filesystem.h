#ifndef MAIAVISFILESYSTEM_H_
#define MAIAVISFILESYSTEM_H_

#include "INCLUDE/maiatypes.h"

namespace maiapv { //using a namespace for Thomas !!!

MString getBasename(const MString& filePath) {
  const std::size_t id = filePath.rfind("/");
  return filePath.substr(id + 1);
}

MString getDirname(const MString& filePath) {
  const std::size_t id = filePath.rfind("/");
  return filePath.substr(0, id + 1);
}

}
#endif // ifndef MAIAVISFILESYSTEM_H_
