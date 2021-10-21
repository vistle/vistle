#ifndef LIBSIM_ENGINE_INTERFACE_H
#define LIBSIM_ENGINE_INTERFACE_H

#include <vistle/util/export.h>

#include <boost/asio/ip/tcp.hpp>

#include <memory>

#if defined(vistle_libsim_engine_interface_EXPORTS)
#define V_ENGINEINTERFACEEXPORT V_EXPORT
#else
#define V_ENGINEINTERFACEEXPORT V_IMPORT
#endif

namespace insitu {
class EngineInterface {
public:
    static void V_ENGINEINTERFACEEXPORT setControllSocket(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    static std::shared_ptr<boost::asio::ip::tcp::socket> V_ENGINEINTERFACEEXPORT getControllSocket();

private:
    static std::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
};

} // namespace insitu

#endif // !MODULEINTERFACE_H
