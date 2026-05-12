#ifndef VISTLE_UTIL_FILEIO_H
#define VISTLE_UTIL_FILEIO_H

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>


#define S_IRUSR _S_IREAD
#define S_IRGRP _S_IREAD
#define S_IROTH _S_IREAD
#define S_IWUSR _S_IWRITE
#define S_IWGRP _S_IWRITE
#define S_IWOTH _S_IWRITE
#define O_BINARY _O_BINARY


#else
#define O_BINARY 0
#include <unistd.h>
#endif

#include "export.h"
#include "ssize_t.h"
#include <cstdio>
namespace file {

V_UTILEXPORT ssize_t tell(int fd);
V_UTILEXPORT ssize_t tell(FILE *file);

V_UTILEXPORT ssize_t seek(int fd, ssize_t off, int whence = SEEK_SET);
V_UTILEXPORT ssize_t seek(FILE *file, ssize_t off, int whence = SEEK_SET);

} // namespace file

#endif
