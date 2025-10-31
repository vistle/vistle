#ifndef VISTLE_VALIDATE_VALIDATE_H
#define VISTLE_VALIDATE_VALIDATE_H

#include <vistle/module/module.h>

class Validate: public vistle::Module {
public:
    Validate(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;
};

#endif
