#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>

#include "VecToScalar.h"
#include <vistle/util/enum.h>

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Choices, (X)(Y)(Z)(AbsoluteValue))

VecToScalar::VecToScalar(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("data_in", "vector to be split");
    createOutputPort("data_out", "extracted scalar data component");
    m_caseParam = addIntParameter("choose_scalar_value", "Choose Scalar Value", 3, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_caseParam, Choices);
}

VecToScalar::~VecToScalar()
{}

bool VecToScalar::compute()
{
    Vec<Scalar, 3>::const_ptr data_in = expect<Vec<Scalar, 3>>("data_in");
    if (!data_in) {
        std::cerr << "VecToScalar: ERROR: Input is not vector data" << std::endl;
        return true;
    }

    std::string spec = data_in->getAttribute("_species");
    if (!spec.empty())
        spec += "_";

    Vec<Scalar>::ptr out;
    switch (m_caseParam->getValue()) {
    case X: {
        spec += "x";
        out = extract(data_in, 0);
        break;
    }

    case Y: {
        spec += "y";
        out = extract(data_in, 1);
        break;
    }

    case Z: {
        spec += "z";
        out = extract(data_in, 2);
        break;
    }

    case AbsoluteValue: {
        spec += "magnitude";
        out = calculateAbsolute(data_in);
        break;
    }
    }

    out->copyAttributes(data_in);
    out->addAttribute("_species", spec);
    out->setGrid(data_in->grid());
    updateMeta(out);
    addObject("data_out", out);

    return true;
}

Vec<Scalar>::ptr VecToScalar::extract(Vec<Scalar, 3>::const_ptr &data, const Index &coord)
{
    Vec<Scalar>::ptr dataOut(new Vec<Scalar>(Object::Initialized));
    dataOut->d()->x[0] = data->d()->x[coord];
    return dataOut;
}

Vec<Scalar>::ptr VecToScalar::calculateAbsolute(Vec<Scalar, 3>::const_ptr &data)
{
    const Scalar *in_data_0 = &data->x()[0];
    const Scalar *in_data_1 = &data->y()[0];
    const Scalar *in_data_2 = &data->z()[0];
    Index dataSize = data->getSize();
    Vec<Scalar>::ptr dataOut(new Vec<Scalar>(dataSize));
    Scalar *out_data = dataOut->x().data();
    for (Index i = 0; i < dataSize; ++i) {
        out_data[i] = sqrt(in_data_0[i] * in_data_0[i] + in_data_1[i] * in_data_1[i] + in_data_2[i] * in_data_2[i]);
    }
    return dataOut;
}

MODULE_MAIN(VecToScalar)
