#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>
#include <iostream>

#include "scalars.h"

#include "paramvector.h"
#include "paramvector_impl.h"

#define V_INST_PARAMVEC(S) \
    template class ParameterVector<S>; \
    template bool operator==(const ParameterVector<S> &v1, const ParameterVector<S> &v2); \
    template bool operator!=(const ParameterVector<S> &v1, const ParameterVector<S> &v2); \
    template bool operator<(const ParameterVector<S> &v1, const ParameterVector<S> &v2); \
    template bool operator>(const ParameterVector<S> &v1, const ParameterVector<S> &v2); \
    template std::ostream &operator<<(std::ostream &out, const ParameterVector<S> &v);

namespace vistle {

V_INST_PARAMVEC(Float)
V_INST_PARAMVEC(Integer)
V_INST_PARAMVEC(std::string)

} // namespace vistle
