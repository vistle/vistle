#ifndef SHOW_GRID_INDEX_H
#define SHOW_GRID_INDEX_H

#include <vistle/module/module.h>

class ShowGridIndex: public vistle::Module {
public:
    ShowGridIndex(const std::string &name, int moduleID, boost::mpi::communicator comm);

private:
    vistle::Port *m_gridIn, *m_grid_out, *m_indexOut;


    virtual bool compute() override;
};

#endif // !SHOW_GRID_INDEX_H
