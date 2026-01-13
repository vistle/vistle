#include "VecToScalar.h"
#include <vistle/module/module.h>
#include <vistle/util/enum.h>
#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <cmath>

#ifdef VECTOSCALARVTKM
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/filter/vector_analysis/VectorMagnitude.h>
#include <vistle/vtkm/convert.h>
#endif

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

template<int d>
vistle::Vec<vistle::Scalar>::ptr extract(typename vistle::Vec<vistle::Scalar, d>::const_ptr &data,
                                         const vistle::Index &coord)
{
    auto dataOut = std::make_shared<Vec<Scalar>>(Object::Initialized);
    dataOut->d()->x[0] = data->d()->x[coord];
    return dataOut;
}

template<int d>
vistle::Vec<vistle::Scalar>::ptr calculateAbsolute(typename vistle::Vec<vistle::Scalar, d>::const_ptr &data)
{
#ifdef VECTOSCALARVTKM
    // Create a Viskores dataset to hold the input mesh and fields
    viskores::cont::DataSet inDs;

    // Convert/attach the Vistle grid to the Viskores dataset
    vistle::vtkmSetGrid(inDs, data->grid());

    // Name for the input vector field once attached to the Viskores dataset
    static const std::string fld_in = "in";
    static const std::string fld_out = "out";

    // Transfer the Vistle vector field into the Viskores dataset under 'fld'
    // (respects whether the data is per-vertex or per-cell)
    vistle::vtkmAddField(inDs, data, fld_in);

    // Set up the Viskores filter that computes vector magnitude
    viskores::filter::vector_analysis::VectorMagnitude mag;

    // Tell the filter which field to use and its association (points vs cells)
    mag.SetActiveField(fld_in, (data->guessMapping() == vistle::DataBase::Vertex)
                                   ? viskores::cont::Field::Association::Points
                                   : viskores::cont::Field::Association::Cells);
    mag.SetOutputFieldName(fld_out);

    // Run the filter on the dataset -> produces a new dataset with a scalar field
    const auto outDs = mag.Execute(inDs);

    // Bring the produced scalar field back into Vistle form
    auto outField = vistle::vtkmGetField(outDs, fld_out, data->mapping());
    if (!outField) {
        std::cerr << "VecToScalar: ERROR: Could not retrieve output field from Viskores filter" << std::endl;
        return nullptr;
    }

    // If the returned field is a Vistle scalar Vec, use it as the result
    return std::dynamic_pointer_cast<vistle::Vec<vistle::Scalar>>(outField);
#else
    const auto n = data->getSize();
    const Scalar *x = data->x().data();
    const Scalar *y = nullptr;
    if (d > 1)
        y = data->y().data();
    const Scalar *z = nullptr;
    if (d > 2)
        z = data->z().data();
    auto out = std::make_shared<vistle::Vec<vistle::Scalar>>(n);
    auto *o = out->x().data();
    if (d == 1) {
        for (vistle::Index i = 0; i < n; ++i)
            o[i] = std::abs(x[i]);
        return out;
    } else if (d == 2) {
        for (vistle::Index i = 0; i < n; ++i)
            o[i] = std::sqrt(x[i] * x[i] + y[i] * y[i]);
        return out;
    } else if (d == 3) {
        for (vistle::Index i = 0; i < n; ++i)
            o[i] = std::sqrt(x[i] * x[i] + y[i] * y[i] + z[i] * z[i]);
    }
    return out;
#endif
}

bool VecToScalar::compute()
{
    auto data_in = expect<DataBase>("data_in");
    if (!data_in) {
        std::cerr << "VecToScalar: ERROR: Input is not vector data\n";
        return true;
    }
    std::string spec = data_in->getAttribute(attribute::Species);
    if (!spec.empty())
        spec += "_";

    Vec<Scalar>::ptr out;
    auto data_in1 = Vec<Scalar, 1>::as(data_in);
    auto data_in2 = Vec<Scalar, 2>::as(data_in);
    auto data_in3 = Vec<Scalar, 3>::as(data_in);

    if (data_in1) {
        out = extract<1>(data_in1, 0);
        switch (m_caseParam->getValue()) {
        case X:
            spec += "x";
            out = extract<1>(data_in1, 0);
            break;
        case AbsoluteValue:
            spec += "magnitude";
            out = calculateAbsolute<1>(data_in1);
            break;
        case Y:
        case Z:
            sendError("input data has only 1 component, cannot extract y nor z component");
            return true;
        }
    } else if (data_in2) {
        switch (m_caseParam->getValue()) {
        case X:
            spec += "x";
            out = extract<2>(data_in2, 0);
            break;
        case Y:
            spec += "y";
            out = extract<2>(data_in2, 1);
            break;
        case AbsoluteValue:
            spec += "magnitude";
            out = calculateAbsolute<2>(data_in2);
            break;
        case Z:
            sendError("input data has only 2 components, cannot extract z component");
            return true;
        }
    } else if (data_in3) {
        switch (m_caseParam->getValue()) {
        case X:
            spec += "x";
            out = extract<3>(data_in3, 0);
            break;
        case Y:
            spec += "y";
            out = extract<3>(data_in3, 1);
            break;
        case Z:
            spec += "z";
            out = extract<3>(data_in3, 2);
            break;
        case AbsoluteValue:
            spec += "magnitude";
            out = calculateAbsolute<3>(data_in3);
            break;
        }
    }

    if (out) {
        out->copyAttributes(data_in);
        out->describe(spec, id());
        out->setGrid(data_in->grid());
        updateMeta(out);
        addObject("data_out", out);
    }

    return true;
}
