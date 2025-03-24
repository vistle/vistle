#ifndef VISTLE_PRINTATTRIBUTES_PRINTATTRIBUTES_H
#define VISTLE_PRINTATTRIBUTES_PRINTATTRIBUTES_H

#include <vistle/module/module.h>

class PrintAttributes: public vistle::Module {
public:
    PrintAttributes(const std::string &name, int moduleID, mpi::communicator comm);
    ~PrintAttributes();

private:
    virtual bool compute();
    void print(vistle::Object::const_ptr obj);
};

#endif
