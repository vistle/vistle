#ifndef GENDATCHECKER_H
#define GENDATCHECKER_H

#include <vistle/module/module.h>

class GendatChecker: public vistle::Module {
public:
    GendatChecker(const std::string &name, int moduleID, mpi::communicator comm);
    ~GendatChecker();

private:
    virtual bool compute();
};

#endif
