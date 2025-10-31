#ifndef VISTLE_GENDAT_GENDATCHECKER_H
#define VISTLE_GENDAT_GENDATCHECKER_H

#include <vistle/module/module.h>

class GendatChecker: public vistle::Module {
public:
    GendatChecker(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;
};

#endif
