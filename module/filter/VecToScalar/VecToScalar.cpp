#include "VecToScalar.h"
#include <vistle/module/module.h>
#include <vistle/util/enum.h>
#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <cmath>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/filter/vector_analysis/VectorMagnitude.h>
#include <vistle/vtkm/convert.h>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Choices, (X)(Y)(Z)(AbsoluteValue))

MODULE_MAIN(VecToScalar)

using namespace vistle;

VecToScalar::VecToScalar(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    auto pin = createInputPort("data_in", "vector to be split");
    auto pout = createOutputPort("data_out", "extracted scalar data component");
    linkPorts(pin, pout);
    m_caseParam = addIntParameter("choose_scalar_value", "Choose Scalar Value", 3, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_caseParam, Choices);
}

VecToScalar::~VecToScalar()
{}

bool VecToScalar::compute()
{
    auto data_in = expect<Vec<Scalar, 3>>("data_in");
    if (!data_in) {
        std::cerr << "VecToScalar: ERROR: Input is not vector data\n";
        return true;
    }
    std::string spec = data_in->getAttribute(attribute::Species);
    if (!spec.empty())
        spec += "_";

    Vec<Scalar>::ptr out;
    switch (m_caseParam->getValue()) {
    case X:
        spec += "x";
        out = extract(data_in, 0);
        break;
    case Y:
        spec += "y";
        out = extract(data_in, 1);
        break;
    case Z:
        spec += "z";
        out = extract(data_in, 2);
        break;
    case AbsoluteValue:
        spec += "magnitude";
        out = calculateAbsolute(data_in);
        break;
    }

    out->copyAttributes(data_in);
    out->describe(spec, id());
    out->setGrid(data_in->grid());
    updateMeta(out);
    addObject("data_out", out);
    return true;
}

vistle::Vec<vistle::Scalar>::ptr VecToScalar::extract(vistle::Vec<vistle::Scalar, 3>::const_ptr &data,
                                                      const vistle::Index &coord)
{
    auto dataOut = std::make_shared<Vec<Scalar>>(Object::Initialized);
    dataOut->d()->x[0] = data->d()->x[coord];
    return dataOut;
}

vistle::Vec<vistle::Scalar>::ptr VecToScalar::calculateAbsolute(vistle::Vec<vistle::Scalar, 3>::const_ptr &data)
{
#if VISTLE_WITH_VTKM
    try {
        // Create a Viskores dataset to hold the input mesh and fields
        viskores::cont::DataSet inDs;

        // Convert/attach the Vistle grid to the Viskores dataset
        vistle::vtkmSetGrid(inDs, data->grid());

        // Name for the input vector field once attached to the Viskores dataset
        static const std::string fld = "in";

        // Transfer the Vistle vector field into the Viskores dataset under 'fld'
        // (respects whether the data is per-vertex or per-cell)
        vistle::vtkmAddField(inDs, data, fld, data->mapping());

        // Set up the Viskores filter that computes vector magnitude
        viskores::filter::vector_analysis::VectorMagnitude mag;

        // Tell the filter which field to use and its association (points vs cells)
        mag.SetActiveField(fld, (data->mapping() == vistle::DataBase::Vertex)
                                    ? viskores::cont::Field::Association::Points
                                    : viskores::cont::Field::Association::Cells);

        // Run the filter on the dataset -> produces a new dataset with a scalar field
        const auto outDs = mag.Execute(inDs);

        // Bring the produced scalar field back into Vistle form
        auto outField = vistle::vtkmGetField(outDs, fld, data->mapping());

        // If the returned field is a Vistle scalar Vec, use it as the result
        if (auto vec = std::dynamic_pointer_cast<vistle::Vec<vistle::Scalar>>(outField))
            return vec;
    } catch (...) {
        // Any failure in the GPU/Viskores path -> fall back to CPU below
    }
#endif

    // (CPU fallback follows…)

    const auto n = data->getSize();
    const auto *x = data->x().data();
    const auto *y = data->y().data();
    const auto *z = data->z().data();
    auto out = std::make_shared<vistle::Vec<vistle::Scalar>>(n);
    auto *o = out->x().data();
    for (vistle::Index i = 0; i < n; ++i)
        o[i] = std::sqrt(x[i] * x[i] + y[i] * y[i] + z[i] * z[i]);
    return out;
}
