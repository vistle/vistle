#ifndef VISTLE_SELECTVERTICES_SELECTVERTICES_H
#define VISTLE_SELECTVERTICES_SELECTVERTICES_H

#include <vistle/module/module.h>
#include <vistle/util/coRestraint.h>
#include <memory>
#include <vistle/module/resultcache.h>

class SelectVertices: public vistle::Module {
public:
    SelectVertices(const std::string &name, int moduleID, mpi::communicator comm);
    ~SelectVertices();

    typedef std::map<vistle::Index, vistle::Index> VerticesMapping;

private:
    static const unsigned NUMPORTS = 3;
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    bool prepare() override;

    bool m_invert = false;
    vistle::IntParameter *p_invert = nullptr;
    vistle::coRestraint m_restraint, m_blockRestraint;
    vistle::StringParameter *p_restraint = nullptr, *p_blockRestraint = nullptr;
    vistle::Port *p_in[NUMPORTS] = {};
    vistle::Port *p_out[NUMPORTS] = {};

    struct CachedResult {
        VerticesMapping vm;
        vistle::Points::ptr points;
    };
    mutable vistle::ResultCache<CachedResult> m_gridCache;
};

#endif
