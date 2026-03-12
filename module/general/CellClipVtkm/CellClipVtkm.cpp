#include <viskores/filter/entity_extraction/ExtractGeometry.h>

#include "CellClipVtkm.h"

using namespace vistle;

MODULE_MAIN(CellClipVtkm)

CellClipVtkm::CellClipVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 3, MappedDataHandling::Use), m_implFuncControl(this)
{
    m_implFuncControl.init();
    m_boundary = addIntParameter("boundary", "keep cells intersecting clipping surface", false, Parameter::Boolean);
    m_flip = addIntParameter("flip", "keep cells inside clipping surface", false, Parameter::Boolean);
}

std::unique_ptr<viskores::filter::Filter> CellClipVtkm::setUpFilter() const
{
    bool flip = m_flip->getValue() != 0;
    bool boundary = m_boundary->getValue() != 0;

    auto filt = std::make_unique<viskores::filter::entity_extraction::ExtractGeometry>();
    filt->SetImplicitFunction(m_implFuncControl.function());
    filt->SetExtractBoundaryCells(boundary);
    filt->SetExtractInside(flip);
    return filt;
}

bool CellClipVtkm::changeParameter(const Parameter *param)
{
    bool ok = m_implFuncControl.changeParameter(param);
    return Module::changeParameter(param) && ok;
}
