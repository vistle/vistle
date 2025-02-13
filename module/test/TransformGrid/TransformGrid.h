#ifndef VISTLE_TRANSFORMGRID_TRANSFORMGRID_H
#define VISTLE_TRANSFORMGRID_TRANSFORMGRID_H

#include <vistle/module/module.h>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(DimensionOrder, (XYZ)(XZY)(YXZ)(YZX)(ZXY)(ZYX))

constexpr std::array<int, 3> dimensionOrder(DimensionOrder order);

template<typename T>
constexpr T reorder(const T &val, const std::array<int, std::tuple_size<T>::value> &newOrder)
{
    T newVal{};
    for (size_t i = 0; i < val.size(); i++) {
        newVal[i] = val[newOrder[i]];
    }
    return newVal;
}


class TransformGrid: public vistle::Module {
public:
    TransformGrid(const std::string &name, int moduleID, mpi::communicator comm);

    bool compute();

    bool prepare();

    bool reduce(int timestep);

private:
    vistle::Port *data_in, *data_out;
    std::array<vistle::IntParameter *, 3> m_reverse;
    vistle::IntParameter *m_order = nullptr;
};


#endif
