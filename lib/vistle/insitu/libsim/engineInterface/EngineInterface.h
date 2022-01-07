#ifndef LIBSIM_ENGINE_INTERFACE_H
#define LIBSIM_ENGINE_INTERFACE_H

#include <vistle/util/export.h>

#include <boost/asio/ip/tcp.hpp>
#include <vistle/insitu/message/TcpMessage.h>
#include <memory>

#if defined(vistle_libsim_engine_interface_EXPORTS)
#define V_ENGINEINTERFACEEXPORT V_EXPORT
#else
#define V_ENGINEINTERFACEEXPORT V_IMPORT
#endif
namespace vistle {
namespace insitu {
namespace libsim {
class V_ENGINEINTERFACEEXPORT EngineInterface {
public:
    static void initialize(boost::mpi::communicator comm);
    static insitu::message::InSituTcp *getHandler();
    static std::unique_ptr<insitu::message::InSituTcp> extractHandler();

private:
    static std::unique_ptr<insitu::message::InSituTcp> m_messageHandler;
};
} // namespace libsim
} // namespace insitu
} // namespace vistle


#endif // !MODULEINTERFACE_H
