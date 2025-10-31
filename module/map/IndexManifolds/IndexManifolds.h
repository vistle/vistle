#ifndef VISTLE_INDEXMANIFOLDS_INDEXMANIFOLDS_H
#define VISTLE_INDEXMANIFOLDS_INDEXMANIFOLDS_H

#include <vistle/module/module.h>
#include <vistle/module/resultcache.h>

class IndexManifolds: public vistle::Module {
public:
    IndexManifolds(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool changeParameter(const vistle::Parameter *p) override;
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;

    vistle::Port *p_data_in = nullptr;
    vistle::Port *p_surface_out = nullptr, *p_line_out = nullptr, *p_point_out = nullptr;

    vistle::IntVectorParameter *p_coord = nullptr;
    vistle::IntParameter *p_direction = nullptr;

    mutable vistle::ResultCache<vistle::Quads::ptr> m_surfaceCache;
};

#endif
