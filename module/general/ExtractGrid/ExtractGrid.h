#ifndef EXTRACTGRID_H
#define EXTRACTGRID_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>

class ExtractGrid: public vistle::Module {
public:
    ExtractGrid(const std::string &name, int moduleID, mpi::communicator comm);
    ~ExtractGrid();

private:
    vistle::Port *m_dataIn, *m_gridOut, *m_normalsOut;
    virtual bool compute();
};

#endif
