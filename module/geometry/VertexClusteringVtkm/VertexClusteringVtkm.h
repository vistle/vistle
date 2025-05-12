#ifndef VISTLE_VERTEXCLUSTERINGVTKM_VERTEXCLUSTERINGVTKM_H
#define VISTLE_VERTEXCLUSTERINGVTKM_VERTEXCLUSTERINGVTKM_H

#include <vistle/vtkm/vtkm_module.h>

class VertexClusteringVtkm: public vistle::VtkmModule {
public:
    VertexClusteringVtkm(const std::string &name, int moduleID, mpi::communicator comm);
    ~VertexClusteringVtkm();

private:
    std::unique_ptr<viskores::filter::Filter> setUpFilter() const override;

    vistle::IntVectorParameter *m_numDivisionsParam = nullptr;
};

#endif
