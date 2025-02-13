#ifndef VISTLE_ATTACHNORMALS_ATTACHNORMALS_H
#define VISTLE_ATTACHNORMALS_ATTACHNORMALS_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>

class AttachNormals: public vistle::Module {
public:
    AttachNormals(const std::string &name, int moduleID, mpi::communicator comm);
    ~AttachNormals();

private:
    vistle::Port *m_gridIn, *m_dataIn, *m_gridOut;
    virtual bool compute();
};

#endif
