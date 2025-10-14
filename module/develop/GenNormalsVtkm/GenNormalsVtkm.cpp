#include <viskores/filter/vector_analysis/SurfaceNormals.h>

#include "GenNormalsVtkm.h"
#include <vistle/vtkm/convert.h>

using namespace vistle;

MODULE_MAIN(GenNormalsVtkm)

GenNormalsVtkm::GenNormalsVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: VtkmModule(name, moduleID, comm, 1, MappedDataHandling::Generate)
{
    m_perVertex = addIntParameter("per_vertex", "interpolate per-vertex normals", false, Parameter::Boolean);
    m_normalize = addIntParameter("normalize", "normalize per-element normals", true, Parameter::Boolean);
    m_autoOrient =
        addIntParameter("auto_orient", "orient normals outward (requires closed surface)", false, Parameter::Boolean);
    m_inward = addIntParameter("inward", "flip auto-oriented normals to point inward", false, Parameter::Boolean);
    setParameterReadOnly(m_inward, m_autoOrient->getValue() == 0);
}

GenNormalsVtkm::~GenNormalsVtkm() = default;

std::string GenNormalsVtkm::getFieldName(int i, bool output) const
{
    if (i == 0 && output) {
        return "Normals";
    }
    return VtkmModule::getFieldName(i, output);
}

bool GenNormalsVtkm::changeParameter(const vistle::Parameter *p)
{
    if (!p || p == m_autoOrient) {
        setParameterReadOnly(m_inward, m_autoOrient->getValue() == 0);
    }
    return VtkmModule::changeParameter(p);
}

std::unique_ptr<viskores::filter::Filter> GenNormalsVtkm::setUpFilter() const
{
    auto filt = std::make_unique<viskores::filter::vector_analysis::SurfaceNormals>();
    filt->SetGenerateCellNormals(m_perVertex->getValue() == 0);
    filt->SetNormalizeCellNormals(m_normalize->getValue() != 0);
    filt->SetAutoOrientNormals(m_autoOrient->getValue() != 0);
    filt->SetFlipNormals(m_inward->getValue() != 0);
    filt->SetConsistency(false); // required for being able to reuse input grid
    return filt;
}

Object::const_ptr GenNormalsVtkm::prepareOutputGrid(const viskores::cont::DataSet &dataset,
                                                    const Object::const_ptr &inputGrid) const
{
    return inputGrid;
}

DataBase::ptr GenNormalsVtkm::prepareOutputField(const viskores::cont::DataSet &dataset,
                                                 const Object::const_ptr &inputGrid,
                                                 const DataBase::const_ptr &inputField, const std::string &fieldName,
                                                 const Object::const_ptr &outputGrid) const
{
    DataBase::Mapping mapping = m_perVertex->getValue() == 0 ? DataBase::Element : DataBase::Vertex;
    if (auto mapped = vtkmGetField(dataset, fieldName, mapping)) {
        updateMeta(mapped);
        if (outputGrid)
            mapped->setGrid(outputGrid);
        mapped->describe("normals", id());
        return mapped;
    } else {
        sendError("Could not generate normals");
    }

    return nullptr;
}
