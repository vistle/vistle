#include <sstream>
#include <iomanip>

#include <core/object.h>
#include <core/vec.h>

#include "ScalarToVec.h"
#include <util/enum.h>

using namespace vistle;

ScalarToVec::ScalarToVec(const std::string &name, int moduleID, mpi::communicator comm)
: Module("combine three scalar fields to a vector field", name, moduleID, comm) {
    for (int i=0; i<NumScalars; ++i) {
        m_scalarIn[i] = createInputPort("data_in"+std::to_string(i));
    }
   m_vecOut = createOutputPort("data_out");
}

ScalarToVec::~ScalarToVec() {
}

bool ScalarToVec::compute() {

   Vec<Scalar>::const_ptr data_in[NumScalars];
   for (int i=0; i<NumScalars; ++i) {
       data_in[i] = expect<Vec<Scalar>>(m_scalarIn[i]);
       if (!data_in[i]) {
           sendError("no scalar input on %s", m_scalarIn[i]->getName().c_str());
           return true;
       }
   }

   Vec<Scalar, NumScalars>::ptr out(new Vec<Scalar, NumScalars>(Index(0)));
   for (int i=0; i<NumScalars; ++i)
       out->d()->x[i] = data_in[i]->d()->x[0];
   for (int i=NumScalars-1; i>=0; --i) {
       out->copyAttributes(data_in[i]);
   }
   out->setGrid(data_in[0]->grid());
   addObject(m_vecOut, out);

   return true;
}

MODULE_MAIN(ScalarToVec)
