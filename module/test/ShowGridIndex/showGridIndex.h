#ifndef SHOW_GRID_INDEX_H
#define SHOW_GRID_INDEX_H

#include <vistle/module/module.h>
#include <vistle/core/vec.h>

class ShowGridIndex: public vistle::Module {
public:
    ShowGridIndex(const std::string &name, int moduleID, boost::mpi::communicator comm);

private:
    vistle::Port *m_gridIn, *m_indexOut;


    virtual bool compute() override;

    vistle::ResultCache<vistle::Vec<vistle::Index>::ptr> m_cache;
};

#endif // !SHOW_GRID_INDEX_H
