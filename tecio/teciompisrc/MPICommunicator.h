 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <mpi.h>
#include "ThirdPartyHeadersEnd.h"
#include "MPIError.h"
#include "SimpleVector.h"
namespace tecplot { namespace teciompi { class MPICommunicator { public: explicit MPICommunicator(MPI_Comm communicator); template<typename T> void sendScalar(T ___4296, int dest, int tag); template<typename T> void receiveScalar(T& ___4296, int ___3654, int tag); template<typename T> void sendVector(SimpleVector<T> const& vec, int dest, int sizeTag, int vecTag); template<typename T> void receiveVector(SimpleVector<T>& vec, int ___3654, int sizeTag, int vecTag); private: MPI_Comm m_communicator; }; }}
