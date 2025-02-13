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

#endif
