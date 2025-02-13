#ifndef VISTLE_EXTRACTGRID_EXTRACTGRID_H
#define VISTLE_EXTRACTGRID_EXTRACTGRID_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include <vistle/core/normals.h>

class ExtractGrid: public vistle::Module {
public:
    ExtractGrid(const std::string &name, int moduleID, mpi::communicator comm);
    ~ExtractGrid();

private:
    vistle::Port *m_dataIn, *m_gridOut, *m_normalsOut;
    virtual bool compute();
    vistle::ResultCache<vistle::Object::ptr> m_gridCache;
    vistle::ResultCache<vistle::Normals::ptr> m_normalsCache;
};

#endif
