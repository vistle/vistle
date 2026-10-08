#include <viskores/filter/vector_analysis/SurfaceNormals.h>

#include "GenNormalsVtkm.h"
#include <vistle/vtkm/convert.h>
#include <vistle/core/normals.h>

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

std::string GenNormalsVtkm::getFieldName(int index, bool output) const
{
    if (index == 0 && output) {
        return "Normals";
    }
    return VtkmModule::getFieldName(index, output);
}

bool GenNormalsVtkm::changeParameter(const vistle::Parameter *p)
{
    if (!p || p == m_autoOrient) {
        setParameterReadOnly(m_inward, m_autoOrient->getValue() == 0);
    }
    return VtkmModule::changeParameter(p);
}

std::unique_ptr<viskores::filter::Filter> GenNormalsVtkm::setUpFilter(const VtkmModule::InputData &input) const
{
    auto filt = std::make_unique<viskores::filter::vector_analysis::SurfaceNormals>();
    filt->SetGenerateCellNormals(m_perVertex->getValue() == 0);
    filt->SetNormalizeCellNormals(m_normalize->getValue() != 0);
    filt->SetAutoOrientNormals(m_autoOrient->getValue() != 0);
    filt->SetFlipNormals(m_inward->getValue() != 0);
    filt->SetConsistency(false); // required for being able to reuse input grid
    return filt;
}

Object::const_ptr GenNormalsVtkm::prepareOutputGrid(const InputData &input, OutputData &output) const
{
    return input.vistleGrid;
}

DataBase::ptr GenNormalsVtkm::prepareOutputField(const InputData &input, OutputData &output, int index,
                                                 const std::string &fieldName) const
{
    DataBase::Mapping mapping = m_perVertex->getValue() == 0 ? DataBase::Element : DataBase::Vertex;

    if (auto mapped = vtkmGetField(output.viskoresDataset, fieldName, mapping)) {
        if (auto vec3 = Vec<Scalar, 3>::as(mapped)) {
            // make Normals
            auto norm = std::make_shared<Normals>(0, vec3->meta());
            for (int i = 0; i < 3; ++i)
                norm->d()->x[i] = vec3->d()->x[i];
            mapped = norm;
        }

        updateMeta(mapped);
        if (output.vistleGrid)
            mapped->setGrid(output.vistleGrid);
        mapped->describe("normals", id());
        return mapped;
    } else {
        sendError("Could not generate normals");
    }

    return nullptr;
}
