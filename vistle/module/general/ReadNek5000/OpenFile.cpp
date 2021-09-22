#include "OpenFile.h"

namespace nek5000 {
OpenFile::OpenFile(std::string file): fileName(file)
{
    fileptr = fopen(fileName.c_str(), "rb");
    if (!fileptr) {
        std::cerr << ".nek5000 could not open file " << fileName << std::endl;
        open = false;
    }
    open = true;
}
OpenFile::~OpenFile()
{
    if (fileptr)
        fclose(fileptr);
}

FILE *OpenFile::file()
{
    return fileptr;
}

std::string OpenFile::name() const
{
    return fileName;
}
} // namespace nek5000
