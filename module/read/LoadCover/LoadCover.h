#ifndef VISTLE_LOADCOVER_LOADCOVER_H
#define VISTLE_LOADCOVER_LOADCOVER_H

#include <vistle/module/module.h>

class LoadCover: public vistle::Module {
public:
    LoadCover(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool prepare() override;
    bool compute() override;
};

#endif
