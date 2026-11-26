#include <vistle/core/points.h>

#include "algo/CalculateThicknessVtkm.h"
#include "algo/VtkmSpheresOverlap.h"

#include "SpheresOverlapVtkm.h"

using namespace vistle;

MODULE_MAIN(SpheresOverlapVtkm)

// TODO: - instead of checking sId < sId2, adjust for range accordingly!

SpheresOverlapVtkm::SpheresOverlapVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Generate)
{
    m_radiusCoefficient = addFloatParameter("multiply_search_radius_by",
                                            "increase search radius for the Cell Lists algorithm by this factor", 1);

    m_thicknessDeterminer = addIntParameter("thickness_determiner", "the line thickness will be mapped to this value",
                                            (Integer)OverlapRatio, Parameter::Choice);
    V_ENUM_SET_CHOICES_SCOPE(m_thicknessDeterminer, ThicknessDeterminer, );

    setReducePolicy(message::ReducePolicy::OverAll);
}

std::string SpheresOverlapVtkm::getFieldName(int index, bool output) const
{
    if (index == 0)
        return output ? "" : "lineThickness";
    return VtkmModule::getFieldName(index, output);
}

ModuleStatusPtr SpheresOverlapVtkm::prepareInputGrid(InputData &input) const
{
    if (!Points::as(input.vistleGrid))
        return Error("Input port must contain spheres!");
    return VtkmModule::prepareInputGrid(input);
}

std::unique_ptr<viskores::filter::Filter> SpheresOverlapVtkm::setUpFilter(const VtkmModule::InputData &input) const
{
    auto filter = std::make_unique<VtkmSpheresOverlap>();
    // If the module uses MappedDataHandling::Generate the active field is, by default, set to "",
    // so lets explicitly set it to the radius field here.
    filter->SetActiveField("_radius");
    filter->SetRadiusFactor(m_radiusCoefficient->getValue());
    filter->SetThicknessDeterminer((ThicknessDeterminer)m_thicknessDeterminer->getValue());
    return filter;
}

vistle::DataBase::ptr SpheresOverlapVtkm::prepareOutputField(const InputData &input, OutputData &output, int index,
                                                             const std::string &fieldName) const
{
    auto mappedField = VtkmModule::prepareOutputField(input, output, index, fieldName);
    mappedField->setMapping(DataBase::Element);
    if (index == 0)
        mappedField->describe("line thickness", id());
    return mappedField;
}
