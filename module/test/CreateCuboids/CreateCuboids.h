#ifndef CREATECUBOID_H
#define CREATECUBOID_H

#include <vistle/module/module.h>

class CreateCuboids: public vistle::Module {
public:
    CreateCuboids(const std::string &name, int moduleID, mpi::communicator comm);
    ~CreateCuboids();

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif // CREATECUBOID_H
