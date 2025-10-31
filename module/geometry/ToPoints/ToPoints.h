#ifndef VISTLE_TOPOINTS_TOPOINTS_H
#define VISTLE_TOPOINTS_TOPOINTS_H

#include <vistle/module/module.h>
#include <vistle/module/resultcache.h>

class ToPoints: public vistle::Module {
public:
    ToPoints(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool compute() override;

    vistle::ResultCache<vistle::Object::ptr> m_gridCache, m_resultCache;
    vistle::IntParameter *m_allCoordinates = nullptr;
};

#endif
