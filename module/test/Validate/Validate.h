#ifndef VALIDATE_H
#define VALIDATE_H

#include <vistle/module/module.h>

class Validate: public vistle::Module {
public:
    Validate(const std::string &name, int moduleID, mpi::communicator comm);
    ~Validate();

private:
    virtual bool compute();
};

#endif
