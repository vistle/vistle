#ifndef TOPOINTS_H
#define TOPOINTS_H

#include <vistle/module/module.h>

class ToPoints: public vistle::Module {
public:
    ToPoints(const std::string &name, int moduleID, mpi::communicator comm);
    ~ToPoints();

private:
    virtual bool compute();
};

#endif
