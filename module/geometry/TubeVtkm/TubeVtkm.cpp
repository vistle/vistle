#include <limits>

#include <viskores/filter/geometry_refinement/Tube.h>

#include <vistle/core/parameter.h>

#include "TubeVtkm.h"

MODULE_MAIN(TubeVtkm)

using namespace vistle;

TubeVtkm::TubeVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Use)
{
    m_radius = addFloatParameter("radius", "radius of the tubes", 1.0f);
    setParameterMinimum(m_radius, (Float)0.);

    m_numberOfSides = addIntParameter("numberOfSides", "number of sides of the tubes", 5);
    setParameterMinimum(m_numberOfSides, (Integer)1);

    m_addCaps = addIntParameter("addCaps", "add caps to the ends of the tubes", false, Parameter::Boolean);
}

std::unique_ptr<viskores::filter::Filter> TubeVtkm::setUpFilter() const
{
    auto filter = std::make_unique<viskores::filter::geometry_refinement::Tube>();
    filter->SetRadius(static_cast<viskores::FloatDefault>(m_radius->getValue()));
    filter->SetNumberOfSides(static_cast<viskores::Id>(m_numberOfSides->getValue()));
    filter->SetCapping(m_addCaps->getValue() != 0);
    return filter;
}
