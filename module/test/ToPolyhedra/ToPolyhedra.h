#ifndef TOPOLYHEDRA_H
#define TOPOLYHEDRA_H

#include <vistle/module/module.h>

class ToPolyhedra: public vistle::Module {
public:
    ToPolyhedra(const std::string &name, int moduleID, mpi::communicator comm);
    ~ToPolyhedra();

private:
    virtual bool compute();
};

#endif
