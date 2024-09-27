#ifndef CREATECUBOID_H
#define CREATECUBOID_H

#include <vistle/module/module.h>

class CreateCuboid: public vistle::Module {
public:
    CreateCuboid(const std::string &name, int moduleID, mpi::communicator comm);
    ~CreateCuboid();

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif // CREATECUBOID_H
