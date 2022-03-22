#include "TransformGrid.h"

#include <sstream>
#include <limits>
#include <algorithm>


#include <vistle/core/vec.h>
#include <vistle/core/scalars.h>
#include <vistle/core/message.h>
#include <vistle/core/coords.h>
#include <vistle/core/lines.h>
#include <vistle/core/triangles.h>
#include <vistle/core/quads.h>
#include <vistle/core/indexed.h>
#include <vistle/core/rectilineargrid.h>


using namespace vistle;
constexpr std::array<char, 3> axisNames{'X', 'Y', 'Z'};

TransformGrid::TransformGrid(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    data_in = createInputPort("data_in", "input data");
    data_out = createOutputPort("data_out", "output data");

    for (size_t i = 0; i < 3; i++) {
        m_reverse[i] = addIntParameter(std::string("reverse ") + axisNames[i] + "-axix", "", false,
                                       Parameter::Presentation::Boolean);
    }

    m_order = addIntParameter("orderOfGridAxes", "", XYZ, Parameter::Presentation::Choice);
    V_ENUM_SET_CHOICES(m_order, DimensionOrder);
}


bool TransformGrid::compute()
{
    Object::const_ptr obj = expect<Object>(data_in);
    if (!obj)
        return true;

    Object::const_ptr geo = obj;
    auto data = DataBase::as(obj);
    if (data && data->grid()) {
        geo = data->grid();
    } else {
        return true;
    }

    auto rgrid = RectilinearGrid::as(geo);
    if (!rgrid) {
        return true;
    }
    std::array<Index, 3> dims;
    std::array<const Scalar *, 3> axes;
    std::array<bool, 3> reverse;
    for (size_t i = 0; i < 3; i++) {
        dims[i] = rgrid->getNumDivisions(i);
        axes[i] = rgrid->coords(i);
        reverse[i] = m_reverse[i]->getValue();
    }
    //reorder
    auto newOrder = dimensionOrder(static_cast<DimensionOrder>(m_order->getValue()));

    dims = reorder(dims, newOrder);
    axes = reorder(axes, newOrder);
    reverse = reorder(reverse, newOrder);

    RectilinearGrid::ptr gridOut(new RectilinearGrid(dims[0], dims[1], dims[2]));

    for (size_t i = 0; i < 3; i++) {
        if (reverse[i]) {
            for (size_t j = 0; j < dims[i]; j++) {
                gridOut->coords(i).data()[j] = axes[i][dims[i] - j - 1];
            }
        } else {
            std::copy(axes[i], axes[i] + dims[i], gridOut->coords(i).data());
        }
    }
    updateMeta(gridOut);
    DataBase::ptr objOut = data->clone();
    objOut->setGrid(gridOut);
    updateMeta(objOut);
    addObject(data_out, objOut);
    return true;
}

bool TransformGrid::prepare()
{
    return true;
}

bool TransformGrid::reduce(int timestep)
{
    return Module::reduce(timestep);
}

MODULE_MAIN(TransformGrid)

constexpr std::array<int, 3> dimensionOrder(DimensionOrder order)
{
    std::array<int, 3> o{};
    switch (order) {
    case XYZ:
        o = {0, 1, 2};
        break;
    case XZY:
        o = {0, 2, 1};
        break;
    case YXZ:
        o = {1, 0, 2};
        break;
    case YZX:
        o = {1, 2, 0};
        break;
    case ZXY:
        o = {2, 0, 1};
        break;
    case ZYX:
        o = {2, 1, 0};
        break;
    default:
        break;
    }
    return o;
}
