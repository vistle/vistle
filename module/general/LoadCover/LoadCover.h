#ifndef LOADCOVER_H
#define LOADCOVER_H

#include <vistle/module/module.h>

class LoadCover: public vistle::Module {
public:
    LoadCover(const std::string &name, int moduleID, mpi::communicator comm);
    ~LoadCover();

private:
    bool prepare() override;
    bool compute() override;
};

#endif
