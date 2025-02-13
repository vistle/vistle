#ifndef VISTLE_TOTRIANGLES_TOTRIANGLES_H
#define VISTLE_TOTRIANGLES_TOTRIANGLES_H

#include <vistle/module/module.h>
#include <vistle/module/resultcache.h>

class ToTriangles: public vistle::Module {
public:
    ToTriangles(const std::string &name, int moduleID, mpi::communicator comm);
    ~ToTriangles();

private:
    virtual bool compute();

    vistle::IntParameter *p_transformSpheres = nullptr;
    vistle::ResultCache<vistle::Object::ptr> m_resultCache;
};

#endif
