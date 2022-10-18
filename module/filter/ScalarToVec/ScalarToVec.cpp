#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>

#include "ScalarToVec.h"
#include <vistle/util/enum.h>
#include <algorithm>
using namespace vistle;

ScalarToVec::ScalarToVec(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    for (int i = 0; i < NumScalars; ++i) {
        m_scalarIn[i] = createInputPort("data_in" + std::to_string(i), "scalar input to be combined");
    }
    m_vecOut = createOutputPort("data_out", "combined vector output");
}

ScalarToVec::~ScalarToVec()
{}

bool ScalarToVec::compute()
{
    Vec<Scalar>::const_ptr data_in[NumScalars]{};
    size_t found = -1;
    for (int i = 0; i < NumScalars; ++i) {
        if (m_scalarIn[i]->isConnected()) {
            found = i;
            data_in[i] = expect<Vec<Scalar>>(m_scalarIn[i]);
        }
    }

    Vec<Scalar, NumScalars>::ptr out(new Vec<Scalar, NumScalars>(size_t(0)));

    for (int i = NumScalars - 1; i >= 0; --i) {
        if (data_in[i]) {
            out->d()->x[i] = data_in[i]->d()->x[0];
            out->copyAttributes(data_in[i]);
        } else {
            out->d()->x[i]->resize(data_in[found]->getSize());
            std::fill(out->d()->x[i]->begin(), out->d()->x[i]->end(), 0);
        }
    }
    out->setGrid(data_in[found]->grid());
    updateMeta(out);
    addObject(m_vecOut, out);

    return true;
}

MODULE_MAIN(ScalarToVec)
