#ifndef NEK_OPENFILE
#define NEK_OPENFILE

#include <iostream>
#include <string>

namespace nek5000 {
class OpenFile {
private:
    bool open = false;
    FILE *fileptr = nullptr;
    std::string fileName;

public:
    OpenFile(std::string file);
    int curTimestep = -1;
    int fileID = -1; //For parallel format, proc associated with file
    int iAsciiFileStart = -1; //For ascii data, file pos where data begins, in current timestep
    int iAsciiFileLineLen = -1; //For ascii data, length of each line, in mesh file
    FILE *file();
    std::string name() const;
    ~OpenFile();
};
} // namespace nek5000
#endif
