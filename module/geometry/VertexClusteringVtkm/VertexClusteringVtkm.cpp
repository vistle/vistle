#include "VertexClusteringVtkm.h"
#include <viskores/filter/geometry_refinement/VertexClustering.h>

using namespace vistle;

VertexClusteringVtkm::VertexClusteringVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 3, MappedDataHandling::Use)
{
    m_numDivisionsParam =
        addIntVectorParameter("num_divisions", "number of divisions in each dimension", IntParamVector{32, 32, 32});
}

std::unique_ptr<viskores::filter::Filter> VertexClusteringVtkm::setUpFilter() const
{
    auto filt = std::make_unique<viskores::filter::geometry_refinement::VertexClustering>();
    auto nd = m_numDivisionsParam->getValue();
    viskores::Id3 numDivisions(nd[0], nd[1], nd[2]);
    filt->SetNumberOfDivisions(numDivisions);
    return filt;
}

MODULE_MAIN(VertexClusteringVtkm)
