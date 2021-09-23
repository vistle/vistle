#ifndef VISTLE_FILE_IO_H
#define VISTLE_FILE_IO_H

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>


#define S_IRUSR _S_IREAD
#define S_IRGRP _S_IREAD
#define S_IROTH _S_IREAD
#define S_IWUSR _S_IWRITE
#define S_IWGRP _S_IWRITE
#define S_IWOTH _S_IWRITE


#else
#include <unistd.h>
#endif

#endif
