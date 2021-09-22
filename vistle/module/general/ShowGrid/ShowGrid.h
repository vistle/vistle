#ifndef SHOWUSG_H
#define SHOWUSG_H

#include <vistle/module/module.h>

class ShowGrid: public vistle::Module {
public:
    ShowGrid(const std::string &name, int moduleID, mpi::communicator comm);
    ~ShowGrid();

private:
    virtual bool compute();

    vistle::IntParameter *m_CellNrMin;
    vistle::IntParameter *m_CellNrMax;
};

#endif
