#ifndef VISTLE_TOPOLYHEDRA_TOPOLYHEDRA_H
#define VISTLE_TOPOLYHEDRA_TOPOLYHEDRA_H

#include <vistle/module/module.h>

class ToPolyhedra: public vistle::Module {
public:
    ToPolyhedra(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;
};

#endif
