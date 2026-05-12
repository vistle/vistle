#include "fileio.h"


ssize_t file::tell(const int fd)
{
#ifdef WIN32
    return _lseeki64(fd, 0, SEEK_CUR);
#else
    return lseek(fd, 0, SEEK_CUR);
#endif
}

ssize_t file::tell(FILE *file)
{
#ifdef WIN32
    return _ftelli64(file);
#else
    return ftell(file);
#endif
}

ssize_t file::seek(int fd, ssize_t off, int whence)
{
#ifdef WIN32
    return _lseeki64(fd, off, whence);
#else
    return lseek(fd, off, whence);
#endif
}

ssize_t file::seek(FILE *file, ssize_t off, int whence)
{
#ifdef WIN32
    return _fseeki64(file, off, whence);
#else
    return fseek(file, off, whence);
#endif
}
