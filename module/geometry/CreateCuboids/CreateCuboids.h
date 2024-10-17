#ifndef CREATECUBOID_H
#define CREATECUBOID_H

#include <vistle/module/module.h>

/*
    This module returns a hexahedral grid of cuboids which it creates out of a grid with 
    (3D vector) mapped data given as input. It interprets the input grid's coordinates as 
    cuboid centers and the mapped data as edge lengths in x-, y-, z-direction.
*/
class CreateCuboids: public vistle::Module {
public:
    CreateCuboids(const std::string &name, int moduleID, mpi::communicator comm);
    ~CreateCuboids();

private:
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
};

#endif // CREATECUBOID_H
