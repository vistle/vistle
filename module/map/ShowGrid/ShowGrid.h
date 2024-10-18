#ifndef SHOWUSG_H
#define SHOWUSG_H

#include <vistle/module/module.h>
#include <vistle/core/object.h>

class ShowGrid: public vistle::Module {
public:
    ShowGrid(const std::string &name, int moduleID, mpi::communicator comm);
    ~ShowGrid();

private:
    virtual bool compute();

    vistle::IntParameter *m_CellNrMin;
    vistle::IntParameter *m_CellNrMax;

    vistle::ResultCache<vistle::Object::ptr> m_cache;
};

#endif
