#ifndef DOMAINSURFACE_H
#define DOMAINSURFACE_H

#include <vistle/module/module.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/quads.h>

class DomainSurface: public vistle::Module {
public:
    DomainSurface(const std::string &name, int moduleID, mpi::communicator comm);
    ~DomainSurface();

    typedef std::vector<vistle::Index> DataMapping;

private:
    bool compute(std::shared_ptr<vistle::BlockTask> task) const override;

    template<class Surface>
    struct Result {
        typename Surface::ptr surface;
        DataMapping surfaceElements;
        vistle::Lines::ptr lines;
        DataMapping lineElements;
    };

    Result<vistle::Polygons> createSurface(vistle::UnstructuredGrid::const_ptr m_grid_in, bool haveElementData,
                                           bool createSurface, bool createLines) const;
    Result<vistle::Quads> createSurface(vistle::StructuredGridBase::const_ptr m_grid_in, bool haveElementData,
                                        bool createSurface, bool createLines) const;
    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Indexed::ptr poly, DataMapping &vm) const;
    void renumberVertices(vistle::Coords::const_ptr coords, vistle::Quads::ptr quad, DataMapping &vm) const;
    //bool checkNormal(vistle::Index v1, vistle::Index v2, vistle::Index v3, vistle::Scalar x_center, vistle::Scalar y_center, vistle::Scalar z_center);

    mutable vistle::ResultCache<vistle::Object::ptr> m_cache;
};

#endif
