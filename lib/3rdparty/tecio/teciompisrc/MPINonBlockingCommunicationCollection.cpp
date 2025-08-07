#include "MPINonBlockingCommunicationCollection.h"
#include "ThirdPartyHeadersBegin.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/bind/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include "ThirdPartyHeadersEnd.h"
#include "basicTypes.h"
#include "CodeContract.h"
#include "MinMax.h"
#include "mpiDatatype.h"
#include "MPIError.h"
namespace tecplot { namespace teciompi { namespace { void throwMPIError(char const* method, char const* mpiRoutine, int errorCode) { std::ostringstream ___2430; ___2430 << "Error in " << method << ": " << mpiRoutine << " returned error code " << errorCode; throw(MPIError(___2430.str())); } } template <typename T> MPINonBlockingCommunicationCollection::MPINonBlockingSendScalar<T>::MPINonBlockingSendScalar( T const& ___4296, int dest, int tag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(dest, tag, "send")
 #else
: MPINonBlockingCommunication(dest, tag)
 #endif
, m_complete(false) { int ___3356 = MPI_Isend(const_cast<T*>(&___4296), 1, mpiDatatype<T>(), dest, tag, comm, &m_request); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingSendScalar", "MPI_Isend", ___3356); } template <> MPINonBlockingCommunicationCollection::MPINonBlockingSendScalar<___2477>::MPINonBlockingSendScalar( ___2477 const& ___4296, int dest, int tag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(dest, tag, "send")
 #else
: MPINonBlockingCommunication(dest, tag)
 #endif
, m_complete(false) { int ___3356 = MPI_Isend(const_cast<___2477*>(&___4296), 2, MPI_DOUBLE, dest, tag, comm, &m_request); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingSendScalar", "MPI_Isend", ___3356); } template <typename T> MPINonBlockingCommunicationCollection::MPINonBlockingSendScalar<T>::~MPINonBlockingSendScalar() { if (!isComplete() && m_request != MPI_REQUEST_NULL) MPI_Request_free(&m_request); } template <typename T> bool tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingSendScalar<T>::isComplete() { if (!m_complete) { int completeFlag; int ___3356 = MPI_Test(&m_request, &completeFlag, MPI_STATUS_IGNORE); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingSendScalar::isComplete", "MPI_Test", ___3356); m_complete = (completeFlag == 1); } return m_complete; } template <typename T> void tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingSendScalar<T>::___4443() { int ___3356 = MPI_Wait(&m_request, MPI_STATUS_IGNORE); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingSendScalar::waitUntilComplete", "MPI_Wait", ___3356); m_complete = true; } template <typename T> MPINonBlockingCommunicationCollection::MPINonBlockingSendScalarCopy<T>::MPINonBlockingSendScalarCopy( T ___4296, int dest, int tag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(dest, tag, "send")
 #else
: MPINonBlockingCommunication(dest, tag)
 #endif
, m_val(___4296) , m_sendScalar(m_val, dest, tag, comm) {} template <typename T> MPINonBlockingCommunicationCollection::MPINonBlockingSendScalarCopy<T>::~MPINonBlockingSendScalarCopy() { if (!isComplete()) {
 #if defined _DEBUG
std::cerr << "Warning: Uncompleted MPINonBlockingSendScalarCopy going out of scope." << std::endl;
 #endif
} } template <typename T> bool tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingSendScalarCopy<T>::isComplete() { return m_sendScalar.isComplete(); } template <typename T> void tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingSendScalarCopy<T>::___4443() { m_sendScalar.___4443(); } template <typename T> tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingReceiveScalar<T>:: MPINonBlockingReceiveScalar(T& ___4296, int ___3654, int tag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(___3654, tag, "recv")
 #else
: MPINonBlockingCommunication(___3654, tag)
 #endif
, m_val(___4296) , m_complete(false) , m_request(MPI_REQUEST_NULL) { int ___3356 = MPI_Irecv(&m_val, 1, mpiDatatype<T>(), ___3654, tag, comm, &m_request); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingReceiveScalar", "MPI_Irecv", ___3356); } template <> tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingReceiveScalar<___2477>:: MPINonBlockingReceiveScalar(___2477& ___4296, int ___3654, int tag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(___3654, tag, "recv")
 #else
: MPINonBlockingCommunication(___3654, tag)
 #endif
, m_val(___4296) , m_complete(false) , m_request(MPI_REQUEST_NULL) { int ___3356 = MPI_Irecv(&m_val, 2, MPI_DOUBLE, ___3654, tag, comm, &m_request); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingReceiveScalar", "MPI_Irecv", ___3356); } template <typename T> MPINonBlockingCommunicationCollection::MPINonBlockingReceiveScalar<T>::~MPINonBlockingReceiveScalar() { if (!isComplete()) { if (m_request != MPI_REQUEST_NULL) MPI_Request_free(&m_request);
 #if defined _DEBUG
std::cerr << "Warning: Uncompleted MPINonBlockingReceiveScalar going out of scope." << std::endl;
 #endif
} } template <typename T> bool tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingReceiveScalar<T>::isComplete() { if (!m_complete) { int completeFlag; int ___3356 = MPI_Test(&m_request, &completeFlag, MPI_STATUS_IGNORE); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingReceiveScalar::isComplete", "MPI_Test", ___3356); m_complete = (completeFlag == 1); } return m_complete; } template <typename T> void tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingReceiveScalar<T>::___4443() { int ___3356 = MPI_Wait(&m_request, MPI_STATUS_IGNORE); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingReceiveScalar::waitUntilComplete", "MPI_Wait", ___3356); m_complete = true; } template <typename T> MPINonBlockingCommunicationCollection::MPINonBlockingSendVector<T>::MPINonBlockingSendVector( SimpleVector<T> const& vec, int dest, int sizeTag, int vecTag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(dest, sizeTag, "send")
 #else
: MPINonBlockingCommunication(dest, sizeTag)
 #endif
, m_vec(vec) , m_complete(false) , m_sizeRequest(MPI_REQUEST_NULL) , m_vecRequest(MPI_REQUEST_NULL) { int ___3356 = MPI_Isend(const_cast<int*>(&m_vec.size()), 1, MPI_INT, dest, sizeTag, comm, &m_sizeRequest); if (___3356 == MPI_SUCCESS) ___3356 = MPI_Isend(const_cast<T*>(m_vec.begin()), m_vec.size(), mpiDatatype<T>(), dest, vecTag, comm, &m_vecRequest); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingSendVectorCopy", "MPI_Isend", ___3356); } template <> MPINonBlockingCommunicationCollection::MPINonBlockingSendVector<___2477>::MPINonBlockingSendVector( SimpleVector<___2477> const& vec, int dest, int sizeTag, int vecTag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(dest, sizeTag, "send")
 #else
: MPINonBlockingCommunication(dest, sizeTag)
 #endif
, m_vec(vec) , m_complete(false) , m_sizeRequest(MPI_REQUEST_NULL) , m_vecRequest(MPI_REQUEST_NULL) { int ___3356 = MPI_Isend(const_cast<int*>(&m_vec.size()), 1, MPI_INT, dest, sizeTag, comm, &m_sizeRequest); if (___3356 == MPI_SUCCESS) ___3356 = MPI_Isend(const_cast<___2477*>(m_vec.begin()), 2 * m_vec.size(), MPI_DOUBLE, dest, vecTag, comm, &m_vecRequest); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingSendVectorCopy", "MPI_Isend", ___3356); } template <typename T> tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingSendVector<T>::~MPINonBlockingSendVector() { if (!isComplete()) { if (m_sizeRequest != MPI_REQUEST_NULL) MPI_Request_free(&m_sizeRequest); if (m_vecRequest != MPI_REQUEST_NULL) MPI_Request_free(&m_vecRequest); } } template <typename T> bool MPINonBlockingCommunicationCollection::MPINonBlockingSendVector<T>::isComplete() { if (!m_complete) { int sizeFlag; int vecFlag = 0; int ___3356 = MPI_Test(&m_sizeRequest, &sizeFlag, MPI_STATUS_IGNORE); if (___3356 == MPI_SUCCESS) ___3356 = MPI_Test(&m_vecRequest, &vecFlag, MPI_STATUS_IGNORE); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingSendVectorCopy::isComplete", "MPI_Test", ___3356); m_complete = (sizeFlag == 1 && vecFlag == 1); } return m_complete; } template <typename T> void MPINonBlockingCommunicationCollection::MPINonBlockingSendVector<T>::___4443() { int ___3356 = MPI_Wait(&m_sizeRequest, MPI_STATUS_IGNORE); if (___3356 == MPI_SUCCESS) ___3356 = MPI_Wait(&m_vecRequest, MPI_STATUS_IGNORE); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingSendVectorCopy::waitUntilComplete", "MPI_Wait", ___3356); m_complete = true; } template <typename T> MPINonBlockingCommunicationCollection::MPINonBlockingSendVectorCopy<T>::MPINonBlockingSendVectorCopy( SimpleVector<T> const& vec, int dest, int sizeTag, int vecTag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(dest, sizeTag, "send")
 #else
: MPINonBlockingCommunication(dest, sizeTag)
 #endif
, m_vec(vec) , m_sendVector(m_vec, dest, sizeTag, vecTag, comm) {} template <typename T> MPINonBlockingCommunicationCollection::MPINonBlockingSendVectorCopy<T>::MPINonBlockingSendVectorCopy( std::vector<T> const& vec, int dest, int sizeTag, int vecTag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(dest, sizeTag, "send")
 #else
: MPINonBlockingCommunication(dest, sizeTag)
 #endif
, m_vec(vec.begin(), vec.end()) , m_sendVector(m_vec, dest, sizeTag, vecTag, comm) {} template <typename T> tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingSendVectorCopy<T>::~MPINonBlockingSendVectorCopy() { if (!isComplete()) {
 #if defined _DEBUG
std::cerr << "Warning: Uncompleted MPINonBlockingSendVectorCopy going out of scope." << std::endl;
 #endif
} } template <typename T> bool MPINonBlockingCommunicationCollection::MPINonBlockingSendVectorCopy<T>::isComplete() { return m_sendVector.isComplete(); } template <typename T> void MPINonBlockingCommunicationCollection::MPINonBlockingSendVectorCopy<T>::___4443() { m_sendVector.___4443(); } template <typename T> MPINonBlockingCommunicationCollection::MPINonBlockingReceiveVector<T>::MPINonBlockingReceiveVector( SimpleVector<T>& vec, int ___3654, int sizeTag, int vecTag, MPI_Comm comm)
 #if defined _DEBUG
: MPINonBlockingCommunication(___3654, sizeTag, "recv")
 #else
: MPINonBlockingCommunication(___3654, sizeTag)
 #endif
, m_size(0) , m_vec(vec) , m_src(___3654) , m_vecTag(vecTag) , m_comm(comm) , m_complete(false) , m_sizeRequest(MPI_REQUEST_NULL) , m_vecRequest(MPI_REQUEST_NULL) { int ___3356 = MPI_Irecv(&m_size, 1, MPI_INT, ___3654, sizeTag, comm, &m_sizeRequest); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingReceiveVector", "MPI_Irecv", ___3356); } template <typename T> tecplot::teciompi::MPINonBlockingCommunicationCollection::MPINonBlockingReceiveVector<T>::~MPINonBlockingReceiveVector() { if (!isComplete()) { if (m_sizeRequest != MPI_REQUEST_NULL) MPI_Request_free(&m_sizeRequest); if (m_vecRequest != MPI_REQUEST_NULL) MPI_Request_free(&m_vecRequest);
 #if defined _DEBUG
std::cerr << "Warning: Uncompleted MPINonBlockingReceiveVector going out of scope." << std::endl;
 #endif
} } template <typename T> bool MPINonBlockingCommunicationCollection::MPINonBlockingReceiveVector<T>::tryToComplete(CompleteFunction completeFunction) { if (!m_complete) { if (m_sizeRequest != MPI_REQUEST_NULL) { int sizeFlag = 1; completeFunction(&m_sizeRequest, &sizeFlag, MPI_STATUS_IGNORE); if (sizeFlag == 1) { ___476(m_sizeRequest == MPI_REQUEST_NULL); m_vec.allocate(m_size); int ___3356 = MPI_Irecv(m_vec.begin(), m_vec.size(), mpiDatatype<T>(), m_src, m_vecTag, m_comm, &m_vecRequest); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingReceiveVector::tryToComplete", "MPI_Irecv", ___3356); } } if (m_sizeRequest == MPI_REQUEST_NULL && m_vecRequest != MPI_REQUEST_NULL) { int vecFlag; completeFunction(&m_vecRequest, &vecFlag, MPI_STATUS_IGNORE); } if (m_sizeRequest == MPI_REQUEST_NULL && m_vecRequest == MPI_REQUEST_NULL) m_complete = true; } return m_complete; } template <> bool MPINonBlockingCommunicationCollection::MPINonBlockingReceiveVector<___2477>::tryToComplete(CompleteFunction completeFunction) { if (!m_complete) { if (m_sizeRequest != MPI_REQUEST_NULL) { int sizeFlag = 1; completeFunction(&m_sizeRequest, &sizeFlag, MPI_STATUS_IGNORE); if (sizeFlag == 1) { ___476(m_sizeRequest == MPI_REQUEST_NULL); m_vec.allocate(m_size); int ___3356 = MPI_Irecv(m_vec.begin(), 2 * m_vec.size(), MPI_DOUBLE, m_src, m_vecTag, m_comm, &m_vecRequest); if (___3356 != MPI_SUCCESS) throwMPIError("MPINonBlockingReceiveVector::tryToComplete", "MPI_Irecv", ___3356); } } if (m_sizeRequest == MPI_REQUEST_NULL && m_vecRequest != MPI_REQUEST_NULL) { int vecFlag; completeFunction(&m_vecRequest, &vecFlag, MPI_STATUS_IGNORE); } if (m_sizeRequest == MPI_REQUEST_NULL && m_vecRequest == MPI_REQUEST_NULL) m_complete = true; } return m_complete; } template <typename T> bool MPINonBlockingCommunicationCollection::MPINonBlockingReceiveVector<T>::isComplete() { using namespace boost::placeholders; return tryToComplete(boost::bind(MPI_Test, _1, _2, _3)); } template <typename T> void MPINonBlockingCommunicationCollection::MPINonBlockingReceiveVector<T>::___4443() { using namespace boost::placeholders; tryToComplete(boost::bind(MPI_Wait, _1, _3)); } MPINonBlockingCommunicationCollection::MPINonBlockingCommunicationCollection(MPI_Comm communicator, int numRequests  ) : m_communicator(communicator) { REQUIRE(numRequests >= 0); if (numRequests > 0) m_requests.reserve(numRequests); } template<typename T> void tecplot::teciompi::MPINonBlockingCommunicationCollection::sendScalar(T const& ___4296, int dest, int tag) { m_requests.push_back(boost::make_shared<MPINonBlockingSendScalar<T> > (___4296, dest, tag, m_communicator)); } template<typename T> void tecplot::teciompi::MPINonBlockingCommunicationCollection::sendScalarCopy(T ___4296, int dest, int tag) { m_requests.push_back(boost::make_shared<MPINonBlockingSendScalarCopy<T> > (___4296, dest, tag, m_communicator)); } template<typename T> void tecplot::teciompi::MPINonBlockingCommunicationCollection::receiveScalar(T& ___4296, int ___3654, int tag) { m_requests.push_back(boost::make_shared<MPINonBlockingReceiveScalar<T> >
(boost::ref(___4296), ___3654, tag, m_communicator)); } template<typename T> void MPINonBlockingCommunicationCollection::sendVector(SimpleVector<T> const& vec, int dest, int sizeTag, int vecTag) { REQUIRE(sizeTag != vecTag); m_requests.push_back(boost::make_shared<MPINonBlockingSendVector<T> > (boost::ref(vec), dest, sizeTag, vecTag, m_communicator)); } template<typename T> void MPINonBlockingCommunicationCollection::sendVectorCopy(SimpleVector<T> const& vec, int dest, int sizeTag, int vecTag) { REQUIRE(sizeTag != vecTag); m_requests.push_back(boost::make_shared<MPINonBlockingSendVectorCopy<T> > (boost::ref(vec), dest, sizeTag, vecTag, m_communicator)); } template<typename T> void tecplot::teciompi::MPINonBlockingCommunicationCollection::sendVectorCopy(std::vector<T> const& vec, int dest, int sizeTag, int vecTag) { REQUIRE(sizeTag != vecTag); m_requests.push_back(boost::make_shared<MPINonBlockingSendVectorCopy<T> > (boost::ref(vec), dest, sizeTag, vecTag, m_communicator)); } void MPINonBlockingCommunicationCollection::sendStringCopy(std::string const& str, int dest, int sizeTag, int vecTag) { REQUIRE(sizeTag != vecTag); SimpleVector<char> vec(str); m_requests.push_back(boost::make_shared<MPINonBlockingSendVectorCopy<char> > (boost::ref(vec), dest, sizeTag, vecTag, m_communicator)); } template<typename T> void MPINonBlockingCommunicationCollection::receiveVector(SimpleVector<T>& vec, int ___3654, int sizeTag, int vecTag) { REQUIRE(sizeTag != vecTag); m_requests.push_back(boost::make_shared<MPINonBlockingReceiveVector<T> > (boost::ref(vec), ___3654, sizeTag, vecTag, m_communicator)); } bool MPINonBlockingCommunicationCollection::isComplete() {
 #if defined MPI_INSTRUMENTATION
std::ofstream outputFile("iscomplete.txt"); bool isComplete = true; for (RequestVector_t::const_iterator request = m_requests.begin(); request != m_requests.end(); ++request) { outputFile << (*request)->m_sendOrReceive << ", " << (*request)->m_other << ", "; outputFile.setf(std::ios::hex, std::ios::basefield); outputFile.setf(std::ios::showbase); outputFile << (*request)->m_tag; outputFile.setf(std::ios::dec, std::ios::basefield); outputFile.unsetf(std::ios::showbase); if ((*request)->isComplete()) { outputFile << ", true"; } else { outputFile << ", false"; isComplete = false; } outputFile << std::endl; } return isComplete;
 #else
for (RequestVector_t::const_iterator request = m_requests.begin(); request != m_requests.end(); ++request) if (!(*request)->isComplete()) return false; return true;
 #endif
} void MPINonBlockingCommunicationCollection::___4443() { size_t numRequests = m_requests.size(); for (size_t i = 0; i < numRequests; ++i) { bool complete = true; for (size_t ___2103 = 0; ___2103 < numRequests; ++___2103) { complete = m_requests[___2103]->isComplete() && complete; } if (complete) break; } for (RequestVector_t::const_iterator request = m_requests.begin(); request != m_requests.end(); ++request) (*request)->___4443(); m_requests.clear(); }
 #define INSTANTIATE_FOR_TYPE(T) \
 template void MPINonBlockingCommunicationCollection::sendScalar<T>(T const& ___4296, int dest, int tag); \
 template void MPINonBlockingCommunicationCollection::sendScalarCopy<T>(T ___4296, int dest, int tag); \
 template void MPINonBlockingCommunicationCollection::receiveScalar<T>(T& ___4296, int dest, int tag); \
 template void MPINonBlockingCommunicationCollection::sendVector<T>(SimpleVector<T> const& vec, int dest, int sizeTag, int vecTag); \
 template void MPINonBlockingCommunicationCollection::sendVectorCopy<T>(SimpleVector<T> const& vec, int dest, int sizeTag, int vecTag); \
 template void MPINonBlockingCommunicationCollection::sendVectorCopy<T>(std::vector<T> const& vec, int dest, int sizeTag, int vecTag); \
 template void MPINonBlockingCommunicationCollection::receiveVector<T>(SimpleVector<T>& vec, int ___3654, int sizeTag, int vecTag);
INSTANTIATE_FOR_TYPE(char) INSTANTIATE_FOR_TYPE(uint8_t) INSTANTIATE_FOR_TYPE(uint16_t) INSTANTIATE_FOR_TYPE(int32_t) INSTANTIATE_FOR_TYPE(uint32_t) INSTANTIATE_FOR_TYPE(int64_t) INSTANTIATE_FOR_TYPE(uint64_t) INSTANTIATE_FOR_TYPE(float) INSTANTIATE_FOR_TYPE(double) INSTANTIATE_FOR_TYPE(___2477)
 #undef INSTANTIATE_FOR_TYPE
}}
