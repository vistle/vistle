# How to Write a Reader Module
<!-- TODO: add a section about the other methods in vistle::Reader (examine, prepareRead, finishRead) -->
This article assumes you are familiar with the [basics on how to write a Vistle module](implementation-basics.md).

Read modules read in data from one or more data files (the filenames are given through a string parameter, not the input port), convert the given data into Vistle data structures and make it available to other Vistle modules through their output port(s).

Implementing read modules is very similar to implementing compute modules: their ports and parameters are defined in the same manner. However, read modules inherit from the `vistle::Reader` instead of the `vistle::Module` class. Moreover, the main work happens in the `read` instead of the `compute` function.

The following code shows the basic structure of an example read module with the name **MyReader**...

```cpp
#ifndef MYREADER_H
#define MYREADER_H

#include <vistle/module/reader.h>

class MyReader: public vistle::Reader {
public:
    MyReader(const std::string &name, int moduleID, mpi::communicator comm);
    ~MyReader() override;

    bool read(vistle::Reader::Token &token, int timestep = -1, int block = -1) override;

private:
    vistle::StringParameter *m_filename;
};

#endif //MYREADER_H
```

... and its corresponding source file:

```cpp
#include "MyReader.h"

MODULE_MAIN(MyReader)

using vistle::Reader;
using vistle::Parameter;

MyReader::MyReader(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    createOutputPort("grid_out", "output grid");

    m_filename = addStringParameter("filename", "the file containing the data that is to be read in", "",
                                    Parameter::ExistingFilename);
}

MyReader::~MyReader()
{}

bool MyReader::read(vistle::Reader::Token &token, int timestep, int block)
{
    // read in the data from the file...
    
    return true;
}
```
