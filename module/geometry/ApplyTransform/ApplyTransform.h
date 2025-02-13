#ifndef VISTLE_APPLYTRANSFORM_APPLYTRANSFORM_H
#define VISTLE_APPLYTRANSFORM_APPLYTRANSFORM_H

#include <vistle/module/module.h>
#include <vistle/module/resultcache.h>

class ApplyTransform: public vistle::Module {
public:
    ApplyTransform(const std::string &name, int moduleID, mpi::communicator comm);
    ~ApplyTransform();

private:
    bool compute() override;

    vistle::ResultCache<vistle::Object::ptr> m_gridCache, m_resultCache;
};

#endif
